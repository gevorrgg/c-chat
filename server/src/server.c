#include "../include/server.h"
#include "../include/connection.h"
#include "../include/network.h"
#include "../include/crypto.h"
#include "../include/packet.h"
#include "../include/client.h"
#include <stdio.h>

void handle_message_errors(client_message_status status)
{
    switch (status)
    {
    case CLIENT_MSG_DECRYPTION_FAILED:
        fprintf(stderr, "Failed to decrypt client's message\n");
        break;
    case CLIENT_MSG_TOO_LARGE:
        fprintf(stderr, "Client's message is too large\n");
        break;
    case CLIENT_MSG_RECV_ERR:
        fprintf(stderr, "Error receiving client's message\n");
        break;
    case CLIENT_MSG_DISCONNECTED:
        fprintf(stderr, "Client disconnected\n");
        break;
    default:
        break;
    }
}

void wait_for_io_events(fd_set *readfds, int server_fd, struct client_in *clients[], size_t clients_count)
{
   		FD_ZERO(readfds);
		FD_SET(server_fd, readfds);
		int max_fd = server_fd;

		for (int i = 0; i < clients_count; i++)
		{	
			if (!clients[i]) continue;

			int fd = clients[i]->socket;

			if (fd >= 0 )
			{
				FD_SET(fd, readfds);
			}
	
			if (fd > max_fd) max_fd = fd;
		}

		select(max_fd + 1, &readfds, NULL, NULL, NULL);
}

void handle_client_messages(const struct client_in *clients[], size_t clients_count, fd_set *readfds,  uint8_t *message_buffer, size_t max_message_size)
{
    for	(int i = 0; i < clients_count; i++)
    {
        if (clients[i] == NULL)
            continue;

        int fd = clients[i]->socket;	// current client

        if (FD_ISSET(fd, readfds)) // ready for read
        {
            int message_status = handle_client_message(
                (const struct client_in **)clients, 
                clients_count, 
                clients[i], 
                clients[i], 
                message_buffer, 
                max_message_size);

            if (message_status != CLIENT_MSG_SUCCESS)
            {
                handle_message_errors(message_status);
            }
        }
    }
}

int handle_client_message(
    const struct client_in *clients[], 
    size_t clients_count, 
    const struct client_in *sender,
    const struct client_in *client, 
    uint8_t *message, 
    size_t max_message_size
)
{
    struct packet_in packet;

    if (recv_packet(client->socket, &packet) ==
        PACKET_RECV_ERR) // client is disconected
    {
        return CLIENT_MSG_RECV_ERR;
    }

    // decrypting
    unsigned long long msg_len;

    if (packet_decrypt(&packet, message, &msg_len, client->rx) ==
        CLIENT_MSG_DECRYPTION_FAILED)
    {
        return CLIENT_MSG_DECRYPTION_FAILED;
    }

    if (msg_len > max_message_size)
    {
        const char *client_err_msg = "Message is too large\n";
        send_all(client->socket, client_err_msg, strlen(client_err_msg));

        return CLIENT_MSG_TOO_LARGE;
    }

    // send the message to all clients
    send_broadcast((const struct client_in **)clients, clients_count, message,
                   msg_len, client->socket);

    return CLIENT_MSG_SUCCESS;
}