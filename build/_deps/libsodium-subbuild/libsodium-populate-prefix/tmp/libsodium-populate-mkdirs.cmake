# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/user/github/c-chat/build/_deps/libsodium-src"
  "/home/user/github/c-chat/build/_deps/libsodium-build"
  "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix"
  "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/tmp"
  "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp"
  "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/src"
  "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/user/github/c-chat/build/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
