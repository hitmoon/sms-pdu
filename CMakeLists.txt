cmake_minimum_required(VERSION 3.0)
project(SMS)

set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(tool_root /home/hitmoon/arm-linux-androideabi-4.8)
set(CMAKE_SYSROOT ${tool_root}/sysroot)

set(CMAKE_C_COMPILER ${tool_root}/bin/arm-linux-androideabi-gcc)
set(CMAKE_CXX_COMPILER ${tool_root}/bin/arm-linux-androideabi-g++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -Wall")

set(SOURCE_FILES main.c SMS.c SMS.h alphabet.h utf.h utf.c)
add_executable(SMS ${SOURCE_FILES})

