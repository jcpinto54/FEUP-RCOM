#include <stdio.h>

int main(int argc, char *argv[]){
	
	if (argc != 2){
		perror("Wrong number of arguments!\nUsage: ./download <FTP url>\n");
		return -1;
	}

	// TODO: Parse URL
	// TODO: Handle errors on the URL
	// TODO: Print information
	// TODO: Call function that downloads file
	// TODO: Handle errors on the receiving of the file

	return 0;
}