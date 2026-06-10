#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sodium.h>
#include <errno.h>
#include <arpa/inet.h>
#include "../include/network.h"
#include "../include/packet.h"
#include "../include/crypto.h"

int recv_all(int socket_fd, void *buf, size_t n)
{
    size_t received = 0;
    size_t bytes_to_read = n;

    while (received < n)
    {
        int read_bytes = recv(socket_fd, buf + received, bytes_to_read, 0);

        if (read_bytes < 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (read_bytes == 0)
        {
            return 0;
        }
    
        received += read_bytes;
        bytes_to_read -= read_bytes;
    }

    return n;
}

int send_all(int socket_fd, const void *buf, size_t n)
{
    size_t sent = 0;
    size_t bytes_to_send = n;

    while (sent < n)
    {
        int sent_bytes = send(socket_fd, buf + sent, bytes_to_send, MSG_NOSIGNAL);

        if (sent_bytes < 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (sent_bytes == 0)
        {
            return 0;
        }
    
        sent += sent_bytes;
        bytes_to_send -= sent_bytes;
    }

    return n;
}

int recv_packet(int client_socket, struct packet_in *packet)
{
	// reading length

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

	if (ciphertext_recv_status <= 0)
	{
		return PACKET_RECV_ERR;
	} 
    else if (ciphertext_recv_status == 0)
    {
        return PACKET_RECV_DISCONNECTED;
    }

	return PACKET_RECV_SUCCESS;
}


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

void handle_recv_error_message(int error_message)
{
    switch (error_message)
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