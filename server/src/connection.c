#include "../include/connection.h"
#include "../include/crypto.h"
#include "../include/network.h"
#include "../include/client.h"
#include "../include/server.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

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

/* Store the client in the first free (NULL) slot of the table. */
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


/*
 * Full life cycle of accepting one client:
 *   accept() -> key exchange -> allocate client_in -> insert into table.
 * Every failure after accept() tells the client why (SERVER_* status word,
 * then SERVER_DISCONNECT) before the socket is closed, so the client can
 * print a meaningful error instead of a bare "connection reset".
 *
 * Note that the key exchange blocks the whole server until this client
 * answers; a slow or stalled client delays everyone else.
 */
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
        // tell the client what went wrong, then announce the disconnect
        int32_t client_request_status = htonl(SERVER_KEY_EXCHANGE_ERR);

        send_all(new_client_fd, &client_request_status, sizeof(int32_t));

        client_request_status = htonl(SERVER_DISCONNECT);

        send_all(new_client_fd, &client_request_status, sizeof(int32_t));

        close(new_client_fd);

        return CONNECTION_KEY_EXCHANGE_ERR;
    }

    struct client_in *new_client = new_client_in(new_client_fd, rx, tx);

    printf("Keys are established\n");

    int insertion_status = insert_client(clients, clients_count, new_client);

    if (insertion_status != CONNECTION_SUCCESS)
    {
        // table is full (MAX_CLIENTS reached): reject politely
        int32_t client_request_status = htonl(SERVER_CONNECTION_ERR);
        send_all(new_client_fd, &client_request_status, sizeof(int32_t));

        client_request_status = htonl(SERVER_DISCONNECT);
        send_all(new_client_fd, &client_request_status, sizeof(int32_t));

        close(new_client_fd);
        free(new_client);

        return CONNECTION_INSERTION_ERR;
    }

    return CONNECTION_SUCCESS;
}


/* Release one client. The table slot itself is cleared by the caller. */
void disconnect_client(struct client_in *client)
{
    if (client)
    {
        close(client->socket);
        free(client);
    }
}

/* Shutdown path: warn every client with SERVER_DISCONNECT, then free them. */
void disconnect_all_clients(struct client_in *clients[], size_t clients_count)
{
    for (int i = 0; i < clients_count; i++)
    {
        if (clients[i] != NULL)
        {
            int32_t status = htonl(SERVER_DISCONNECT);
            send_all(clients[i]->socket, &status, sizeof(int32_t)); // sending disconnect status
            disconnect_client(clients[i]);
            clients[i] = NULL;
        }
    }
}


