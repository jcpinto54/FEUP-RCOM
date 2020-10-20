#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "2ndAssignment.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

applicationLayer app;

int writeFrame(frame_t frame);
int readFrame(frame_t *frame);
void llopen(int porta, int appStatus);

int main(int argc, char *argv[])
{
    struct termios oldtio, newtio;

    if ((argc < 3)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }

    if ((strcmp("r", argv[1]) != 0) && (strcmp("s", argv[1]) != 0)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }
    
    if ((strcmp("/dev/ttyS0", argv[2]) != 0) && (strcmp("/dev/ttyS1", argv[2]) != 0)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }
    
    if (strcmp("s", argv[1])== 0) app.status = TRANSMITTER;
    if (strcmp("r", argv[1])== 0) app.status = RECEIVER;
    

    app.fd = open(argv[2], O_RDWR | O_NOCTTY);
    if (app.fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(app.fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // set input mode (non-canonical, no echo,...) 
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 30; // time to time-out in deciseconds
    newtio.c_cc[VMIN] = 5;  // min number of chars to read


    llopen(COM1, app.status);
    
}

int writeFrame(frame_t frame) {
    int sizeWritten;
    if ((sizeWritten = write(app.fd, frame.bytes, frame.size)) < 0) {
        perror("write");
        exit(1);
    }  
    return sizeWritten;
}

int readFrame(frame_t *frame) {
    char aux;
    bool inPacket = false;
    while (1) {
        read(app.fd, &aux, 1);
        if (aux == FLAG) {
            if (inPacket) {
                if (frame->size == 4) {
                    frame->bytes[frame->size] = aux;
                    frame->size++;
                    inPacket = false;
                    break;
                }
                else if (frame->size > 4) {
                    perror("frame too long");
                    exit(2);
                }
                else {
                    continue;   // makes it impossible to process more than one frame at once
                }
            }
            else {
                inPacket = true;
            }
        }
        else if (!inPacket) continue;
    
        frame->bytes[frame->size] = aux;
        frame->size++;
    }
    return frame->size;
}


void llopen(int porta, int appStatus) {
    frame_t setFrame;
    frame_t receiverFrame;
    switch(appStatus) {
        case TRANSMITTER:
            buildSETFrame(&setFrame, true);
            writeFrame(setFrame);
            frame_t responseFrame;
            readFrame(&responseFrame);
            if (!bccVerifier(responseFrame.bytes, 1, 2, responseFrame.bytes[2])) {
                perror("bcc doesn't match in response");
                exit(2);   
            }

            printf("Done, Transmitter Out!\n");
        break;
        case RECEIVER:
            readFrame(&receiverFrame);
            if (!bccVerifier(receiverFrame.bytes, 1, 2, receiverFrame.bytes[2])) {
                perror("bcc doesn't match in receiver");
                exit(2);   
            }
            frame_t uaFrame;
            buildUAFrame(&uaFrame, true);
            writeFrame(uaFrame);
            
            printf("Done, Receiver Out!\n");
        break;
    }
}


// ---


u_int8_t getBit(u_int8_t byte, u_int8_t bit) {
    return (byte >> bit) & BIT(0);
}

u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length) {
    int onesCounter = 0;
    for (int i = start; i < start + length; i++) {
        for (int j = 0; j < 8; j++) {
            onesCounter += getBit(bytes[i], j);
        }
    }
    return onesCounter % 2;
}

bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity) {
    if (bccCalculator(bytes, start, length) == parity) return true;
    return false;
}

void buildSETFrame(frame_t *frame, bool transmitterToReceiver) {
    frame->bytes = malloc(5);
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else 
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = SET;
    frame->bytes[3] = 0;
    frame->bytes[4] = FLAG;
}

void buildUAFrame(frame_t * frame, bool transmitterToReceiver) {
    frame->bytes = malloc(5);
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else 
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = SET;
    frame->bytes[3] = 1;
    frame->bytes[4] = FLAG;
    
}

void destroyFrame(frame_t *frame) {
    free(frame->bytes);
}



