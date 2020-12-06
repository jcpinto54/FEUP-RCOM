/*      (C)2000 FEUP  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "clientTCP.h"
#include "macros.h"


int interpretReplyCode(int code) {
	int lsd = code/100;
	if (lsd >= 4) {
		printf("Received bad code from ftp server. Exiting Program...\n");
		return FAILURE;
	}
	return OK;
}

int getReply(int sockfd) {
	FILE *sock_file = fdopen(sockfd, "r+");

	size_t bufSize= 2000;
	char *buf = malloc(bufSize);

	while(1) {
		bzero(buf, bufSize);
		getline(&buf, &bufSize, sock_file);
		if (strstr(buf, "-") == NULL) {
			int replyCode = atoi(buf);
			break;
		}
	}
}


int downloadFTPFile(url_t url) {

	int	sockfd;
	struct	sockaddr_in server_addr;
	// int	bytes;

	struct addrinfo hints, *res_addr;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;

	getaddrinfo(url.host, NULL, &hints, &res_addr);
	char ipv4[20];
	getnameinfo(res_addr->ai_addr, res_addr->ai_addrlen, ipv4, 20, NULL, 0, NI_NUMERICHOST);

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ipv4);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(21);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	return 1;
    	}
	/*connect to the server*/
    	if(connect(sockfd, 
	           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
        	perror("connect()");
			return 2;
	}



	int replyCode = getReply(sockfd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_READY) return FAILURE;

	close(sockfd);

	return OK;
}





