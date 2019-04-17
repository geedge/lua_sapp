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


class http_sess_ctx{	
public:
    std::unordered_map<std::string, std::string> request_headers;
    std::unordered_map<std::string, std::string> response_headers;
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
