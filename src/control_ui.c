#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mgmt.h"

#define BUF_SIZE 1024
#define testprintf				printf

//static const uint32_t protocol_form_len[] = {19, 1, 1, 5, 1};

//static const uint8_t protocol_form[6][30] = {//dumy data included!
//	{
//	0x00,					// OPCODE (message) DUMMY
//	0x00, 0x00, 0x00, 0x0D,	// Length 14
//	'p', 'l', 'e', 'a', 's', 'e', '\n',	'e', 'c', 'h', 'o', ' ', 'm', 'e', // message
//	},{
//	0x01,					// OPCODE (add PDL)
//	/*leng :4 byte*/		// Length = ?
//	/*leng :4 byte*/		// Name Length = ?
//	/*leng :?? byte*/		// Name
//	/*leng :?? byte*/		// message
//	},{
//	0x02, 					// OPCODE (delete PDL)
//	/*leng :4 byte*/		// Length = ?
//	/*leng :?? byte*/		// NAME = ?
//	},{						// total len : 5
//	0x03, 					// OPCODE (delete PDL)
//	0x00, 0x00, 0x00, 0x00,	// Length 0
//	},{						// total len : 9
//	0x04, 					// OPCODE (add PDL)
//	/*leng :4 byte*/		// Length = ?
//	/*leng :?? byte*/		// NAME = ?
//	}
//};

typedef void (*PdlProtocolFunc)(char*);

static void send_message(char* buf_data){
	printf("send message to engine (dummy message)\n");
	int len = atoi("testmessage: echo me");

	u32tob(buf_data - DATALEN_LEN, len);
	sprintf(buf_data, "testmessage: echo me");
}

static void add_pdl(char* buf_data) {
	printf("input name of PDL to add\t: ");

	size_t len_getline;    //maybe dummy parameter
	char* console_buf = NULL;
	ssize_t console_buf_len = getline(&console_buf, &len_getline, stdin);
	int len = console_buf_len = console_buf_len - 1; //= namelen, in console_buf, '\n' included

	u32tob(buf_data, console_buf_len);
	//*(uint32_t*)(buf_data) = htobe32(console_buf_len);	//put namelen in buf
	strncpy(buf_data + NAMELEN_LEN, console_buf, console_buf_len);
	free(console_buf), console_buf = NULL;

	printf("input message of PDL to add\t: ");
	console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf_len--; //'\n' nothanks

	strncpy((buf_data + NAMELEN_LEN + len), console_buf, console_buf_len);
	len = NAMELEN_LEN + len + console_buf_len; //data_len = ~~ + name_leng + mess_leng
	
	u32tob(buf_data - DATALEN_LEN, len);
	free(console_buf), console_buf = NULL;
}


static void remove_pdl(char* buf_data) {
	printf("input PDL name to del : ");

	size_t len_getline;    //maybe dummy parameter
	char* console_buf = NULL;
	ssize_t console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf_len--;
	strncpy(buf_data, console_buf, console_buf_len);

	u32tob(buf_data - DATALEN_LEN, console_buf_len);
	free(console_buf), console_buf = NULL;
}

static void list_pdl(char* buf_data){
	puts("U select pdl list");
	//DO NOTHING, ONLY NEEDED IS opcode & leng. leng is setted as memset, opcode will be set after
}

static void get_pdl(char* buf_data){
	printf("U select pdl get -> input PDL name to get : ");

	size_t len_getline;    //maybe dummy parameter
	char* console_buf = NULL;
	ssize_t console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf_len--;
	strncpy(buf_data, console_buf, console_buf_len);

	u32tob(buf_data - DATALEN_LEN, console_buf_len);
	free(console_buf), console_buf = NULL;
}

static void send_error_message(char* buf_data){
	printf("opcode error!!\n send error message to server\n\n");
	int len = atoi("UI input error, nothing to send!\n");
	u32tob(buf_data - DATALEN_LEN, len);
	sprintf(buf_data, "UI input error, nothing to send!\n");
}

static PdlProtocolFunc fill_protocol[OPCODE__COUNT__ + 1] = {
	send_message,		//send dummy message
	add_pdl, remove_pdl, list_pdl, get_pdl,	//operations in real use
	send_error_message	//send error message
};

int main(int argc, char** argv) {
	int client_socket;
	int client_socket_new;
	int port_num = 40000;
	int i = 0;
	int len = 0;
	size_t len_getline;    	//dummy parameter??
	ssize_t console_buf_len;//USED FOR getline() len
	char* console_buf = NULL;
	char buf[BUF_SIZE];
	char str1[BUF_SIZE];
	struct sockaddr_in server_addr;

	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(client_socket < 0)
		perror("socket 생성 실패"), exit(EXIT_FAILURE);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");        //MUST CHANGE Loopbakc address!!!
	//server_addr.sin_port		= htons(port_num = 40000);
	for(port_num = 40000, i = -1; i == -1; port_num++) {
		server_addr.sin_port = htons(port_num);
		i = connect(client_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));
		if(40004 < port_num) {
			printf("40000~ 40004 tcp port, all failed to connect. exit\n");
			exit(1);
		}
	}

	
	printf("1: add pdl\n2: remove pdl\n3: show pdl list\n4: show pdl selected\n-----------------------\n");
	int opcode;
	memset(buf, 0x00, BUF_SIZE);

	console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf[console_buf_len - 1] = '\0';
	printf("%s\n", console_buf);
	opcode = atoi(console_buf);
	free(console_buf), console_buf = NULL;

	if(opcode < 0 || OPCODE__COUNT__ <= opcode){//if OPCODE INPUT ERROR
		opcode = OPCODE__COUNT__;//NO OPCODE LIKE THIS, temporary value for error message sending
	}

	buf[0] = (opcode != OPCODE__COUNT__)? opcode : 0;	//fill protocol opcode,   error message is also message opcode
	fill_protocol[opcode](buf + HEADER_LEN);			//fill protocol data and data_len

	write_all(client_socket, buf, HEADER_LEN + be32toh(*(uint32_t*) (buf + OPCODE_LEN)));//send protocol

	testprintf("what U send :(lenth : %d)\n0x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n0x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\n\n",
		   be32toh(*(uint32_t*) (buf + OPCODE_LEN)),
		   buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
		   buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17], buf[18], buf[19]);

	
	read_all(client_socket, buf, HEADER_LEN);
	len = be32toh(*(uint32_t*) (buf + OPCODE_LEN));
	//SHOW REPLY FROM ENGINE
	printf("message U receive\topcode %d\tlen %d\n", opcode, len);
	if(len < BUF_SIZE){
		read_all(client_socket, buf, len);
		printf("%s", buf);
	} else {
		int nread = 0;
		for(int left = len; 0 < left; left -= nread){
			int read_len = (BUF_SIZE < left)? BUF_SIZE : left;
			nread = read_all(client_socket, buf, read_len);
			if(nread < 0){//read ERROR
				break;
			}
			printf("%s",buf);
		}
	}
	printf("\n\nendUI\n\n\n");

	close(client_socket);
	return 0;
}