/**
 * @file network.h
 * @brief Socket helpers, packet (de)serialisation and broadcasting.
 *
 * recv_all()/send_all() guarantee full reads and writes over TCP,
 * recv_packet()/send_packet() move a whole @ref packet_in, and
 * send_broadcast() re-encrypts a plaintext message for every connected
 * client with that client's own session key.
 */
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/** Result of recv_packet(). */
typedef enum
{
	PACKET_RECV_SUCCESS = 0,        /**< A full packet was read into the struct. */
	PACKET_RECV_ERR = -1,           /**< Socket error or malformed length field. */
	PACKET_RECV_DISCONNECTED = -2   /**< Peer closed the connection mid-packet. */
} packet_recv_status; 

/** Result of send_packet(). */
typedef enum
{
	PACKET_SEND_SUCCESS = 0,        /**< Every byte of the packet was sent. */
	PACKET_SEND_ERR = -1,           /**< Socket error while sending. */
	PACKET_SEND_DISCONNECTED = -2   /**< send() returned 0, peer is gone. */
} packet_send_status;

struct packet_in;
struct client_in;

/**
 * @brief Read exactly `n` bytes from a socket, retrying on EINTR.
 * @return `n` on success, `0` if the peer closed the connection, `-1` on error.
 */
int recv_all(int socket_fd, void *buf, size_t n);

/**
 * @brief Write exactly `n` bytes to a socket, retrying on EINTR.
 *
 * Uses MSG_NOSIGNAL so writing to a closed socket returns EPIPE instead of
 * killing the server with SIGPIPE.
 *
 * @return `n` on success, `0` if send() reported zero bytes, `-1` on error.
 */
int send_all(int socket_fd, const void *buf, size_t n);

/**
 * @brief Receive one packet (length, nonce, ciphertext) from a client.
 *
 * The length field is checked against MAX_CIPHERTEXT_SIZE before any
 * ciphertext is read, so a hostile client cannot overflow the buffer.
 *
 * @return A @ref packet_recv_status value.
 */
int recv_packet(int client_socket, struct packet_in *packet);

/**
 * @brief Send one packet (length, nonce, ciphertext) to a client.
 * @return A @ref packet_send_status value.
 */
int send_packet(int client_socket, const struct packet_in *packet);

/**
 * @brief Forward a plaintext message to every client except the sender.
 *
 * For each recipient the message is encrypted separately with that
 * recipient's `tx` key, prefixed with SERVER_REQUEST_SUCCESS and sent.
 * If encryption fails for a recipient, SERVER_ENCRYPTION_FAILED is sent to
 * that recipient instead and the loop continues.
 *
 * @param clients          Client table; NULL slots are skipped.
 * @param clients_count    Number of slots in `clients`.
 * @param message          Decrypted message bytes.
 * @param message_len      Length of `message`.
 * @param sender_socket_fd Socket of the original sender, which is skipped.
 */
void send_broadcast(const struct client_in *clients[], size_t clients_count, const uint8_t *message, size_t message_len, int sender_socket_fd);

#endif
