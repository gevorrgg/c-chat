#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sodium.h>
#include <signal.h>
#include <stdatomic.h>

#include "include/packet.h"
#include "include/network.h"
#include "include/crypto.h"
#include "include/server_errors.h"

/*
 * Chat client.
 *
 * Two threads share one TCP socket:
 *   - the main thread reads lines from stdin, encrypts them and sends them;
 *   - receive_msg() blocks on the socket, decrypts incoming packets and
 *     prints them.
 * The socket and session keys are process-wide globals because both threads
 * need them; `mux` only serialises writes to stdout so the prompt and
 * incoming messages do not interleave.
 */

#define DEFAULT_SERVER_IP   "127.0.0.1"
#define DEFAULT_PORT        8080

int sock;                                  // TCP socket to the server
uint8_t rx[crypto_kx_SESSIONKEYBYTES]; // receive key
uint8_t tx[crypto_kx_SESSIONKEYBYTES]; // transmit key
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER; // guards stdout
atomic_int quitting = 0;                   // set by main when the user ends input

/* Status word sent before every packet; must match server/include/client.h. */
typedef enum
{
	CLIENT_REQUEST_SUCCESS = 0,
	CLIENT_DISCONNECTED = -1
} client_status;


/*
 * Receiver thread. Loop: read the server's status word; if it signals an
 * error, report it and wait for the next word; otherwise read the packet
 * that follows, decrypt it and print it above the prompt.
 */
void *receive_msg(void *arg)
{
	char decrypted_msg[MAX_PLAINTEXT_LEN + 1]; // +1 for the terminating NUL

	while(1)
	{
		int32_t status;

		// reading status; 0 means the server closed the socket, <0 is an error
		if (recv_all(sock, &status, sizeof(int32_t)) <= 0)
		{
			// main shut the socket down on purpose: exit quietly
			if (atomic_load(&quitting))
			{
				return NULL;
			}

			print_recv_error_message(PACKET_RECV_DISCONNECTED);
			close(sock);
			_exit(0);
		}

		status = ntohl(status); // the server sends it in network byte order

		// negative status = error notification, no packet follows
		if (status < 0)
		{
			handle_server_message_errors(status);
			continue;
		}

		struct packet_in packet;

		int recv_packet_status = recv_packet(sock, &packet);

		if (recv_packet_status != PACKET_RECV_SUCCESS)
		{
			print_recv_error_message(recv_packet_status);
			close(sock);
			// _exit() skips atexit handlers and stdio flushing; exit() would
			// try to take stdio locks the main thread may hold inside fgets()
			_exit(0); // deadlock if exit(0) used instead
		}

		unsigned long long msg_len;

		if (packet_decrypt(&packet, (uint8_t *)decrypted_msg, &msg_len, rx) == PACKET_DECRYPTION_FAILED)
		{
			fprintf(stderr, "Could not decrypt message\n");
			continue;
		}

		// the sender's fgets() keeps the trailing newline; strip it for display
		decrypted_msg[msg_len] = '\0';

		if (msg_len > 0 && decrypted_msg[msg_len - 1] == '\n')
		{
			decrypted_msg[msg_len - 1] = '\0';
		}
	
		pthread_mutex_lock(&mux);

		// re-print the prompt so the user's input line is not left dangling
		printf("\nReceived: %s\n> ", decrypted_msg);
		fflush(stdout);

		pthread_mutex_unlock(&mux);
    }

    return NULL;
}

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

static void print_usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [server_ip] [port]\n", prog);
	fprintf(stderr, "  server_ip  IPv4 address of the server (default %s)\n", DEFAULT_SERVER_IP);
	fprintf(stderr, "  port       TCP port of the server (default %d)\n", DEFAULT_PORT);
}

int main(int argc, char *argv[])
{
	// command line: [server_ip] [port]; both optional, defaults above
	const char *server_ip = DEFAULT_SERVER_IP;
	int port = DEFAULT_PORT;

	if (argc > 3)
	{
		print_usage(argv[0]);
		return 1;
	}

	if (argc >= 2)
	{
		server_ip = argv[1];
	}

	if (argc == 3)
	{
		port = parse_port(argv[2]);

		if (port < 0)
		{
			fprintf(stderr, "Invalid port: %s\n", argv[2]);
			print_usage(argv[0]);
			return 1;
		}
	}

	// must be called before any other libsodium function
	if (sodium_init() < 0)
	{
		perror("sodium_init()");

		return 1;
	}

	struct sockaddr_in serv_addr;
	char buffer[1024];
	pthread_t recv_thread;
	
	// connect to the server
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0)
	{
		perror("socket");
		return 1;
	}
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) != 1)
	{
		fprintf(stderr, "Invalid IPv4 address: %s\n", server_ip);
		print_usage(argv[0]);
		close(sock);
		return 1;
	}
	
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("connect");
		close(sock);
		return 1;
	}

	printf("Exchanging keys with server...\n");

	if (dh_key_exchange_client(sock, rx, tx) != DH_EXCHANGE_SUCCESS)
	{
		fprintf(stderr, "Error during key exchange with server\n");
		close(sock);
		exit(1);
	}

	printf("Keys are established\n");

	printf("Connected to server %s:%d\n", server_ip, port);	

	// start the receiver; from here on the socket is shared by two threads
	pthread_create(&recv_thread, NULL, receive_msg, NULL);

	
	// input loop: one line typed = one encrypted packet sent
	int server_gone = 0;

	while(1)
	{
		pthread_mutex_lock(&mux);

		printf("> ");
		fflush(stdout);

		pthread_mutex_unlock(&mux);

		// NULL = EOF (Ctrl-D) or read error: stop sending and leave
		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
		{
			break;
		}

		size_t len = strlen(buffer);

		struct packet_in out_packet;

		if (packet_encrypt((uint8_t *)buffer, len, &out_packet, tx) != PACKET_ENCRYPTION_SUCCESS)
		{
			fprintf(stderr, "Could not encrypt packet\n");
			continue;
		}

		// protocol: status word first, then the packet; a failed send means
		// the server is gone, so leave the loop and exit
		int32_t status = htonl(CLIENT_REQUEST_SUCCESS);

		if (send_all(sock, &status, sizeof(int32_t)) <= 0) { server_gone = 1; break; }
		if (send_packet(sock, &out_packet) != PACKET_SEND_SUCCESS) { server_gone = 1; break; }
	}	
	
		
	if (server_gone)
	{
		printf("Server currently is not working\n");	
	}

	// stop the receiver: shutdown() wakes it up out of recv(), the flag tells
	// it not to treat that as a server failure
	atomic_store(&quitting, 1);
	shutdown(sock, SHUT_RDWR);
	pthread_join(recv_thread, NULL);

	close(sock);

	return 0;
}
