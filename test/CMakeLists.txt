### test_coroutine
add_executable(test_coroutine test_coroutine/main.cpp)
#target_include_directories(test_coroutine PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(test_coroutine lua-static dl)


### test_plugin
add_library(http_service SHARED test_plugin/http_service.cpp)
#target_include_directories(http_service PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(http_service MESA_handle_logger MESA_htable MESA_prof_load)

### new_http_service
add_library(new_http_service SHARED new_http_service/adapter.cpp)
#target_include_directories(new_http_service PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(new_http_service lua-static dl MESA_handle_logger MESA_htable MESA_prof_load)