#include "../include/connection.h"
#include "../include/crypto.h"
#include "../include/network.h"
#include "../include/client.h"
#include "../include/server.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <stdbool.h>

void handle_connection_errors(connection_status status)
{
    switch (status)
    {
    case CONNECTION_ACCEPT_ERR:
        fprintf(stderr, "Error accepting new connection\n");
        break;
    case CONNECTION_KEY_EXCHANGE_ERR:
        fprintf(stderr, "Error during key exchange with client\n");
        break;
    case CONNECTION_INSERTION_ERR:
        fprintf(stderr, "Error inserting new client into clients array\n");
        break;
    case CONNECTION_CLIENT_DISCONNECTED:
        fprintf(stderr, "Client disconnected\n");
        break;
    default:
        break;
    }
}

int handle_new_connection(int server_fd, struct client_in *clients[], size_t clients_count)
{
    struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

    int new_client_fd =
        accept(server_fd, (struct sockaddr *)&address, &addrlen);

    if (new_client_fd < 0)
    {
        return CONNECTION_ACCEPT_ERR;
    }

    printf("New connection: socket fd: %d, IP: %s\n", new_client_fd,
           inet_ntoa(address.sin_addr));

    // diffie-helman key exchange
    uint8_t rx[crypto_kx_SESSIONKEYBYTES]; // receive key
    uint8_t tx[crypto_kx_SESSIONKEYBYTES]; // transmit key

    if (dh_key_exchange_server(new_client_fd, rx, tx) != DH_EXCHANGE_SUCCESS)
    {
        int client_request_status = htonl(SERVER_KEY_EXCHANGE_ERR);

        send_all(new_client_fd, &client_request_status, sizeof(client_request_status));

        client_request_status = htonl(SERVER_DISCONNECT);

        send_all(new_client_fd, &client_request_status, sizeof(client_request_status));

        close(new_client_fd);

        return CONNECTION_KEY_EXCHANGE_ERR;
    }

    struct client_in *new_client = new_client_in(new_client_fd, rx, tx);

    printf("Keys are established\n");
    send_all(new_client_fd, "Keys are established\n", 22);

    int insertion_status = insert_client(clients, clients_count, new_client);

    if (insertion_status != CONNECTION_SUCCESS)
    {
        int client_request_status = htonl(SERVER_CONNECTION_ERR);
        send_all(new_client_fd, &client_request_status, sizeof(client_request_status));

        client_request_status = htonl(SERVER_DISCONNECT);
        send_all(new_client_fd, &client_request_status, sizeof(client_request_status));

        close(new_client_fd);
        free(new_client);

        return CONNECTION_INSERTION_ERR;
    }

    return CONNECTION_SUCCESS;
}

static int insert_client(struct client_in *clients[], size_t clients_count, struct client_in *new_client)
{
    for (int i = 0; i < clients_count; i++)
    {
        if (clients[i] == NULL)
        {
            clients[i] = new_client;
            return CONNECTION_SUCCESS;
        }
    }

    return CONNECTION_INSERTION_ERR;
}


void disconnect_client(struct client_in *client)
{
    if (client)
    {
        close(client->socket);
        free(client);
    }
}

void disconnect_all_clients(struct client_in *clients[], size_t clients_count)
{
    for (int i = 0; i < clients_count; i++)
    {
        if (clients[i] != NULL)
        {
            disconnect_client(clients[i]);
            clients[i] = NULL;
        }
    }
}


