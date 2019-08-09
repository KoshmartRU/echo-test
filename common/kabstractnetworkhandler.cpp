#include "kabstractnetworkhandler.h"

#include "../common/kglobaldefs.h"

KAbstractNetworkHandler::KAbstractNetworkHandler() :
    kIsInitialized(false)
{
    kHost.sin_family = AF_INET;

    kSendTimeout.tv_sec = 1;
    kSendTimeout.tv_usec = 0;

    kRecvTimeout.tv_sec = 1;
    kRecvTimeout.tv_usec = 0;
}

int KAbstractNetworkHandler::sendAll(int socketHandle, const char *buffer, int bufferLength, int flags) const {
    int total = 0;
    int bytesSend;

    while(total < bufferLength) {
        bytesSend = send(socketHandle, buffer + total, bufferLength - total, flags);
        if(bytesSend < 0)
            break;

        total += bytesSend;
    }

    return (bytesSend < 0 ? bytesSend : total);
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

    return (total >= 0 ? total : bytesRead);
}
