#include "kserverhandler.h"

#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <algorithm>

#define CONNECTION_LIMIT 10
#define CONNECTION_TIMEOUT 100000 //us -> 100 ms

#include "../common/kglobaldefs.h"

KServerHandler::KServerHandler() :
    KAbstractNetworkHandler(),
    kIsInitialized(false),
    kTCPListenerHandle(-1),
    kUDPListenerHandle(-1)
{
    kHost.sin_family = AF_INET;
    kHost.sin_addr.s_addr = INADDR_ANY;

    kSendTimeout.tv_sec = 0;
    kSendTimeout.tv_usec = SEND_TIMEOUT;

    kRecvTimeout.tv_sec = 0;
    kRecvTimeout.tv_usec = RECV_TIMEOUT;
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
    while(true/* && kIsListening*/) {
        //fill client sockets
        fd_set handleSet;
        FD_ZERO(&handleSet);

        FD_SET(kTCPListenerHandle, &handleSet);
        for(const int &clientHandle : kTCPClientHandles)
            FD_SET(clientHandle, &handleSet);
        FD_SET(kUDPListenerHandle, &handleSet);

        // get socket with event
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = CONNECTION_TIMEOUT;

        int maxHandle = max(max(kUDPListenerHandle, kTCPListenerHandle),
                            *max_element(kTCPClientHandles.begin(), kTCPClientHandles.end()));
        int handleCount = select(maxHandle + 1, &handleSet, nullptr, nullptr, &timeout);
        if(handleCount <= 0) //timeout or error occured
            continue;

        //handle events
        //handle TCP listener event
        if(FD_ISSET(kTCPListenerHandle, &handleSet)) {
            //accept new socket
            sockaddr_in  clientHost;
            int clientHostLength;

            int socketHandle = accept(kTCPListenerHandle, (sockaddr *)&clientHost, (socklen_t*)&clientHostLength);
            if(socketHandle < 0) {
                cout << "TCP client socket accept failed!" << endl;
                return;
            } else {
                cout << "TCP client connected: " << inet_ntoa(clientHost.sin_addr) << endl;

                int yes = 1;
                setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
                setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));
                setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int));

                //add socket to client sockets set
                fcntl(socketHandle, F_SETFL, O_NONBLOCK);
                kTCPClientHandles.insert(socketHandle);
            }
        }

        //echo incoming TCP client messages
        for(const int &clientHandle : kTCPClientHandles) {
            if(FD_ISSET(clientHandle, &handleSet)) {
                sockaddr_in  clientHost;
                int clientHostLength;
                char buffer[BUFFER_SIZE] = {0};

                int bytesRead = recvAll(clientHandle, buffer, sizeof(buffer), 0);
                if(bytesRead <= 0) { //client disconnected or error occured

                    getpeername(clientHandle , (struct sockaddr*)&clientHost , (socklen_t*)&clientHostLength);
                    cout << "Client disconnected: " << inet_ntoa(clientHost.sin_addr) << endl;

                    //erase connection
                    close(clientHandle);
                    kTCPClientHandles.erase(clientHandle);
                    continue;
                } else { //send message back
                    if(sendAll(clientHandle, buffer, bytesRead, 0) > 0)
                        cout << "TCP message echoed to client: " << inet_ntoa(clientHost.sin_addr) << endl;
                    else
                        cout << "TCP message echo failed to client: " << inet_ntoa(clientHost.sin_addr) << endl;
                }
            }
        }

        //handle UDP listener event
        if(FD_ISSET(kUDPListenerHandle, &handleSet)) {
            sockaddr_in  clientHost;
            int clientHostLength;
            char buffer[BUFFER_SIZE] = {0};

            int bytesRead = recvAllFrom(kUDPListenerHandle, buffer, sizeof(buffer), 0,
                                        (sockaddr*)&clientHost, &clientHostLength);

            if (bytesRead <= 0) {  //error occured
                continue;
            } else { //send message back
                if(sendAllTo(kUDPListenerHandle, buffer, bytesRead, 0,
                             (sockaddr *)&clientHost, sizeof(clientHost)) > 0)
                    cout << "UDP message echoed to client: " << inet_ntoa(clientHost.sin_addr) << endl;
                else
                    cout << "UDP message echo failed to client: " << inet_ntoa(clientHost.sin_addr) << endl;
            }
        }
    }
}
