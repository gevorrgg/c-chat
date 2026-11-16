/**
 * @file server_errors.h
 * @brief Status codes the server prepends to every message it sends.
 *
 * Before each packet the server writes a 4-byte big-endian status word.
 * SERVER_REQUEST_SUCCESS means "a packet follows"; any negative value is an
 * error notification and is *not* followed by a packet. The values here must
 * stay in sync with `server_status` in server/include/server.h.
 */
#ifndef SERVER_ERRORS_H
#define SERVER_ERRORS_H

#include <stdint.h>

/** Status word sent by the server before each packet. */
typedef enum
{
    SERVER_REQUEST_SUCCESS = 0,      /**< An encrypted packet follows. */
    SERVER_KEY_EXCHANGE_ERR = -1,    /**< Key exchange failed; server will disconnect. */
    SERVER_CONNECTION_ERR = -2,      /**< Server is full; server will disconnect. */
    SERVER_MESSAGE_ERR = -3,         /**< Our last message exceeded the size limit. */
    SERVER_SELECT_ERR = -4,          /**< Internal select() failure on the server. */
    SERVER_DISCONNECT = -5,          /**< Server is closing this connection. */
    SERVER_ENCRYPTION_FAILED = -6,   /**< Server could not encrypt a packet for us. */
    SERVER_DECRYPTION_FAILED = -7,   /**< Server could not decrypt our last packet. */
    SERVER_PACKET_RECV_ERR = -8      /**< Server failed to read our last packet. */
} server_status;

/**
 * @brief Print a human readable description of a server status code.
 *
 * SERVER_REQUEST_SUCCESS and unknown codes print nothing.
 *
 * @param status Status word received from the server (host byte order).
 */
void handle_server_message_errors(server_status status);

#endif
