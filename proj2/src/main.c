#include <stdio.h>
#include "urlHandler.h"
#include "macros.h"

int main(int argc, char *argv[]){
	
	if (argc != 2){
		perror("Wrong number of arguments!\nUsage: ./download <FTP url>\n");
		return -1;
	}

	// TODO: Parse URL (and handle errors on the URL)
	url_t url = parseURL(argv[1]);

	// TODO: Print information
	printf("Success: %d\n", url.success);
	printf("Protocol: %s\n", url.protocol);
	printf("User: %s\n", url.username);
	printf("Password: %s\n", url.password);
	printf("Host: %s\n", url.host);
	printf("Path: %s\n", url.path);
	// printf("Filename: %s\n", url.filename);

	if(url.success == FAILURE){
		perror("ERROR: There was a problem parsing the URL!\n");
		return -1;
	}

	// TODO: Call function that downloads file
	// TODO: Handle errors on the receiving of the file

	return OK;
}