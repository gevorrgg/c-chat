cmake_minimum_required(VERSION 3.16)

project(client LANGUAGES C)

add_executable(client 
   client.c)

target_include_directories(client PRIVATE ${libsodium_INCLUDE_DIRS})
target_link_libraries(client PRIVATE libsodium)


