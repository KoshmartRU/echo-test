#ifndef KGLOBALDEFS_H
#define KGLOBALDEFS_H

#define SERVER_PORT 4753

#define MESSAGE_SIZE_LIMIT 1024

enum MessageType {
    Data = 0x5301,
    Ack = 0x4946
};

#endif // KGLOBALDEFS_H
