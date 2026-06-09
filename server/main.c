#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sodium.h>
#include <string.h>
#include <stddef.h>

#include "include/client.h"
#include "include/network.h"
#include "include/crypto.h"
#include "include/packet.h"
#include "include/connection.h"
#include "include/server.h"

#define PORT        8080
#define MAX_CLIENTS 10 

#define MAX_MESSAGE_SIZE    1024


int main()
{	
	if (sodium_init() < 0)
	{
		perror("sodium_init()");

		return 1;
	}

	int server_fd;	// socket fd for server
	struct client_in* clients[MAX_CLIENTS] = {0};
	struct sockaddr_in address;
	uint8_t decrypted_buffer[MAX_MESSAGE_SIZE]; 
	fd_set readfds;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0)
	{
		perror("Socket Failed!");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	bind(server_fd, (struct sockaddr *) &address, sizeof(address));
	listen(server_fd, 5);
	printf("Server started on port %d\n", PORT);

	while (true)
	{
		// selecting for new connections and messages
		wait_for_io_events(&readfds, server_fd, clients, MAX_CLIENTS);

		// handling new connection
		if (FD_ISSET(server_fd, &readfds))
		{
			connection_status status = handle_new_connection(server_fd, clients, MAX_CLIENTS);

			if (status != CONNECTION_SUCCESS)
			{
				handle_connection_errors(status);
			}
		}	

		// reading messages
		handle_client_messages((const struct client_in **)clients, MAX_CLIENTS, &readfds, decrypted_buffer, MAX_MESSAGE_SIZE);
	}

	// cleanup
	disconnect_all_clients(clients, MAX_CLIENTS);
	close(server_fd);

	return 0;
}
