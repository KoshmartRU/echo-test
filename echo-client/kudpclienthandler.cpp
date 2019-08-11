#include "kudpclienthandler.h"

#include <sys/socket.h>
#include <string.h>
#include <chrono>
#include <ctime>
#include <iostream>

#include "../common/kglobaldefs.h"

KUDPClientHandler::KUDPClientHandler() :
    KAbstractClientHandler()
{
    srand(time(0));
}

bool KUDPClientHandler::connectToHost(const string &host, int port) {
    if(kSocketHandle >= 0) {
        cout << "UDP socket is already valid!" << endl;
        return false;
    }
    
    //validate host
    if(!validateHost(host, port))
        return false;
    
    //create socket
    kSocketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(kSocketHandle < 0) {
        cout << "UDP socket creation failed!" << endl;
        return false;
    }
    
    //set timeout
    //setsockopt(kSocketHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
    setsockopt(kSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));
    
    return true;
}


int KUDPClientHandler::write(const string &message) {
    if(kSocketHandle < 0) {
        cout << "UDP socket is invalid!" << endl;
        return -1;
    }

    int headerSize = sizeof(KUDPMessageHeader);
    int messageSize = message.length();

    if (message.size() == 0 || messageSize > (MESSAGE_SIZE_LIMIT - headerSize)) {
        cout << "Message size is invalid!" << endl;
        return -2;
    }

    KUDPMessageHeader header;
    header.type = MessageType::Data;
    header.id = generateMessageID();



    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    memcpy(buffer, &header, headerSize);
    memcpy(buffer + headerSize, message.data(), messageSize);

    int bytesSend = sendto(kSocketHandle, buffer, headerSize + messageSize, 0,
                           (sockaddr *)&kHost, sizeof(kHost));

    if(bytesSend > 0)
        kLastMessageID = header.id;

    return bytesSend;
}

int KUDPClientHandler::read(string &message) {
    if(kSocketHandle < 0) {
        cout << "UDP socket is invalid!" << endl;
        return -1;
    }
    
    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    int headerSize = sizeof(KUDPMessageHeader);

    while(true) {
        int bytesRead = recvfrom(kSocketHandle, buffer, sizeof(buffer), 0,
                                 nullptr, nullptr);

        if(bytesRead < 0) { //check server
            //create ack
            KUDPMessageHeader ackHeader1;
            ackHeader1.type = MessageType::Ack;
            ackHeader1.id = generateMessageID();

            time_t time = 0;
            ctime(&time);
            ackHeader1.id = time;

            int messageSize = sizeof(kLastMessageID);

            memcpy(buffer, &ackHeader1, headerSize);
            memcpy(buffer + headerSize, &kLastMessageID, messageSize);

            //send ack
            if(sendto(kSocketHandle, buffer, headerSize + messageSize, 0,
                      (sockaddr *)&kHost, sizeof(kHost)) <= 0) {
                return -1;
            }

            //check server ack
            while(true) {
                bytesRead = recvfrom(kSocketHandle, buffer, sizeof(buffer), 0,
                                     nullptr, nullptr);

                if(bytesRead < 0) {
                    cout << "Server not responding!" << endl;
                    return -1;
                } else {
                    KUDPMessageHeader *ackHeader2 = (KUDPMessageHeader*)&buffer;
                    if(ackHeader2->type != MessageType::Ack) {
                        continue;
                    }

                    if(kDatagrammIDs.find(ackHeader2->id) != kDatagrammIDs.end()) {
                        cout << "Already got this message!" << endl;
                        continue;
                    }

                    if(kDatagrammIDs.size() >= DATAGRAMM_LIMIT)
                        kDatagrammIDs.erase(kDatagrammIDs.begin());
                    kDatagrammIDs.insert(ackHeader2->id);

                    char state = buffer[headerSize + sizeof(uint64_t)];
                    if(state == 0x00) {
                        cout << "Message not received by server!" << endl;
                        return -1;
                    } else {
                        cout << "Message received by server and lost in network!" << endl;
                        return -1;
                    }
                }
            }
        } else if(bytesRead == 0) {
            return bytesRead;
        } else if(bytesRead < headerSize) {
            cout << "Invalid message header size!" << endl;
            return -2;
        } else {
            //check header
            KUDPMessageHeader *header = (KUDPMessageHeader*)&buffer;
            if(header->type != MessageType::Data) {
                cout << "Invalid message type!" << endl;
                return -2;
            }

            if(kDatagrammIDs.find(header->id) != kDatagrammIDs.end()) {
                cout << "Already got this message!" << endl;
                continue;
            }

            if(kDatagrammIDs.size() >= DATAGRAMM_LIMIT)
                kDatagrammIDs.erase(kDatagrammIDs.begin());
            kDatagrammIDs.insert(header->id);

            message = (buffer + headerSize);
            return bytesRead;
        }

    }
}

uint64_t KUDPClientHandler::generateMessageID () {
    uint64_t id = chrono::system_clock::to_time_t(chrono::system_clock::now());
    id ^= 1 + rand() % numeric_limits<uint64_t>::max();
    return id;
}
