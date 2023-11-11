# # for each "src/x.cpp", generate target "x"
# file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)
# foreach(v ${all_srcs})
#     string(REGEX MATCH "src/.*" relative_path ${v})
#     # message(${relative_path})
#     string(REGEX REPLACE "src/" "" target_name ${relative_path})
#     string(REGEX REPLACE ".cpp" "" target_name ${target_name})

#     add_executable(${target_name} ${v})
# endforeach()


# # for each "src/x.c", generate target "x"
# file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.c)
# foreach(v ${all_srcs})
#     string(REGEX MATCH "src/.*" relative_path ${v})
#     # message(${relative_path})
#     string(REGEX REPLACE "src/" "" target_name ${relative_path})
#     string(REGEX REPLACE ".c" "" target_name ${target_name})

#     add_executable(${target_name} ${v})
# endforeach()


file(GLOB_RECURSE log_srcs CONFIGURE_DEPENDS ./logger/*.cpp)
file(GLOB_RECURSE file_srcs CONFIGURE_DEPENDS ./file/*.cpp)
file(GLOB_RECURSE thread_srcs CONFIGURE_DEPENDS ./thread/*.cpp)
file(GLOB_RECURSE reactor_srcs CONFIGURE_DEPENDS ./reactor/*.cpp)
file(GLOB_RECURSE http_srcs CONFIGURE_DEPENDS ./http/*.cpp)
file(GLOB_RECURSE server_srcs CONFIGURE_DEPENDS ./server/*.cpp)
file(GLOB_RECURSE utils_srcs CONFIGURE_DEPENDS ./utils/*.cpp)
file(GLOB_RECURSE timer_srcs CONFIGURE_DEPENDS ./timer/*.cpp)


add_library(serveutils STATIC ${log_srcs} ${file_srcs} ${thread_srcs} ReadConfig.cpp)
target_link_libraries(serveutils pthread)
# set_target_properties(serveutils PROPERTIES OUTPUT_NAME "server")


add_executable(server main.cpp 
    ${log_srcs} 
    ${file_srcs} 
    ${thread_srcs}
    ${reactor_srcs} 
    ${http_srcs} 
    ${server_srcs} 
    ${utils_srcs}
    ${timer_srcs} 
    ReadConfig.cpp    
)