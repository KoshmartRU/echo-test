#include "ktcpclienthandler.h"
#include "kudpclienthandler.h"

#include <iostream>
#include "../common/kglobaldefs.h"

#define SERVER_HOST "127.0.0.1"

int main() {
    KAbstractClientHandler *handler;
    string message;

    //set network mode
    cout << "Set network mode: ";
    getline(cin, message);

    //parse message
    if(message == "tcp") { //set tcp mode
        handler = new KTCPClientHandler();
        if(handler->connectToHost(SERVER_HOST, SERVER_PORT)) {
            cout << "TCP mode initialized" << endl << endl;
        } else {
            delete handler;
            return 0;
        }
    } else if(message == "udp") { //set udp mode
        handler = new KUDPClientHandler();
        if(handler->connectToHost(SERVER_HOST, SERVER_PORT)) {
            cout << "UDP mode initialized" << endl << endl;
        } else {
            delete handler;
            return 0;
        }
    } else {
        cout << "Mode is unknown" << endl;
        return 0;
    }

    //event loop
    int messageSize;

    while(true) {
        //get message
        cout << "Enter the message: ";
        getline(cin, message);

        //parse message
        if(message == "/quit") { //quit event loop
            break;
        } else { //send message
            messageSize = handler->write(message);
            if(messageSize > 0) {
                cout << "Message send: " << message << endl;

                messageSize = handler->read(message);
                if(messageSize > 0) {
                    cout << "Message received: " << message << endl;
                } else {
                    cout << "Message not received" << endl;
                    break;
                }
            }
            cout << endl;
        }
    }

    handler->disconnectFromHost();
    delete handler;

    return 0;
}
