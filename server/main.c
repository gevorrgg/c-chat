#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

/*
 * Chat server.
 *
 * Single-threaded select() loop. Listens on PORT, holds up to MAX_CLIENTS
 * simultaneous clients and relays every message from one client to all the
 * others, re-encrypting it for each recipient. There is no persistence,
 * no authentication and no rooms: everyone connected sees everything.
 */

#define DEFAULT_PORT    8080   // TCP port to listen on
#define MAX_CLIENTS     10     // size of the client table

#define MAX_MESSAGE_SIZE    1024   // largest plaintext accepted from a client


/*
 * Parse "<port>" into a valid TCP port number.
 * Returns the port, or -1 if the string is not a number in 1..65535.
 */
static int parse_port(const char *text)
{
	char *end;
	long value = strtol(text, &end, 10);

	if (*text == '\0' || *end != '\0' || value < 1 || value > 65535)
	{
		return -1;
	}

	return (int)value;
}

static void print_usage(FILE *out, const char *prog)
{
	fprintf(out, "Usage: %s [port] [bind_ip]\n", prog);
	fprintf(out, "       %s --help\n", prog);
	fprintf(out, "\n");
	fprintf(out, "Run a c-chat server that relays encrypted messages between clients.\n");
	fprintf(out, "\n");
	fprintf(out, "Arguments:\n");
	fprintf(out, "  port        TCP port to listen on, 1-65535 (default %d)\n", DEFAULT_PORT);
	fprintf(out, "  bind_ip     IPv4 address to listen on (default 0.0.0.0, all interfaces)\n");
	fprintf(out, "\n");
	fprintf(out, "Options:\n");
	fprintf(out, "  -h, --help  show this help and exit\n");
	fprintf(out, "\n");
	fprintf(out, "Up to %d clients are served at the same time.\n", MAX_CLIENTS);
}

/* Print an error, then the usage text, and return the exit code for main(). */
static int usage_error(const char *prog, const char *fmt, const char *arg)
{
	fprintf(stderr, "Error: ");
	fprintf(stderr, fmt, arg);
	fprintf(stderr, "\n\n");
	print_usage(stderr, prog);
	return 1;
}

int main(int argc, char *argv[])
{	
	// command line: [port] [bind_ip]; both optional
	int port = DEFAULT_PORT;
	const char *bind_ip = NULL; // NULL = INADDR_ANY
	struct in_addr bind_addr;

	// --help / -h anywhere on the command line wins over everything else
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			print_usage(stdout, argv[0]);
			return 0;
		}
	}

	if (argc > 3)
	{
		return usage_error(argv[0], "too many arguments (expected at most 2)%s", "");
	}

	// any other option-looking argument is unknown
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			return usage_error(argv[0], "unknown option '%s'", argv[i]);
		}
	}

	if (argc >= 2)
	{
		port = parse_port(argv[1]);

		if (port < 0)
		{
			return usage_error(argv[0], "invalid port '%s' (expected a number 1-65535)", argv[1]);
		}
	}

	if (argc == 3)
	{
		bind_ip = argv[2];

		if (inet_pton(AF_INET, bind_ip, &bind_addr) != 1)
		{
			return usage_error(argv[0], "invalid IPv4 address '%s'", bind_ip);
		}
	}

	// must be called before any other libsodium function
	if (sodium_init() < 0)
	{
		perror("sodium_init()");

		return 1;
	}

	// line-buffer stdout so log lines show up immediately even when redirected
	setvbuf(stdout, NULL, _IOLBF, 0);

	int server_fd;	// socket fd for server
	struct client_in* clients[MAX_CLIENTS] = {0};	// NULL = free slot
	struct sockaddr_in address;
	uint8_t decrypted_buffer[MAX_MESSAGE_SIZE]; 
	fd_set readfds;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0)
	{
		perror("Socket Failed!");
		exit(EXIT_FAILURE);
	}

	// allow restarting the server while old connections sit in TIME_WAIT
	int reuse = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	// listen on every local interface unless a bind address was given
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind_ip)
	{
		address.sin_addr = bind_addr; // validated above
	}

	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		perror("bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 5) < 0)
	{
		perror("listen");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	printf("Server started on %s:%d\n", bind_ip ? bind_ip : "0.0.0.0", port);

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
		handle_client_messages((struct client_in **)clients, MAX_CLIENTS, &readfds, decrypted_buffer, MAX_MESSAGE_SIZE);
	}

	// cleanup (unreachable while the loop above is infinite; kept for when a
	// shutdown signal handler is added)
	disconnect_all_clients(clients, MAX_CLIENTS);
	close(server_fd);

	return 0;
}
