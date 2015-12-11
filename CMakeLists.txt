cmake_minimum_required(VERSION 3.3)
project(SMS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(SOURCE_FILES main.cpp SMS.cpp SMS.h alphabet.h)
add_executable(SMS ${SOURCE_FILES})