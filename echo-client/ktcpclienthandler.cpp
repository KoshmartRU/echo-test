#include "ktcpclienthandler.h"

#include <sys/socket.h>

#include <iostream>

#include "../common/kglobaldefs.h"

KTCPClientHandler::KTCPClientHandler() :
    KAbstractClientHandler()
{}

bool KTCPClientHandler::connectToHost(const string &host, int port) {
    if(kSocketHandle >= 0) {
        cout << "TCP connection is already established!" << endl;
        return false;
    }

    //validate host
    if(!validateHost(host, port))
        return false;

    //create socket
    kSocketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(kSocketHandle < 0) {
        cout << "TCP socket creation failed!" << endl;
        return false;
    }

    setsockopt(kSocketHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&kSendTimeout, sizeof(kSendTimeout));
    setsockopt(kSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&kRecvTimeout, sizeof(kRecvTimeout));

    //connect to server
    if (connect(kSocketHandle,
                (struct sockaddr *)&kHost,
                sizeof(kHost)) < 0) {
        kSocketHandle = -1;

        cout << "TCP connection to server failed!" << endl;
        return false;
    }

    cout << "Connected to TCP server: " << inet_ntoa(kHost.sin_addr) << endl;

    return true;
}


int KTCPClientHandler::write(const string &message) const {
    if(kSocketHandle < 0) {
        cout << "TCP connection not established!" << endl;
        return -1;
    }

    if (message.size() == 0) {
        cout << "Message is invalid!" << endl;
        return -1;
    }

    return sendAll(kSocketHandle, message.data(), message.length(), 0);
}

int KTCPClientHandler::read(string &message) const {
    if(kSocketHandle < 0) {
        cout << "TCP connection not established!" << endl;
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = recvAll(kSocketHandle, buffer, sizeof(buffer), 0);

    if(bytesRead > 0)
        message = buffer;

    return bytesRead;
}
