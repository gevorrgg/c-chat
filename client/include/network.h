#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

typedef enum
{
	PACKET_RECV_SUCCESS = 0,
	PACKET_RECV_ERR = -1,
	PACKET_RECV_DISCONNECTED = -2
} packet_recv_status; 

typedef enum
{
	PACKET_SEND_SUCCESS = 0,
	PACKET_SEND_ERR = -1,
	PACKET_SEND_DISCONNECTED = -2
} packet_send_status;

struct packet_in;
struct client_in;

int recv_all(int socket_fd, void *buf, size_t n);
int send_all(int socket_fd, const void *buf, size_t n);
int recv_packet(int client_socket, struct packet_in *packet);
int send_packet(int client_socket, const struct packet_in *packet);
void handle_recv_error_message(int error_message);
	
#endif