#include "kudpclienthandler.h"

#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <iostream>

#include "../common/kglobaldefs.h"

#define DATAGRAMM_LIMIT 100

KUDPClientHandler::KUDPClientHandler() :
    KAbstractClientHandler(),
    kMessageCount(0)
{}

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

    int headerSize = sizeof(Header);
    int messageSize = message.length();

    if (message.size() == 0 || messageSize > (MESSAGE_SIZE_LIMIT - headerSize)) {
        cout << "Message size is invalid!" << endl;
        return -2;
    }

    Header header;
    header.type = MessageType::Data;
    header.id = ++kMessageCount;

    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    memcpy(buffer, &header, headerSize);
    memcpy(buffer + headerSize, message.data(), messageSize);

    return sendto(kSocketHandle, buffer, headerSize + messageSize, 0,
                  (sockaddr *)&kHost, sizeof(kHost));
}

int KUDPClientHandler::read(string &message) {
    if(kSocketHandle < 0) {
        cout << "UDP socket is invalid!" << endl;
        return -1;
    }
    
    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    int headerSize = sizeof(Header);

    while(true) {
        int bytesRead = recvfrom(kSocketHandle, buffer, sizeof(buffer), 0,
                                 nullptr, nullptr);

        if(bytesRead < 0) { //check server
            /*Header checkHeader;
            checkHeader.type = MessageType::Ack;
            checkHeader.id = 0;

            time_t time = 0;
            ctime(&time);
            checkHeader.id = time;

            char buffer[MESSAGE_SIZE_LIMIT] = {0};
            memcpy(buffer, &checkHeader, headerSize);

            if(sendto(kSocketHandle, buffer, headerSize , 0,
                          (sockaddr *)&kHost, sizeof(kHost) <= 0))
                    return -1;

            bytesRead = recvfrom(kSocketHandle, buffer, sizeof(buffer), 0,
                                     nullptr, nullptr);

            */
            return bytesRead;
        } else if(bytesRead == 0) {
            return bytesRead;
        } else if(bytesRead < headerSize) {
            cout << "Invalid message header size!" << endl;
            return -2;
        } else {
            //check header
            Header *header = (Header*)&buffer;
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
