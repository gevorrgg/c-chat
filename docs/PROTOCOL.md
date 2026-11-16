# c-chat wire protocol

This document describes exactly what crosses the TCP connection between a
client and the server. All multi-byte integers are big-endian (network byte
order). Key and nonce sizes come from libsodium's `crypto_kx` and
`crypto_aead_chacha20poly1305_ietf` primitives.

## 1. Handshake

Immediately after the TCP connection is established, both peers send their
ephemeral X25519 public key and read the other side's. Both sides send first
and receive second, so the two keys cross on the wire.

```
client                                  server
  |-- client_pk (32 bytes, raw) --------->|
  |<-------- server_pk (32 bytes, raw) ---|
```

Each side then derives two 32-byte session keys:

| side   | call                             | rx (decrypt incoming) | tx (encrypt outgoing) |
|--------|----------------------------------|-----------------------|-----------------------|
| client | `crypto_kx_client_session_keys`  | = server's tx         | = server's rx         |
| server | `crypto_kx_server_session_keys`  | = client's tx         | = client's rx         |

Keys are ephemeral: a new pair is generated for every connection and the
secret keys never leave the stack frame of the exchange function.

If the exchange fails on the server (network error or invalid point), the
server sends `SERVER_KEY_EXCHANGE_ERR` followed by `SERVER_DISCONNECT` and
closes the socket. If the client table is full, it sends
`SERVER_CONNECTION_ERR` followed by `SERVER_DISCONNECT` instead.

## 2. Framing

After the handshake, every transmission in either direction starts with a
4-byte signed status word. A status of `0` means "an encrypted packet
follows"; a negative status is an error notification and stands alone.

```
+------------------+------------------------------------------+
| status (int32)   | packet (only when status == 0)           |
+------------------+------------------------------------------+
```

### Packet layout

```
+------------------+------------------------+------------------------------+
| length (uint32)  | nonce (12 bytes)       | ciphertext (length bytes)    |
+------------------+------------------------+------------------------------+
```

| field      | size            | meaning                                                      |
|------------|-----------------|--------------------------------------------------------------|
| length     | 4               | number of ciphertext bytes that follow                       |
| nonce      | 12              | random per-packet nonce (`crypto_aead_chacha20poly1305_ietf_NPUBBYTES`) |
| ciphertext | `length`        | ChaCha20-Poly1305 output: encrypted plaintext + 16-byte tag  |

`length` is always `plaintext_len + 16` and may not exceed
`MAX_CIPHERTEXT_SIZE = 1024 + 16 = 1040`. A receiver rejects larger values
before reading the ciphertext, so the bound also protects against buffer
overflows from a hostile peer.

No additional authenticated data (AD) is used. The plaintext is the raw bytes
of the line typed by the user, including the trailing `\n` from `fgets()`;
the receiving client strips that newline before printing.

## 3. Status codes

### Client to server (`client_status`)

| value | name                     | meaning                                   |
|-------|--------------------------|-------------------------------------------|
| 0     | `CLIENT_REQUEST_SUCCESS` | a packet follows                          |
| -1    | `CLIENT_DISCONNECTED`    | client is leaving (reserved; the current client never sends it, the server detects a closed socket instead) |

### Server to client (`server_status`)

| value | name                       | followed by packet? | when it is sent                                   |
|-------|----------------------------|---------------------|---------------------------------------------------|
| 0     | `SERVER_REQUEST_SUCCESS`   | yes                 | a message from another client is being delivered  |
| -1    | `SERVER_KEY_EXCHANGE_ERR`  | no                  | handshake failed; `SERVER_DISCONNECT` follows      |
| -2    | `SERVER_CONNECTION_ERR`    | no                  | server is full; `SERVER_DISCONNECT` follows        |
| -3    | `SERVER_MESSAGE_ERR`       | no                  | decrypted message exceeded the server's buffer    |
| -4    | `SERVER_SELECT_ERR`        | no                  | reserved, currently unused                        |
| -5    | `SERVER_DISCONNECT`        | no                  | server is closing this connection                 |
| -6    | `SERVER_ENCRYPTION_FAILED` | no                  | server could not encrypt a packet for this client |
| -7    | `SERVER_DECRYPTION_FAILED` | no                  | the client's last packet failed authentication    |
| -8    | `SERVER_PACKET_RECV_ERR`   | no                  | the client's last packet was malformed            |

The two enums are defined twice, once per binary:
`client/include/server_errors.h` and `server/include/server.h` for
`server_status`, `client/main.c` and `server/include/client.h` for
`client_status`. Keep them in sync.

## 4. Message flow

A normal message from client A to clients B and C:

```
A -> S : [0][len][nonce_A][ct_A]           encrypted with A.tx (= S.rx for A)
S      : decrypt with S.rx(A) -> plaintext
S -> B : [0][len][nonce_B][ct_B]           encrypted with S.tx(B)
S -> C : [0][len][nonce_C][ct_C]           encrypted with S.tx(C)
```

The sender never receives its own message back. Each recipient gets its own
ciphertext and nonce; the server is the only party that sees the plaintext.

## 5. Disconnects

- A client that exits simply closes the socket. The server's `recv_all()`
  returns 0, the client is removed from the table and its slot is freed.
- If the server exits through `disconnect_all_clients()`, every client first
  receives `SERVER_DISCONNECT`.
- If the client's receiver thread fails to read a packet (socket closed or
  malformed data), it prints a message, closes the socket and terminates the
  process.

## 6. Security properties

Provided:
- confidentiality and integrity of every message between a client and the
  server (ChaCha20-Poly1305 with a per-connection key and per-packet nonce);
- forward secrecy per connection, since keys are ephemeral.

Not provided:
- authentication of the server or clients: the handshake has no certificates
  or pre-shared keys, so an active attacker on the path can perform a
  man-in-the-middle attack;
- end-to-end encryption: the server decrypts every message;
- replay protection beyond what the random nonce gives (a replayed packet
  decrypts again under the same session key).
