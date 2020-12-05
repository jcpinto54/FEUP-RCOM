#include "urlHandler.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>


url_t parseURL(char *url){
	// ftp://[<user>:<password>@]<host>/<url-path>

	char *protocol, *auth;
	url_t result;
	result.success = FAILURE;

	// Checks for a protocol at the beginning of URL
	protocol = strstr(url, "://");
	if(protocol == NULL){
		perror("No protocol chosen\nExiting...\n");
		return result;
	}

	int protocolLength = protocol - url;
	strncpy(result.protocol, url, protocolLength);

	// Checks if protocol is FTP
	if(strcmp(result.protocol, "ftp") != 0){
		perror("Protocol is not FTP\nExiting...\n");
		return result;
	}

	auth = strstr(url, "@");
	if(auth != NULL){
		printf("There is authentication!\n");
	}



	result.success = OK;
	return result;
}