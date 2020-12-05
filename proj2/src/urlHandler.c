#include "urlHandler.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>


url_t parseURL(char *url){
	// ftp://[<user>:<password>@]<host>/<url-path>

	char *protocol, *auth, *user, *password, *host, *marker;
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
	//TODO: Add an option to type only user and no password
	auth = strstr(url, "@");
	if(auth != NULL){
		user = strstr(protocol+3, ":");
		length = user - (protocol + 3); // "protocol+3" to ignore characters "://"
		strncpy(result.username, protocol + 3, length);

		password = strstr(user, "@");
		length = password - (user + 1); // "user+1" to ignore character ":"
		strncpy(result.password, user + 1, length);
		marker = password + 1;
	}
	else{
		marker = protocol + 3;
	}
	

	//Parsing <host>/<url-path> block
	host = strstr(marker, "/");
	if(host == NULL){
		perror("File not chosen\nExiting...\n");
		return result;
	}
	length = host - marker;
	strncpy(result.host, marker, length);
	strncpy(result.path, host + 1, 1024);

	if(strstr(result.path, "/") == NULL){ // File is in root directory
		strncpy(result.filename, result.path, 512);
		memset(result.path, 0, 1024);
	}
	else{
		char* index = result.path;
		char* oldIndex;

		while(index != NULL){
			oldIndex = index;
			index = strstr(index+1, "/");
		}

		strcpy(result.filename, oldIndex + 1);
		memset(oldIndex + 1, 0, oldIndex - result.path);
	}


	result.success = OK;
	return result;
}