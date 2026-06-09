cmake_minimum_required(VERSION 3.16)

project(c-chat LANGUAGES C)

include(FetchContent)

FetchContent_Declare(
  libsodium
  GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
  GIT_TAG stable
)

FetchContent_MakeAvailable(libsodium)

add_subdirectory(client)
add_subdirectory(server)
