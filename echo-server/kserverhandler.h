#ifndef KSERVERHANDLER_H
#define KSERVERHANDLER_H

#include "../common/kabstractnetworkhandler.h"

#include <string>
#include <set>

using namespace std;

class KServerHandler : public KAbstractNetworkHandler {
public:
    KServerHandler();
    ~KServerHandler();

    bool initialize(int port);

    void listen();

    //void stop(); TODO

private:
    bool kIsInitialized;

    struct sockaddr_in kHost;

    int kTCPListenerHandle;
    int kUDPListenerHandle;
    set<int> kTCPClientHandles;

    timeval kSendTimeout;
    timeval kRecvTimeout;
};

#endif // KSERVERHANDLER_H
