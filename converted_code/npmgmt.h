#ifndef NPMGMT_H
#define NPMGMT_H

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
	OPCODE__COUNT__
} OPCode;

#define OPCODE_LEN        1
#define DATALEN_LEN       4
#define HEADER_LEN        OPCODE_LEN+DATALEN_LEN
#define NAMELEN_LEN       4

#define GOTO_OPCODE		0
#define GOTO_DATALEN	GOTO_OPCODE + OPCODE_LEN
#define GOTO_DATA		GOTO_DATALEN+ DATALEN_LEN
#define GOTO_NAMELEN	GOTO_DATA
#define GOTO_NAME		GOTO_DATA + NAMELEN_LEN


#define NPMGMT_DEFAULT_PORT     40000
#define TCP_PORT_TRY      		5

/**
 * Write int32 on char*buf as big-endian
 * @param buf buffer
 * @param num to load on buffer
 */

#endif
