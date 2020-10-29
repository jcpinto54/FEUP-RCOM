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

#define ESC 0x7d
#define FLAG_STUFFING 0x5e
#define ESC_STUFFING 0x5d

#define RECEIVER 0
#define TRANSMITTER 1
#define SERIAL_PORT_1 "/dev/ttyS1"
#define SERIAL_PORT_2 "/dev/ttyS0"

#define MAX_WRITE_ATTEMPTS 5
#define TIMEOUT 3.0

#define MAX_FRAME_SIZE 255
#define MAX_FRAME_RETRANSMISSIONS 3
