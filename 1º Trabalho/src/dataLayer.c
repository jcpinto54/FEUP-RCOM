#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "dataLayer.h"
#include "dataLayerPrivate.h"
extern applicationLayer application;

int llopen(char *port, int appStatus)
{
    printf("ENTERED LLOPEN\n");
    
    struct termios oldtio, newtio;

    application.fd = open(port, O_RDWR | O_NOCTTY);
    if (application.fd < 0) {
        perror(port);
        return -1;
    }

    if (tcgetattr(application.fd, &oldtio) == -1) {
        perror("tcgetattr");
        return -2;
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; // time to time-out in deciseconds
    newtio.c_cc[VMIN] = 0;  // min number of chars to read

    if (tcsetattr(application.fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return -3;
    }

    frame_t setFrame;
    frame_t responseFrame;
    frame_t receiverFrame;
    frame_t uaFrame;

    switch (appStatus) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("Max Number of retransmissions reached. Exiting program.\n");
                    exit(1);
                }

                buildSETFrame(&setFrame, true);
                if (sendMessage(setFrame)) return -5;

                prepareToReceive(&responseFrame, 5);
                int responseReceive = receiveNotIMessage(&responseFrame); 
                if (responseReceive == 1 || responseReceive == 2) continue;         // in a timeout or wrong bcc, retransmit frame
                else if (responseReceive > 2) return -7;
                if (!isUAFrame(&responseFrame)) continue;       // wrong frame received

                printf("Done, Transmitter Ready\n");
                break;
            }
            break;
        case RECEIVER:;
            prepareToReceive(&receiverFrame, 5);
            if (receiveNotIMessage(&receiverFrame)) return -7;
            if (!isSETFrame(&receiverFrame)) return -8;

            buildUAFrame(&uaFrame, true);
            if (sendMessage(uaFrame)) return -5;
            
            printf("Done, Receiver Ready\n");
            break;
    }
    return application.fd;
}

int llclose(int fd) {
    printf("ENTERED LLCLOSE\n");

    frame_t discFrame;
    frame_t receiveFrame;
    frame_t uaFrame;

    int receiveReturn;
    switch (application.status) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("Max Number of retransmissions reached. Exiting program.\n");
                    exit(1);
                }

                buildDISCFrame(&discFrame, true);
                if (sendMessage(discFrame)) return -2;

                prepareToReceive(&receiveFrame, 5);
                receiveReturn = receiveNotIMessage(&receiveFrame);
                if (receiveReturn == 1 || receiveReturn == 2) continue;        //in a timeout or wrong bcc, retransmit frame
                else if (receiveReturn > 2) return -4;
                if (!isDISCFrame(&receiveFrame)) continue;      // wrong frame received

                buildUAFrame(&uaFrame, true);
                if (sendMessage(uaFrame)) return -2;
                
                printf("Done, Transmitter Out\n");
                break;
            }
        break;
        case RECEIVER:;
            prepareToReceive(&receiveFrame, 5);
            if (receiveNotIMessage(&receiveFrame)) return -7;
            if (!isDISCFrame(&receiveFrame)) return -5;

            buildDISCFrame(&discFrame, true);
            if (sendMessage(discFrame)) return -2;

            prepareToReceive(&receiveFrame, 5);
            if (receiveNotIMessage(&receiveFrame)) return -4;
            if (!isUAFrame(&receiveFrame)) return -5;

            printf("Done, Receiver Out\n");
        break;
    }
    if (close(fd) == -1) return -8;
    return 1;
}

int llread(int fd, char * buffer){
    frame_t frame;
    //bool response;
    int size;
    if ((size = receiveIMessage(&frame)) < 0) {
        perror("Error in receiveIMessage");
        return -1;
    }
    // checkIMessage(&frame);
    // sendResponse(response);

    return size;
}

int llwrite(int fd, char * buffer, int length)
{
    // frame_t **info = NULL, *responseFrame = NULL;
    // int framesToSend = prepareI(buffer, length, info); //Prepara a trama de informação
    // bool stop = false;
    // int attempts = 0, result = -1;
    
    // do
    // {
    //     if(attempts >= MAX_WRITE_ATTEMPTS) 
    //     {
    //         perror("ERROR: Too many write attempts\n");
    //         stop = true;
    //     }

    //     // sendMessage(*info);                  
    //     receiveNotIMessage(responseFrame);

    //     if(responseFrame->bytes[2] == RR) 
    //     {
    //         // result = info->size;
    //         stop = true;
    //     }
    //     else if(responseFrame->bytes[2] == REJ)
    //     {
    //         attempts++;
    //     }
    //     else
    //     {
    //         perror("ERROR: Unknown response frame");
    //     }
    // }while(!stop);

    // destroyFrame(info);
    // destroyFrame(responseFrame);
    
    // return result;
    return 0;
}
