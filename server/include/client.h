#ifndef CLIENT_H
#define CLIENT_H

#include <sodium.h>

struct client_in
{
	int socket;
	uint8_t rx[crypto_kx_SESSIONKEYBYTES];
	uint8_t tx[crypto_kx_SESSIONKEYBYTES];
};

struct client_in* new_client_in(int socket, uint8_t *rx, uint8_t *tx);

#endif