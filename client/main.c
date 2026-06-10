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

#define PORT 8080
int sock;
uint8_t rx[crypto_kx_SESSIONKEYBYTES]; // receive key
uint8_t tx[crypto_kx_SESSIONKEYBYTES]; // transmit key
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

typedef enum
{
	CLIENT_REQUEST_SUCCESS = 0,
	CLIENT_DISCONNECTED = -1
} client_status;


void *receive_msg(void *arg)
{
	char decrypted_msg[1024];

	while(1)
	{
		int32_t status;

		recv_all(sock, &status, sizeof(int32_t)); // reading status

		if (status < 0)
		{
			handle_server_message_errors(status);
			continue;
		}

		struct packet_in packet;

		int recv_packet_status = recv_packet(sock, &packet);

		if (recv_packet_status != PACKET_RECV_SUCCESS)
		{
			handle_recv_error_message(recv_packet_status);
			close(sock);
			_exit(0);
		}

		unsigned long long msg_len;

		if (packet_decrypt(&packet, decrypted_msg, &msg_len, rx) == PACKET_DECRYPTION_FAILED)
		{
			fprintf(stderr, "Could not decrypt message\n");
			continue;
		}

		decrypted_msg[msg_len] = '\0';

		if (decrypted_msg[msg_len - 1] == '\n')
		{
			decrypted_msg[msg_len - 1] = '\0';
		}
	
		pthread_mutex_lock(&mux);

		printf("\nReceived: %s\n> ", decrypted_msg);
		fflush(stdout);

		pthread_mutex_unlock(&mux);
    }

    return NULL;
}

int main()
{
	if (sodium_init() < 0)
	{
		perror("sodium_init()");

		return 1;
	}

	struct sockaddr_in serv_addr;
	char buffer[1024];
	pthread_t recv_thread;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
	
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("connect");

		exit(0);
	}

	printf("Exchanging keys with server...\n");

	if (dh_key_exchange_client(sock, rx, tx) != DH_EXCHANGE_SUCCESS)
	{
		fprintf(stderr, "Error during key exchange with server\n");
		close(sock);
		exit(1);
	}

	printf("Keys are established\n");

	printf("Connected to server with port %d\n", PORT);	

	pthread_create(&recv_thread, NULL, receive_msg, NULL);

	
	while(1)
	{
		pthread_mutex_lock(&mux);

		printf("> ");
		fflush(stdout);

		pthread_mutex_unlock(&mux);
		fgets(buffer, sizeof(buffer), stdin);

		size_t len = strlen(buffer);

		struct packet_in out_packet;

		if (packet_encrypt((uint8_t *)buffer, len, &out_packet, tx) != PACKET_ENCRYPTION_SUCCESS)
		{
			fprintf(stderr, "Could not encrypt packet\n");
			continue;
		}

		int32_t status = htonl(CLIENT_REQUEST_SUCCESS);

		if (send_all(sock, &status, sizeof(int32_t)) <= 0) break;
		if (send_packet(sock, &out_packet) != PACKET_SEND_SUCCESS) break;
	}	
	
		
	printf("Server currently is not working\n");	
	close(sock);

	return 0;
}
