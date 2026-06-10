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

- C11 compiler (gcc/clang)
- pthread
- libsodium

## Build
### Client
```bash
mkdir build
cd build
cmake ..
cmake --build . --target client
```

### Server
```bash
mkdir build
cd build
cmake ..
cmake --build . --target build
