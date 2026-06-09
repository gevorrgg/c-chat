cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES C)

add_executable(server 
    src/server.c 
    src/client.c 
    src/connection.c 
    src/crypto.c 
    src/network.c)

target_include_directories(server PRIVATE ${libsodium_INCLUDE_DIRS})
target_link_libraries(server PRIVATE libsodium)


