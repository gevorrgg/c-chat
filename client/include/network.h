/**
 * @file network.h
 * @brief Low-level socket helpers and packet (de)serialisation.
 *
 * Two layers live here:
 *
 *  - recv_all() / send_all() wrap recv()/send() so that the caller always
 *    gets exactly `n` bytes or a clear error, hiding partial reads/writes and
 *    EINTR retries.
 *  - recv_packet() / send_packet() move a whole @ref packet_in across the
 *    socket using the wire format documented in packet.h.
 */
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

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
 * @brief Read exactly `n` bytes from a socket.
 *
 * Loops over recv() until the buffer is full, retrying on EINTR.
 *
 * @param socket_fd Connected TCP socket.
 * @param buf       Destination buffer of at least `n` bytes.
 * @param n         Number of bytes to read.
 * @return `n` on success, `0` if the peer closed the connection before all
 *         bytes arrived, `-1` on a socket error (errno is set).
 */
int recv_all(int socket_fd, void *buf, size_t n);

/**
 * @brief Write exactly `n` bytes to a socket.
 *
 * Loops over send() until everything is written, retrying on EINTR.
 * MSG_NOSIGNAL is used so a dead peer yields EPIPE instead of SIGPIPE.
 *
 * @param socket_fd Connected TCP socket.
 * @param buf       Data to send.
 * @param n         Number of bytes to send.
 * @return `n` on success, `0` if send() reported zero bytes, `-1` on error.
 */
int send_all(int socket_fd, const void *buf, size_t n);

/**
 * @brief Receive one packet (length, nonce, ciphertext) from the socket.
 *
 * The length field is validated against MAX_CIPHERTEXT_SIZE before the
 * ciphertext is read, so a malicious peer cannot overflow the buffer.
 *
 * @param client_socket Connected TCP socket.
 * @param packet        Output packet; `length` is converted to host order.
 * @return A @ref packet_recv_status value.
 */
int recv_packet(int client_socket, struct packet_in *packet);

/**
 * @brief Send one packet (length, nonce, ciphertext) to the socket.
 *
 * @param client_socket Connected TCP socket.
 * @param packet        Packet filled by packet_encrypt().
 * @return A @ref packet_send_status value.
 */
int send_packet(int client_socket, const struct packet_in *packet);

/**
 * @brief Print a human readable message for a recv_packet() failure.
 *
 * Only PACKET_RECV_DISCONNECTED and PACKET_ENCRYPTION_FAILED produce output;
 * any other code is silently ignored.
 *
 * @param error_code Value returned by recv_packet().
 */
void print_recv_error_message(int error_code);

#endif
