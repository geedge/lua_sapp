#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unordered_set>
#include <unistd.h>
using namespace std;
extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

static void output_stack_size(lua_State *L, int line){
    int n = lua_gettop(L);
    printf("line %d: stack size is %d\n", line, n);
    for(int i = -1; i >= 0 - n; i--){
        string type = string(lua_typename(L, lua_type(L, i)));
        string value = "unknown";
        if(type == "string"){
            value = lua_tostring(L, i);
        }
        printf("index is %d, type is %s, value is %s\n", i, type.c_str(), value.c_str());
    }
}


static void lua_traceback(const char *func_name, lua_State *L){
	int n = lua_gettop(L);
	printf("%s error: stack size is %d, traceback is:\n", func_name, n);
	for(int i = -1; i >= 0 - n; i--){
        string type = string(lua_typename(L, lua_type(L, i)));
        if(type == "string"){
			printf("%s\n", lua_tostring(L, i));
		}
    }
	lua_pop(L, n);
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

static void test_coroutine(int argc, char *argv[]){
    int max_state_num = 10;
    int max_coroutine_num = 100;
    if(argc > 2){
        max_state_num = atoi(argv[1]);
        max_coroutine_num = atoi(argv[2]);
    }
    const char *filename = "../../test/test_coroutine/test.lua";
    vector<lua_State*> states;
    for(int i = 0; i < max_state_num; i++){
    	lua_State *L = luaL_newstate();
    	luaL_openlibs(L);
    	luaL_dofile(L, filename);
	    states.push_back(L);
    }
    vector<pair<lua_State*, lua_State*>> coroutines;
    for(auto state : states){
        int i = 0;
	    for(; i < max_coroutine_num; i++){
            lua_State* l = lua_newthread(state);
            int ret = lua_checkstack(state, 1);
            if(ret != 1)
                printf("do not have enough space, ret is %d\n", ret);
            coroutines.push_back({l, state});
            lua_getglobal(l, "test");
            //cout<<"after getglobal, size = "<<lua_gettop(l)<<endl;
            lua_pushinteger(l, i);
            //cout<<"after pushinterger, size = "<<lua_gettop(l)<<endl;
            lua_resume(l, state, 1);
            //cout<<"after resume, size = "<<lua_gettop(L)<<endl;
	    }
    }
    sleep(2);
    cout<<"coroutine size: "<<coroutines.size()<<endl;
    while(true){
        for(auto coroutine : coroutines){
            lua_resume(coroutine.first, coroutine.second, 0);
        }
    }
}


static int foo (lua_State *L) {
    cout<<"call foo"<<endl;
    output_stack_size(L, 84);
    lua_Number sum = 0.0;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        if (!lua_isnumber(L, i)) {
            printf("index %d is not number\n", i);
            //lua_pushliteral(L, "incorrect argument");
            //lua_error(L);
        }
        sum += lua_tonumber(L, i);
    }
    printf("sum is %f\n", sum);
    lua_pop(L, n);
    output_stack_size(L, 97);
    lua_pushnumber(L, sum/n);        //first result 
    lua_pushnumber(L, sum);         //second result
    output_stack_size(L, 100);
    return 2;                   //number of results
}

static int foo2(lua_State *L, int n, long int m){
    cout<<"cpp: after lua_yield"<<endl;
    return 0;
}

int flag = 0;

static int _foo1(lua_State *L, int status, lua_KContext yieldk_ctx){
    printf("call foo1\n");
    output_stack_size(L, 121);
    /*
    unordered_set<string> regions; 
    output_stack_size(L);
    lua_pushnil(L);
    while(lua_next(L, -2) != 0){
        printf("%s - %s\n", lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)));
        regions.insert(string(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
    for(auto region : regions)
        cout<<region<<endl;
    output_stack_size(L);
    */
    //lua_pop(L, 1);
    if(flag == 1){
        lua_newtable(L);
        lua_push_string_buffer(L, "name", 4);
        lua_newtable(L);
        lua_push_string_buffer(L, "leo", 3);
        lua_rawseti(L, -2, 1);
        lua_push_string_buffer(L, "cui", 3);
        lua_rawseti(L, -2, 2);
        lua_settable(L, -3);
        return 1;
    }
    lua_yieldk(L, 0, 0, _foo1);
}

static int foo1(lua_State *L){
    return _foo1(L, 0, 0);
    /*
    lua_getglobal(L, "g_http_sess_ctx");
    output_stack_size(L);
    int *ctx = (int *)lua_touserdata(L, 1);
    if(ctx == NULL)
        cout<<"ctx is null"<<endl;
    else
        cout<<"ctx is "<<*ctx<<endl;
    cout<<"cpp: call foo1"<<endl;
    */
    //lua_yieldk(L, 0, 0, foo2);
}

static void test_lua_CFunction(){
    const char *filename = "../../test/test_coroutine/test.lua";
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    int ret = luaL_dofile(L, filename);
    printf("luaL_dofile, ret is %d\n", ret);
    lua_register(L, "myfoo", foo1);
    lua_getglobal(L, "lua_call_c");
    /*
    output_stack_size(L);
    lua_getglobal(L, "lua_call_c");
    output_stack_size(L);
    int *ctx = (int *)malloc(sizeof(int));
    *ctx = 3;
    lua_pushlightuserdata(L, (void *)ctx);
    output_stack_size(L);
    lua_setglobal (L, "g_http_sess_ctx");
    output_stack_size(L);
    //output_stack_size(L);
    //lua_pushinteger(L, 2);
    //lua_pushinteger(L, 3);
    */
    //output_stack_size(L);

    ret = lua_resume(L, NULL, 0);
    if(ret != LUA_OK && ret != LUA_YIELD){
        lua_traceback("lua_resume", L);
        return;
    }
    printf("lua resume: ret is %d\n", ret);
    //output_stack_size(L, 190);
    //flag = 1;

    ret = lua_resume(L, NULL, 0);
    printf("lua resume: ret is %d\n", ret);
    output_stack_size(L, 194);


    //printf("lua_resume: ret is %d\n", ret);
    //cout<<"cpp: atfer lua_call"<<endl;
    //lua_resume(L, NULL, 0);
}


int main(int argc, char *argv[]){
    //test_coroutine(argc, argv);
    test_lua_CFunction();
    return 0;    
}