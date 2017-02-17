#include <stdio.h>
#include <malloc.h>

#include "netpdl.h"
#include "element.h"
#include "parse.h"
#include "process.h"`
#include "misc.h"

NetPDL* netpdl_create() {
    return parse_create_netpdl(false);
}

bool netpdl_add_start(NetPDL* netpdl) {
    return parse_prepare(netpdl);
}

bool netpdl_add(NetPDL* netpdl, char* xml, int size) {
    return parse_update(netpdl, xml, size);
}

bool netpdl_add_done(NetPDL* netpdl) {
    return parse_finalize(netpdl);
}

bool netpdl_parse(NetPDL* netpdl, FILE* pdl) {
    return parse_parse(netpdl, pdl);
}

bool netpdl_delete(NetPDL* netpdl) {
    return netpdl ? (free_netpdl(netpdl, NULL), true) : false;
}

bool netpdl_remove(NetPDL* netpdl, char* protocol_name) {
    NPProtocol* protocol = map_get(netpdl->protocols, protocol_name);
    return protocol ? (free_protocol(netpdl, (NPElement*) protocol), true) : false;
}

void netpdl_prepare_process(NetPDL* netpdl, char* entry_protocol_name,
                            NPCallBack callback_default, void* callback_context) {
    netpdl->startproto = map_get(netpdl->protocols, entry_protocol_name);
    if(!netpdl->startproto)
        printf("Warning: netpdl_prepare_process(): entry protocol '%s' is not found\n", entry_protocol_name);
    netpdl->func = callback_default;
    netpdl->func_context = callback_context;
}

void netpdl_process(NetPDL* netpdl, uint8_t* data, int data_len) {
    NPProtocol*         current_protocol = netpdl->startproto;

    NPBufferVariable*   packetBuffer = (NPBufferVariable*) netpdl->npvar[NPVAR_PACKETBUFFER];
    netpdl->context.packetbuf = packetBuffer->buffer = data;

    NPNumberVariable*   framelength = (NPNumberVariable*) netpdl->npvar[NPVAR_FRAMELENGTH];
    NPNumberVariable*   packetlength = (NPNumberVariable*) netpdl->npvar[NPVAR_PACKETLENGTH];
    netpdl->context.packetlen = packetBuffer->size = framelength->number = packetlength->number = (uint32_t) data_len;

    NPNumberVariable*   currentoffset = (NPNumberVariable*) netpdl->npvar[NPVAR_CURRENTOFFSET];
    currentoffset->number = 0;

    NPProtocolVariable* prev = (NPProtocolVariable*) netpdl->npvar[NPVAR_PREVPROTO];
    NPProtocolVariable* next = (NPProtocolVariable*) netpdl->npvar[NPVAR_NEXTPROTO];
    NPNumberVariable*   protoverify = (NPNumberVariable*) netpdl->npvar[NPVAR_PROTOVERIFYRESULT];

    netpdl->context.field_offset_remainder = 0;

    while(currentoffset->number < data_len && current_protocol) {
        prev->protocol      = current_protocol;
        next->protocol      = NULL;
        protoverify->number = NP_PROTOVERIFY_NOTFOUND;

        if(current_protocol->before_process)
            process_elements(netpdl, current_protocol->before_process);

        current_protocol->process(netpdl, (NPElement*) current_protocol);

        if(current_protocol->after_process)
            process_elements(netpdl, current_protocol->after_process);

        current_protocol = next->protocol; // NetPDL selects a next protocol while processing
    }

    void** garbages = netpdl->context.garbage->array;
    for(size_t i = netpdl->context.garbage->index; i;)
        free(garbages[--i]);
    netpdl->context.garbage->index = 0;
}

size_t netpdl_size(NetPDL* netpdl) {
    return map_size(netpdl->protocols);
}

Vector* netpdl_list(NetPDL* netpdl) {
    MapIterator iter;
    map_iterator_init(&iter, netpdl->protocols);
    Vector* protocol_names = vector_create((size_t) netpdl_size(netpdl), NULL);

    while(map_iterator_has_next(&iter)) {
        MapEntry* entry = map_iterator_next(&iter);
        NPProtocol* protocol = (NPProtocol*) entry->data;
        vector_addx(protocol_names, protocol->name);
    }

    return protocol_names;
}


void netpdl_desc(NetPDL* netpdl, char* protocol_name) {
    NPProtocol* protocol = protocol_name ? map_get(netpdl->protocols, protocol_name) : NULL;
    if(!protocol)
        printf("Protocol '%s' is not found\n", protocol_name);
    else
        print_element((NPElement*) protocol, 0);
}

int netpdl_register_customfunc(NetPDL* netpdl, char* name, NPCallBack func) {
    return map_put(netpdl->customfuncs, name, func);
}

char* netpdl_args_dumps(NetPDLArg** args, int argc) {
    char*   json;
    size_t  json_length;
    FILE*   stream = open_memstream(&json, &json_length);

    fprintf(stream, "[");
    for(int index = 0; index < argc; ++index) {
        NetPDLArg* arg = args[index];
        uint8_t* bytes = arg->value;

        fprintf(stream, "{");
        fprintf(stream, "\"type\":%d,", arg->type);
        fprintf(stream, "\"name\":\"%s\",", arg->name);
        fprintf(stream, "\"size\":%d,", arg->size);
        fprintf(stream, "\"mask\":%d,", arg->mask);

        switch(arg->type) {
            case NETPDL_ARG_STRING:
                fprintf(stream, "\"value\":\"%s\"", (char*) arg->value);
                break;
            case NETPDL_ARG_INTEGER:
                fprintf(stream, "\"value\":%d", *(int*) arg->value);
                break;
            case NETPDL_ARG_BUFFER:
                fprintf(stream, "\"value\":\"");
                for(int i = 0; i < arg->size; ++i)
                    fprintf(stream, "%02x", bytes[i]);
                fprintf(stream, "\"");
                break;
            default:
                break;
        }
        fprintf(stream, "},");
    }
    fclose(stream), json[json_length - 1] = ']';

    return json;
}

void netpdl_args_dumps_callback(NetPDL* netpdl, NetPDLArg** args, int argc, void* context) {
    printf("%s\n", netpdl_args_dumps(args, argc));
}
