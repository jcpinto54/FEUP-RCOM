#include <stdio.h>
#include "urlHandler.h"
#include "macros.h"
#include "clientTCP.h"

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
	printf("Password: %s\n", url.password);
	printf("Host: %s\n", url.host);
	printf("Path: %s\n", url.path);
	printf("Filename: %s\n", url.filename);

	if(url.success == FAILURE){
		perror("ERROR: There was a problem parsing the URL!\n");
		return -1;
	}

	printf("Enter to start download... (Ctrl-C to exit)\n");
	getc(stdin);
	int error = downloadFTPFile(url);
	if (error == FAILURE) {
		printf("There was an error downloading\n");
	}
	else if (error == OK) {
		printf("Download Completed\n");
	}
	return OK;
}