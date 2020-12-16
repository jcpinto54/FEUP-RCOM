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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "clientTCP.h"
#include "macros.h"


int interpretReplyCode(int code) {
	int lsd = code/100;
	if (lsd >= 4) {
		printf("Received bad code from ftp server. Exiting Program...\n");
		return FAILURE;
	}
	if (code < 0) {
		printf("A function in getReply or getPASVReply returned an error. Exiting Program...\nError code: %d\n", code);
		return FAILURE;
	}
	return OK;
}

int getReply(int sockfd) {
	FILE *sock_file = fdopen(sockfd, "r+");

	size_t bufSize = MAX_BUF_SIZE;
	char *buf = malloc(bufSize);
	int replyCode;
	while(1) {
		bzero(buf, bufSize);
		getline(&buf, &bufSize, sock_file);
		if (buf[3] != '-') {
			char codeBuf[4];
			bzero(codeBuf, 4);
			codeBuf[0] = buf[0];
			codeBuf[1] = buf[1];
			codeBuf[2] = buf[2];
			replyCode = atoi(codeBuf);
			break;
		}
	}

	free(buf);

	return replyCode;
}

int parseIPandPort(char *ipAndPort, char *ip, int *port) {
	bzero(ip, MAX_IP_SIZE);
	char *partIp = strtok(ipAndPort, ",");
	int i = 0;
	int msb_port;
	int lsb_port;
	while(partIp != NULL) {
		if (i <= 2) {
			strcat(ip, partIp);
			strcat(ip, ".");
		}
		else if (i == 3) {
			strcat(ip, partIp);
		}
		else if (i == 4) {
			msb_port = atoi(partIp);
		}
		else if (i == 5) {
			lsb_port = atoi(partIp);
		}
		partIp = strtok (NULL, ",");
		i++;
	}

	*port = 256 * msb_port + lsb_port;

	return OK;
}

int getPASVReply(int sockfd, char *ip, int *port) {
	FILE *sock_file = fdopen(sockfd, "r+");

	size_t bufSize = MAX_BUF_SIZE;
	char *buf = malloc(bufSize);
	int replyCode;
	while(1) {
		bzero(buf, bufSize);
		getline(&buf, &bufSize, sock_file);
		if (strstr(buf, "-") == NULL) {
			char codeBuf[4];
			bzero(codeBuf, 4);
			codeBuf[0] = buf[0];
			codeBuf[1] = buf[1];
			codeBuf[2] = buf[2];
			replyCode = atoi(codeBuf);
			
			char* ipStart = strstr(buf, "(");
			ipStart++;
			int ipStartSize = strlen(ipStart);
			ipStart[ipStartSize - 4] = 0;
			parseIPandPort(ipStart, ip, port);
			break;
		}
	}

	free(buf);
	return replyCode;
}

int sendCommand(char *cmd, char *arg, int sockfd) {
	char cmdWArg[MAX_CMD_SIZE];
	bzero(cmdWArg, MAX_CMD_SIZE);
	if (sprintf(cmdWArg, "%s %s\r\n", cmd, arg) < 0) {
		printf("write in sendCommand\n");
		return FAILURE;
	}
	if (write(sockfd, cmdWArg, strlen(cmdWArg)) < 0) {
		perror("write in sendCommand\n");
		return FAILURE;
	}
	return OK;
}

int openSocket(char *ip, int port) {
	int sockfd;
	struct	sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	return -1;
    	}
	/*connect to the server*/
    	if(connect(sockfd, 
	           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
        	perror("connect()");
			return -2;
	}
	return sockfd;

}




int readAndStoreFile(int sockfd, char *filename) {
	FILE *f = fopen(filename, "w");

	size_t bufSize = MAX_BUF_SIZE;
	char *buf = malloc(bufSize);
	int size;
	while (1) {
		bzero(buf, bufSize);
		if ((size = read(sockfd, buf, bufSize)) < 0) {
			perror("read in readAndStoreFile");
			return -1;
		}
		if (size == 0) break;
		if (fwrite(buf, size, 1, f) < 0) {
			perror("write in readAndStoreFile");
			return -2;
		}
	}

	fclose(f);
	free(buf);
	return OK;
}

int downloadFTPFile(url_t url) {
	
	struct addrinfo hints, *res_addr;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(url.host, NULL, &hints, &res_addr) != 0) return FAILURE;
	char ipv4[20];
	if (getnameinfo(res_addr->ai_addr, res_addr->ai_addrlen, ipv4, 20, NULL, 0, NI_NUMERICHOST) != 0) return FAILURE;

	
	int mainSockFd = openSocket(ipv4, 21);

	int replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_READY) return FAILURE;

	sendCommand("USER", url.username, mainSockFd);
	
	replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_NEEDS_PASS) return FAILURE;

	sendCommand("PASS", url.password, mainSockFd);

	replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_LOGGED_IN) return FAILURE;	

	sendCommand("TYPE", "I", mainSockFd);

	replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_BINARY_MODE) return FAILURE;	

	sendCommand("PASV", "", mainSockFd);

	char ipToGetFile[MAX_IP_SIZE];
	int portToGetFile;
	replyCode = getPASVReply(mainSockFd, ipToGetFile, &portToGetFile);
	if (interpretReplyCode(replyCode) || replyCode != SERV_PASSV_MODE) return FAILURE;		

	char fileWPath[2000];
	sprintf(fileWPath, "%s%s", url.path, url.filename);
	sendCommand("RETR", fileWPath, mainSockFd);

	int fileSockFd = openSocket(ipToGetFile, portToGetFile);

	replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_READY_TO_SEND_FILE) return FAILURE;	

	readAndStoreFile(fileSockFd, url.filename);

	replyCode = getReply(mainSockFd);
	if (interpretReplyCode(replyCode) || replyCode != SERV_CLOSE) return FAILURE;	

	close(mainSockFd);
	close(fileSockFd);

	return OK;
}





