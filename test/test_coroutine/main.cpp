#include <iostream>
#include <stdio.h>
#include <vector>
#include <unistd.h>
using namespace std;
extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

static void output_stack_size(lua_State *L){
    int n = lua_gettop(L);
    cout<<"stack size is "<<n<<endl;
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
    output_stack_size(L);
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
    output_stack_size(L);
    lua_pushnumber(L, sum/n);        //first result 
    lua_pushnumber(L, sum);         //second result
    output_stack_size(L);
    return 2;                   //number of results
}

static int foo2(lua_State *L, int n, long int m){
    cout<<"cpp: after lua_yield"<<endl;
    return 0;
}

static int foo1(lua_State *L){
    output_stack_size(L);
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
    luaL_dofile(L, filename);
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
    lua_resume(L, NULL, 0);
    cout<<"cpp: atfer lua_call"<<endl;
    //lua_resume(L, NULL, 0);
}


int main(int argc, char *argv[]){
    //test_coroutine(argc, argv);
    test_lua_CFunction();
    return 0;    
}