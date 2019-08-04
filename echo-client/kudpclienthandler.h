#ifndef KUDPCLIENTHANDLER_H
#define KUDPCLIENTHANDLER_H

#include "kabstractclienthandler.h"

class KUDPClientHandler : public KAbstractClientHandler {
public:
    KUDPClientHandler();

    bool connectToHost(const string &host, int port) override;

    int write(const string &message) const override;
    int read(string &message) const override;
};

#endif // KUDPCLIENTHANDLER_H
