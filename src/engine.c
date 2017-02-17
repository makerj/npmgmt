#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "mgmt.h"

#define testprintf                printf
#define EMPTY_PORT_NAME ""
#define streq(str1, str2)        strcmp(str1,str2)==0


//static int debug_phase = 0;//for gdb
typedef char* (* PdlOperateFuncs)(struct dummypdl*, char*);

static int find_pdl_by_name(struct dummypdl* pdls, char* name) {
	int i;
	for(i = 0; i < 100; i++) {
		if(!strcmp(pdls[i].name, name)) {
			break;
		}
	}
	return i;
}

static int sprintf_pdl(char* buf_mess, struct dummypdl pdl) {
	sprintf(buf_mess, "portnum:%d\t name:%s\tmessage:%s\n", pdl.port_num, pdl.name, pdl.message);
	return strlen(buf_mess);
}

static void fprintf_pdl(FILE* stream, struct dummypdl pdl) {
	fprintf(stream, "portnum:%d\t name:%s\tmessage:%s\n", pdl.port_num, pdl.name, pdl.message);
}

static int set_socket(/*~~~~*/) {
	struct sockaddr_in server_addr;
	int success = -1;
	int port_num = NPMGMT_DEFAULT_PORT;
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);

	if(server_socket == -1) {
		printf("server socket 생성 실패\n");
		exit(1);
	}
	//testprintf("server socket 생성 완료\n");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	while(1) {
		server_addr.sin_port = htons(port_num);
		success = bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));

		if(success == 0) {
			break;
		} else if(!(port_num < NPMGMT_DEFAULT_PORT + TCP_PORT_TRY)) {
			printf("failed bind() in %d~ %d tcp port. exit\n", NPMGMT_DEFAULT_PORT, NPMGMT_DEFAULT_PORT + TCP_PORT_TRY - 1);
			exit(1);
		}

		port_num++;
	}

	printf("TCPport%d bind() fin,  ", port_num);
	return server_socket;
}

static int accept_ui(const int server_socket) {
	int success = listen(server_socket, 5);
	if(success < 0)
		printf("대기상태 모드 설정 실패\n"), exit(1);
	printf("listen() set success  //waiting for UI\n");

	struct sockaddr_in ui_addr;
	int ui_addr_size = sizeof(ui_addr);
	int ui_socket = accept(server_socket, (struct sockaddr*) &ui_addr, &ui_addr_size);

	if(ui_socket < 0) {
		printf("연결 실패\n"), exit(1);
	}

	printf("ui와 연결 성공\n");
	return ui_socket;
}


static char* ui_message_echo(struct dummypdl* pdls, char* buf_data) {
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);
    int     data_len = 0;
    
    char op = OPCODE_MESSAGE;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);    //data_len is not yet ready, re-inputed later
    fprintf(stream, "<message echo>\n%s\n", buf_data);
    fclose(stream);

    data_len = strlen(buf_out + HEADER_LEN);
    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static char* add_pdl(struct dummypdl* pdls, char* buf_data) {

	uint32_t data_len = be32toh(*(uint32_t*) (buf_data - NAMELEN_LEN));
	uint32_t name_len = be32toh(*(uint32_t*) buf_data);
	uint32_t mess_len = data_len - name_len - NAMELEN_LEN;
	uint32_t target_num = find_pdl_by_name(pdls, EMPTY_PORT_NAME);        //pdls[target_num]

	pdls[target_num].port_num = target_num;
	strncpy(pdls[target_num].name, buf_data + NAMELEN_LEN, name_len);
	strncpy(pdls[target_num].message, buf_data + NAMELEN_LEN + name_len, mess_len);
	pdls[target_num].name[name_len] = '\0';            //prevnet contamination(not deleted by memset, just name[0]='\0')
	pdls[target_num].message[mess_len] = '\0';



    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    data_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);//no len yet
    fprintf(stream, "New pdl in port%d\nname : %s\nmess : %s\n",
			pdls[target_num].port_num, pdls[target_num].name, pdls[target_num].message);
    fclose(stream);
//    data_len = strlen(buf_out + HEADER_LEN);
//    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static char* remove_pdl(struct dummypdl* pdls, char* buf_data) {
	uint32_t target_num = find_pdl_by_name(pdls, buf_data);    //pdls[target_num]

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int data_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);            //header

	if(target_num != 100) {
		pdls[target_num].port_num = 0;
		pdls[target_num].name[0] = '\0';
		pdls[target_num].message[0] = '\0';

		fprintf(stream, "delete pdl data\n");
	} else {    //no pdl find
		fprintf(stream, "CAN NOT FIND pdl U request!\n");
	}
    fclose(stream);

//    data_len = strlen(buf_out + HEADER_LEN);
//    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static char* get_pdl(struct dummypdl* pdls, char* buf_data) {
	uint32_t target_num = find_pdl_by_name(pdls, buf_data);    //pdls[target_num]
   
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int data_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);            //header

	if(target_num != 100) {
		fprintf_pdl(stream, pdls[target_num]);
	} else {    //no pdl find
		fprintf(stream, "CAN NOT FIND pdl U request!\n");
	}
    fclose(stream);

//    data_len = strlen(buf_out + HEADER_LEN);
//    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static char* list_pdl(struct dummypdl* pdls, char* buf_data) {
	uint32_t counter = 0;
	uint32_t target_num = 0;

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int data_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);

	for(target_num = 0; target_num < 100; target_num++) {    //pdls[] size100 is temp code!!!
		if(!streq(pdls[target_num].name, EMPTY_PORT_NAME)) {
			fprintf_pdl(stream, pdls[target_num]);
			counter++;
		}
	}
    fprintf(stream, "total pdls num : %d\n\n", counter);
    fclose(stream);

//	data_len = strlen(buf_out + HEADER_LEN);
//    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static char* send_error_message(struct dummypdl* pdls, char* buf_data) {
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int data_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);
    fprintf(stream, "OPCODE ERROR!!!\n");

    fclose(stream);

//	data_len = strlen(buf_out + HEADER_LEN);
//    sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static PdlOperateFuncs process_ui_request[OPCODE__COUNT__ + 1] = {
	ui_message_echo,                        // for test only
	add_pdl, remove_pdl, list_pdl, get_pdl, //operations in real use
    send_error_message                      //send error message
};


static int ui_control_onmessage(int* pt_ui_socket, const int server_socket, struct dummypdl* pdls) {
	/*	if(select()) {	//미구현
		//if no input
		//	return 0;
	}*/

	if(*pt_ui_socket == 0) {                    //need select() also
		*pt_ui_socket = accept_ui(server_socket);
	}

	char buf_input[1024];                //1024 - >안전성 없음. 한방에 훅감.
	const int ui_socket = *pt_ui_socket;
	uint32_t opcode, len;

	memset(buf_input, 0x00, 1024);

	testprintf("start reading packet\n");
	//READ HEADER NOW
	read_all(ui_socket, buf_input, HEADER_LEN);
	opcode = *(uint32_t*) buf_input;
	len = be32toh(*(uint32_t*) (buf_input + OPCODE_LEN)); // = ntoh32(buf_input + OPCODE_LEN);

	testprintf("ui_input for 5 bytes : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf_input[0], buf_input[1], buf_input[2],
			   buf_input[3], buf_input[4]);
	testprintf("opcode : %d\nlen :%d == 0x%02x\n", opcode, len, len);

	//READ DATA NOW
	read_all(ui_socket, buf_input + HEADER_LEN, len);
	testprintf("opcode : %d\nlen :%x\n", opcode, len);
	testprintf("<ui send>\n0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			buf_input[0], buf_input[1], buf_input[2], buf_input[3], buf_input[4],
			buf_input[5], buf_input[6], buf_input[7], buf_input[8], buf_input[9],
			buf_input[10], buf_input[11], buf_input[12], buf_input[13], buf_input[14],
			buf_input[15], buf_input[16], buf_input[17], buf_input[18], buf_input[19]);

	//OPERATES BY OPCODE
	if(opcode < 0 || OPCODE__COUNT__ <= opcode){//if OPCODE INPUT ERROR
		opcode = OPCODE__COUNT__;//NO OPCODE LIKE THIS, temporary value for error message sending
	}
	char* buf_output = process_ui_request[opcode](pdls, buf_input + HEADER_LEN);

	//filling 'header'
	buf_output[0] = OPCODE_MESSAGE;
	u32tob(buf_output + OPCODE_LEN, strlen(buf_output + HEADER_LEN));//only sending message to ui, possible to use strlen casus no '\0'in message
	//IF disable headermaking here, should activate headermaking code in each  process_ui_request[]

	//send reply to ui
	write_all(ui_socket, buf_output, strlen(buf_output + HEADER_LEN) + HEADER_LEN);
    free(buf_output);

	testprintf("buf_output header&mess: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			   buf_output[0], buf_output[1], buf_output[2], buf_output[3], buf_output[4]);
	testprintf("%s\n\n", buf_output + HEADER_LEN);    //stdout display message(header not included)
	close(*pt_ui_socket);
	*pt_ui_socket = 0;
    
}

int main(int argc, char** argv) {
	struct sockaddr_in server_addr;
	struct dummypdl pdls[100] = {0,};        //vector, heap등으로 변환 필요. 현재는 꽉차면 프로그램 터짐
	int server_socket;
	int ui_socket = NULL;

	memset(pdls, 0x00, 100 * sizeof(struct dummypdl));
	server_socket = set_socket(/*~~~*/);            //create()~listen()

	while(1) {
		ui_control_onmessage(&ui_socket, server_socket, pdls);

		//==============================//
		//								//
		//		run program here		//
		//								//
		//==============================//

		testprintf("[engine loop end]\n\n\n");
	}
}
