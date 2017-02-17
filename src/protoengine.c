#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/types.h>

#include "mgmt.h"
#include "netpdl-types.h"
#include "dummy_netpdl.h"

#define testprintf                printf
#define streq(str1, str2)        strcmp(str1,str2)==0

//static int debug_phase = 0;//for gdb
typedef char* (* PdlOperateFuncs)(NetPDL* , char* );
/*
static int sprintf_pdl(char* buf_mess, struct dummypdl pdl) {
	sprintf(buf_mess, "portnum:%d\t name:%s\tmessage:%s\n", pdl.port_num, pdl.name, pdl.message);
	return strlen(buf_mess);
}

static void fprintf_pdl(FILE* stream, struct dummypdl pdl) {
	fprintf(stream, "portnum:%d\t name:%s\tmessage:%s\n", pdl.port_num, pdl.name, pdl.message);
}*/

static uint32_t get_data_len(char* buf_input){
    return be32toh(*(uint32_t*) (buf_input + GOTO_DATALEN));
}

static void namecpy(char* name, char* buf_input){//name lengt limit uidefined!!
	int name_len = be32toh(*(uint32_t*) (buf_input + GOTO_NAMELEN));
    strncpy(name, buf_input + GOTO_NAME, name_len);
    name[name_len] = '\0';
}

static int mk_pdlfile(char* fname, char* buf_input){
	FILE* fd = NULL;
	time_t tm_time;
	int nwrite;

	memset(fname, 0, 1024);
	time(&tm_time);
	struct tm* local_time = localtime(&tm_time);
	strftime(fname, 23, "/tmp/pdl_%y%m%d%H%M%S_", local_time);
	namecpy(fname + strlen(fname), buf_input);

	testprintf("file name : %s\n", fname);
	
	fd = fopen(fname, "w");

	testprintf("fd is %d(int convertend manually)\n",(int)fd);

	if(fd == NULL) {
		testprintf("Cannot create pdl pdlfile\n");
		return -1;
	}
	nwrite = fwrite(buf_input + GOTO_NAME, 1, strlen(buf_input + GOTO_NAME), fd);

	testprintf("written words : %d\n", nwrite);

	fclose(fd);
	return nwrite;
}

static int set_socket(int tcp_port_use) {
	struct sockaddr_in server_addr;
	int success = -1;
	int port_num = tcp_port_use;
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);

	if(server_socket == -1) {
		perror("Npmgmt :socket() failed!\n");
		return -1;
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
		} else if(!(port_num < tcp_port_use + TCP_PORT_TRY)) {
			perror("failed bind() in %d~ %d tcp port. exit\n", tcp_port_use, tcp_port_use + TCP_PORT_TRY - 1);	//need change in final version to only use designated port
			return -1;
		}

		port_num++;
	}

	testprintf("TCPport%d bind() fin,  ", port_num);
	success = listen(server_socket, 5);
	if(success < 0){
		printf("Npmgmt :listen() failed!\n");
		return -1;
	}
	printf("listen() set success  //waiting for UI\n");

	return server_socket;
}

static int accept_ui(const int server_socket) {

	struct sockaddr_in ui_addr;
	int ui_addr_size = sizeof(ui_addr);
	int ui_socket = accept(server_socket, (struct sockaddr*) &ui_addr, &ui_addr_size);

	if(ui_socket < 0) {
		printf("연결 실패\n"), exit(1);
	}

	printf("ui와 연결 성공\n");
	return ui_socket;
}

static char* ui_message_echo(NetPDL* netpdl, char* buf_input) {
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);
    int     data_len = 0;
    
    char op = OPCODE_MESSAGE;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);    //data_len is not yet ready, re-inputed later
    fprintf(stream, "<message echo>\n%s\n", buf_input + GOTO_DATA);
    fclose(stream);

    data_len = strlen(buf_out + GOTO_DATA);
    sprintf(buf_out + GOTO_DATALEN, "%d", data_len);
	return buf_out;
}

static char* add_pdl(NetPDL* netpdl, char* buf_input) {
	int success = NULL;
	char fname[1024];     //warning!! 1024 array!

    //namecpy(fname, buf_input);
	//testprintf("namecp done\n");

    int nwrite = mk_pdlfile(fname, buf_input);
	testprintf("opening maden file : %s\n", fname);
    FILE* fd =fopen(fname, "r");
	if(fd == NULL){	printf("pdl file opening failed!\n");}
    success = netpdl_parse(netpdl, fd);
	testprintf("debugpoint1\n");
	success = fclose(fd);
	if(success != 0){	printf("pdl file closing failed!\n");}
	testprintf("debugpoint2\n");
	
	

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

	char op = OPCODE_MESSAGE;
    int templen = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&templen, 4, 1, stream);//no len yet

	fprintf(stream, "New pdl saved!\npath : %s\nlen : %dbyte\n", fname, nwrite);
   	fclose(stream); 
    
	testprintf("<writing on buffer>\n%s\n", buf_out + GOTO_DATA);
	
	//testprintf("buf_out data_len is %d\n", data_len);

	//int data_len = strlen(buf_out + GOTO_DATA);
    //sprintf(buf_out + GOTO_DATALEN, "%d", data_len);
	
	return buf_out;
}

static char* remove_pdl(NetPDL* netpdl, char* buf_input) {
	int success = NULL;
    char pdlname[1024];     //warning!! static array!
	namecpy(pdlname, buf_input);
    success = netpdl_remove(netpdl, pdlname);

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int32_t data_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);//no len yet

    if(!success) {
        fprintf(stream, "FAIL to remove pdl %s\n", pdlname);
    } else {
        fprintf(stream, "success : remove pdl %s\n", pdlname);
    }
    
    //data_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", data_len);
    fclose(stream);   
	return buf_out;
}

static char* get_pdl(NetPDL* netpdl, char* buf_input) {
	int success = NULL;
    char pdlname[1024];     //warning!! static array!
	namecpy(pdlname, buf_input);
    success = netpdl_get(netpdl, pdlname);

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int32_t data_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);//no len yet

    if(!success) {
        fprintf(stream, "FAIL to get pdl %s\n", pdlname);
    } else {
        fprintf(stream, "success : get pdl %s\n", pdlname);
    }
    
    //data_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", data_len);
    fclose(stream);   
	return buf_out;
}

static char* list_pdl(NetPDL* netpdl, char* buf_input) {
	Vector* list = netpdl_list(netpdl);



    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int32_t data_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);//no len yet

    if(list == NULL){   //error
        fprintf(stream, "FAIL to get pdl list\n");
    } else/* if (list.size != 0)*/ {
        /*for(int i = 0; i < size){
            ~~~~
        }
        */
    }/*else{~~~~}*/
    
    //data_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", data_len);
    fclose(stream);   
	return buf_out;
}

static char* send_error_message(NetPDL* netpdl, char* buf_input) {
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int data_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&data_len, 4, 1, stream);
    fprintf(stream, "OPCODE ERROR!!!\n");

    fclose(stream);

    //data_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", data_len);
	return buf_out;
}

static PdlOperateFuncs process_ui_request[OPCODE__COUNT__ + 1] = {
	ui_message_echo,                        // for test only
	add_pdl, remove_pdl, list_pdl, get_pdl, //operations in real use
    send_error_message                      //send error message
};


static int ui_control_onmessage(NetPDL* netpdl, fd_set readfds, int* pt_ui_socket, const int server_socket) {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec= 1;
	int any_connect = select(server_socket + 1, &readfds, NULL, NULL, &tv);
	
	/*	if(select()) {	//미구현
		//if no input
		//	return 0;
	}*/

	if(!any_connect){
		return 0;
	}else{	//connect
		if(*pt_ui_socket == NULL) {                    //need select() also
			*pt_ui_socket = accept_ui(server_socket);
		}else{
			//continue from last step...
			//Is handling needed?
		}
	}

	unsigned char* buf_input = NULL;                //1024 - >안전성 없음. 한방에 훅감.
	const int ui_socket = *pt_ui_socket;
	uint32_t opcode, len;


	testprintf("start reading packet\n");
	//READ HEADER NOW
	buf_input = (unsigned char*) calloc(HEADER_LEN, sizeof(unsigned char));
	read_all(ui_socket, buf_input, HEADER_LEN);
	opcode = buf_input[0];
	len = be32toh(*(uint32_t*) (buf_input + GOTO_DATALEN)); // = ntoh32(buf_input + GOTO_DATALEN);

	testprintf("ui_input for 5 bytes : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf_input[0], buf_input[1], buf_input[2],
			   buf_input[3], buf_input[4]);
	testprintf("opcode : %d\nlen :%d == 0x%02x\n", opcode, len, len);

	//READ DATA NOW
	buf_input = (unsigned char*) realloc(buf_input, (HEADER_LEN + len) * sizeof(unsigned char));
	read_all(ui_socket, buf_input + GOTO_DATA, len);
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
	char* buf_output = process_ui_request[opcode](netpdl, buf_input);
	free(buf_input);
	//filling 'header'
	buf_output[0] = OPCODE_MESSAGE;
	u32tob(buf_output + GOTO_DATALEN, strlen(buf_output + GOTO_DATA));//only sending message to ui, possible to use strlen casus no '\0'in message
	//IF disable headermaking here, should activate headermaking code in each  process_ui_request[]

	//send reply to ui
	write_all(ui_socket, buf_output, strlen(buf_output + GOTO_DATA) + HEADER_LEN);
    free(buf_output);

	testprintf("buf_output header&mess: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			   buf_output[0], buf_output[1], buf_output[2], buf_output[3], buf_output[4]);
	testprintf("%s\n\n", buf_output + HEADER_LEN);    //stdout display message(header not included)
	close(*pt_ui_socket);
	*pt_ui_socket = 0;
    
}

int main(int argc, char** argv) {
	struct sockaddr_in server_addr;
	NetPDL* netpdl;
	int server_socket;
	int ui_socket = NULL;

	server_socket = set_socket(NPMGMT_DEFAULT_PORT);            //create()~bind()


	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(server_socket, &readfds);
	//FD_ZERO(&readfds);

	while(1) {
		ui_control_onmessage(netpdl, readfds, &ui_socket, server_socket);//listen()~close()

		//==============================//
		//								//
		//		run program here		//
		//								//
		//==============================//

		sleep(1);

		testprintf("--------------  sleep for 1000ms, engine loop end  --------------\n\n\n");
	}
}
