#include "control.c"

#define BUF_SIZE 100
#define testprintf				printf
#define OPCODE_ADD_DUMMY 5

//warning :these dummys cant 

const uint32_t protocol_form_len[] = {20, 1, 1, 5, 1, 25};//NULL=> 0 isnt used for opcode

const uint8_t protocol_form[6][30] = {//dumy data included!
	{
	0x00,							// OPCODE (message)
	0x00, 0x00, 0x00, 0x0F,			// Length 15		//dummy data included
	'p', 'l', 'e', 'a', 's', 'e', '\n',	// Name
	'e', 'c', 'h', 'o', ' ', 'm', 'e', NULL, // message
	},{
	0x01,							// OPCODE (add PDL)
	0x00, 0x00, 0x00, 0x14,			// Length 20		//dummy data included
	0x00, 0x00, 0x00, 0x06, 		// Name Length (include NULL)
	'a', '.', 'x', 'm', 'l', NULL,	// Name
	's', 'o', 'm', 'e', 't', 'h', 'i', 'n', 'g', NULL, // message
	},{								// total len : ?
	0x02, 							// OPCODE (delete PDL)
									// Length ? 
	},{								// total len : 5
	0x03, 							// OPCODE (delete PDL)
	0x00, 0x00, 0x00, 0x00,			// Length 0
	},{								// total len : 9
	0x04, 							// OPCODE (add PDL)
	0x00, 0x00, 0x00, 0x00, 		// Length ?
	0x00, 0x00, 0x00, 0x00,			//input port_num here
	},
	
	/*-----------------*/
	/*- DUMMY OPCODE  -*/
	/*-----------------*/
	{
	0x01,							// OPCODE (add PDL DUMMY VERSION!!)
	0x00, 0x00, 0x00, 0x14,			// Length 20		//dummy data included
	0x00, 0x00, 0x00, 0x06, 		// Name Length (include NULL)
	'a', '.', 'x', 'm', 'l', NULL,	// Name
	's', 'o', 'm', 'e', 't', 'h', 'i', 'n', 'g', NULL, // message
	}
};

void make_packet_add_pdl(char* buf){
	printf("input name of PDL to add\t: ");

	uint32_t len_getline;	//maybe dummy parameter		
	char* console_buf = NULL;	
	ssize_t console_buf_len = getline(&console_buf, &len_getline, stdin);
	int len = console_buf_len = console_buf_len - 1; //= namelen, in console_buf, '\n' included

	onBufUint32(buf + HEADER_LEN, console_buf_len);
	//*(uint32_t*)(buf + HEADER_LEN) = htobe32(console_buf_len);	//put namelen in buf
	strncpy(buf + HEADER_LEN + NAMELENTH_LEN, console_buf, console_buf_len);
	free(console_buf), console_buf = NULL;

	printf("input message of PDL to add\t: ");
	console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf_len--; //'\n' nothanks

	strncpy((buf + HEADER_LEN + NAMELENTH_LEN + len), console_buf, console_buf_len);
	len = NAMELENTH_LEN + len + console_buf_len; //data_len = ~~ + name_leng + mess_leng
	free(console_buf), console_buf = NULL;
			
	onBufUint32(buf + OPCODE_LEN, len);
	//*(uint32_t*)(buf + OPCODE_LEN) = htobe32(len);
}

int debug_phase = 0;

int	main(){
	int client_socket;
	int client_socket_new;
	int port_num = 40000;
	int i =  0;
	uint32_t len_getline;	//maybe dummy parameter
	size_t len = 0;			//USED FOR getline()
	ssize_t console_buf_len;
	char* console_buf = NULL;
	char buf[BUF_SIZE];
	char str1[BUF_SIZE];
	struct sockaddr_in server_addr;
	
	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(client_socket < 0)
		perror("socket 생성 실패"), exit(EXIT_FAILURE);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= inet_addr("127.0.0.1");		//MUST CHANGE Loopbakc address!!! 
	//server_addr.sin_port		= htons(port_num = 40000);
	for(port_num = 40000, i = -1; i == -1; port_num++){
		server_addr.sin_port		 = htons(port_num);
		i = connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if(40004 < port_num){
			printf("40000~ 40004 tcp port, all failed to connect. exit\n");
			exit(1);
		}
	}
	
	


	int opcode;
	memset(buf, 0x00, BUF_SIZE);
	printf("1: add pdl\n2: remove pdl\n3: show pdl list\n4: show pdl selected\n-----------------------\n");
	
	console_buf_len = getline(&console_buf, &len_getline, stdin);
	console_buf[console_buf_len - 1] = '\0';
	printf("%s\n", console_buf);
	opcode = atoi(console_buf);
	free(console_buf), console_buf = NULL;

	if(OPCODE_MESSAGE <= opcode && opcode <= OPCODE_ADD_DUMMY) {
		for(i = 0; i < protocol_form_len[opcode]; i++){	//FORMAT -> buf
			buf[i] = protocol_form[opcode][i];				//will not be used at final version
		}
	}

	switch(opcode){
		case OPCODE_MESSAGE:
			//sending dummy data
			break;

		case OPCODE_ADD_DUMMY:
			puts("U select pdl add(dummy data send)");
			//sending dummy data   // already inputed
			break;

		case OPCODE_PDL_ADD:
			make_packet_add_pdl(buf);
			break;

		case OPCODE_PDL_REMOVE:
			printf("input PDL name to del : ");
			console_buf_len = getline(&console_buf, &len_getline, stdin); //주의: \n까지 같이넣
			console_buf_len--;
			strncpy(buf + HEADER_LEN, console_buf, console_buf_len);
			
			onBufUint32(buf + OPCODE_LEN, console_buf_len);
			//*(uint32_t*)(buf + OPCODE_LEN) = htobe32(strlen(console_buf));
			free(console_buf), console_buf = NULL;
			break;

		case OPCODE_PDL_LIST:
			puts("U select pdl list");
			break;

		case OPCODE_PDL_GET:
			printf("U select pdl get -> input PDL name to get : ");
			console_buf_len = getline(&console_buf, &len_getline, stdin); //주의: \n까지 같이넣
			console_buf_len--;
			strncpy(buf + HEADER_LEN, console_buf, console_buf_len);
			
			onBufUint32(buf + OPCODE_LEN, console_buf_len);
			//*(uint32_t*)(buf + OPCODE_LEN) = htobe32(strlen(console_buf));
			free(console_buf), console_buf = NULL;
			break;

		default:
			printf("opcode error!! opcode : 0x%02x\n send error message to server & exit\n\n", opcode);
			for(i = 0; i < protocol_form_len[OPCODE_MESSAGE]; i++){
				buf[i] = protocol_form[OPCODE_MESSAGE][i];
			}

			len = atoi("UI input error, nothing to send!\n");
			
			onBufUint32(buf + OPCODE_LEN, len);
			//*(uint32_t*)(buf + OPCODE_LEN) = htobe32(len);
			fprintf(buf + HEADER_LEN ,"UI input error, nothing to send!\n");
			writeAll(client_socket, buf, len + HEADER_LEN);
			exit(1);
	}

	writeAll(client_socket, buf, HEADER_LEN + be32toh(*(uint32_t*)(buf + OPCODE_LEN)));

	printf("what U send :(lenth : %d)\n%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\n\n",
		protocol_form_len[opcode],
		buf[0], buf[1], buf[2], buf[3], buf[4], 
		buf[5], buf[6], buf[7], buf[8], buf[9],
		buf[10], buf[11], buf[12], buf[13], buf[14], 
		buf[15], buf[16], buf[17], buf[18], buf[19]);
		
	//SHOW SIGNAL FROM ENGINE
	readAll(client_socket, buf, HEADER_LEN);
	

	opcode	= buf[0];
	len	= be32toh(*(uint32_t*)(buf + OPCODE_LEN));
	printf("message U receive\topcode %d\tlen %d\n", opcode, len);
	readAll(client_socket, buf, len);
	printf("%s\n\nendUI\n\n\n", buf);

	close(client_socket);

	return 0;
}