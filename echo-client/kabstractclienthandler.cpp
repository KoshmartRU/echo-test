#include "kabstractclienthandler.h"

#include <unistd.h>
#include <iostream>

KAbstractClientHandler::KAbstractClientHandler() :
    KAbstractNetworkHandler(),
    kSocketHandle(-1)
{}

KAbstractClientHandler::~KAbstractClientHandler() {
    if(kSocketHandle >= 0)
        close(kSocketHandle);
}

void KAbstractClientHandler::disconnectFromHost() {
    if(kSocketHandle >= 0)
        close(kSocketHandle);

    kSocketHandle = -1;
}

bool KAbstractClientHandler::validateHost(string hostAddress, int port) {
    if(inet_pton(AF_INET, hostAddress.data(), &kHost.sin_addr) <= 0) {
        cout << "Host address is invalid!" << endl;
        return false;
    } else {
        kHost.sin_port = htons(port);
        return true;
    }
}
