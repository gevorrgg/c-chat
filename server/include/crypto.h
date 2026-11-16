/**
 * @file crypto.h
 * @brief Key exchange and authenticated encryption built on libsodium (server side).
 *
 * Mirrors client/include/crypto.h. The server runs the server half of the
 * X25519 key exchange (`crypto_kx_server_session_keys`) so that its `rx`
 * key matches the client's `tx` key and vice versa. Messages are sealed
 * with ChaCha20-Poly1305 (IETF) and a random 12-byte nonce per packet.
 *
 * The exchange is unauthenticated: clients have no way to verify they are
 * talking to the real server.
 */
#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include <sodium.h>

/** Result of dh_key_exchange_server(). */
typedef enum
{
	DH_EXCHANGE_SUCCESS = 0,   /**< Session keys were derived. */
	DH_EXCHANGE_FAILED         /**< Network error or invalid peer public key. */
} dh_exchange_status;

/** Failure code of packet_decrypt(); on success the plaintext length is returned instead. */
typedef enum
{
	PACKET_DECRYPTION_FAILED = -1,   /**< Bad tag, wrong key or NULL argument. */
	PACKET_DECRYPTION_SUCCESS = 0    /**< Reserved; packet_decrypt() returns the length. */
} packet_decrypt_status;

/** Result of packet_encrypt(). */
typedef enum
{
    PACKET_ENCRYPTION_SUCCESS = 0,   /**< Packet is filled and ready to send. */
    PACKET_ENCRYPTION_FAILED = -1    /**< NULL argument or ciphertext would not fit. */
} packet_encrypt_status;

struct packet_in;

/**
 * @brief Perform the server side of the key exchange with one client.
 *
 * Generates an ephemeral key pair, sends the public key, receives the
 * client's public key and derives the session keys with
 * crypto_kx_server_session_keys().
 *
 * @param client_socket_fd Freshly accepted client socket.
 * @param rx  Output buffer of crypto_kx_SESSIONKEYBYTES: key for decrypting this client's packets.
 * @param tx  Output buffer of crypto_kx_SESSIONKEYBYTES: key for encrypting packets to this client.
 * @return DH_EXCHANGE_SUCCESS or DH_EXCHANGE_FAILED.
 */
int dh_key_exchange_server(size_t client_socket_fd, uint8_t *rx, uint8_t *tx);

/**
 * @brief Decrypt and authenticate a packet received from a client.
 *
 * @param packet        Packet filled by recv_packet().
 * @param plaintext     Output buffer of at least MAX_PLAINTEXT_LEN bytes.
 * @param plaintext_len Receives the number of plaintext bytes written.
 * @param rx            That client's receive session key.
 * @return The plaintext length (>= 0) on success, PACKET_DECRYPTION_FAILED otherwise.
 */
int packet_decrypt(const struct packet_in *packet, uint8_t *plaintext, unsigned long long *plaintext_len, const uint8_t *rx);

/**
 * @brief Encrypt a message for one client into a packet with a fresh nonce.
 *
 * @param plaintext     Message bytes.
 * @param plaintext_len Message length; must not exceed MAX_PLAINTEXT_LEN.
 * @param packet        Output packet; `nonce`, `ciphertext` and `length` are set.
 * @param tx            That client's transmit session key.
 * @return PACKET_ENCRYPTION_SUCCESS or PACKET_ENCRYPTION_FAILED.
 */
int packet_encrypt(const uint8_t *plaintext, size_t plaintext_len, struct packet_in *packet, const uint8_t *tx);

#endif
