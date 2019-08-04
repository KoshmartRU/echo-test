#include <iostream>

#include "kserverhandler.h"

#include "../common/kglobaldefs.h"

int main() {
    KServerHandler handler;
    if(handler.initialize(SERVER_PORT)) {
        cout << "Server started" <<  endl;
        handler.listen();
    }

    return 0;
}
