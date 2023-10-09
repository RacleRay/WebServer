# for each "src/x.cpp", generate target "x"
file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)
foreach(v ${all_srcs})
    string(REGEX MATCH "src/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "src/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v})
endforeach()


# for each "src/x.c", generate target "x"
file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.c)
foreach(v ${all_srcs})
    string(REGEX MATCH "src/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "src/" "" target_name ${relative_path})
    string(REGEX REPLACE ".c" "" target_name ${target_name})

    add_executable(${target_name} ${v})
endforeach()
