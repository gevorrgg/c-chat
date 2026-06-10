#ifndef SERVER_H
#define SERVER_H

#include "network.h"
#include <sys/select.h>

typedef enum
{
    CLIENT_MSG_SUCCESS = 0,
    CLIENT_MSG_TOO_LARGE = -1,
    CLIENT_MSG_DECRYPTION_FAILED = -2,
    CLIENT_MSG_RECV_ERR = -3,
    CLIENT_MSG_DISCONNECTED = -4
} client_message_status;

typedef enum
{
    SERVER_REQUEST_SUCCESS = 0,
    SERVER_KEY_EXCHANGE_ERR = -1,
    SERVER_CONNECTION_ERR = -2,
    SERVER_MESSAGE_ERR = -3,
    SERVER_SELECT_ERR = -4,
    SERVER_DISCONNECT = -5,
    SERVER_ENCRYPTION_FAILED = -6,
    SERVER_DECRYPTION_FAILED = -7,
    SERVER_PACKET_RECV_ERR = -8
} server_status;

void handle_message_errors(client_message_status status);
int handle_client_message(
    const struct client_in *clients[],
    size_t clients_count, 
    const struct client_in *sender, 
    uint8_t *message, 
    size_t max_message_size);

void wait_for_io_events(fd_set *readfds, int server_fd, struct client_in *clients[], size_t clients_count);
void handle_client_messages(struct client_in *clients[], size_t clients_count, fd_set *readfds, uint8_t *message_buffer, size_t max_message_size);


#endif