#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <endian.h>
#include <errno.h>

#include "mgmt.h"

size_t read_all(const int socket, char* buf, const uint32_t len) {
	size_t nowread = 0;
	size_t nread = 0;

	while(nread < len){	
		nowread = read(socket, buf + nread, len - nread);
		nread += nowread;
		if(nowread == -1) {
			return nowread;
		}
	}

	if(nread != len)
		printf("ERROR!! %d more chars nread!!\n", (int)(nread - len));
	return (int)nread;
}

size_t write_all(const int socket, char* buf, const uint32_t len) {
	size_t nowwrite = 0;
	size_t nwrite = 0;

	while(nwrite < len){
		nowwrite = write(socket, buf + nwrite, len - nwrite);
		nwrite += nowwrite;
		
		if(nowwrite == -1){
			printf("ERROR!! disconnected while write_all() function working, nwirte :%d, nowwrite: %d\n", nwrite, nowwrite);
			return nowwrite;
		}
	}

	if(nwrite != len){
		printf("ERROR!! %d more chars nwrite!!\n", (int)(nwrite - len));
	}
	return nwrite;
}

void u32tob(uint8_t* buf, const uint32_t num){
	*(uint32_t*)buf = htobe32(num);
}
