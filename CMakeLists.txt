cmake_minimum_required(VERSION 3.16)

project(c-chat LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  sodium
  GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
  GIT_TAG stable
)

FetchContent_MakeAvailable(sodium)

add_subdirectory(client)
add_subdirectory(server)
