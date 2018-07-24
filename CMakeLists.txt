cmake_minimum_required(VERSION 3.10)
project(threadPoolProj)

set(CMAKE_CXX_STANDARD 11)

add_executable(threadPoolProj main.cpp myThreadPool.h myThreadPool.cpp)