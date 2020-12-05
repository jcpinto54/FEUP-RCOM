#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define DOWNLOAD_BUFFER_MAX_SIZE 1024
#define BACKLOG_SIZE 10


void send_file(FILE *fp, int socketfd){
    char data[SIZE] = {0};

    while(fgets(data, SIZE, fp) != NULL){
        if(send(sockfd, data, sizeof(data), 0) < 0){
            perror("Error sending data...");
            return -1;
        }
        bzero(data, SIZE);
    }
}


int main(){
    char *ip = "127.0.0.1"; //ip especial que representa o local host INADDR_LOOPBACK
    int port = 8080; //Vai depender do ip

    int socketfd;

    struct sockaddr_in server_addr;
    File *fp;
    char *filename = "file1.txt";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        perror("Error in socket creation");
        return socketfd;
    }

    printf("server socket was created...\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    int error = connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if(error < 0){
        perror("Error connecting to server.");
        return -1;
    }

    printf("Connected to server.\n");

    fp = open(filename, "r");
    if(fp == NULL){
        perror("Error reading file...");
        return -1;
    }

    send_file(fp, socketfd);
    printf("Sent Data succefully.\n");
    close(sockfd);
    printf("Disconnected from the server.\n");

}