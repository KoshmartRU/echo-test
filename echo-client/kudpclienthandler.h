#ifndef KUDPCLIENTHANDLER_H
#define KUDPCLIENTHANDLER_H

#include "kabstractclienthandler.h"

#include <set>

class KUDPClientHandler : public KAbstractClientHandler {
public:
    KUDPClientHandler();

    bool connectToHost(const string &host, int port) override;

    int write(const string &message) override;
    int read(string &message) override;

private:
    uint64_t kLastMessageID;
    set<uint64_t> kDatagrammIDs;

    uint64_t generateMessageID ();
};

#endif // KUDPCLIENTHANDLER_H
