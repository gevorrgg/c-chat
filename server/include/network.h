#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

typedef enum
{
	PACKET_RECV_SUCCESS = 0,
	PACKET_RECV_ERR = -1,
} packet_recv_status; 

struct packet_in;
struct client_in;

int recv_all(int socket_fd, uint8_t *buf, size_t n);
int send_all(int socket_fd, uint8_t *buf, size_t n);
int recv_packet(int client_socket, struct packet_in *packet);
void send_packet(int client_socket, const struct packet_in *packet);
void send_broadcast(const struct client_in *clients[], size_t clients_count, uint8_t *message, size_t message_len, int sender_socket_fd);

#endif