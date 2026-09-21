cmake_minimum_required(VERSION 4.2)
project(meow_protocol)

set(CMAKE_CXX_STANDARD 20)

add_executable(meow_protocol main.cpp
        TcpNode.cpp
        TcpNode.h
#        server.cpp
        server.h
#        client.cpp
        client.h)
