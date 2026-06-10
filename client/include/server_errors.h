#ifndef SERVER_ERRORS_H
#define SERVER_ERRORS_H

#include <stdint.h>

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

void handle_server_message_errors(server_status status);

#endif