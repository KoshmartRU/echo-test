#ifndef KUDPCLIENTHANDLER_H
#define KUDPCLIENTHANDLER_H

#include "kabstractclienthandler.h"

#include <set>

class KUDPClientHandler : public KAbstractClientHandler {
public:
    struct Header {
        uint16_t type;
        uint64_t id;
    };

    KUDPClientHandler();

    bool connectToHost(const string &host, int port) override;

    int write(const string &message) override;
    int read(string &message) override;

private:
    set<uint64_t> kDatagrammIDs;

    int kMessageCount;
};

#endif // KUDPCLIENTHANDLER_H
