#include "urlHandler.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>


url_t parseURL(char *url){
	// ftp://[<user>:<password>@]<host>/<url-path>

	char *protocol, *auth, *user, *password, *host, *marker;
	url_t result;
	memset(result.protocol, 0, 6);
	memset(result.username, 0, 256);
	memset(result.password, 0, 256);
	memset(result.host, 0, 256);
	memset(result.path, 0, 1024);
	memset(result.filename, 0, 512);

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
	if(auth != NULL){ // In case there is some kind of authentication
		user = strstr(protocol+3, ":");
		if(user == NULL) { // In case there is no password
			user = strstr(protocol+3, "@");
			char* aux = strstr(user + 1, "@"); 
			if(aux == NULL){ // In case the username does not have "@"
				length = user - (protocol + 3);
				strncpy(result.username, protocol + 3, length);
				strncpy(result.password, "randomPassword", 15);
				marker = user + 1; 
			}
			else{ // In case the username has "@"
				length = aux - (protocol + 3);
				strncpy(result.username, protocol+3, length);
				strncpy(result.password, "randomPassword", 15);
				marker = aux + 1; 
			}
		}
		else{ // In case there is both username and password
			length = user - (protocol + 3); // "protocol+3" to ignore characters "://"
			strncpy(result.username, protocol + 3, length);

			password = strstr(user, "@");
			length = password - (user + 1); // "user+1" to ignore character ":"
			strncpy(result.password, user + 1, length);
			marker = password + 1;
		}
		
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
	// printf("", host)
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

	if(strlen(result.username) == 0){
		strcpy(result.username, "anonymous");
		strcpy(result.password, "randomPassword");
	}


	result.success = OK;
	return result;
}