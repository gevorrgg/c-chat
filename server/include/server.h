/**
 * @file server.h
 * @brief Main event loop pieces: waiting for I/O and dispatching client messages.
 *
 * The server is single-threaded and multiplexes all sockets with select().
 * One iteration of the loop in main() is:
 *
 *  1. wait_for_io_events()     - block until a socket is readable;
 *  2. handle_new_connection()  - if the listening socket is readable;
 *  3. handle_client_messages() - for every client socket that is readable,
 *     read one status word plus one packet and broadcast it to the others.
 */
#ifndef SERVER_H
#define SERVER_H

#include "network.h"
#include <sys/select.h>

/** Result of handle_client_message(). */
typedef enum
{
    CLIENT_MSG_SUCCESS = 0,             /**< Message decrypted and broadcast. */
    CLIENT_MSG_TOO_LARGE = -1,          /**< Plaintext exceeds the caller's buffer. */
    CLIENT_MSG_DECRYPTION_FAILED = -2,  /**< Authentication tag did not verify. */
    CLIENT_MSG_RECV_ERR = -3,           /**< Socket error or malformed packet. */
    CLIENT_MSG_DISCONNECTED = -4        /**< Client closed the connection. */
} client_message_status;

/**
 * @brief Status word the server prepends to everything it sends.
 *
 * SERVER_REQUEST_SUCCESS is followed by a packet; negative values are sent
 * alone. Must stay in sync with `server_status` in
 * client/include/server_errors.h.
 */
typedef enum
{
    SERVER_REQUEST_SUCCESS = 0,      /**< An encrypted packet follows. */
    SERVER_KEY_EXCHANGE_ERR = -1,    /**< Key exchange failed; connection will be closed. */
    SERVER_CONNECTION_ERR = -2,      /**< Server is full; connection will be closed. */
    SERVER_MESSAGE_ERR = -3,         /**< Client's message exceeded the size limit. */
    SERVER_SELECT_ERR = -4,          /**< Internal select() failure (currently unused). */
    SERVER_DISCONNECT = -5,          /**< Server is closing the connection. */
    SERVER_ENCRYPTION_FAILED = -6,   /**< Could not encrypt a packet for this client. */
    SERVER_DECRYPTION_FAILED = -7,   /**< Could not decrypt the client's packet. */
    SERVER_PACKET_RECV_ERR = -8      /**< Failed to read the client's packet. */
} server_status;

/**
 * @brief Print a human readable description of a message handling error.
 * @param status Value returned by handle_client_message().
 */
void handle_message_errors(client_message_status status);

/**
 * @brief Read one packet from `sender`, decrypt it and broadcast it.
 *
 * On failure the sender is informed with the matching SERVER_* status word.
 *
 * @param clients          Client table used for the broadcast.
 * @param clients_count    Number of slots in `clients`.
 * @param sender           Client whose socket is readable.
 * @param message          Scratch buffer that receives the plaintext.
 * @param max_message_size Size of `message`.
 * @return A @ref client_message_status value.
 */
int handle_client_message(
    const struct client_in *clients[],
    size_t clients_count, 
    const struct client_in *sender, 
    uint8_t *message, 
    size_t max_message_size);

/**
 * @brief Build the read set from the listening socket and all clients, then block in select().
 *
 * @param readfds       Output set; readable descriptors are set on return.
 * @param server_fd     Listening socket.
 * @param clients       Client table; NULL slots are skipped.
 * @param clients_count Number of slots in `clients`.
 */
void wait_for_io_events(fd_set *readfds, int server_fd, struct client_in *clients[], size_t clients_count);

/**
 * @brief Service every client whose socket is readable.
 *
 * Reads the client's status word, then either removes the client (on
 * CLIENT_DISCONNECTED or a closed socket) or passes the connection to
 * handle_client_message(). Slots of disconnected clients are set to NULL.
 *
 * @param clients          Client table.
 * @param clients_count    Number of slots in `clients`.
 * @param readfds          Set filled by wait_for_io_events().
 * @param message_buffer   Scratch buffer for decrypted messages.
 * @param max_message_size Size of `message_buffer`.
 */
void handle_client_messages(struct client_in *clients[], size_t clients_count, fd_set *readfds, uint8_t *message_buffer, size_t max_message_size);


#endif
