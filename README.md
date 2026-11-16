# c-chat

 Encrypted client-server chat over TCP using libsodium and custom packet protocol in C.

---

## Features

- TCP client-server architecture
- Encrypted communication using libsodium
- Diffie-Hellman key exchange (crypto_kx)
- Authenticated encryption (ChaCha20-Poly1305 IETF)
- Multithreaded client (receiver thread + input loop)
- Custom binary packet protocol
- Basic server status/error handling

---

## Architecture

### Client
- Connects to server via TCP
- Performs key exchange
- Creates session keys (rx/tx)
- Runs receive thread for incoming messages
- Encrypts outgoing messages before sending

### Server
- Accepts TCP connections
- Performs key exchange with client
- Sends encrypted packets
- Sends status codes for protocol control

---

## Dependencies

- C11 compiler (gcc/clang/MSVC)
- CMake 3.16+ and git
- pthread (POSIX)

libsodium is downloaded and built by CMake automatically via FetchContent,
no manual installation is needed. The build scripts also install `cmake`
and `git` if they are missing (Homebrew on macOS, apt/dnf/yum/pacman/zypper/apk
on Linux, winget on Windows).

## Build

Use the build script for your platform. It configures CMake, fetches
libsodium and builds both `client` and `server` into `./build`.

macOS / Linux:
```bash
./build.sh            # Debug
./build.sh release    # Release
./build.sh clean      # wipe ./build and rebuild
```

Windows:
```bat
build                 # Debug
build release         # Release
build clean           # wipe .\build and rebuild
```

Or manually:
```bash
cmake -S . -B build
cmake --build build
```
