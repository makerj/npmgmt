cmake_minimum_required(VERSION 3.5)
project(npmgmt)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

include_directories("include")

FILE(GLOB_RECURSE SOURCE_FILES src/*.c)
add_executable(ProtocolConverter ${SOURCE_FILES})
