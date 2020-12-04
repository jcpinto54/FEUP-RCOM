#include <stdio.h>

int main(int argc, char *argv[]){
	if (argc != 2){
		perror("Wrong number of arguments!\nUsage: ./download <FTP url>\n");
		return -1;
	}

	return 0;
}