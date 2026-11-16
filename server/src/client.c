#include "../include/client.h"
#include <string.h>

/*
 * The keys are copied, not referenced: the caller's rx/tx buffers live on
 * the stack of handle_new_connection() and go away when it returns.
 */
struct client_in* new_client_in(int socket, uint8_t *rx, uint8_t *tx)
{
	struct client_in *client = malloc(sizeof(struct client_in));

	if (!client)
	{
		return NULL;
	}

	client->socket = socket;
	memcpy(client->rx, rx, crypto_kx_SESSIONKEYBYTES);
	memcpy(client->tx, tx, crypto_kx_SESSIONKEYBYTES);

	return client;
}