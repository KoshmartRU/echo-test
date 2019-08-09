#ifndef KTCPCLIENTHANDLER_H
#define KTCPCLIENTHANDLER_H

#include "kabstractclienthandler.h"
#include <stdint.h>

class KTCPClientHandler : public KAbstractClientHandler {
public:
    struct Header {
        uint16_t type;
        uint16_t size;
    };

    KTCPClientHandler();

    bool connectToHost(const string &host, int port) override;

    int write(const string &message) override;
    int read(string &message) override;
};

#endif // KTCPCLIENTHANDLER_H
