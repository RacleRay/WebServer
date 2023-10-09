# for each "example/x.cpp", generate target "x"
file(GLOB_RECURSE all_examples *.cpp)
foreach(v ${all_examples})
    string(REGEX MATCH "example/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "example/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v})
endforeach()


# for each "example/x.c", generate target "x"
file(GLOB_RECURSE all_examples *.c)
foreach(v ${all_srcs})
    string(REGEX MATCH "example/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "example/" "" target_name ${relative_path})
    string(REGEX REPLACE ".c" "" target_name ${target_name})

    add_executable(${target_name} ${v})
endforeach()
