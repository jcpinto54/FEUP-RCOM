#include "application.h"
#include "2ndAssignment.h"


applicationLayer app;



int main(int argc, char *argv[])
{
    printf("111\n");

    struct termios oldtio, newtio;

    if ((argc < 3)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }

    if ((strcmp("-r", argv[1]) != 0) && (strcmp("-s", argv[1]) != 0)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }
    
    if ((strcmp("/dev/ttyS0", argv[2]) != 0) && (strcmp("/dev/ttyS1", argv[2]) != 0)) {
        printf("Usage:\t %s -r/-s serialPort\n\t\tex: %s -s /dev/ttyS1\n", argv[0], argv[0]);
        exit(1);
    }
    
    if (strcmp("-s", argv[1])== 0) app.status = TRANSMITTER;
    if (strcmp("-r", argv[1])== 0) app.status = RECEIVER;
    

    printf("111\n");
    app.fd = open(argv[2], O_RDWR | O_NOCTTY);
    if (app.fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(app.fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    printf("111\n");

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // set input mode (non-canonical, no echo,...) 
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 30; // time to time-out in deciseconds
    newtio.c_cc[VMIN] = 5;  // min number of chars to read

    printf("111\n");
    llopen(COM1, app.status);
    
}

