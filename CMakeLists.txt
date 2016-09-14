project(PRIMES)

cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall -Werror")


set(Folder "src")
file(GLOB Files "${Folder}/*.c")
foreach (File ${Files})

    STRING(REGEX MATCH "[^/]+.c$" Basename ${File})
    STRING(REGEX REPLACE ".c" "" Title "${Basename}")
    add_executable("${Title}" "${Folder}/${Basename}" src/vish.c)
    set_target_properties("${Title}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${Folder}")

endforeach ()
