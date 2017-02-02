#ifndef CONTROL_H
#define CONTROL_H


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <byteswap.h>

//-------------------------OPCODEs WARNING------------------------------//
//opcode must start from 0                                              //
//not allowed to make empty opcode between  FIRST_OPCODE~LAST_OPCODE    //
//      because engine code use "func[opcode](~~~)"                     //
//----------------------------------------------------------------------//
typedef enum {
	OPCODE_MESSAGE,
	OPCODE_PDL_ADD,
	OPCODE_PDL_REMOVE,
	OPCODE_PDL_LIST,
	OPCODE_PDL_GET,
	OPCODE__COUNT__,
} OPCode;

#define FIRST_OPCODE        OPCODE_MESSAGE
#define LAST_OPCODE         OPCODE_PDL_GET

#define OPCODE_LEN          1
#define LENTH_LEN           4
#define HEADER_LEN          OPCODE_LEN+LENTH_LEN
#define NAMELENTH_LEN       4

#define TCP_PORT_USE        40000
#define TCP_PORT_TRY        5

struct dummypdl{
	int port_num;
	char name[32];
	char message[1024];
};
#endif
