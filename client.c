#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
int sock;

void *receive_msg(void *arg)
{
	char buffer[1024];
	while(1)
	{
		int val = read(sock, buffer, 1024);
		if(val > 0)
		{
			buffer[val] = '\0';
			printf("Message: %s\n", buffer);
			fflush(stdout);
		}
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
