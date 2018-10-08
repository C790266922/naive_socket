#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include "message.h"


struct Params{
	int sockfd;
	pthread_t thread_id;
};

void to_time(time_t timestamp, char *dst)
{
	struct tm p;
	p = *localtime(&timestamp);
	strftime(dst, 1000, "%Y-%m-%d %H:%M:%S", &p);
}

void *recv_msg(void *p_fd)
{
	int fd = *(int *)p_fd;
	char buff[sizeof(struct Message)];

	while(1){
		int n = recv(fd, buff, sizeof(struct Message), 0);
		if(n <= 0){
			// printf("received 0 bytes in recv_msg\n");
			return 0;
		} else {
			// printf("received %d bytes\n", n);
		}

		struct Message server_msg = *(struct Message *)buff;
	
		/*
		if (server_msg.flag == 1) {
			printf("operation succeed\n");
		}
		*/

		if (server_msg.operation == GET_TIME) {
			char *time_now = malloc(100);
			to_time(server_msg.t, time_now);
			printf("\e[01;33;41m current_time: %s \e[0m\n", time_now);
		} else if (server_msg.operation == GET_SERVER_NAME) {
			printf("\e[01;33;41m server_name: %s \e[0m\n", server_msg.server_name);
		} else if (server_msg.operation == GET_CONNECTION_LIST) {
			printf("\e[01;33;41m connection_number: %d \e[0m\n", server_msg.connection_num);
			int i = 0;
			printf("\e[01;33;41m index    ip        port \e[0m\n");
			for( ; i < server_msg.connection_num; ++i){
				printf("\e[01;33;41m %d    %s        %d \e[0m\n", server_msg.clients[i].index,\
						server_msg.clients[i].ip, server_msg.clients[i].port);
			}
		} else if(server_msg.operation == SEND_MESSAGE){
			if(server_msg.flag == 1){
				printf("\e[01;33;41m message sent \e[0m\n");
			} else {
				printf("\e[01;33;41m error: %s \e[0m\n", server_msg.error_msg);
			}
		} else if(server_msg.operation == RECV_MESSAGE){
			printf("\e[01;33;41m received message: %s\e[0m\n", server_msg.msg);
		}
	}
	return 0;
}

void *send_msg(void *pp)
{
	struct Params p = *(struct Params *)pp;
	int sockfd = p.sockfd;
	pthread_t receiver_id = p.thread_id;

	while (1) {
		int operation;

		printf("\e[01;33;44m Please choose operation(1~7): \e[0m\n");
		printf("\e[01;33;44m ---------------------------------------- \e[0m\n");
		printf("\e[01;33;44m      1: Get current time                 \e[0m\n");
		printf("\e[01;33;44m      2: Get server name                  \e[0m\n");
		printf("\e[01;33;44m      3: Get connection list              \e[0m\n");
		printf("\e[01;33;44m      4: Send message                     \e[0m\n");
		printf("\e[01;33;44m      5: Disconnect                       \e[0m\n");
		printf("\e[01;33;44m      6: Exit                             \e[0m\n");
		printf("\e[01;33;44m ---------------------------------------- \e[0m\n");


		// printf("Choose operation: ");
		scanf("%d", &operation);
		if(operation < 1 || operation > 7){
			printf("Wrong operation\n");
			continue;
		}

		struct Message client_msg;

		switch (operation) {
			case GET_TIME:
				client_msg.operation = GET_TIME;
				break;
			case GET_SERVER_NAME:
				client_msg.operation = GET_SERVER_NAME;
				break;
			case GET_CONNECTION_LIST:
				client_msg.operation = GET_CONNECTION_LIST;
				break;
			case SEND_MESSAGE:
				printf("please give a client No: ");
				scanf("%d", &client_msg.client_no);
				printf("please give the message to send: ");
				scanf("%s", client_msg.msg);
				client_msg.operation = SEND_MESSAGE;
				break;
			case CLOSE_SOCKET:
				printf("disconnected from server\n");
				pthread_cancel(receiver_id);
				close(sockfd);
				return 0;
			case SHUTDOWN_CLIENT:
				printf("\e[01;35mGood Bye!\n");
				exit(0);
			default:
				printf("Wrong operation\n");
				continue;
		}
		// send buffer
		char buff[sizeof(struct Message)];
		// send client message
		bzero(buff, sizeof(struct Message));
		memcpy(buff, &client_msg, sizeof(client_msg));
		int sent_bytes = send(sockfd, buff, sizeof(client_msg), 0);
	}
}

int main(int argc, char *argv[])
{
	int connection_status = 0;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int sockfd;
	pthread_t receiver_id, sender_id;

	while(1){
		int operation;

		printf("\e[01;33;44m   Please choose operation(6~7):          \e[0m\n");
		printf("\e[01;33;44m ---------------------------------------- \e[0m\n");
		printf("\e[01;33;44m      1: Connect to server                \e[0m\n");
		printf("\e[01;33;44m      2: Exit                             \e[0m\n");
		printf("\e[01;33;44m ---------------------------------------- \e[0m\n");

		// printf("Choose operation: ");
		scanf("%d", &operation);

		if(operation == 1){
			char ip[256];
			int port;
			printf("Please enter the sever IP: ");
			scanf("%s", ip);
			printf("Please enter the port number: ");
			scanf("%d", &port);

			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0)
				herror("error opening socket");

			// set the server address
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(port);
			serv_addr.sin_addr.s_addr = inet_addr(ip);
			bzero(&(serv_addr.sin_zero), 8);

			// send the request
			printf("\e[01;32;46m Connecting... \e[0m\n");
			if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
			{
				printf("\e[01;32;46m Connect failed! \e[0m\n");
				close(sockfd);
				exit(0);
			}
			// connected
			printf("\e[01;32;46m Connected! \e[0m\n");

			struct Params p;
			p.sockfd = sockfd;
			p.thread_id = receiver_id;
			pthread_create(&sender_id, NULL, send_msg, (void *) &p);
			pthread_create(&receiver_id, NULL, recv_msg, (void *) &sockfd);

			pthread_join(receiver_id, NULL);
			pthread_join(sender_id, NULL);
		} else if (operation == 2){
			exit(0);
		} else {
			printf("Wrong operation\n");
		}
	}

	close(sockfd);
	return 0;
}
