#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

#include "npmgmt.h"

typedef enum {
	READ_STATE_STARTPROTOCOL,
	READ_STATE_HEADER,
	READ_STATE_DATA,
	READ_STATE_FINISH
} ReadState;
/*
typedef enum{
    ERROR_READFAIL,
    ERROR_
}ErrorCode;
*/

typedef struct _NpmgmtLog{
    int engine_socket;
    int ui_socket;    
    int error;       //will use after code completed
    char* buf;
    int nread;    
    int read_state;
}NpmgmtLog;

typedef char* (*prcess_function)(NpmgmtLog*);//used for 6 API calling functions

static void u32tob(uint8_t* buf, const uint32_t num){
	*(uint32_t*)buf = htobe32(num);
}

static int get_opcode(NpmgmtLog* context){
    if(context->nread < OPCODE_LEN){
        return 0;       //consider using errorcode
    }
    return (int) (context->buf[0]);
}

static int get_data_len(NpmgmtLog* context){
    if(context->nread < OPCODE_LEN + DATALEN_LEN){
        return 0;       //consider using errorcode
    }
    return be32toh(*(uint32_t*) (context->buf + GOTO_DATALEN));
}

static int is_all_read(NpmgmtLog* context){
    if(context->nread < get_data_len(context) + HEADER_LEN){
        return false;
    }
    else if(context->nread == get_data_len(context) + HEADER_LEN){
        return true;
    }else{
        return -1;
    }
}

static void read_protocol(NpmgmtLog* context){
    int ret = 0;
    if(context->error){
        return;
    }
    if(is_all_read(context)){
        context->read_state = READ_STATE_FINISH;
        return;
    }
    switch(context->read_state){
    case READ_STATE_STARTPROTOCOL:
        context->buf = malloc(HEADER_LEN * sizeof(char));
        context->read_state = READ_STATE_HEADER;

    case READ_STATE_HEADER:
        if(context->nread < HEADER_LEN){
            ret = read(context->ui_socket, context->buf + context->nread, HEADER_LEN - context->nread);
            if (ret == -1)
                goto error;
            context->nread += ret;
        }
        if(context->nread == HEADER_LEN){
            context->buf = (char*)realloc(context->buf, HEADER_LEN + get_data_len(context) * sizeof(char));
            context->read_state = READ_STATE_DATA;
        }
        
    case READ_STATE_DATA:
        if(HEADER_LEN <= context->nread && context->nread < HEADER_LEN + get_data_len(context)){
            ret = read(context->ui_socket, context->buf + context->nread, get_data_len(context) + HEADER_LEN - context->nread);
            if (ret == -1)
                goto error;
            context->nread += ret;
        }
    } 
    return;
    
    error:
    context->error = -1;
    return;
}

static int check_protocol_error(NpmgmtLog* context){
    if(get_opcode(context) < 0 || OPCODE__COUNT__ <= get_opcode(context)){
        context->error = -1;
    }
    if(context->nread < HEADER_LEN){
        context->error = -1;
    }
    if(get_data_len(context) != context->nread - HEADER_LEN){
        context->error = -1;
    }
}

static void input_refresh(NpmgmtLog* context){
    context->read_state = READ_STATE_STARTPROTOCOL;
    context->error = false;
    context->nread = 0;

    if(context->buf != 0){
        free(context->buf);
        context->buf = NULL;
    }
    if(context->ui_socket != 0){
        close(context->ui_socket);
        context->ui_socket = 0;
    }
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
			perror("failed bind()\n");	//need change in final version to only use designated port
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

static char* ui_message_echo(NpmgmtLog* context){
    char*   buf_out;
    size_t  buf_out_len;    //dummy val???
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);
    
    char op = OPCODE_MESSAGE;
    int mess_len;
    fwrite(&op,       1, 1, stream);
    fwrite(&mess_len, 4, 1, stream);    //mess_len is not yet ready, re-inputed later
    fprintf(stream, "<message echo>\n%s\n", context->buf + GOTO_DATA);
    fclose(stream);

    mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);
    return buf_out;
}

static char* add_pdl(NpmgmtLog* context){
	char fname[1024];     //warning?? 1024 array for file path+name

    int file_size = mk_pdlfile(fname, context->buf);

    /*
        //need to save {pdl_name : file_name} map to use list_pdl(~~)
    */

    FILE* fd =fopen(fname, "r");
	if(fd == NULL){	printf("pdl file opening failed!\n");}

    int success = netpdl_parse(netpdl, fd);     //CALLING API!
    if(success != 0){	printf("npmgmt: netpdl_parse() failed!\n");}
	success = fclose(fd);
	if(success != 0){	printf("pdl file closing failed!\n");}
	
	
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

	char op = OPCODE_PDL_ADD;
    int templen = 0;//dummy
    fwrite(&op,      1, 1, stream);
    fwrite(&templen, 4, 1, stream);//no len yet
	fprintf(stream, "New pdl saved!\npath : %s\nsize : %dbyte\n", fname, file_size);
   	fclose(stream); 

	int mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);	
	return buf_out;
}

static char* remove_pdl(NpmgmtLog* context){
    char pdlname[1024];     //warning!! static array!
	namecpy(pdlname, context->buf);
    int success = netpdl_remove(netpdl, pdlname);
    if(success != 0){	printf("npmgmt: netpdl_remove() failed!\n");}

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_PDL_REMOVE;
    int mess_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&mess_len, 4, 1, stream);//no len yet

    if(!success) {
        fprintf(stream, "FAIL to remove pdl %s\n", pdlname);
    } else {
        fprintf(stream, "success : remove pdl %s\n", pdlname);
        /*
            //need to remove data in {pdl_name : file_name} map
        */
    }
    
    fclose(stream);
    mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);   
	return buf_out;
}
static char* get_pdl(NpmgmtLog* context){
    char pdlname[1024];
    namecpy(pdlname, context->buf);
    int success = netpdl_get(netpdl, pdlname);
    if(success != 0){	printf("npmgmt: netpdl_get() failed!\n");}

    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int mess_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&mess_len, 4, 1, stream);//no len yet

    if(!success) {
        fprintf(stream, "FAIL to get pdl %s\n", pdlname);
    } else {
        fprintf(stream, "success : get pdl %s\n", pdlname);
        /*
            //need to use data in {pdl_name : file_name} map, detailed data of pdl
        */
    }
    
    //mess_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", mess_len);
    fclose(stream);
    mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);   
	return buf_out;
}
static char* list_pdl(NpmgmtLog* context){
    Vector* list = netpdl_list(netpdl);
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int mess_len = 0;//dummy
    fwrite(&op,       1, 1, stream);
    fwrite(&mess_len, 4, 1, stream);//no len yet

    if(list == NULL){   //error
        fprintf(stream, "FAIL to get pdl list\n");
    } else/* if (list.size != 0)*/ {
        /*for(int i = 0; i < size){
            //(pdl_name, filename) >> stream
        }
        */
    }/*else{
        //empty message >> stream
    }*/
    
    //mess_len = strlen(buf_out + HEADER_LEN);
    //sprintf(buf_out + OPCODE_LEN, "%d", mess_len);
    fclose(stream);
    mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);   
	return buf_out;
}
static char* make_error_message(NpmgmtLog* context){
    char*   buf_out;
    size_t  buf_out_len;
    FILE*   stream = open_memstream(&buf_out, &buf_out_len);    

    char op = OPCODE_MESSAGE;
    int mess_len = 0;
    fwrite(&op,       1, 1, stream);
    fwrite(&mess_len, 4, 1, stream);//no len yet
    fprintf(stream, "npmgmt: error occured!, error_code %d\n", context->error);

    fclose(stream);
    mess_len = strlen(buf_out + GOTO_DATA);
    u32tob(buf_out, mess_len);   
	return buf_out;
}

static int npmgmt_init(void* init_data, void** _context){
    NpmgmtLog* context = (NpmgmtLog*) calloc(1, sizeof(NpmgmtLog));
    *_context = context;
    const unsigned int tcp_port_use = init_data ? *(int*)init_data : NPMGMT_DEFAULT_PORT;
    int server_socket = set_socket(tcp_port_use);            //create()~listen(), able to use (tcp_port_use ~ ++5) port_num
    if(server_socket == 0){
		return -1;  //perror printed already in set_socket()
    }
    input_refresh(context);
	context->engine_socket = server_socket;
    return server_socket;
}

static int npmgmt_read(NetPDL* netpdl, void* _context){
    NpmgmtLog* context = _context;
    if(context->ui_socket == 0){    //UI not accepted yet
        context->ui_socekt = accept_ui(context->engine_socket);
        //error code needed
        return 0;
    }
    read_protocol(context);

    char* write_message;
    check_protocol_error(context);

    if(is_read_all(context) || context->error != 0){
        if(context->error != 0){
            write_message = make_error_message(context);
        }else if(is_read_all(context){
                prcess_function do_process[OPCODE__COUNT] = {
                ui_message_echo,                        //for test only
                add_pdl, remove_pdl, list_pdl, get_pdl, //operations in real use
            }
            int opcode = get_opcode(context);
            write_message = do_process[opcode](context);
        }

        //npmgmt_write(write_message);  //no writing process in endpoint functions
        free(write_buffer);
        int error_ = context->error;
        input_refresh(context);
        if(error_ != 0){
            return -1;
        }
    }
    return 0;
}

static int npmgmt_destroy(void* _context){
    NpmgmtLog* context = _context;
    input_refresh(context);
    close(context->engine_socket);
    free(context);
    return 0;
}

EndpointDriver npmgmt_endpoint = {
    .init = npmgmt_init,
	.destroy = npmgmt_destroy,
	.read = npmgmt_read,
	.write = NULL, //howto wirte in endpoint system?
	.loop = NULL
};
