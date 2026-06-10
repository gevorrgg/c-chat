cmake_minimum_required(VERSION 3.16)

project(client LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(client 
   main.c
   src/crypto.c
   src/network.c
   src/server_errors.c)

target_include_directories(client PRIVATE ${sodium_INCLUDE_DIRS})
target_link_libraries(client PRIVATE sodium)


