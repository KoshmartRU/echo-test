#ifndef KSERVERHANDLER_H
#define KSERVERHANDLER_H

#include "../common/kabstractnetworkhandler.h"

#include <string.h>
#include <map>
#include <set>

#include "../common/kglobaldefs.h"

using namespace std;

class KServerHandler : public KAbstractNetworkHandler {
public:
    struct ClientContext {
        enum State {
            Read = 0,
            Write
        };

        ClientContext(sockaddr_in host) :
            host(host),
            state(Read),
            totalSize(0),
            currentSize(0),
            lastActivityTime(0)
        {
            memset(buffer, 0x00, sizeof(buffer));
        }

        sockaddr_in  host;
        int state; //ClientContext::State
        char buffer[MESSAGE_SIZE_LIMIT];
        int totalSize;
        //tcp
        int currentSize;
        //udp
        set<uint64_t> datagrammIDs;
        time_t lastActivityTime;
    };

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

    map<uint32_t, ClientContext> kTCPClientContext; //<ip, context>
    map<uint32_t, ClientContext> kUDPClientContext; //<ip, context>

    void acceptTCPConnections(const fd_set &readSet);
    void readTCPMessages(const fd_set &readSet);
    void writeTCPMessages(const fd_set &writeSet);
    void readUDPMessages(const fd_set &readSet);
    void writeUDPMessages(const fd_set &writeSet);
    void checkUDPClientActivity();
};

#endif // KSERVERHANDLER_H
