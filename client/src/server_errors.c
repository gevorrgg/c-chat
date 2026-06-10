#include "../include/server_errors.h"
#include <stdio.h>
#include <stdlib.h>

void handle_server_message_errors(server_status status)
{
    switch (status)
    {
    case SERVER_KEY_EXCHANGE_ERR:
        fprintf(stderr, "Error during key exchange with server\n");
        break;
    case SERVER_CONNECTION_ERR:
        fprintf(stderr, "Error connecting to server\n");
        break;
    case SERVER_MESSAGE_ERR:
        fprintf(stderr, "Error sending/receiving message\n");
        break;
    case SERVER_SELECT_ERR:
        fprintf(stderr, "Error during select()\n");
        break;
    case SERVER_DISCONNECT:
        fprintf(stderr, "Disconnected from server\n");
    
        break;
    case SERVER_ENCRYPTION_FAILED:
        fprintf(stderr, "Error during message encryption\n");
        break;
    case SERVER_DECRYPTION_FAILED:
        fprintf(stderr, "Error during message decryption\n");
        break;
    case SERVER_PACKET_RECV_ERR:
        fprintf(stderr, "Error receiving packet from server\n");
        break;
    default:
        break;
    }
}
