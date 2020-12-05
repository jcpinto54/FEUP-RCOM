#pragma once

typedef struct url_t{
	char success;
	char protocol[6];
	char username[256];
	char password[256];
	char host[256];
	char path[1024];
	char filename[512];
} url_t;

url_t parseURL(char *url);