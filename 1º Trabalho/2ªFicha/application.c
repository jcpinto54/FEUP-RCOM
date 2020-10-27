#include "application.h"

void llopen(char *port, int appStatus)
{
    struct termios oldtio, newtio;

    app.fd = open(port, O_RDWR | O_NOCTTY);
    if (app.fd < 0) {
        perror(port);
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

    tcsetattr(app.fd, TCSANOW, &newtio);

    // ---

    switch (appStatus) {
        case TRANSMITTER:
            ;
            frame_t setFrame;
            buildSETFrame(&setFrame, true);
            sendMessage(setFrame);

            frame_t responseFrame;
            prepareToReceive(&responseFrame, 5);
            receiveNotIMessage(&responseFrame);
            if (!bccVerifier(responseFrame.bytes, 1, 2, responseFrame.bytes[2]))
            {
                perror("bcc doesn't match in response");
                exit(2);
            }

            printf("Done, Transmitter Ready\n");
            break;
        case RECEIVER:
            ;
            frame_t receiverFrame;
            prepareToReceive(&receiverFrame, 5);
            receiveNotIMessage(&receiverFrame);
            if (!bccVerifier(receiverFrame.bytes, 1, 2, receiverFrame.bytes[2]))
            {
                perror("bcc doesn't match in receiver");
                exit(2);
            }
            frame_t uaFrame;
            buildUAFrame(&uaFrame, true);
            sendMessage(uaFrame);

            printf("Done, Receiver Ready\n");
            break;
    }
}

int llwrite(int fd, char * buffer, int length)
{
    int res;
    res = write(fd, buffer, length);   
    printf("%d bytes written\n", res);

    for(int i = 0; i < res; i++)
    {
      printf("%d: %c\n", buffer[i], buffer[i]); 
    }

    char receive[length];
    strcpy(receive, "");

    char* buf;

    while(STOP == false) {       /* loop for input */
      res = read(fd,buf,5);      /* returns after 5 chars have been input */
      buf[res]=0;                /* so we can printf... */
      printf(":%s:%d\n", buf, res);
      if (buf[0]=='\n') 
      {
        STOP = true;
        printf("\n");
      }
      receive[length] = buf[0];
    }

    printf("sent: %s", send);
    printf("received: %s\n", receive);

    if(strcmp(send, receive) != 0)
    {
      perror("Message corrupted!\n");
      return -1;
    }
 
    printf("Sucess!\n");
    return length;

}

int prepareI(char* data, int size)
{
    frame_t info = malloc(sizeof(u_int8_t) * (4 + size + 2));

    info->size = size;

    info->bytes[0] = FLAG; //F
    info->bytes[1] = TRANSMITTER_TO_RECEIVER; //A
    info->bytes[2] = 0; //C: ID da trama, suposto mudar depois
    info->bytes[3] = bccCalculator(info->bytes, 1, 2); //BCC1, calculado com A e C
}

void receiveNotIMessage(frame_t *frame)
{
    unsigned char c;
    receive_state_t state = INIT;

    while (1)
    {
        read(app.fd, &c, 1);
        printf("Char: %c - State: %d\n", c, state);
        switch (state)
        {
        case INIT:
            if (c == FLAG) {
                state = RCV_FLAG;
                frame->bytes[0] = c;
            }
            break;
        case RCV_FLAG:
            if (c == TRANSMITTER_TO_RECEIVER || c == RECEIVER_TO_TRANSMITTER) {
                state = RCV_A;
                frame->bytes[1] = c;
            }
            else
                state = INIT;
            break;
        case RCV_A:
            if (c == SET || c == UA || c == DISC || c == RR || c == REJ) {
                state = RCV_C;
                frame->bytes[2] = c;
            }
            else if (c == FLAG)
                state = RCV_FLAG;
            else
                state = INIT;
            break;
        case RCV_C:
            if (c == bccCalculator(frame->bytes, 1, 2)) {
                state = RCV_BCC;
                frame->bytes[3] = c;
            }
            else if (c == FLAG)
                state = RCV_FLAG;
            else
                state = INIT;
            break;
        case RCV_BCC:
            if (c == FLAG) {
                state = RCV_FLAG;
                frame->bytes[4] = c;
            }
            else if (c == FLAG)
                state = COMPLETE;
            else
                state = INIT;
            break;
        case COMPLETE:
            break;
        }
        
        if (state == COMPLETE) break;
    }
}

void sendMessage(frame_t frame) {
    int attempts = 0;
    int sentBytes = 0;
    while (sentBytes != frame.size) {
        if (attempts >= MAX_ATTEMPTS) {
            perror("Too many failed attempts to send. Time out!\n");
            exit(-1);
        }

        printf("Write attempt %d\n", attempts);
        while (sentBytes != frame.size) {
            sentBytes += write(app.fd, frame.bytes, frame.size);
            printf("%d bytes sent\n", sentBytes);
        }
        attempts++;
    }
}

// ---

u_int8_t getBit(u_int8_t byte, u_int8_t bit)
{
    return (byte >> bit) & BIT(0);
}

u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length)
{
    int onesCounter = 0;
    for (int i = start; i < start + length; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            onesCounter += getBit(bytes[i], j);
        }
    }
    return onesCounter % 2;
}

bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity)
{
    if (bccCalculator(bytes, start, length) == parity)
        return true;
    return false;
}

void buildSETFrame(frame_t *frame, bool transmitterToReceiver)
{
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

void buildUAFrame(frame_t *frame, bool transmitterToReceiver)
{
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

void destroyFrame(frame_t *frame)
{
    free(frame->bytes);
}

void printString(char *str)
{
    printf("\nStarting printString...\n\tSize: %ld\n", strlen(str));
    for (int i = 0; i < strlen(str); i++)
    {
        printf("\tstr[%d]: %c\n", i, str[i]);
    }
    printf("printString ended\n");
}

int prepareToReceive(frame_t *frame, size_t size)
{
    frame->size = size;
    return (frame->bytes = malloc(frame->size)) == NULL;
}
