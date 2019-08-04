#include "kudpclienthandler.h"

#include <sys/socket.h>

#include <iostream>

#include "../common/kglobaldefs.h"

KUDPClientHandler::KUDPClientHandler() :
    KAbstractClientHandler()
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


int KUDPClientHandler::write(const string &message) const {
    if(kSocketHandle < 0) {
        cout << "UDP socket is invalid!" << endl;
        return -1;
    }

    if (message.size() == 0) {
        cout << "Message is invalid!" << endl;
        return -1;
    }

    return sendAllTo(kSocketHandle, message.data(), message.length(), 0,
                     (sockaddr *)&kHost, sizeof(kHost));
}

int KUDPClientHandler::read(string &message) const {
    if(kSocketHandle < 0) {
        cout << "UDP socket is invalid!" << endl;
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = recvAllFrom(kSocketHandle, buffer, sizeof(buffer), 0,
            nullptr, nullptr);

    if(bytesRead > 0)
        message = buffer;

    return bytesRead;
}
