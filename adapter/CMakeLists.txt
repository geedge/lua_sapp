### http_adapter
add_library(http_adapter SHARED http_adapter/src/adapter.cpp)
target_link_libraries(http_adapter lua-static dl MESA_handle_logger MESA_htable MESA_prof_load)