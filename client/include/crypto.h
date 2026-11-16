/**
 * @file crypto.h
 * @brief Key exchange and authenticated encryption built on libsodium.
 *
 * Security model:
 *
 *  1. On connect, client and server run an X25519 key exchange
 *     (`crypto_kx_*`). Each side ends up with two 32-byte session keys:
 *     `rx` to decrypt what the peer sends and `tx` to encrypt what we send.
 *     The client's `tx` equals the server's `rx` and vice versa.
 *  2. Every message is sealed with ChaCha20-Poly1305 (IETF variant) using a
 *     fresh random 12-byte nonce. The tag guarantees that a tampered packet
 *     fails to decrypt instead of producing garbage.
 *
 * The exchange is *not* authenticated: nothing proves the server's identity
 * to the client, so an active man-in-the-middle could substitute keys.
 */
#ifndef CRYPTO_H
#define CRYPTO_H

#include <sodium.h>
#include <stdint.h>

/** Result of dh_key_exchange_client(). */
typedef enum
{
	DH_EXCHANGE_SUCCESS = 0,   /**< Session keys were derived. */
	DH_EXCHANGE_FAILED = -1    /**< Network error or invalid peer public key. */
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
 * @brief Perform the client side of the key exchange.
 *
 * Generates an ephemeral key pair, sends the public key, receives the
 * server's public key and derives the session keys with
 * crypto_kx_client_session_keys().
 *
 * @param client_socket_fd Connected TCP socket to the server.
 * @param rx  Output buffer of crypto_kx_SESSIONKEYBYTES: key for decrypting server packets.
 * @param tx  Output buffer of crypto_kx_SESSIONKEYBYTES: key for encrypting our packets.
 * @return DH_EXCHANGE_SUCCESS or DH_EXCHANGE_FAILED.
 */
int dh_key_exchange_client(size_t client_socket_fd, uint8_t *rx, uint8_t *tx);

/**
 * @brief Decrypt and authenticate a received packet.
 *
 * @param packet        Packet filled by recv_packet().
 * @param plaintext     Output buffer of at least MAX_PLAINTEXT_LEN bytes.
 * @param plaintext_len Receives the number of plaintext bytes written.
 * @param rx            Receive session key from the key exchange.
 * @return The plaintext length (>= 0) on success, PACKET_DECRYPTION_FAILED otherwise.
 */
int packet_decrypt(const struct packet_in *packet, uint8_t *plaintext, unsigned long long *plaintext_len, const uint8_t *rx);

/**
 * @brief Encrypt a message into a packet with a fresh random nonce.
 *
 * @param plaintext     Message bytes (not required to be NUL-terminated).
 * @param plaintext_len Message length; must not exceed MAX_PLAINTEXT_LEN.
 * @param packet        Output packet; `nonce`, `ciphertext` and `length` are set.
 * @param tx            Transmit session key from the key exchange.
 * @return PACKET_ENCRYPTION_SUCCESS or PACKET_ENCRYPTION_FAILED.
 */
int packet_encrypt(const uint8_t *plaintext, size_t plaintext_len, struct packet_in *packet, const uint8_t *tx);

#endif
