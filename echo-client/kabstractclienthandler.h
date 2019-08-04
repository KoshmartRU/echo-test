#ifndef KABSTRACTCLIENTHANDLER_H
#define KABSTRACTCLIENTHANDLER_H

#include "../common/kabstractnetworkhandler.h"

#include <string>

using namespace std;

class KAbstractClientHandler : public KAbstractNetworkHandler {
public:
    KAbstractClientHandler();
    virtual ~KAbstractClientHandler();

    virtual bool connectToHost(const string &host, int port) = 0;
    virtual void disconnectFromHost() ;

    virtual int write(const string &message) const = 0;
    virtual int read(string &message) const = 0;

protected:
    int kSocketHandle;

    bool validateHost(string hostAddress, int port);
};

#endif // KABSTRACTCLIENTHANDLER_H
