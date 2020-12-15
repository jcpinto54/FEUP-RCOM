#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define DOWNLOAD_BUFFER_MAX_SIZE 1024
#define BACKLOG_SIZE 10


void write_file(int sockfd){
    int n;
    FILE *fp;
    char *filename = "file2.txt";
    char buffer;

    fp = open(filename, "w");
    if(fp == NULL){
        perror("Error creating file...");
        return -1;
    }

    while(1){
        n = recv(sockfd, buffer, SIZE, 0);
        if(n <= 0){
            break;;
        }
        fprintf(fp, "%s", buffer);
    }
    return ;
}


/*
struct sockaddr_in {
    sa_family_t    sin_family; // address family: AF_INET
    in_port_t      sin_port;   // port in network byte order
    struct in_addr sin_addr;   // internet address
};
*/

int main(){
    char *ip = "127.0.0.1"; //ip especial que representa o local host INADDR_LOOPBACK
    int port = 8080; //Vai depender do ip

    int socketfd;
    struct sockaddr_in server_addr;
    socklen_t addr_size;
    char downloadBuffer[DOWNLOAD_BUFFER_MAX_SIZE];

    /*
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    raw_socket = socket(AF_INET, SOCK_RAW, protocol);*/

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        perror("Error in socket creation");
        return socketfd;
    }

    printf("server socket was created...\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    //a função bind nomeia o socket; o socket embora que criado não tem nenhum ip associado

    int error = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(error < 0 ){
        perror("Error in binding socket");
        return error;
    }

    printf("named socket...\n");

    //a função listen marca o socket "socketfd" como passivo, ou seja, este socket vai ser usado para aceitar pedidos de conexão usando o accept(2)

    error = listen(socketfd, BACKLOG_SIZE);

    if(error < 0){
        perror("Error in listening process...");
        return error;
    }

    struct sockaddr_in new_addr;

    addr_size = sizeof(new_addr);
    int new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);

    write_file(new_sock);
    printf("Data received and written in text file.\n");

}
