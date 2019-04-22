#include "adapter.h"

//TODO:
//每个函数都要判断返回值 done
// lua错误处理，比如函数没有注册，resume的返回值判断然后打印错误, 不再resume该协程  done
//压栈之前要check_stack  done
//流结束clear_ctx之外还要释放该协程，自动垃圾回收？做实验验证一下  done 
//destroy，释放lua_state  done
//多线程 m * n个虚拟机
//dumpfile改成在线试一下
//错误处理，日志, 工程化，日志完备

static void* g_logger = NULL;
static std::vector<std::vector<lua_plug>> g_lua_plugs(LUA_ENTRY_TYPE_NUM, std::vector<lua_plug>());

static void lua_traceback(const char *func_name, lua_State *lua_state, int ret){
	int n = lua_gettop(lua_state);
	printf("%s error: ret is %d, traceback is:\n", func_name, ret);
	for(int i = -1; i >= 0 - n; i--){
        std::string type = std::string(lua_typename(lua_state, lua_type(lua_state, i)));
        if(type == "string"){
			printf("%s\n", lua_tostring(lua_state, i));
		}
    }
	lua_pop(lua_state, n);
}

std::string _inet_ntoa(uint32_t ip)
{
	 const char *_ip = inet_ntoa(*(struct in_addr *)&ip);
	 return std::string(_ip);
}

//copy from suricata
static int lua_push_string_buffer(lua_State *lua_state, const char *input, size_t input_len)
{
    if (input_len % 4 != 0) {
        /* we're using a buffer sized at a multiple of 4 as lua_pushlstring generates
         * invalid read errors in valgrind otherwise. Adding in a nul to be sure.
         * Buffer size = len + 1 (for nul) + whatever makes it a multiple of 4 */
        size_t buflen = input_len + 1 + ((input_len + 1) % 4);
        char buf[buflen];
        memset(buf, 0x00, buflen);
        memcpy(buf, input, input_len);
        buf[input_len] = '\0';
        /* return value through lua_state, as a luastring */
        lua_pushlstring(lua_state, buf, input_len);
    } else {
        lua_pushlstring(lua_state, input, input_len);
    }
    return 1;
}

static int get_stream_tuple5(lua_State *lua_state){
	lua_getglobal(lua_state, "cur_http_sess_ctx");
    http_sess_ctx *sess_ctx = (http_sess_ctx *)lua_touserdata(lua_state, -1);
	lua_pop(lua_state, 1);
	if(sess_ctx == NULL){
		//do log
		std::cout<<"sess_ctx is null"<<std::endl; 
		return -1;
	}
	stream_tuple5& tuple5 = sess_ctx->tuple5;
	if(tuple5.parse_done == true){
		lua_newtable(lua_state);
		lua_push_string_buffer(lua_state, "ip_version", std::string("ip_version").length());
		lua_push_string_buffer(lua_state, tuple5.ip_version.c_str(), tuple5.ip_version.length());
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "stream_type", std::string("stream_type").length());
		lua_push_string_buffer(lua_state, tuple5.stream_type.c_str(), tuple5.stream_type.length());
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "sip", std::string("sip").length());
		lua_push_string_buffer(lua_state, tuple5.sip.c_str(), tuple5.sip.length());
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "dip", std::string("dip").length());
		lua_push_string_buffer(lua_state, tuple5.dip.c_str(), tuple5.dip.length());
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "sport", std::string("sport").length());
		lua_pushinteger(lua_state, tuple5.sport);
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "dport", std::string("dport").length());
		lua_pushinteger(lua_state, tuple5.dport);
		lua_settable(lua_state, -3);
		return 0;
	}
	return -1;
}

static int _get_stream_info(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	int ret = get_stream_tuple5(lua_state);
	if(ret < 0){
		lua_yieldk(lua_state, 0, 0, _get_stream_info);
	}
	return 1;
}

static int get_stream_info(lua_State* lua_state){
	//printf("call get_strem_info\n");
	return _get_stream_info(lua_state, 0, 0);
}

static int get_http_header(lua_State *lua_state, int type){
	lua_getglobal(lua_state, "cur_http_sess_ctx");
    http_sess_ctx *sess_ctx = (http_sess_ctx *)lua_touserdata(lua_state, -1);
	lua_pop(lua_state, 1);
	if(sess_ctx == NULL){
		//do log
		std::cout<<"sess_ctx is null"<<std::endl; 
		return -1;
	}
	std::unordered_set<std::string> required_regions; 
    lua_pushnil(lua_state);
    while(lua_next(lua_state, -2) != 0){
        required_regions.insert(std::string(lua_tostring(lua_state, -1)));
        lua_pop(lua_state, 1);
	}
	http_header& header = (type == HTTP_TYPE_REQUEST ? sess_ctx->req_header : sess_ctx->resp_header);
	if(header.parse_done == true){
		//printf("header parse done\n");
		lua_newtable(lua_state);
		for(auto region : header.std_regions){
			if(required_regions.find("ALL") != required_regions.end() || required_regions.find(region.first) != required_regions.end()){
				lua_push_string_buffer(lua_state, region.first.c_str(), region.first.length());
				lua_push_string_buffer(lua_state, region.second.c_str(), region.second.length());
				lua_settable(lua_state, -3);
			}
		}
		if(!header.other_regions.empty()){
			if(required_regions.find("ALL") != required_regions.end() || required_regions.find("HTTP_OTHER_REGIONS") != required_regions.end()){
				lua_push_string_buffer(lua_state, "HTTP_OTHER_REGIONS", std::string("HTTP_OTHER_REGIONS").length());
				lua_newtable(lua_state);
				int i = 1;
				for(auto region : header.other_regions){
					lua_push_string_buffer(lua_state, region.c_str(), region.length());
					lua_rawseti(lua_state, -2, i++);
				}
				lua_settable(lua_state, -3);
			}
		}
		return 0;
	}
	return -1;
}

static int _get_http_request_header(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	//printf("call get_http_request_header\n");
	int ret = get_http_header(lua_state, HTTP_TYPE_REQUEST);
	if(ret < 0){
		lua_yieldk(lua_state, 0, 0, _get_http_request_header);
	}
	return 1;
}

static int get_http_request_header(lua_State* lua_state){
	return _get_http_request_header(lua_state, 0, 0);
}

static int _get_http_response_header(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	int ret = get_http_header(lua_state, HTTP_TYPE_RESPONSE);
	if(ret < 0){
		lua_yieldk(lua_state, 0, 0, _get_http_response_header);
	}
	return 1;
}

static int get_http_response_header(lua_State* lua_state){
	return _get_http_response_header(lua_state, 0, 0);
}

static int get_http_body(lua_State *lua_state, int type){
	//printf("call _get_http_response_body\n");
	lua_getglobal(lua_state, "cur_http_sess_ctx");
    http_sess_ctx *sess_ctx = (http_sess_ctx *)lua_touserdata(lua_state, -1);
	lua_pop(lua_state, 1);
	if(sess_ctx == NULL){
		//do log
		std::cout<<"sess_ctx is null"<<std::endl; 
		return -1;
	}
	http_body& body = (type ==  HTTP_TYPE_REQUEST? sess_ctx->req_body : sess_ctx->resp_body);
	if(body.data_end == true || body.buf != nullptr){
		lua_newtable(lua_state);
		lua_push_string_buffer(lua_state, "block_id", std::string("block_id").length());
		lua_pushinteger(lua_state, body.block_id);
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "data_end", std::string("data_end").length());
		lua_pushboolean(lua_state, body.data_end);
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "buf", std::string("buf").length());
		lua_pushlightuserdata(lua_state, body.buf);
		lua_settable(lua_state, -3);
		lua_push_string_buffer(lua_state, "buflen", std::string("buflen").length());
		lua_pushinteger(lua_state, body.buflen);
		lua_settable(lua_state, -3);
		body.buf = nullptr;
		body.buflen = 0;
		return 0;
	}
	return -1;
}

static int _get_http_request_body(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	int ret = get_http_body(lua_state, HTTP_TYPE_REQUEST);
	if(ret < 0){
		lua_yieldk(lua_state, 0, 0, _get_http_request_body);
	}
	return 1;
}

static int get_http_request_body(lua_State* lua_state){
	return _get_http_request_body(lua_state, 0, 0);
}

static int _get_http_response_body(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	int ret = get_http_body(lua_state, HTTP_TYPE_RESPONSE);
	if(ret < 0){
		lua_yieldk(lua_state, 0, 0, _get_http_response_body);
	}
	return 1;
}

static int get_http_response_body(lua_State* lua_state){
	return _get_http_response_body(lua_state, 0, 0);
}


static int load_lua_plug(const char *profile){
	std::unordered_map<std::string, int> type_map = {
		{"ip", LUA_ENTRY_TYPE_IP},
		{"tcp", LUA_ENTRY_TYPE_TCP},
		{"udp", LUA_ENTRY_TYPE_UDP},
		{"http", LUA_ENTRY_TYPE_HTTP},
		{"tls", LUA_ENTRY_TYPE_TLS},
		{"dns", LUA_ENTRY_TYPE_DNS},
		{"mail", LUA_ENTRY_TYPE_MAIL},
		{"ftp", LUA_ENTRY_TYPE_FTP}
	}; 
	const char *section = "main";
	char file_path[LUA_SAPP_PATH_MAX] = "";
	char entry_type[LUA_SAPP_SYMBOL_MAX] = "";
	MESA_load_profile_string_def(profile, section, "file_path", file_path, sizeof(file_path), "");
	MESA_load_profile_string_def(profile, section, "entry_type", entry_type, sizeof(entry_type), "http");
	
	lua_State *lua_state = luaL_newstate();
	if(lua_state == NULL){
		printf("failed to LuaL_newstate\n");
		return -1;
	}
    luaL_openlibs(lua_state);
    int ret = luaL_dofile(lua_state, file_path);
	if(ret){
		//log error
		printf("error: ret is %d, file_path is %s\n", ret, file_path);
		return -1;
	}
	//TODO: what if not register, rusume handle error
	lua_register(lua_state, "get_stream_info", get_stream_info);
    lua_register(lua_state, "get_http_request_header", get_http_request_header);
	lua_register(lua_state, "get_http_response_header", get_http_response_header);
	lua_register(lua_state, "get_http_request_body", get_http_request_body);
	lua_register(lua_state, "get_http_response_body", get_http_response_body);
	lua_plug plug;
	plug.file_path = std::string(file_path);
	plug.entry_type = (enum lua_entry_type)type_map[std::string(entry_type)];
	plug.lua_state = lua_state;
	g_lua_plugs[plug.entry_type].push_back(plug);
	return 0;
}

static int process_lua_plug_conflist(const char* filename)
{
	char lua_plug_conf_path[LUA_SAPP_PATH_MAX] = {0};
	FILE* fp = fopen(filename, "r");
	if(fp == NULL){
		MESA_handle_runtime_log(g_logger, RLOG_LV_FATAL, LOG_MODULE_NAME, "process_conflist() fopen %s error!\n", filename);
		return -1;	
	}
	while(feof(fp) == 0){
		if((fgets(lua_plug_conf_path, LUA_SAPP_PATH_MAX, fp)) == NULL){
			fclose(fp);
			fp=NULL;
			return 0;
		}
		if(lua_plug_conf_path[0]=='#'){
			continue;
		}
		int len = strnlen(lua_plug_conf_path, LUA_SAPP_PATH_MAX);
		lua_plug_conf_path[len - 1] = '\0';
		int ret = load_lua_plug(lua_plug_conf_path);
		if(ret < 0){
			printf("failed to load_lua_plug: conf_path is %s\n", lua_plug_conf_path);
		}
	}
	fclose(fp);
	fp=NULL;
	return 0;
}

static http_sess_ctx* init_http_sess_ctx(){
    http_sess_ctx *ctx = new http_sess_ctx();
	for(lua_plug plug : g_lua_plugs[LUA_ENTRY_TYPE_HTTP]){
		lua_State* lua_state = plug.lua_state;
		lua_State* coroutine = lua_newthread(lua_state);
        int ret = lua_checkstack(lua_state, 1);
        if(ret != 1){
			//do log
            printf("do not have enough space, ret is %d\n", ret);
			break;
		}
		ctx->coroutines.push_back(coroutine);
	}
	return ctx;
}

static void clear_http_sess_ctx(http_sess_ctx *ctx){
    delete ctx;
    ctx = NULL;
}


std::string utils_inet_ntoa(uint32_t ip)
{
	 const char *_ip = inet_ntoa(*(struct in_addr *)&ip);
	 return std::string(_ip);
}

uchar NEW_HTTP_SERVICE_ENTRY(stSessionInfo* session_info, void **param, int thread_seq, struct streaminfo *a_tcp, void *a_packet){	
	uchar ret = PROT_STATE_GIVEME;
	http_sess_ctx *ctx = (http_sess_ctx*)*param;	
	http_infor *a_http = (http_infor *)(session_info->app_info);
	bool stream_begin = false;
	if(session_info == nullptr){
		return PROT_STATE_DROPME;
	}
	if(ctx == nullptr){
		ctx = init_http_sess_ctx();
		*param = ctx;
		stream_begin = true;
	}	
	if(ctx->tuple5.parse_done == false){
		struct stream_tuple4_v4 *tuple4_v4 = a_tcp->addr.tuple4_v4;
		ctx->tuple5.ip_version = "IPV4";
		if(a_tcp->type == STREAM_TYPE_TCP){
			ctx->tuple5.stream_type = "TCP";
		}
		if(a_tcp->type == STREAM_TYPE_UDP){
			ctx->tuple5.stream_type = "UDP";
		}
		ctx->tuple5.sip = utils_inet_ntoa(tuple4_v4->saddr);
		ctx->tuple5.dip = utils_inet_ntoa(tuple4_v4->daddr);
		ctx->tuple5.sport = ntohs(tuple4_v4->source);
		ctx->tuple5.dport = ntohs(tuple4_v4->dest);
		ctx->tuple5.parse_done = true;
	}
    uchar curdir = a_http->curdir;
	switch(session_info->prot_flag)
	{
		case HTTP_STATE: 
		case HTTP_CONTENT:
			break;
		case HTTP_UNGZIP_CONTENT:	
			if(curdir == DIR_C2S){
				ctx->req_body.block_id++;
				ctx->req_body.buf = session_info->buf;
				ctx->req_body.buflen = session_info->buflen;
			}
			if(curdir == DIR_S2C){
				ctx->resp_body.block_id++;
				ctx->resp_body.buf = session_info->buf;
				ctx->resp_body.buflen = session_info->buflen;
			}
			break;
		default:
			std::string key(http_proto_flag2region(session_info->prot_flag));
            std::string value((const char*)(session_info->buf), session_info->buflen);
			if(curdir == DIR_C2S){
				if(key == "HTTP_OTHER_REGIONS"){
					ctx->req_header.other_regions.insert(value);
				}
				else{
            		ctx->req_header.std_regions.insert({key, value});
				}
			}
			else{
				if(key == "HTTP_OTHER_REGIONS"){
					ctx->resp_header.other_regions.insert(value);
				}
				else{
					ctx->resp_header.std_regions.insert({key, value});
				}
			}
			break;			
	}
    uchar http_state = a_http->http_state;
	//printf("curdir is %d, http_state is %d\n", curdir, http_state);
    //header over
	if(http_state == HTTP_DATA_BEGIN){
		if(curdir == DIR_C2S){
			ctx->req_header.parse_done = true;
		}
		if(curdir == DIR_S2C){
			ctx->resp_header.parse_done = true;
		}
    }
	//data over
	if(http_state == HTTP_DATA_END){
		if(curdir == DIR_C2S){
			ctx->req_body.data_end = true;
		}
		if(curdir == DIR_S2C){
			ctx->resp_body.data_end = true;
		}
	}
	//resume coroutine
	for(lua_State*& coroutine : ctx->coroutines){
		if(coroutine == nullptr){
			continue;
		}
		/*
		int ret = lua_checkstack(coroutine, 1);
        if(ret != 1){
			//do log
            printf("do not have enough space, ret is %d\n", ret);
			continue;
		}
		*/
		lua_pushlightuserdata(coroutine, (void *)ctx);
		lua_setglobal(coroutine, "cur_http_sess_ctx");
		if(stream_begin){
			lua_getglobal(coroutine, "process");
		}
		ret = lua_resume(coroutine, NULL, 0);
		if(ret == LUA_OK){
			coroutine = nullptr;
			continue;
		}
		if(ret != LUA_YIELD){
			lua_traceback("lua_resume", coroutine, ret);
			coroutine = nullptr;
		}
	}
	if(session_info->session_state & SESSION_STATE_CLOSE){	
		//printf("close tcp stream\n");
		clear_http_sess_ctx(ctx);
		*param = NULL;
	}
	return ret;
}

int NEW_HTTP_SERVICE_INIT(void)
{
	g_logger = MESA_create_runtime_log_handle("./log/http/http_service", 10);
	if(g_logger == NULL){		
		printf("%s init : get log handle error!\n", HTTP_SERVICE_PLUGNAME);
		return -1;
	} 

	// get all business lua script which register http
	const char *conflist_path = "./plug/lua/conflist_lua.inf";
	if(g_lua_plugs[LUA_ENTRY_TYPE_HTTP].size() == 0){
		process_lua_plug_conflist(conflist_path);
	}
	return 0;
}

void NEW_HTTP_SERVICE_DESTROY(void)
{
	for(auto plug : g_lua_plugs[LUA_ENTRY_TYPE_HTTP]){
		lua_close(plug.lua_state);
	}
    return ;
}

