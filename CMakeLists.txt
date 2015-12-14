cmake_minimum_required(VERSION 3.3)
project(SMS)

#set(CMAKE_SYSTEM_NAME Android)
#set(CMAKE_SYSTEM_PROCESSOR arm)
#set(tool_root /home/hitmoon/arm-linux-androideabi-4.8)
#set(CMAKE_SYSROOT ${tool_root}/sysroot)
#
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
#
#set(CMAKE_C_COMPILER ${tool_root}/bin/arm-linux-androideabi-gcc)
#set(CMAKE_CXX_COMPILER ${tool_root}/bin/arm-linux-androideabi-g++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

set(SOURCE_FILES main.cpp SMS.cpp SMS.h alphabet.h)
add_executable(SMS ${SOURCE_FILES})