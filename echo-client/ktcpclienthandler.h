#ifndef KTCPCLIENTHANDLER_H
#define KTCPCLIENTHANDLER_H

#include "kabstractclienthandler.h"

class KTCPClientHandler : public KAbstractClientHandler {
public:
    KTCPClientHandler();

    bool connectToHost(const string &host, int port) override;

    int write(const string &message) const override;
    int read(string &message) const override;
};

#endif // KTCPCLIENTHANDLER_H
