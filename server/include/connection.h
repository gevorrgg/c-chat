/**
 * @file connection.h
 * @brief Accepting new clients and tearing connections down.
 *
 * handle_new_connection() is called whenever the listening socket becomes
 * readable. It accepts the TCP connection, runs the key exchange and stores
 * the client in the first free slot of the client table. On any failure the
 * client is told why via a status word and the socket is closed.
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>

/** Result of handle_new_connection(). */
typedef enum
{
    CONNECTION_SUCCESS = 0,              /**< Client accepted and stored. */
    CONNECTION_ACCEPT_ERR = -1,          /**< accept() failed. */
    CONNECTION_KEY_EXCHANGE_ERR = -2,    /**< Key exchange failed; socket closed. */
    CONNECTION_INSERTION_ERR = -3,       /**< Client table is full; socket closed. */
    CONNECTION_CLIENT_DISCONNECTED = -4  /**< Reserved for future use. */
    
} connection_status;

struct client_in;

/**
 * @brief Print a human readable description of a connection error.
 * @param status Value returned by handle_new_connection().
 */
void handle_connection_errors(connection_status status);

/**
 * @brief Accept a pending connection, run the key exchange and register the client.
 *
 * @param server_fd     Listening socket that select() reported as readable.
 * @param clients       Client table; the new client goes into the first NULL slot.
 * @param clients_count Number of slots in `clients`.
 * @return A @ref connection_status value.
 */
int handle_new_connection(int server_fd, struct client_in *clients[], size_t clients_count);

/**
 * @brief Close a client's socket and free its entry.
 *
 * Safe to call with NULL. The caller is responsible for clearing the slot
 * in the client table afterwards.
 */
void disconnect_client(struct client_in *client);

/**
 * @brief Notify every client with SERVER_DISCONNECT, then close and free them all.
 *
 * Every slot in the table is set to NULL afterwards.
 */
void disconnect_all_clients(struct client_in *clients[], size_t clients_count);

#endif
