/**
 * @file client.h
 * @brief Per-client connection state.
 *
 * The server keeps a fixed-size array of `struct client_in *`; a NULL slot
 * means "free". Each entry owns one socket and the pair of session keys
 * negotiated with that client.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <sodium.h>

/**
 * @brief Status word a client prepends to every packet it sends.
 *
 * Mirrors the client's own `client_status` enum. Currently the client only
 * ever sends CLIENT_REQUEST_SUCCESS; a disconnect is detected by recv()
 * returning 0 rather than by an explicit CLIENT_DISCONNECTED word.
 */
typedef enum
{
	CLIENT_REQUEST_SUCCESS = 0,   /**< An encrypted packet follows. */
	CLIENT_DISCONNECTED = -1      /**< Client announces it is leaving (unused by current client). */
} client_status;

/** State of one connected client. */
struct client_in
{
	int socket;                              /**< Connected TCP socket. */
	uint8_t rx[crypto_kx_SESSIONKEYBYTES];   /**< Key for decrypting packets from this client. */
	uint8_t tx[crypto_kx_SESSIONKEYBYTES];   /**< Key for encrypting packets to this client. */
};

/**
 * @brief Allocate a client entry and copy the session keys into it.
 *
 * The caller owns the returned pointer and must release it with
 * disconnect_client() (which also closes the socket) or free().
 *
 * @param socket Connected client socket.
 * @param rx     Receive key (crypto_kx_SESSIONKEYBYTES bytes).
 * @param tx     Transmit key (crypto_kx_SESSIONKEYBYTES bytes).
 * @return New entry, or NULL if malloc() failed.
 */
struct client_in* new_client_in(int socket, uint8_t *rx, uint8_t *tx);

#endif
