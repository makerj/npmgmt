#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "mgmt.h"

#define testprintf				printf
#define EMPTY_PORT_NAME ""
#define streq(str1,str2)		strcmp(str1,str2)==0


int debug_phase = 0;//for gdb 
typedef void (*pdl_operate_funcs)(struct dummypdl*, char*, char*);

int findPdlByPortnum(struct dummypdl * pdls, int port_num) {
	int i;
	for(i = 0; i < 100; i++) {
		if(pdls[i].port_num == port_num) {
			break;
		}
	}
	return i;
}

int findPdlByName(struct dummypdl * pdls, char* name) {
	int i;
	for(i = 0; i < 100; i++) {
		if(!strcmp(pdls[i].name, name)) {
			break;
		}
	}

	return i;
}

int sprintf_PDLdata(char* buf_mess, struct dummypdl pdl) {
	sprintf(buf_mess, "portnum:%d\t name:%s\tmessage:%s\n", pdl.port_num, pdl.name, pdl.message);
	return strlen(buf_mess);
}

int setServerSocket(/*~~~~*/) {
	struct sockaddr_in server_addr;
	int success			= -1;
	int port_num		= TCP_PORT_USE;
	int server_socket	= socket(PF_INET, SOCK_STREAM, 0);

	if(server_socket == -1) {
		printf("server socket 생성 실패\n");
		exit(1);
	}
	//printf("server socket 생성 완료\n");
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	//server_addr.sin_port		= htons(port_num);
		
	while(1) {
		server_addr.sin_port = htons(port_num);
		success = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
		
		if(success == 0) {
			break;
		} else if(!(port_num < TCP_PORT_USE + TCP_PORT_TRY)) {
			printf("failed bind() in %d~ %d tcp port. exit\n", TCP_PORT_USE, TCP_PORT_USE + TCP_PORT_TRY - 1);
			exit(1);
		}

		port_num++;
	}

	printf("TCPport%d bind() fin,  ", port_num);
	return server_socket;
}

int acceptUI(const int server_socket) {
	int success = listen(server_socket, 5);
	if(success < 0)
		printf("대기상태 모드 설정 실패\n"), exit(1);
	printf("listen() set success  //waiting for UI\n");
	
	struct sockaddr_in ui_addr;
	int ui_addr_size = sizeof(ui_addr);
	int ui_socket    = accept(server_socket, (struct sockaddr*)&ui_addr, &ui_addr_size);

	if(ui_socket < 0){
		printf("연결 실패\n"), exit(1);
	}
	
	printf("ui와 연결 성공\n");
	return ui_socket;
}

void uiMessageEcho(struct dummypdl * pdls, char* buf_data, char* buf_mess){
	sprintf(buf_mess, "<message echo>\n%s\n", buf_data);
}

void addPdl(struct dummypdl * pdls, char* buf_data, char* buf_mess) {
	uint32_t data_len	= be32toh(*(uint32_t*)(buf_data - NAMELENTH_LEN));
	uint32_t name_len	= be32toh(*(uint32_t*) buf_data);
	uint32_t mess_len	= data_len - name_len - NAMELENTH_LEN;
	uint32_t target_num	= findPdlByName(pdls, EMPTY_PORT_NAME);		//pdls[target_num]

	pdls[target_num].port_num = target_num;
	strncpy(pdls[target_num].name,	  buf_data + NAMELENTH_LEN, name_len);
	strncpy(pdls[target_num].message, buf_data + NAMELENTH_LEN + name_len, mess_len);
	pdls[target_num].name[name_len]		= '\0';			//prevnet contamination(not deleted by memset, just name[0]='\0')
	pdls[target_num].message[mess_len]	= '\0';

	sprintf(buf_mess, "New pdl in port%d\nname : %s\nmess : %s\n",
			pdls[target_num].port_num, pdls[target_num].name, pdls[target_num].message);
}

void removePdl(struct dummypdl * pdls, char* buf_data, char* buf_mess) {
	uint32_t target_num	= findPdlByName(pdls, buf_data);	//pdls[target_num]

	if(target_num != 100) {
		sprintf(buf_mess, "delete pdl data\n", target_num);
		pdls[target_num].port_num	= 0;
		pdls[target_num].name[0]	= NULL;
		pdls[target_num].message[0]	= NULL;

	} else {	//no pdl find
		sprintf(buf_mess, "CAN NOT FIND pdl U request!\n");
	}
}

void getPdl(struct dummypdl * pdls, char* buf_data, char* buf_mess) {
	uint32_t target_num	= findPdlByName(pdls, buf_data);	//pdls[target_num]

	if(target_num != 100) {
		sprintf_PDLdata(buf_mess, pdls[target_num]);
	} else {	//no pdl find
		sprintf(buf_mess, "CAN NOT FIND pdl U request!\n");
	}
}

void listPdl(struct dummypdl * pdls, char* buf_data, char* buf_mess) {
	uint32_t nwrite 	= 0;
	uint32_t counter 	= 0;
	uint32_t target_num = 0;

	for(target_num = 0; target_num < 100; target_num++) {	//pdls[] size100 is temp code!!!
		if(!streq(pdls[target_num].name, EMPTY_PORT_NAME)) {
			nwrite += sprintf_PDLdata(buf_mess + nwrite, pdls[target_num]);
			counter++;
		}		
	}

	nwrite += sprintf(buf_mess + nwrite, "total pdls num : %d\n\n", counter);
	buf_mess[nwrite] = '\0';
}

int ui_control_onmessage(int* pt_ui_socket, const int server_socket, struct dummypdl * pdls) {	
	/*	if(select()) {	//미구현
		//if no input
		//	return 0;
	}*/
	
	if(*pt_ui_socket == 0) {					//need select() also
		*pt_ui_socket = acceptUI(server_socket);
	}
	
	char buf_output[1024];
	char buf_input[1024];				//1024 - >안전성 없음. 한방에 훅감.
	char*buf_data = buf_input + HEADER_LEN;
	char*buf_mess = buf_output+ HEADER_LEN;
	const int ui_socket = *pt_ui_socket;
	uint32_t opcode, len;

	memset(buf_output, 0x00, 1024);
	memset(buf_input,  0x00, 1024);
	
	testprintf("start reading packet\n");
	//READ HEADER NOW
	readAll(ui_socket, buf_input, HEADER_LEN);
	opcode	= *(uint32_t*)buf_input;
	len = be32toh(*(uint32_t*)(buf_input + OPCODE_LEN)); // = ntoh32(buf_input + OPCODE_LEN);

	testprintf("ui_input for 5 bytes : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",	buf_input[0], buf_input[1], buf_input[2], buf_input[3], buf_input[4]);
	testprintf("opcode : %d\nlen :%d == 0x%02x\n",opcode, len, len);

	//READ DATA NOW
	readAll(ui_socket, buf_data, len);
	testprintf("opcode : %d\nlen :%x\n", opcode, len);
	testprintf("ui_input:\n%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			buf_input[0], buf_input[1], buf_input[2], buf_input[3], buf_input[4], 
			buf_input[5], buf_input[6], buf_input[7], buf_input[8], buf_input[9],
			buf_input[10], buf_input[11], buf_input[12], buf_input[13], buf_input[14], 
			buf_input[15], buf_input[16], buf_input[17], buf_input[18], buf_input[19]);

	//OPERATES BY OPCODE
	pdl_operate_funcs ProcessUIRequest[LAST_OPCODE - FIRST_OPCODE + 1] = {
		uiMessageEcho,	// for test only
		addPdl, removePdl, listPdl, getPdl
	};
	if(FIRST_OPCODE <= opcode && opcode <= LAST_OPCODE){
		ProcessUIRequest[opcode](pdls, buf_data, buf_mess);
	}else {
		sprintf(buf_mess, "OPCODE ERROR!!!\n no op code : %d\n",opcode);
	}//replying packet 'data' inputed too
	
	//replying packet 'header' now
	buf_output[0] = OPCODE_MESSAGE;			//protocol 'opcode' input
	u32tob(buf_output + OPCODE_LEN, strlen(buf_mess));
	
	//send reply to ui
	writeAll(ui_socket, buf_output, strlen(buf_mess) + HEADER_LEN);

	testprintf("buf_output header: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
			buf_output[0], buf_output[1], buf_output[2], buf_output[3], buf_output[4]);
	testprintf("<buf_mess to ui>\n%s\n\n", buf_mess);						//stdout display message(header not included)

	close(*pt_ui_socket);
	*pt_ui_socket = NULL;
}

/**
 * Start of main function
 * ...__a
 * ...
 */
int main() {
	struct sockaddr_in server_addr;
	struct dummypdl pdls[100] = {0,};		//vector, heap등으로 변환 필요. 현재는 꽉차면 프로그램 터짐
	int server_socket;
	int ui_socket = NULL;

	memset(pdls, 0x00, 100 * sizeof(struct dummypdl));	
	server_socket = setServerSocket(/*~~~*/);			//create()~listen()
	
	while(1) {
		ui_control_onmessage(&ui_socket, server_socket, pdls);

		/*
		*
		*	run program here
		*
		*/

		testprintf("[engine loop end]\n\n\n");
	}
}
