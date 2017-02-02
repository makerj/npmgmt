#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "mgmt.h"

int readAll(const int socket, char* buf, const int len) {
	int nowread = 0;
	int nread = 0;

	while(nread < len){
		nowread = (int) read(socket, buf + nread, (size_t) (len - nread));
		nread += nowread;
		if(nowread == -1) {
			printf("ERROR!! disconnected while readAll() function working\n");
			return nowread;
		}
	}

	if(nread != len)
		printf("ERROR!! %d more chars nread!!\n", nread - len);
	
	return nread;
}

int writeAll(const int socket, char* buf, const int len) {
	int nowwrite = 0;
	int nwrite = 0;

	while(nwrite < len){
		nowwrite = (int) write(socket, buf + nwrite, (size_t) (len - nwrite));
		nwrite += nowwrite;
		if(nowwrite == -1){
			printf("ERROR!! disconnected while writeAll() function working\n");
			return nowwrite;
		}
	}

	if (nwrite != len)
		printf("ERROR!! %d more chars nwrite!!\n", nwrite - len);
	
	return nwrite;
}

void u32tob(uint8_t* buf, const uint32_t num){
	*(uint32_t*)buf = htobe32(num);
}
