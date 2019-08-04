#ifndef KABSTRACTNETWORKHANDLER_H
#define KABSTRACTNETWORKHANDLER_H

#include <arpa/inet.h>

class KAbstractNetworkHandler {
public:
    KAbstractNetworkHandler();
    virtual ~KAbstractNetworkHandler() {}

protected:
    bool kIsInitialized;

    struct sockaddr_in kHost;

    timeval kSendTimeout;
    timeval kRecvTimeout;

    int sendAll(int socketHandle, const char *buffer, int bufferLength, int flags) const;
    int sendAllTo(int socketHandle, const char *buffer, int bufferLength, int flags,
                  sockaddr * addr, int addrLength) const;

    int recvAll(int socketHandle, char *buffer, int bufferLength, int flags) const;
    int recvAllFrom(int socketHandle, char *buffer, int bufferLength, int flags,
                    sockaddr * addr, int *addrLength) const;
};

#endif // KABSTRACTNETWORKHANDLER_H
