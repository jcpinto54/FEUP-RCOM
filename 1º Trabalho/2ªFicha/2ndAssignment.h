#pragma once

#define FLAG 0x7e

#define TRANSMITTER_TO_RECEIVER 0x03
#define RECEIVER_TO_TRANSMITTER 0x01

#define SET 0x03
#define DISC 0x0b
#define UA 0x07
// #define RR      0b R 0 0 0 0 1 0 1    // o que é o R? um octeto?



#ifndef BIT
    #define BIT(x) ((1 << x) -1)
#endif

typedef enum {
    false,
    true
} bool;

#define RECEIVER 0
#define TRANSMITTER 1
#define COM1 1 //??
#define COM2 2 //??

typedef struct {
    int fd; // serial port
    int status; // TRANSMITTER | RECEIVER
} applicationLayer;

typedef struct {
    u_int8_t *bytes;
    size_t size;
} frame_t;

u_int8_t getBit(u_int8_t byte, u_int8_t bit);
u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length);
bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity);

void buildSETFrame(frame_t *frame, bool transmitterToReceiver);
void buildUAFrame(frame_t * frame, bool transmitterToReceiver);
void destroyFrame(frame_t *frame);


