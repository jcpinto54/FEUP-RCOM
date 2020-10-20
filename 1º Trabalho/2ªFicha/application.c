#include "application.h"


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
        if (read(app.fd, &aux, 1) == 0);
        printString(&aux);
        printString(frame->bytes);
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
    prepareToReceive(&setFrame, 5); prepareToReceive(&receiverFrame, 5);
    switch(appStatus) {
        case TRANSMITTER:
            buildSETFrame(&setFrame, true);
            printf("aaa\n");
            writeFrame(setFrame);
            frame_t responseFrame;
            printf("bbb\n");
            readFrame(&responseFrame);
            if (!bccVerifier(responseFrame.bytes, 1, 2, responseFrame.bytes[2])) {
                perror("bcc doesn't match in response");
                exit(2);   
            }

            printf("Done, Transmitter Out!\n");
        break;
        case RECEIVER:
            printf("aaa\n");
            readFrame(&receiverFrame);
            if (!bccVerifier(receiverFrame.bytes, 1, 2, receiverFrame.bytes[2])) {
                perror("bcc doesn't match in receiver");
                exit(2);   
            }
            frame_t uaFrame;
            buildUAFrame(&uaFrame, true);
            printf("bbb\n");
            writeFrame(uaFrame);
            
            printf("Done, Receiver Out!\n");
        break;
    }
}




// void receiveUA()
// {
//   unsigned char c;
//   int state = 0;

//   while(state < 5)
//   {
//     read(app.fd, &c, 1);
//     printf("Char: %c - State: %d\n", c, state);
//     switch(state)
//     {
//       case 0:
//         if(c == UA[0])
//           state = 1;
//         break;
//       case 1:
//         if(c == UA[1])
//           state = 2;
//         else if(c != UA[0])
//           state = 0;
//         break;
//       case 2:
//         if(c == UA[2])
//           state = 3;
//         else if(c == UA[0])
//           state = 1;
//         else
//           state = 0;
//         break;
//       case 3:
//         if(c == UA[3])
//           state = 4;
//         else if(c == UA[0])
//           state = 1;
//         else
//           state = 0;
//         break;
//       case 4:
//         if(c == UA[4])
//           state = 5;
//         else if(c == UA[0])
//           state = 1;
//         else
//           state = 0; 
//         break;
//     }
//   }
// }

// void sendSET()
// {
//   if(attempts >= MAX_ATTEMPTS)
//   {
//     perror("Too many failed attempts to send. Time out!\n");
//     exit(-1);
//   }

//   printf("Attempt %d\n", attempts);
//   int sentBytes = 0;
//   while(sentBytes != 5)
//   {
//     sentBytes = write(fd, SET, 5);
//     printf("%d bytes sent\n", sentBytes);
//   }
//   attempts++;
// }

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

void printString(char * str) {
  printf("\nStarting printString...\n\tSize: %ld\n",strlen(str));
  for (int i = 0; i < strlen(str); i++) {
    printf("\tstr[%d]: %c\n", i, str[i]);
  }
  printf("printString ended\n");
}

int prepareToReceive(frame_t *frame, size_t size) {
    frame->size = size;
    return (frame->bytes = malloc(frame->size)) == NULL;
}

