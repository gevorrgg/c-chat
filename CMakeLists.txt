cmake_minimum_required(VERSION 3.16)

project(c-chat LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

# Upstream libsodium is built with autotools and has no CMakeLists.txt,
# so we fetch a CMake wrapper that pulls libsodium in as a git submodule
# and exposes a `sodium` target.
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(SODIUM_MINIMAL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  sodium
  GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
  GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d
  GIT_SUBMODULES_RECURSE ON
)

FetchContent_MakeAvailable(sodium)

add_subdirectory(client)
add_subdirectory(server)
