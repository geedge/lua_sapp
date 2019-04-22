/*
 * HTTP_Service.h
 *
 *  Created on: 2013-8-19
 *      Author: lishu
 */

#ifndef HTTP_SERVICE_H_
#define HTTP_SERVICE_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>  
#include <regex.h>
#include "stream.h"
#include "http.h"
#include "MESA/MESA_prof_load.h"
#include "MESA/MESA_handle_logger.h"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}
#define HTTP_SERVICE_PLUGNAME 		"new_http_service.so"
#define LOG_PATH 					"./log/new_http_service/"

#define LOG_MODULE_NAME "SAPP_LUA"
#define LUA_SAPP_SYMBOL_MAX 64
#define LUA_SAPP_PATH_MAX 256
#define LUA_SAPP_STRING_MAX 2048
#define LUA_ENTRY_TYPE_NUM 8
enum lua_entry_type{
	LUA_ENTRY_TYPE_IP = 0,
	LUA_ENTRY_TYPE_TCP,
	LUA_ENTRY_TYPE_UDP,
	LUA_ENTRY_TYPE_HTTP, 
	LUA_ENTRY_TYPE_TLS,
	LUA_ENTRY_TYPE_DNS,
	LUA_ENTRY_TYPE_MAIL,
	LUA_ENTRY_TYPE_FTP
};

enum http_type{
    HTTP_TYPE_REQUEST = 0, 
    HTTP_TYPE_RESPONSE
};
class http_header{
public:
    bool parse_done;
    std::unordered_map<std::string, std::string> std_regions;
    std::unordered_set<std::string> other_regions;
    http_header() : parse_done(false){}
};

class http_body{
public:
    bool data_end;
    int block_id;
    void *buf;
    int buflen;
    http_body() : data_end(false), block_id(-1), buf(nullptr), buflen(0){}
};

class stream_tuple5{
public:
    bool parse_done;
    std::string ip_version;   //IPV4 or IPV6
    std::string stream_type;  //TCP or UDP
    std::string sip;
    std::string dip;
    uint16_t sport;
    uint16_t dport;
    stream_tuple5() : parse_done(false){}
};

class http_sess_ctx{	
public:
    stream_tuple5 tuple5;
    http_header req_header;
    http_header resp_header;
    http_body req_body;
    http_body resp_body;
    std::vector<lua_State*> coroutines;
};

class lua_plug{
public:
	std::string file_path;
	enum lua_entry_type entry_type;
	lua_State *lua_state;
};

#ifdef __cplusplus
extern "C" {
#endif

uchar NEW_HTTP_SERVICE_ENTRY(stSessionInfo* session_info, void **param, int thread_seq, struct streaminfo *a_tcp, void *a_packet);
int NEW_HTTP_SERVICE_INIT(void);
void NEW_HTTP_SERVICE_DESTROY(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVICE_H_ */
