#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>

typedef enum
{
    CONNECTION_SUCCESS = 0,
    CONNECTION_ACCEPT_ERR = -1,
    CONNECTION_KEY_EXCHANGE_ERR = -2,
    CONNECTION_INSERTION_ERR = -3,
    CONNECTION_CLIENT_DISCONNECTED = -4
    
} connection_status;

struct client_in;

void handle_connection_errors(connection_status status);
int handle_new_connection(int server_fd, struct client_in *clients[], size_t clients_count);

void disconnect_client(struct client_in *client);
void disconnect_all_clients(struct client_in *clients[], size_t clients_count);

#endif