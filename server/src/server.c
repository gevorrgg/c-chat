#include "../include/server.h"
#include "../include/connection.h"
#include "../include/network.h"
#include "../include/crypto.h"
#include "../include/packet.h"
#include "../include/client.h"
#include <stdio.h>
#include <arpa/inet.h>

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

		select(max_fd + 1, readfds, NULL, NULL, NULL);
}

void handle_client_messages( struct client_in *clients[], size_t clients_count, fd_set *readfds,  uint8_t *message_buffer, size_t max_message_size)
{
    for	(int i = 0; i < clients_count; i++)
    {
        if (clients[i] == NULL)
            continue;

        int fd = clients[i]->socket;	// current client

        if (FD_ISSET(fd, readfds)) // ready for read
        {
            int32_t client_status;

            recv_all(fd, &client_status, sizeof(int32_t)); // reading status

            client_status = ntohl(client_status);

            if (client_status == CLIENT_DISCONNECTED)
            {
                disconnect_client(clients[i]);
                printf("Client with socket fd %d disconnected\n", fd);
                clients[i] = NULL;

                continue;
            }
        
           
             // handling message
            int message_status = handle_client_message(
                (const struct client_in **)clients, 
                clients_count, 
                clients[i], 
                message_buffer, 
                max_message_size);

            if (message_status == CLIENT_MSG_DISCONNECTED)
            { 
                disconnect_client(clients[i]);
                printf("Client with socket fd %d disconnected\n", fd);
                clients[i] = NULL;

                continue;
            }
            
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
    const struct client_in *client, 
    uint8_t *message, 
    size_t max_message_size
)
{
    struct packet_in packet;

    int packet_recv_status = recv_packet(client->socket, &packet);

    if (packet_recv_status == PACKET_RECV_ERR) // client is disconected
    {
        int32_t status = htonl(SERVER_PACKET_RECV_ERR);
        send_all(client->socket, &status, sizeof(int32_t)); // sending status

        return CLIENT_MSG_RECV_ERR;
    }
    else if (packet_recv_status == PACKET_RECV_DISCONNECTED)
    {
        return CLIENT_MSG_DISCONNECTED;
    }

    // decrypting
    unsigned long long msg_len;

    if (packet_decrypt(&packet, message, &msg_len, client->rx) ==
        CLIENT_MSG_DECRYPTION_FAILED)
    {
        int32_t status = htonl(SERVER_DECRYPTION_FAILED);
        send_all(client->socket, &status, sizeof(int32_t)); // sending status

        return CLIENT_MSG_DECRYPTION_FAILED;
    }

    if (msg_len > max_message_size)
    {
        int32_t status = htonl(SERVER_MESSAGE_ERR);
        send_all(client->socket, &status, sizeof(int32_t)); // sending status

        return CLIENT_MSG_TOO_LARGE;
    }

    // send the message to all clients
    send_broadcast((const struct client_in **)clients, clients_count, message,
                   msg_len, client->socket);

    return CLIENT_MSG_SUCCESS;
}