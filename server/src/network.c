#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sodium.h>
#include <errno.h>
#include "../include/network.h"
#include "../include/packet.h"
#include "../include/client.h"
#include "../include/crypto.h"

ssize_t recv_all(int socket_fd, uint8_t *buf, size_t n)
{
    size_t received = 0;
    size_t bytes_to_read = n;

    while (received < n)
    {
        ssize_t read_bytes = recv(socket_fd, buf + received, bytes_to_read, 0);

        if (read_bytes <= 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }
    
        received += read_bytes;
        bytes_to_read -= read_bytes;
    }

    return n;
}

ssize_t send_all(int socket_fd, uint8_t *buf, size_t n)
{
    size_t sent = 0;
    size_t bytes_to_send = n;

    while (sent < n)
    {
        ssize_t sent_bytes = send(socket_fd, buf + sent, bytes_to_send, MSG_NOSIGNAL);

        if (sent_bytes <= 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }
    
        sent += sent_bytes;
        bytes_to_send -= sent_bytes;
    }

    return n;
}

int recv_packet(int client_socket, struct packet_in *packet)
{
    // reading status
    int8_t net_status;

    if (recv_all(client_socket, (uint8_t *)&net_status, sizeof(int8_t)) <= 0)
    {
        return PACKET_RECV_ERR;
    }

	// reading length

	uint32_t net_len;

	if (recv_all(client_socket, &net_len, sizeof(uint32_t)) <= 0)
	{
		return PACKET_RECV_ERR;
	} 

	packet->length = ntohl(net_len);

	if (packet->length > MAX_CIPHERTEXT_SIZE)
	{
		return PACKET_RECV_ERR;
	}

	// reading nonce
	if (recv_all(client_socket,  packet->nonce, crypto_aead_chacha20poly1305_ietf_NPUBBYTES) <= 0)
	{
		return PACKET_RECV_ERR;
	}

	// reading ciphertext
	if (recv_all(client_socket, packet->ciphertext, packet->length) <= 0)
	{
		return PACKET_RECV_ERR;
	} 

	return PACKET_RECV_SUCCESS;
}


void send_packet(int client_socket, const struct packet_in *packet)
{
	uint32_t net_len = htonl(packet->length);
    int8_t net_status = htonl(packet->status);

    send_all(client_socket, (uint8_t *)&net_status, sizeof(int8_t)); // sending status
	send_all(client_socket, &net_len, sizeof(uint32_t)); // sending length
	send_all(client_socket, packet->nonce, crypto_aead_chacha20poly1305_ietf_NPUBBYTES); // sending nonce
	send_all(client_socket, packet->ciphertext, packet->length); // sending ciphertext
}

void send_broadcast(const struct client_in *clients[], size_t clients_count,uint8_t *message, size_t message_len, int sender_socket_fd)
{
    for (int j = 0; j < clients_count; j++)
    {
        if (clients[j] == NULL)
            continue;

        if (clients[j]->socket != sender_socket_fd)
        {
            struct packet_in out_packet;

            if (packet_encrypt(message, message_len, &out_packet,
                               clients[j]->tx) != PACKET_ENCRYPTION_SUCCESS)
            {
                fprintf(stderr, "Could not encrypt packet\n");
                continue;
            }

            send_packet(clients[j]->socket, &out_packet);
        }
    }
}
