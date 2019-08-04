#include "ktcpclienthandler.h"
#include "kudpclienthandler.h"

#include <iostream>

#define SERVER_HOST "127.0.0.1"

#include "../common/kglobaldefs.h"

int main() {
    KAbstractClientHandler *handler;

    //initial
    handler = new KTCPClientHandler();
    if(handler->connectToHost(SERVER_HOST, SERVER_PORT))
        cout << "TCP mode initialized" << endl;
    cout << endl;

    //event loop
    string message;
    int messageSize;

    while(true) {
        //get message
        cout << "Enter the message: ";
        getline(cin, message);

        //parse message
        if(message == "/quit") { //quit event loop
            break;
        } else if(message == "/tcp") { //set tcp mode
            handler->disconnectFromHost();
            delete handler;

            handler = new KTCPClientHandler();
            if(handler->connectToHost(SERVER_HOST, SERVER_PORT))
                cout << "TCP mode initialized" << endl;
            cout << endl;

            continue;
        } else if(message == "/udp") { //set udp mode
            handler->disconnectFromHost();
            delete handler;

            handler = new KUDPClientHandler();
            if(handler->connectToHost(SERVER_HOST, SERVER_PORT))
                cout << "UDP mode initialized" << endl;

            cout << endl;

            continue;
        } else { //send message
            messageSize = handler->write(message);
            if(messageSize > 0) {
                cout << "Message send: " << message << " (" << messageSize << " bytes)" << endl;

                messageSize = handler->read(message);
                if(messageSize > 0)
                    cout << "Message received: " << message << " (" << messageSize << " bytes)" << endl;
                else
                    cout << "Message NOT received, timeout occured" << endl;
            }
            cout << endl;
        }
    }

    handler->disconnectFromHost();
    delete handler;

    return 0;
}
