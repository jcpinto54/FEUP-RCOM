#include <stdio.h>
#include "urlHandler.h"
#include "macros.h"
#include "clientTCP.h"
#include <string.h>



int main(int argc, char *argv[]){
	if (argc != 2){
		perror("Wrong number of arguments!\nUsage: ./download <FTP url>\n");
		return -1;
	}

	// Parses URL (TODO: handle some errors on the URL)
	url_t url = parseURL(argv[1]);

	// Prints information
	printf("Protocol: %s\n", url.protocol);
	printf("User: %s\n", url.username);
	if (strlen(url.password) != 0)
		printf("Password: %s\n", url.password);
	printf("Host: %s\n", url.host);
	printf("Path: %s\n", url.path);
	printf("Filename: %s\n", url.filename);

	if(url.success == FAILURE){
		perror("ERROR: There was a problem parsing the URL!\n");
		return -1;
	}

	if(strcmp(url.password, "") == 0){
		printf("Input your password: ");
		char str[256], c;
		int i = 0;
		while (i<256){
		    str[i]=getchar();
		    c=str[i];
		    if(c=='\n') break;
		    i++;
		}
		str[i]='\0';
		strcpy(url.password, str);
	}


	printf("\nPress enter to start download... (Ctrl-C to exit)\n");
	getc(stdin);
	int error = downloadFTPFile(url);
	if (error == FAILURE) {
		printf("There was an error downloading\n");
	}
	else if (error == OK) {
		printf("\nDownload Completed\n");
	}
	return OK;
}