#ifndef KGLOBALDEFS_H
#define KGLOBALDEFS_H

#define SERVER_PORT 4753

#define MESSAGE_SIZE_LIMIT 1024

#define DATAGRAMM_LIMIT 100

#include <stdint.h>

enum MessageType {
    Data = 0x5301,
    Ack = 0x4946
};

struct KTCPMessageHeader {
    uint16_t type;
    uint16_t size;
};

struct KUDPMessageHeader {
    uint16_t type;
    uint64_t id;
};

#endif // KGLOBALDEFS_H
