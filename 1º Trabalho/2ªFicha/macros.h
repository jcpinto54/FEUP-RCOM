#pragma once

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FLAG 0x7e           // F

#define TRANSMITTER_TO_RECEIVER 0x03    // A
#define RECEIVER_TO_TRANSMITTER 0x01

#define SET 0x03        // C
#define DISC 0x0b
#define UA 0x07
#define RR 0x05  // 0b R 0 0 0 0 1 0 1
#define REJ 0x01 // 0b R 0 0 0 0 0 0 1

#define RECEIVER 0
#define TRANSMITTER 1
#define SERIAL_PORT_1 "/dev/ttyS1"
#define SERIAL_PORT_2 "/dev/ttyS0"

#define MAX_ATTEMPTS 5

#ifndef BIT
    #define BIT(x) ((1 << x) -1)
#endif

typedef enum {
    false,
    true
} bool;


typedef struct {
    int fd; // serial port
    int status; // TRANSMITTER | RECEIVER
} applicationLayer;


typedef enum {
    INIT,
    RCV_FLAG,
    RCV_A,
    RCV_C,
    RCV_BCC,
    COMPLETE
} receive_state_t;

typedef struct {
    u_int8_t *bytes;
    int size;
} frame_t;
