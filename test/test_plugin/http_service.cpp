/*
 * HTTP_Service.c
 *
 *  Created on: 2013-8-19
 *  Author: lishu
  */

#include <math.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <regex.h>
#include <string>
#include "http_service.h"
#include "MESA_handle_logger.h"

int HTTP_SERVICE_VERSION_1_20160412 = 0;
void http_service_version_1_20160412()
{
	//20160412   create project	
}

void* g_log_handle = NULL;

int init_pmeinfo(service_pmeinfo **service_pme, int thread_seq)
{
	service_pmeinfo* pme = (service_pmeinfo*)dictator_malloc(thread_seq, sizeof(service_pmeinfo));
	*service_pme = pme;
	return 0;
}

void clear_pmeinfo(service_pmeinfo *service_pme, int thread_seq)
{
	if(service_pme!=NULL)
	{		
		dictator_free(thread_seq, service_pme);
	}   
    service_pme = NULL;
}

std::string trans_to_binary(unsigned int n){
	std::string res;
	while(n){
		res = std::to_string(n % 2) + res;
		n /= 2;
	}
	return res;
}

std::string _inet_ntoa(uint32_t ip)
{
	 const char *_ip = inet_ntoa(*(struct in_addr *)&ip);
	 return std::string(_ip);
}


static int get_stream_tuple4(struct streaminfo *a_tcp){
	struct stream_tuple4_v4 *tuple4_v4 = a_tcp->addr.tuple4_v4;
	printf("saddr: %s\n", _inet_ntoa(tuple4_v4->saddr).c_str());
	printf("daddr: %s\n", _inet_ntoa(tuple4_v4->daddr).c_str());
	printf("source: %d\n", ntohs(tuple4_v4->source));
	printf("dest: %d\n", ntohs(tuple4_v4->dest));
	return 0;
}

uchar HTTP_SERVICE_ENTRY(stSessionInfo* session_info, void **param, int thread_seq, struct streaminfo *a_tcp, void *a_packet)
{	
	//get_stream_tuple4(a_tcp);     
	uchar			rec 			= PROT_STATE_GIVEME;
	service_pmeinfo *service_pme 	= (service_pmeinfo*)*param;	
	http_infor 		*a_http 		= (http_infor *)(session_info->app_info);
	char			filename[512] = {0};
	const char*		region = NULL;
	//static int		header_len = 0;
	//static int		content_len = 0;
	
	uchar http_state = a_http->http_state;

	if(NULL==session_info)
	{
		return PROT_STATE_DROPME;
	}

	if(service_pme == NULL)
	{
		if(init_pmeinfo(&service_pme, thread_seq) <0)
		{ 
			return PROT_STATE_DROPME;
		}
		*param = service_pme;
	}	
	std::string prot_flag_str = trans_to_binary(session_info->prot_flag);
	uchar curdir = a_http->curdir;
	printf("curdir is %d, http_state is %d\n", curdir, http_state);
	/*
	MESA_handle_runtime_log(g_log_handle, RLOG_LV_INFO, HTTP_SERVICE_PLUGNAME, 
			"call http_service entry, http_state is %02x\n, prot_flag mask is %s : %d", http_state, prot_flag_str.c_str(), prot_flag_str.length() - 1);

	if(curdir == DIR_C2S){
		int i = 0;
		printf("data begin: curdir is %d, http_state is %d, prot_flag mask is %d\n", curdir, http_state, prot_flag_str.length() - 1);
	}
	*/
	switch(session_info->prot_flag)
	{
		case HTTP_STATE: 
		case HTTP_CONTENT:
			break;
		case HTTP_UNGZIP_CONTENT:	
			snprintf(filename, sizeof(filename), "%s/HTTP_%s_%u", LOG_PATH, printaddr(&a_tcp->addr, thread_seq), a_http->http_session_seq);
			service_pme->fp = fopen(filename, "a+");
			if(NULL==service_pme->fp)
			{
				MESA_handle_runtime_log(g_log_handle, RLOG_LV_FATAL, HTTP_SERVICE_PLUGNAME, "%s file open error.", filename);			
				return PROT_STATE_DROPME;
			}	
			MESA_handle_runtime_log(g_log_handle, RLOG_LV_DEBUG, HTTP_SERVICE_PLUGNAME, "%s file open.",filename);
			
			fwrite(session_info->buf, session_info->buflen, 1, service_pme->fp);			
			fflush(service_pme->fp);			
			//content_len += session_info->buflen;
			//printf("content_len:%d\n", content_len);
			
			snprintf(filename, sizeof(filename), "%s/HTTP_%s_%u", LOG_PATH, printaddr(&a_tcp->addr, thread_seq), a_http->http_session_seq);
			MESA_handle_runtime_log(g_log_handle, RLOG_LV_DEBUG, HTTP_SERVICE_PLUGNAME, "%s file close.",filename);
			fclose(service_pme->fp);
			break;
		default:
			snprintf(filename, sizeof(filename), "%s/HTTP_%s_%u", LOG_PATH, printaddr(&a_tcp->addr, thread_seq), a_http->http_session_seq);
			service_pme->fp = fopen(filename, "a+");
			if(NULL==service_pme->fp)
			{
				MESA_handle_runtime_log(g_log_handle, RLOG_LV_FATAL, HTTP_SERVICE_PLUGNAME, "%s file open error.", filename);			
				return PROT_STATE_DROPME;
			}	
			MESA_handle_runtime_log(g_log_handle, RLOG_LV_DEBUG, HTTP_SERVICE_PLUGNAME, "%s file open.",filename);

			region = http_proto_flag2region(session_info->prot_flag);

			fwrite(region, strlen(region), 1, service_pme->fp);				
			fwrite(":", 1, 1, service_pme->fp);	
			fwrite(session_info->buf, session_info->buflen, 1, service_pme->fp);			
			fwrite("\r\n", 2, 1, service_pme->fp);	
			fflush(service_pme->fp);
			//header_len += session_info->buflen;				
			//printf("header_len:%d\n", header_len);

			snprintf(filename, sizeof(filename), "%s/HTTP_%s_%u", LOG_PATH, printaddr(&a_tcp->addr, thread_seq), a_http->http_session_seq);
			MESA_handle_runtime_log(g_log_handle, RLOG_LV_DEBUG, HTTP_SERVICE_PLUGNAME, "%s file close.",filename);
			fclose(service_pme->fp);
			break;			
	}	
		
	if(session_info->session_state&SESSION_STATE_CLOSE)
	{	
		if(NULL!=service_pme->fp)
		{			

			service_pme->fp = NULL;
		}
		clear_pmeinfo((service_pmeinfo*)*param, thread_seq);
		*param = NULL;
	}
	return rec;
}

int HTTP_SERVICE_INIT(void)
{
	g_log_handle = MESA_create_runtime_log_handle("./log/http/http_service", 10);
	if(g_log_handle == NULL)
	{		
		printf("%s init : get log handle error!\n", HTTP_SERVICE_PLUGNAME);
		return -1;
	} 
	return 0;
}

void HTTP_SERVICE_DESTROY(void)
{
    return ;
}

