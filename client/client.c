#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sodium.h>

#define PORT 8080
int sock;
int decryption_key;

typedef enum
{
    SERVER_REQUEST_SUCCESS = 0,
    SERVER_KEY_EXCHANGE_ERR = -1,
    SERVER_CONNECTION_ERR = -2,
    SERVER_MESSAGE_ERR = -3,
    SERVER_SELECT_ERR = -4,
    SERVER_DISCONNECT = -5
} server_status;


void *receive_msg(void *arg)
{
	char buffer[1024];
	while(1)
	{
		int8_t status;

		read_all(sock, &status, sizeof(int8_t));

		if (status < 0)
		{
			handle_server_message_errors(status);
			continue;
		}

		read_
    }

	return NULL;
}


int main()
{
	struct sockaddr_in serv_addr;
	char buffer[1024];
	pthread_t recv_thread;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

	connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));


	printf("Exchanging keys with server...\n");
	handshake();
	printf("Keys are established\n");

	printf("Connected to server with port %d\n", PORT);	

	pthread_create(&recv_thread, NULL, receive_msg, NULL);

	
	
	while(1)
	{
		printf("> ");
		fgets(buffer, sizeof(buffer), stdin);
		send(sock, buffer, strlen(buffer), 0);
	}	
		
	printf("End\n");	

	return 0;
}
