/*
 * HTTP_Service.h
 *
 *  Created on: 2013-8-19
 *      Author: lishu
 */

#ifndef HTTP_SERVICE_H_
#define HTTP_SERVICE_H_

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include </usr/include/net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include </usr/include/linux/sockios.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>  
#include <regex.h>


#include "stream.h"
#include "http.h"

#define HTTP_SERVICE_PLUGNAME 		"http_service.so"
#define LOG_PATH 						"./log/http/"


typedef struct service_pmeinfo
{	
	FILE* fp;
}service_pmeinfo;


#ifdef __cplusplus
extern "C" {
#endif

uchar HTTP_SERVICE_ENTRY(stSessionInfo* session_info, void **param, int thread_seq, struct streaminfo *a_tcp, void *a_packet);
int HTTP_SERVICE_INIT(void);
void HTTP_SERVICE_DESTROY(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVICE_H_ */
