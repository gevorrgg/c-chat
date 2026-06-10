cmake_minimum_required(VERSION 3.16)

project(server LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(server 
    main.c
    src/server.c 
    src/client.c 
    src/connection.c 
    src/crypto.c 
    src/network.c)

target_include_directories(server PRIVATE ${sodium_INCLUDE_DIRS})
target_link_libraries(server PRIVATE sodium)


