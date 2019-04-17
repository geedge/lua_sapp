#include "adapter.h"

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
	LUA_ENTRY_TYPE_FTP,
};


//TODO:
// other_regions 有多个, 怎么处理
// 增加根据region返回header功能

class lua_plug{
public:
	std::string file_path;
	enum lua_entry_type entry_type;
	int dir; 
	std::unordered_set<std::string> regions;
	lua_State *lua_state;
};

static void* g_logger = NULL;
static std::vector<std::vector<lua_plug>> g_lua_plugs(LUA_ENTRY_TYPE_NUM, std::vector<lua_plug>());

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


static int _get_http_headers(lua_State *lua_state, int status, lua_KContext yieldk_ctx){
	lua_getglobal(lua_state, "cur_http_sess_ctx");
    http_sess_ctx *sess_ctx = (http_sess_ctx *)lua_touserdata(lua_state, -1);
	lua_getglobal(lua_state, "cur_http_sess_dir");
	int curdir = (int)lua_tonumber(lua_state, -1);
	lua_pop(lua_state, 2);
	if(sess_ctx == NULL){
		//do log
		std::cout<<"sess_ctx is null"<<std::endl; 
		return 0;
	}
	lua_newtable(lua_state);
    for(auto header : curdir == DIR_C2S ? sess_ctx->request_headers : sess_ctx->response_headers){
		lua_push_string_buffer(lua_state, header.first.c_str(), header.first.length() + 1);
		lua_push_string_buffer(lua_state, header.second.c_str(), header.second.length() + 1);
		lua_settable(lua_state, -3);
	}
	//return 1 means one return value
	return 1;
}

static int get_http_headers(lua_State *lua_state){
	lua_yieldk(lua_state, 0, 0, _get_http_headers);
	return 1;
}

int regions_parse(char *str, std::unordered_set<std::string>& regions){
	char *token;
	char *rest = str;
	while((token = strtok_r(rest, ",", &rest))){
		std::string _token = std::string(token);
		if(_token == "all"){
			regions.clear();
			regions.insert(_token);
			return 0;
		}
		regions.insert(_token);
	}
	return 0;
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
	std::unordered_map<std::string, int> dir_map{
		{"c2s", DIR_C2S},
		{"s2c", DIR_S2C},
		{"all", DIR_DOUBLE}
	};
	const char *section = "main";
	char file_path[LUA_SAPP_PATH_MAX] = "";
	char entry_type[LUA_SAPP_SYMBOL_MAX] = "";
	char dir[LUA_SAPP_SYMBOL_MAX] = "";
	char regions[LUA_SAPP_STRING_MAX] = "";
	MESA_load_profile_string_def(profile, section, "file_path", file_path, sizeof(file_path), "");
	MESA_load_profile_string_def(profile, section, "entry_type", entry_type, sizeof(entry_type), "http");
	MESA_load_profile_string_def(profile, section, "dir", dir, sizeof(dir), "all");
	MESA_load_profile_string_def(profile, section, "regions", regions, sizeof(regions), "all");
	
	lua_State *lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    luaL_dofile(lua_state, file_path);
    lua_register(lua_state, "get_http_headers", get_http_headers);
    lua_getglobal(lua_state, "main");

	lua_plug plug;
	plug.file_path = std::string(file_path);
	plug.entry_type = (enum lua_entry_type)type_map[std::string(entry_type)];
	plug.dir = dir_map[std::string(dir)];
	regions_parse(regions, plug.regions);
	plug.lua_state = lua_state;
	g_lua_plugs[plug.entry_type].push_back(plug);
    lua_resume(lua_state, NULL, 0);
	return 0;
}

int process_lua_plug_conflist(const char* filename)
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
		load_lua_plug(lua_plug_conf_path);
	}
	fclose(fp);
	fp=NULL;
	return 0;
}

static http_sess_ctx* init_http_sess_ctx(){
    http_sess_ctx *ctx = new http_sess_ctx();
	return ctx;
}

static void clear_http_sess_ctx(http_sess_ctx *ctx){
    delete ctx;
    ctx = NULL;
}

static std::string trans_to_binary(unsigned int n){
	std::string res;
	while(n){
		res = std::to_string(n % 2) + res;
		n /= 2;
	}
	return res;
}

static void output_stack_size(lua_State *lua_state){
    int n = lua_gettop(lua_state);
    std::cout<<"stack size is "<<n<<std::endl;
}

uchar NEW_HTTP_SERVICE_ENTRY(stSessionInfo* session_info, void **param, int thread_seq, struct streaminfo *a_tcp, void *a_packet){	
	uchar rec = PROT_STATE_GIVEME;
	http_sess_ctx *ctx = (http_sess_ctx*)*param;	
	http_infor *a_http = (http_infor *)(session_info->app_info);
	//static int		header_len = 0;
	//static int		content_len = 0;
	if(NULL==session_info){
		return PROT_STATE_DROPME;
	}
	if(ctx == NULL){
		ctx = init_http_sess_ctx();
		*param = ctx;
	}	
	std::string prot_flag_str = trans_to_binary(session_info->prot_flag);
	//MESA_handle_runtime_log(g_logger, RLOG_LV_INFO, HTTP_SERVICE_PLUGNAME, 
	//		"call http_service entry, http_state is %02x\n, prot_flag mask is %s : %d", http_state, prot_flag_str.c_str(), prot_flag_str.length() - 1);
	std::string temp = (a_http->http_state == HTTP_DATA_BEGIN) ? "data begin" : "not data begin";
	//curdir:  request or response 
    uchar curdir = a_http->curdir;
	switch(session_info->prot_flag)
	{
		case HTTP_STATE: 
		case HTTP_CONTENT:
			break;
		case HTTP_UNGZIP_CONTENT:	
			//fwrite(session_info->buf, session_info->buflen, 1, service_pme->fp);			
			//fflush(service_pme->fp);			
			break;
		default:
			std::string key(http_proto_flag2region(session_info->prot_flag));
            std::string value((const char*)(session_info->buf), session_info->buflen);
			if(curdir == DIR_C2S){
            	ctx->request_headers.insert({key, value});
			}
			else{
				ctx->response_headers.insert({key, value});
			}
			break;			
	}
    uchar http_state = a_http->http_state;
    //header over, resume lua
	if(http_state == HTTP_DATA_BEGIN){
		for(auto plug : g_lua_plugs[LUA_ENTRY_TYPE_HTTP]){
			if(plug.dir == DIR_DOUBLE || plug.dir == curdir){
				lua_pushlightuserdata(plug.lua_state, (void *)ctx);
    			lua_setglobal(plug.lua_state, "cur_http_sess_ctx");
				lua_pushinteger(plug.lua_state, curdir);
				lua_setglobal(plug.lua_state, "cur_http_sess_dir");
        		lua_resume(plug.lua_state, NULL, 0);
			}
		}
    }
	if(session_info->session_state & SESSION_STATE_CLOSE){	
		clear_http_sess_ctx(ctx);
		*param = NULL;
	}
	return rec;
}

int NEW_HTTP_SERVICE_INIT(void)
{
	g_logger = MESA_create_runtime_log_handle("./log/http/http_service", 10);
	if(g_logger == NULL){		
		printf("%s init : get log handle error!\n", HTTP_SERVICE_PLUGNAME);
		return -1;
	} 
	// get thread num
	// get all business lua script which register http
	const char *conflist_path = "./plug/lua/conflist_lua.inf";
	if(g_lua_plugs[LUA_ENTRY_TYPE_HTTP].size() == 0){
		process_lua_plug_conflist(conflist_path);
	}
	return 0;
}

void NEW_HTTP_SERVICE_DESTROY(void)
{
    return ;
}

