#include "ktcpclienthandler.h"

#include <sys/socket.h>
#include <string.h>
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
        cout << "TCP socket creation failed! " << endl;
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


int KTCPClientHandler::write(const string &message) {
    if(kSocketHandle < 0) {
        cout << "TCP connection not established!" << endl;
        return -1;
    }

    int headerSize = sizeof(KTCPMessageHeader);
    int messageSize = message.length();

    if (message.size() == 0 || messageSize > (MESSAGE_SIZE_LIMIT - headerSize)) {
        cout << "Message size is invalid!" << endl;
        return -2;
    }

    KTCPMessageHeader header;
    header.type = MessageType::Data;
    header.size = messageSize;

    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    memcpy(buffer, &header, headerSize);
    memcpy(buffer + headerSize, message.data(), messageSize);

    return sendAll(kSocketHandle, buffer, headerSize + messageSize, 0);
}

int KTCPClientHandler::read(string &message) {
    if(kSocketHandle < 0) {
        cout << "TCP connection not established!" << endl;
        return -1;
    }

    char buffer[MESSAGE_SIZE_LIMIT] = {0};
    int bytesRead = 0;
    int headerSize = sizeof(KTCPMessageHeader);

    bytesRead = recvAll(kSocketHandle, buffer, headerSize, 0);

    if(bytesRead <= 0)
        return bytesRead;

    //check header
    KTCPMessageHeader *header = (KTCPMessageHeader*)&buffer;
    if(header->type != MessageType::Data) {
        cout << "Message type is invalid" << endl;
        return -2;
    }
    if(header->size > MESSAGE_SIZE_LIMIT) {
        cout << "Message size is too large" << endl;
        return -2;
    }

    bytesRead = recvAll(kSocketHandle, buffer + headerSize, header->size , 0);

    if(bytesRead > 0) {
        message = (buffer + headerSize);
        return bytesRead + headerSize;
    } else {
        return bytesRead;
    }
}
