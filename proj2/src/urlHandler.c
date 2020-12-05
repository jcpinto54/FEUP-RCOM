#include "urlHandler.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>


url_t parseURL(char *url){
	// ftp://[<user>:<password>@]<host>/<url-path>

	char *protocol, *auth, *user, *password;
	url_t result;
	result.success = FAILURE;

	// Checks for a protocol at the beginning of URL
	protocol = strstr(url, "://");
	if(protocol == NULL){
		perror("No protocol chosen\nExiting...\n");
		return result;
	}

	int length = protocol - url;
	strncpy(result.protocol, url, length);

	// Checks if protocol is FTP
	if(strcmp(result.protocol, "ftp") != 0){
		perror("Protocol is not FTP\nExiting...\n");
		return result;
	}

	// Checks if there's [<user>:<password>@] block
	auth = strstr(url, "@");
	if(auth != NULL){
		user = strstr(protocol+3, ":");
		length = user - (protocol + 3);
		strncpy(result.username, protocol + 3, length);

		password = strstr(user, "@");
		length = password - (user + 1);
		strncpy(result.password, user + 1, length);
	}
	else{
		
	}



	result.success = OK;
	return result;
}