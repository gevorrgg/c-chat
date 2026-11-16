#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sodium.h>
#include <errno.h>
#include <arpa/inet.h>
#include "../include/network.h"
#include "../include/packet.h"
#include "../include/crypto.h"

/*
 * recv() may return fewer bytes than requested, so keep calling it until the
 * whole buffer is filled. A return value of 0 from recv() means the peer
 * performed an orderly shutdown; we propagate that as 0 so callers can tell
 * "disconnected" apart from "error".
 */
int recv_all(int socket_fd, void *buf, size_t n)
{
    size_t received = 0;
    size_t bytes_to_read = n;

    while (received < n)
    {
        int read_bytes = recv(socket_fd, buf + received, bytes_to_read, 0);

        if (read_bytes < 0)
        {
            if (errno == EINTR) /* interrupted by a signal: just retry */
                continue;

            return -1;
        }

        if (read_bytes == 0) /* peer closed the connection */
        {
            return 0;
        }
    
        received += read_bytes;
        bytes_to_read -= read_bytes;
    }

    return n;
}

/*
 * Counterpart of recv_all(): send() may write only part of the buffer, so
 * loop until everything is out. MSG_NOSIGNAL turns a write to a closed
 * socket into an EPIPE error instead of a process-killing SIGPIPE.
 */
int send_all(int socket_fd, const void *buf, size_t n)
{
    size_t sent = 0;
    size_t bytes_to_send = n;

    while (sent < n)
    {
        int sent_bytes = send(socket_fd, buf + sent, bytes_to_send, MSG_NOSIGNAL);

        if (sent_bytes < 0)
        {
            if (errno == EINTR) /* interrupted by a signal: just retry */
                continue;

            return -1;
        }

        if (sent_bytes == 0) /* nothing could be written, treat as gone */
        {
            return 0;
        }
    
        sent += sent_bytes;
        bytes_to_send -= sent_bytes;
    }

    return n;
}

/*
 * Reads one packet in three steps that mirror the wire layout:
 * 4-byte big-endian length, 12-byte nonce, then `length` bytes of ciphertext.
 * The length is validated before the ciphertext read so that a bogus value
 * can never write past packet->ciphertext.
 */
int recv_packet(int client_socket, struct packet_in *packet)
{
	// reading length (network byte order on the wire)

	uint32_t net_len;

    int len_recv_status = recv_all(client_socket, &net_len, sizeof(uint32_t));

	if (len_recv_status < 0)
	{
		return PACKET_RECV_ERR;
	} 
    else if (len_recv_status == 0)
    {
        return PACKET_RECV_DISCONNECTED;
    }

	packet->length = ntohl(net_len);

	// reject oversized packets before touching the ciphertext buffer
	if (packet->length > MAX_CIPHERTEXT_SIZE)
	{
		return PACKET_RECV_ERR;
	}

	// reading nonce
    int nonce_recv_status = recv_all(client_socket,  packet->nonce, crypto_aead_chacha20poly1305_ietf_NPUBBYTES);

	if (nonce_recv_status < 0)
	{
		return PACKET_RECV_ERR;
	}
    else if (nonce_recv_status == 0)
    {
        return PACKET_RECV_DISCONNECTED;
    }

	// reading ciphertext
    int ciphertext_recv_status = recv_all(client_socket, packet->ciphertext, packet->length);

	if (ciphertext_recv_status < 0)
	{
		return PACKET_RECV_ERR;
	} 
    else if (ciphertext_recv_status == 0)
    {
        return PACKET_RECV_DISCONNECTED;
    }

	return PACKET_RECV_SUCCESS;
}


/*
 * Writes one packet in the same three steps recv_packet() reads it:
 * length (converted to network byte order), nonce, ciphertext.
 */
int send_packet(int client_socket, const struct packet_in *packet)
{
	uint32_t net_len = htonl(packet->length);

    int len_send_status = send_all(client_socket, &net_len, sizeof(uint32_t));

	if (len_send_status < 0) // sending length
	{
		return PACKET_SEND_ERR;
	}
    else if (len_send_status == 0)
    {
        return PACKET_SEND_DISCONNECTED;
    }

    int nonce_send_status = send_all(client_socket, packet->nonce, crypto_aead_chacha20poly1305_ietf_NPUBBYTES);

	if (nonce_send_status < 0) // sending nonce
	{
		return PACKET_SEND_ERR;
	}
    else if (nonce_send_status == 0)
    {
        return PACKET_SEND_DISCONNECTED;
    }
    
    int ciphertext_send_status = send_all(client_socket, packet->ciphertext, packet->length);

	if (ciphertext_send_status < 0) // sending ciphertext
	{
		return PACKET_SEND_ERR;
	}
    else if (ciphertext_send_status == 0)
    {
        return PACKET_SEND_DISCONNECTED;
    }

    return PACKET_SEND_SUCCESS;
}

void print_recv_error_message(int error_code)
{
    switch (error_code)
    {
        case PACKET_RECV_DISCONNECTED:
            fprintf(stderr, "\nServer is unreachable\n");
            break;
        case PACKET_ENCRYPTION_FAILED:
            fprintf(stderr, "\nError receiving packet from server\nResync required\nClosing Connection\n");
            break;
        default:
            break;
    }
}