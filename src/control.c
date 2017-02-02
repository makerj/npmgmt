#include "../include/control.h"

//void teststop(){};


int readAll(const int socket, char* buf, const int len) {
	int nowread = 0;
	int nread = 0;

	while(nread < len){
		nowread = read(socket, buf + nread, len - nread);
		nread += nowread;
		if(nowread == -1) {
			printf("ERROR!! disconnected while readAll() function working\n");
			return nowread;
		}
	}

	if(nread != len)
		printf("ERROR!! %d more chars nread!!", nread - len);
	
	return nread;
}

int writeAll(const int socket, char* buf, const int len) {
	int nowwrite = 0;
	int nwrite = 0;

	while(nwrite < len){
		nowwrite = write(socket, buf + nwrite, len - nwrite);
		nwrite += nowwrite;
		if(nowwrite == -1){
			printf("ERROR!! disconnected while writeAll() function working\n");
			return nowwrite;
		}
	}

	if (nwrite != len)
		printf("ERROR!! %d more chars nwrite!!", nwrite - len);
	
	return nwrite;
}

void onBufUint32(uint8_t* buf, const uint32_t num){
	*(uint32_t*)buf = htobe32(num);
}
