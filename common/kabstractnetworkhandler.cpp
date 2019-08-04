#include "kabstractnetworkhandler.h"

#include "../common/kglobaldefs.h"

KAbstractNetworkHandler::KAbstractNetworkHandler() :
    kIsInitialized(false)
{
    kHost.sin_family = AF_INET;

    kSendTimeout.tv_sec = 0;
    kSendTimeout.tv_usec = SEND_TIMEOUT;

    kRecvTimeout.tv_sec = 0;
    kRecvTimeout.tv_usec = RECV_TIMEOUT;
}

int KAbstractNetworkHandler::sendAll(int socketHandle, const char *buffer, int bufferLength, int flags) const {
    int total = 0;
    int bytesSend;

    while(total < bufferLength) {
        bytesSend = send(socketHandle, buffer + total, bufferLength - total, flags);
        if(bytesSend == -1)
            break;

        total += bytesSend;
    }

    return (bytesSend == -1 ? -1 : total);
}

int KAbstractNetworkHandler::sendAllTo(int socketHandle, const char *buffer, int bufferLength, int flags,
                                       sockaddr * addr, int addrLength) const {
    int total = 0;
    int bytesSend;

    while(total < bufferLength) {
        bytesSend = sendto(socketHandle, buffer + total, bufferLength - total, flags, addr, addrLength);
        if(bytesSend == -1)
            break;

        total += bytesSend;
    }

    return (bytesSend == -1 ? -1 : total);
}

int KAbstractNetworkHandler::recvAll(int socketHandle, char *buffer, int bufferLength, int flags) const {
    int total = 0;
    int bytesRead;

    while(total < bufferLength) {
        bytesRead = recv(socketHandle, buffer + total, bufferLength - total, flags);
        if(bytesRead <= 0)
            break;

        total += bytesRead;
    }

    return (total >= 0 ? total : -1);
}

int KAbstractNetworkHandler::recvAllFrom(int socketHandle, char *buffer, int bufferLength, int flags,
                                         sockaddr *addr, int *addrLength) const {
    int total = 0;
    int bytesRead;

    while(total < bufferLength) {
        bytesRead = recvfrom(socketHandle, buffer + total, bufferLength - total, flags, addr, (socklen_t*)addrLength);
        if(bytesRead <= 0)
            break;

        total += bytesRead;
    }

    if(total >= 0)
        bytesRead = total;

    return (total >= 0 ? total : -1);
}
