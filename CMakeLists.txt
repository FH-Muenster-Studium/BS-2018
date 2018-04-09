cmake_minimum_required(VERSION 3.10)
project(OSMP C)
project(echoall C)

set(CMAKE_C_STANDARD 11)

#set(CMAKE_C_FLAGS "-fsanitize=addaddressress -fsanitize-address-use-after-scope -O1 -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "fsanitize=leak")

add_executable(OSMP main.c)

add_executable(echoall echoall.c)