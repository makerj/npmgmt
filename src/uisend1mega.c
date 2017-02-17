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

int main(int argc, char** argv) {
	int client_socket;
	int client_socket_new;
	int port_num = 40000;
	int i = 0;
	int len = 0;
	size_t len_getline;    	//dummy parameter??
	uint32_t console_buf_len;//USED FOR getline() len
	unsigned char* console_buf = NULL;
	unsigned char buf[BUF_SIZE];
	unsigned char str1[BUF_SIZE];
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

	
	printf("set name of 1mega data : ");
	console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf_len--;
	
	
	

	unsigned char*   buf_out = NULL;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

	const int cycle = 100000;
	char op = OPCODE_PDL_ADD;
	uint32_t data_len = 4 + console_buf_len + cycle*10;
    fwrite(&op,       		 1, 1, stream);
    fwrite(&data_len	   , 4, 1, stream);//no len yet			//unable to input! must use u32tob
	fwrite(&console_buf_len, 4, 1, stream);//this is name_len	//unalbe
	fprintf(stream, "%s", console_buf, console_buf_len);
	free(console_buf), console_buf = NULL;

	for(int i = 0; i < cycle; i++){
		fprintf(stream, "%09d\n", i, 10);
	}
	
   	fclose(stream);
	u32tob(buf_out + GOTO_DATALEN, data_len);
	u32tob(buf_out + GOTO_NAMELEN, console_buf_len);

	testprintf("datalen%d == %d+@\nnamelen%d\n", data_len, strlen(buf_out + GOTO_NAME), console_buf_len);
	testprintf("what U send :(lenth : %d)\n0x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n0x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\n\n",
		   be32toh(*(uint32_t*) (buf_out + OPCODE_LEN)),
		   buf_out[0], buf_out[1], buf_out[2], buf_out[3], buf_out[4], buf_out[5], buf_out[6], buf_out[7], buf_out[8], buf_out[9],
		   buf_out[10], buf_out[11], buf_out[12], buf_out[13], buf_out[14], buf_out[15], buf_out[16], buf_out[17], buf_out[18], buf_out[19]);

	testprintf("len : %u\n", HEADER_LEN + be32toh(*(uint32_t*) (buf_out + OPCODE_LEN)));
	testprintf("buf_out_len : %u\n", buf_out_len);
	write_all(client_socket, buf_out, HEADER_LEN + be32toh(*(uint32_t*) (buf_out + OPCODE_LEN)));//send protocol
	free(buf_out);

	read_all(client_socket, buf, HEADER_LEN);

	len = be32toh(*(uint32_t*) (buf + OPCODE_LEN));
	//SHOW REPLY FROM ENGINE
	printf("message U receive\topcode %d\tlen %d\n", buf[0], len);
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