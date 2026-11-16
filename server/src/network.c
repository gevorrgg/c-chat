#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sodium.h>
#include <errno.h>
#include <arpa/inet.h>
#include "../include/network.h"
#include "../include/packet.h"
#include "../include/client.h"
#include "../include/crypto.h"
#include "../include/server.h"

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


/*
 * Each client has its own session keys, so the message cannot simply be
 * relayed: it is re-encrypted once per recipient with that recipient's tx key.
 * A fresh nonce is generated for every encryption inside packet_encrypt().
 */
void send_broadcast(const struct client_in *clients[], size_t clients_count, const uint8_t *message, size_t message_len, int sender_socket_fd)
{
    for (int j = 0; j < clients_count; j++)
    {
        if (clients[j] == NULL) /* free slot */
            continue;

        if (clients[j]->socket != sender_socket_fd) /* do not echo to the author */
        {
            struct packet_in out_packet;

            if (packet_encrypt(message, message_len, &out_packet,
                               clients[j]->tx) != PACKET_ENCRYPTION_SUCCESS)
            {
                fprintf(stderr, "Could not encrypt packet\n");
                int32_t status = htonl(SERVER_ENCRYPTION_FAILED);

                send_all(clients[j]->socket, &status, sizeof(int32_t)); // sending status

                continue;
            }
            
            // protocol: status word first, then the packet
            int32_t status = htonl(SERVER_REQUEST_SUCCESS);

            send_all(clients[j]->socket, &status, sizeof(int32_t)); // sending status
            send_packet(clients[j]->socket, &out_packet);
        }
    }
}
