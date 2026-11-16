# c-chat

Encrypted client-server chat over TCP, written in C11 on top of
[libsodium](https://libsodium.org). Every message is end-to-server encrypted
with ChaCha20-Poly1305 using per-connection session keys negotiated through an
X25519 key exchange.

---

## Features

- TCP client-server architecture, single-threaded `select()` server
- Per-connection session keys via libsodium `crypto_kx` (X25519)
- Authenticated encryption with ChaCha20-Poly1305 (IETF, 96-bit nonce)
- Fresh random nonce for every packet
- Multithreaded client: receiver thread plus an interactive input loop
- Compact binary packet protocol with a status word before every packet
  (see [docs/PROTOCOL.md](docs/PROTOCOL.md))
- Broadcast: a message from one client is re-encrypted and delivered to every
  other connected client
- Address and port configurable from the command line, with sensible defaults
- Tampered or undecryptable packets are rejected and reported to the sender
  instead of being relayed
- Clean disconnect handling on both sides: closed sockets are detected, slots
  are freed, Ctrl-D ends the client without spurious errors
- One-command build (`build.sh`) that fetches libsodium and installs
  missing tools

---

## How it works

```
   client A                 server                   client B
      |                        |                        |
      |--- TCP connect ------->|                        |
      |<== key exchange ======>|                        |
      |                        |<------ TCP connect ----|
      |                        |<===== key exchange ===>|
      |                        |                        |
      |-- [status][packet] --->|                        |
      |      (encrypted A->S)  |-- decrypt, re-encrypt  |
      |                        |-- [status][packet] --->|
      |                        |      (encrypted S->B)  |
```

1. A client connects and both sides run the key exchange. Each side derives a
   receive key (`rx`) and a transmit key (`tx`); the client's `tx` is the
   server's `rx` and vice versa.
2. The client reads a line from stdin, encrypts it into a packet and sends a
   4-byte status word followed by the packet.
3. The server decrypts the packet with that client's `rx`, then encrypts the
   plaintext once per other client with that client's `tx` and sends it.
4. Each client's receiver thread reads the status word, then the packet,
   decrypts it and prints it above the prompt.

The server does not store messages and does not authenticate clients. The
key exchange is unauthenticated, so the design protects against passive
eavesdropping but not against an active man-in-the-middle.

---

## Project layout

```
.
├── CMakeLists.txt          # top-level build, fetches libsodium via FetchContent
├── build.sh                # one-shot build script (installs cmake/git if missing)
├── docs/
│   └── PROTOCOL.md         # wire format, handshake and status codes
├── client/
│   ├── main.c              # entry point: CLI args, connect, key exchange, input loop, receiver thread
│   ├── include/
│   │   ├── packet.h        # struct packet_in and size limits
│   │   ├── network.h       # recv_all/send_all, recv_packet/send_packet
│   │   ├── crypto.h        # key exchange, packet_encrypt/packet_decrypt
│   │   └── server_errors.h # SERVER_* status codes and their messages
│   └── src/
│       ├── network.c
│       ├── crypto.c
│       └── server_errors.c
└── server/
    ├── main.c              # entry point: CLI args, listen + select() loop
    ├── include/
    │   ├── packet.h        # same wire format as the client
    │   ├── network.h       # socket helpers + send_broadcast()
    │   ├── crypto.h        # server half of the key exchange, encrypt/decrypt
    │   ├── client.h        # struct client_in (socket + session keys)
    │   ├── connection.h    # accepting and disconnecting clients
    │   └── server.h        # event loop helpers and SERVER_* status codes
    └── src/
        ├── network.c
        ├── crypto.c
        ├── client.c
        ├── connection.c
        └── server.c
```

The client and server deliberately keep separate copies of the shared pieces
(`packet.h`, `network.c`, `crypto.c`) so each binary is self-contained. When
changing the wire format or the status codes, update both sides.

All public functions are documented with Doxygen-style comments in the
headers; running `doxygen` over `client/include` and `server/include` produces
a browsable API reference.

---

## Dependencies

- C11 compiler (gcc/clang)
- CMake 3.16+ and git
- POSIX sockets, `select()` and pthread

Supported platforms are Linux and macOS. The code uses POSIX networking
APIs directly, so it does not build natively on Windows; use WSL there.

libsodium is downloaded and built by CMake automatically via FetchContent,
no manual installation is needed. The build script also installs `cmake`
and `git` if they are missing (Homebrew on macOS, apt/dnf/yum/pacman/zypper/apk
on Linux).

---

## Build

The build script configures CMake, fetches libsodium and builds both
`client` and `server` into `./build` (on Windows, run it inside WSL):

```bash
./build.sh            # Debug
./build.sh release    # Release
./build.sh clean      # wipe ./build and rebuild
```

Or manually:
```bash
cmake -S . -B build
cmake --build build
```

The first configure takes about a minute because libsodium is cloned; later
builds reuse `build/_deps`.

---

## Run

Start the server, then one or more clients in separate terminals:

```bash
./build/server/server [port] [bind_ip]
./build/client/client [server_ip] [port]
```

Both arguments are optional and positional.

| program | argument    | meaning                              | default      |
|---------|-------------|--------------------------------------|--------------|
| server  | `port`      | TCP port to listen on                | `8080`       |
| server  | `bind_ip`   | IPv4 address to listen on            | `0.0.0.0`    |
| client  | `server_ip` | IPv4 address of the server           | `127.0.0.1`  |
| client  | `port`      | TCP port of the server               | `8080`       |

Ports must be in `1..65535`, addresses must be IPv4 literals (hostnames are
not resolved). `--help` or `-h` prints the usage text and exits with status 0.
Any other mistake (unknown option, too many arguments, invalid port or
address) prints the reason followed by the usage text and exits with
status 1, as does a failed `connect()` or `bind()`.

```bash
./build/server/server                  # 0.0.0.0:8080
./build/server/server 9000             # 0.0.0.0:9000
./build/server/server 9000 127.0.0.1   # localhost only

./build/client/client                  # 127.0.0.1:8080
./build/client/client 192.168.1.10     # 192.168.1.10:8080
./build/client/client 192.168.1.10 9000
```

Type a line and press Enter to send it. Every other connected client prints it
as `Received: <text>`. Stop a client with Ctrl-C or Ctrl-D; the server notices
the closed socket and frees the slot.

When the server goes away, clients print `Server is unreachable` and exit.
Server-side problems with a specific message (too large, failed to decrypt)
are reported back to the sending client as an error line.

The maximum number of clients (`MAX_CLIENTS`, 10) is compiled into
[server/main.c](server/main.c).

---

## Limits and known gaps

- Messages are limited to 1024 bytes of plaintext (`MAX_PLAINTEXT_LEN`).
- The key exchange is not authenticated; there is no server certificate or
  pre-shared key, so an active attacker can impersonate the server.
- No usernames, rooms or history. Every connected client sees every message.
- The server processes the key exchange synchronously, so a client that
  connects and stalls blocks the event loop until it answers or disconnects.
- Everything is IPv4 only, and addresses must be given as literals, not
  hostnames.
- The server is single-threaded: `MAX_CLIENTS` is a hard cap and a blocked
  `send()` to one client stalls delivery to the others.
