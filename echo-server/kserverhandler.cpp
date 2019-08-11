#include "kserverhandler.h"

#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

#define CONNECTION_LIMIT 10
#define CONNECTION_TIMEOUT 100000 //us -> 100 ms
#define UDP_INACTIVITY_TIMEOUT 10 //secs

KServerHandler::KServerHandler() :
    KAbstractNetworkHandler(),
    kIsInitialized(false),
    kTCPListenerHandle(-1),
    kUDPListenerHandle(-1)
{
    kHost.sin_family = AF_INET;
    kHost.sin_addr.s_addr = INADDR_ANY;
}

KServerHandler::~KServerHandler() {
    if(kTCPListenerHandle >= 0)
        close(kTCPListenerHandle);

    if(kUDPListenerHandle >= 0)
        close(kUDPListenerHandle);

    for(const int &clientHandle : kTCPClientHandles)
        if(clientHandle >= 0)
            close(clientHandle);
}

bool KServerHandler::initialize(int port) {
    if(kIsInitialized) {
        cout << "Already initialized!" << endl;
        return false;
    }

    kHost.sin_port = htons(port);

    //create TCP listener --------------------------------------
    kTCPListenerHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(kTCPListenerHandle < 0) {
        cout << "TCP listener creation failed!" << endl;
        return false;
    }

    fcntl(kTCPListenerHandle, F_SETFL, O_NONBLOCK);

    //bind TCP listener
    if(bind(kTCPListenerHandle, (struct sockaddr *)&kHost, sizeof(kHost)) < 0) {
        cout << "TCP listener bind failed!" << endl;

        kTCPListenerHandle = -1;
        return false;
    }

    int yes = 1;
    setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
    setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));
    setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int));
    setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_REUSEPORT, (char*)&yes, sizeof(int));

    cout << "TCP listener initialized" << endl;


    //create UDP listener --------------------------------------
    kUDPListenerHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(kUDPListenerHandle < 0) {
        cout << "UDP listener creation failed!" << endl;
        return false;
    }

    fcntl(kUDPListenerHandle, F_SETFL, O_NONBLOCK);

    //bind UDP listener
    if(bind(kUDPListenerHandle, (struct sockaddr *)&kHost, sizeof(kHost)) < 0) {
        cout << "UDP listener bind failed!" << endl;

        kUDPListenerHandle = -1;
        return false;
    }

    //setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
    setsockopt(kTCPListenerHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));

    cout << "UDP listener initialized" << endl;

    kIsInitialized = true;

    return true;
}

void KServerHandler::listen() {
    if(!kIsInitialized) {
        cout << "Listeners is not initialized!" << endl;
        return;
    }

    //start tcp listening
    ::listen(kTCPListenerHandle, CONNECTION_LIMIT);

    //start event loop
    while(true) {
        //read set
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(kTCPListenerHandle, &readSet);
        for(const int &clientHandle : kTCPClientHandles)
            FD_SET(clientHandle, &readSet);
        FD_SET(kUDPListenerHandle, &readSet);

        //write set
        fd_set writeSet;
        FD_ZERO(&writeSet);
        for(const int &clientHandle : kTCPClientHandles)
            FD_SET(clientHandle, &writeSet);
        FD_SET(kUDPListenerHandle, &writeSet);

        // get socket with event
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int maxHandle = max(max(kUDPListenerHandle, kTCPListenerHandle),
                            (kTCPClientHandles.size() > 0 ? *kTCPClientHandles.rbegin() : 0));
        int handleCount = select(maxHandle + 1, &readSet, &writeSet, nullptr, &timeout);
        if(handleCount == 0) {//timeout occured
            continue; //do nothing
        } else if(handleCount < 0) { //error occured
            cout << "Select failed!" << endl;
            return;
        } else { //handle events
            acceptTCPConnections(readSet);
            readTCPMessages(readSet);
            writeTCPMessages(writeSet);
            readUDPMessages(readSet);
            writeUDPMessages(writeSet);
            checkUDPClientActivity();
        }
    }
}


void KServerHandler::acceptTCPConnections(const fd_set &readSet) {
    if(FD_ISSET(kTCPListenerHandle, &readSet)) {
        //accept new socket
        sockaddr_in  clientHost;
        int clientHostLength = sizeof(clientHost);

        int socketHandle = accept(kTCPListenerHandle, (sockaddr *)&clientHost, (socklen_t*)&clientHostLength);
        if(socketHandle < 0) {
            if(errno != EWOULDBLOCK) {
                cout << "TCP connection  accept failed!" << endl;
                return;
            }
        } else {
            cout << "TCP connection accepted: " << inet_ntoa(clientHost.sin_addr) << endl;

            int yes = 1;
            setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
            setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));
            setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int));

            //add socket to client sockets set
            fcntl(socketHandle, F_SETFL, O_NONBLOCK);
            kTCPClientHandles.insert(socketHandle);
            kTCPClientContext.insert(pair<int,ClientContext>(clientHost.sin_addr.s_addr, ClientContext(clientHost)));
        }
    }
}

void KServerHandler::readTCPMessages(const fd_set &readSet) {
    for(auto it = kTCPClientHandles.begin(); it != kTCPClientHandles.end();) {
        int clientHandle = *it;
        if(FD_ISSET(clientHandle, &readSet)) {
            //get host
            sockaddr_in clientHost;
            int clientHostLength = sizeof(clientHost);
            getpeername(clientHandle, (struct sockaddr*)&clientHost, (socklen_t*)&clientHostLength);

            //get context
            if(kTCPClientContext.find(clientHost.sin_addr.s_addr) == kTCPClientContext.end()) {
                ++it;
                continue;
            }
            ClientContext &context = kTCPClientContext.at(clientHost.sin_addr.s_addr);

            //check client state
            if(context.state != ClientContext::Read) { //block reading until writing completed
                ++it;
                continue;
            }

            //get message
            int bytesToRead;
            if(context.totalSize > 0) { //already got valid header - > get message data
                bytesToRead = context.totalSize - context.currentSize;
            } else { //get header
                bytesToRead = sizeof(KTCPMessageHeader) - context.currentSize;
            }

            int bytesRead = recv(clientHandle, context.buffer + context.currentSize, bytesToRead, 0);

            //handle result
            if(bytesRead < 0) { //error
                if(errno != EWOULDBLOCK) {
                    cout << "TCP connection closed due to recv error: " << inet_ntoa(clientHost.sin_addr) << endl;
                    //erase connection
                    close(clientHandle);
                    kTCPClientContext.erase(clientHost.sin_addr.s_addr);
                    kTCPClientHandles.erase(it++);
                    continue;
                }
            } else if(bytesRead == 0) { //connection closed
                //erase connection
                cout << "TCP connection closed: " << inet_ntoa(clientHost.sin_addr) << endl;
                close(clientHandle);
                kTCPClientContext.erase(clientHost.sin_addr.s_addr);
                kTCPClientHandles.erase(it++);
                continue;
            } else { //handle data
                if(context.totalSize > 0) { //already got valid header - > handle message data
                    context.currentSize += bytesRead;

                    if(context.currentSize == context.totalSize) {//got full message
                        context.state = ClientContext::Write;
                        cout << "Message received from TCP client: " << inet_ntoa(clientHost.sin_addr) << endl;
                    }
                } else { //handle header
                    context.currentSize += bytesRead;
                    if(context.currentSize >= sizeof(KTCPMessageHeader)) {
                        KTCPMessageHeader *header = (KTCPMessageHeader*)&(context.buffer);
                        if(header->type != MessageType::Data) { //invalid message type
                            context.totalSize = 0;
                            context.currentSize = 0;
                            memset(context.buffer, 0x00, sizeof(context.buffer));
                        } else if(header->size > MESSAGE_SIZE_LIMIT) { //invalid message size
                            context.totalSize = 0;
                            context.currentSize = 0;
                            memset(context.buffer, 0x00, sizeof(context.buffer));
                        } else { //valid header
                            context.totalSize = context.currentSize + header->size;
                        }
                    }
                }
            }
        }
        ++it;
    }
}

void KServerHandler::writeTCPMessages(const fd_set &writeSet) {
    for(auto it = kTCPClientHandles.begin(); it != kTCPClientHandles.end();) {
        int clientHandle = *it;
        if(FD_ISSET(clientHandle, &writeSet)) {
            //get host
            sockaddr_in clientHost;
            int clientHostLength = sizeof(clientHost);
            getpeername(clientHandle, (struct sockaddr*)&clientHost, (socklen_t*)&clientHostLength);

            //get context
            if(kTCPClientContext.find(clientHost.sin_addr.s_addr) == kTCPClientContext.end()) {
                ++it;
                continue;
            }
            ClientContext &context = kTCPClientContext.at(clientHost.sin_addr.s_addr);

            //check state
            if(context.state != ClientContext::Write) { //block writing until reading completed
                ++it;
                continue;
            }

            //send data
            int bytesSend = send(clientHandle, context.buffer + (context.totalSize - context.currentSize),
                                 context.currentSize, 0);

            //handle result
            if(bytesSend < 0) { //error
                if(errno != EWOULDBLOCK) {
                    cout << "Connection closed due to send error: " << inet_ntoa(clientHost.sin_addr) << endl;
                    //erase connection
                    close(clientHandle);
                    kTCPClientContext.erase(clientHost.sin_addr.s_addr);
                    kTCPClientHandles.erase(it++);
                    continue;
                }
            } else if(bytesSend == 0) { //connection closed
                cout << "Connection closed: " << inet_ntoa(clientHost.sin_addr) << endl;
                //erase connection
                close(clientHandle);
                kTCPClientContext.erase(clientHost.sin_addr.s_addr);
                kTCPClientHandles.erase(it++);
                continue;
            } else { //handle data
                context.currentSize -= bytesSend;

                if(context.currentSize == 0) {//sent full message
                    context.totalSize = 0;
                    context.currentSize = 0;
                    memset(context.buffer, 0x00, sizeof(context.buffer));
                    context.state = ClientContext::Read;
                    cout << "Message sent to TCP client: " << inet_ntoa(clientHost.sin_addr) << endl;
                }
            }
        }
        ++it;
    }
}

void KServerHandler::readUDPMessages(const fd_set &readSet) {
    if(FD_ISSET(kUDPListenerHandle, &readSet)) {
        sockaddr_in  clientHost;
        int clientHostLength = sizeof(clientHost);

        //get host
        int bytesRead = recvfrom(kUDPListenerHandle, nullptr, 0, MSG_PEEK,
                                 (sockaddr*)&clientHost, (socklen_t*)&clientHostLength);
        if(bytesRead < 0)
            return;

        //get context
        if(kUDPClientContext.find(clientHost.sin_addr.s_addr) == kUDPClientContext.end()) {
            kUDPClientContext.insert(pair<int,ClientContext>(clientHost.sin_addr.s_addr, ClientContext(clientHost)));
            cout << "new UDP client: " << inet_ntoa(clientHost.sin_addr) << endl;
        }

        ClientContext &context = kUDPClientContext.at(clientHost.sin_addr.s_addr);

        //check state
        if(context.state != ClientContext::Read) //block reading until writing completed
            return;

        //read data
        bytesRead = recvfrom(kUDPListenerHandle, context.buffer, sizeof(context.buffer), 0,
                             (sockaddr*)&clientHost, (socklen_t*)&clientHostLength);

        if (bytesRead < 0) {  //error occured
            if(errno != EWOULDBLOCK) {
                cout << "Message by UDP not received: " << inet_ntoa(clientHost.sin_addr) << endl;
                kUDPClientContext.erase(clientHost.sin_addr.s_addr);
            }
        } else if (bytesRead == 0) { //null data
            cout << "Message by UDP not received: " << inet_ntoa(clientHost.sin_addr) << endl;
        } else { //handle data
            //check header
            KUDPMessageHeader *header = (KUDPMessageHeader*)&context.buffer;
            if(context.datagrammIDs.find(header->id) != context.datagrammIDs.end()) { //double message
                memset(context.buffer, 0x00, sizeof(context.buffer));
                return;
            } else if(header->type == MessageType::Data) { //data message
                if(context.datagrammIDs.size() >= DATAGRAMM_LIMIT)
                    context.datagrammIDs.erase(context.datagrammIDs.begin());
                context.datagrammIDs.insert(header->id);

                context.lastActivityTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

                context.totalSize = bytesRead;
                context.state = ClientContext::Write;
            } else if(header->type == MessageType::Ack) { //ack message
                if(context.datagrammIDs.size() >= DATAGRAMM_LIMIT)
                    context.datagrammIDs.erase(context.datagrammIDs.begin());
                context.datagrammIDs.insert(header->id);

                //check stored message ids
                char result;
                int headerSize = sizeof(KUDPMessageHeader);
                uint64_t id;
                memcpy(&id, context.buffer + headerSize, sizeof(id));
                if(context.datagrammIDs.find(id) != context.datagrammIDs.end())
                    result = 0x01;
                else
                    result = 0x00;

                memset(context.buffer + bytesRead, result, sizeof(result));

                context.lastActivityTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

                context.totalSize = bytesRead + 1;
                context.state = ClientContext::Write;
            } else { //unknown message type
                memset(context.buffer, 0x00, sizeof(context.buffer));
            }
        }
    }
}

void KServerHandler::writeUDPMessages(const fd_set &writeSet) {
    if(FD_ISSET(kUDPListenerHandle, &writeSet)) {
        for(auto it = kUDPClientContext.begin(); it != kUDPClientContext.end();) {
            //get context
            ClientContext &context = it->second;

            //check state
            if(context.state != ClientContext::Write) { //block writing until reading completed
                ++it;
                continue;
            }

            int bytesSend = sendto(kUDPListenerHandle, context.buffer, context.totalSize, 0,
                                   (sockaddr *)&context.host, sizeof(context.host));
            if(bytesSend > 0) {
                cout << "Message sent to UDP client: " << inet_ntoa(context.host.sin_addr) << endl;
            } else {
                if(errno != EWOULDBLOCK) {
                    cout << "Message transmission to UDP client failed: " << inet_ntoa(context.host.sin_addr) << endl;
                    kUDPClientContext.erase(it++);
                    continue;
                }
            }
            context.totalSize = 0;
            memset(context.buffer, 0x00, sizeof(context.buffer));

            context.lastActivityTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

            context.state = ClientContext::Read;

            ++it;
        }
    }
}

void KServerHandler::checkUDPClientActivity() {
    for(auto it = kUDPClientContext.begin(); it != kUDPClientContext.end();) {
        ClientContext &context = it->second;
        time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

        if(currentTime - context.lastActivityTime >= UDP_INACTIVITY_TIMEOUT) {
            //erase context
            cout << "UDP client inactive: " << inet_ntoa(context.host.sin_addr) << endl;
            kUDPClientContext.erase(it++);
        } else {
            ++it;
        }
    }
}
