#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "message.h"

// current client number
int current_connections = 0;
// client information
struct sockaddr_in clients[MAX_CONNECTION];
// client fds
int client_fds[MAX_CONNECTION];

int remove_client(int idx)
{
	if(idx < 0 || idx >= current_connections){
		printf("client %d not exist", idx + 1);
		return 0;
	}
	int i = 0;
	for(i = idx; i < current_connections - 1; ++i){
			clients[i] = clients[i + 1];
			client_fds[i] = client_fds[i + 1];
	}
	--current_connections;
	printf("client %d removed from list, current_connections: %d\n", idx + 1, current_connections);
	return 1;
}
// handle session with one client
void *handle_session(void * fd)
{
	int socketfd = *(int*)fd;
	int idx = 0;
	char *buff = malloc(sizeof(struct Message));
	int i = 0; // used in for loops

	while(1){
		// find the index of the client in current session
		for(i = 0; i < current_connections; ++i){
			if(client_fds[i] == socketfd){
				idx = i;
				break;
			}	
		}
		
		// clear buffer	
		bzero(buff, sizeof(struct Message));
		// read from socket
		int n = recv(socketfd, buff, sizeof(struct Message), 0);
		// printf("received %d bytes from client %d\n", n, idx + 1);
		if(n <= 0){
			printf("client %d offline\n", idx + 1);
			remove_client(idx);
			break;
		} 
		
		// recover received client message
		struct Message client_msg = *(struct Message *) buff;
		printf("received %d bytes from client %d, operation: %d\n", n, idx + 1, client_msg.operation);

		// server message to be sent to client
		struct Message server_msg;
		// current time, for GET_TIME operation
		time_t t;
		// client index to send message to, for SEND_MESSAGE operation
		int client_no;
		// server name, for GET_SERVER_NAME operation
		char server_name[MAX_MSG_SIZE];
		
		switch(client_msg.operation){
			case GET_TIME:
				t = time(NULL);
				server_msg.t = t;
				server_msg.flag = 1;
				server_msg.operation = GET_TIME;
				break;
			case GET_SERVER_NAME:
				gethostname(server_name, MAX_MSG_SIZE);
				strcpy(server_msg.server_name, server_name);
				server_msg.flag = 1;
				server_msg.operation = GET_SERVER_NAME;
				break;
			case GET_CONNECTION_LIST:
				// printf("connected clients: \n");
				for(i = 0; i < current_connections; ++i){
					struct Client client;
					client.index = i + 1;
					char *ip = inet_ntoa(clients[i].sin_addr);
					memcpy(client.ip, ip, strlen(ip) + 2);
					client.port = clients[i].sin_port;
					// printf("%d %s %d\n", client.index, client.ip, client.port);
					memcpy(&server_msg.clients[i], &client, sizeof(client));
				}
				server_msg.connection_num = current_connections;
				server_msg.flag = 1;
				server_msg.operation = GET_CONNECTION_LIST;
				break;
			case SEND_MESSAGE:
				// sent message to given client
				server_msg.operation = RECV_MESSAGE;
				client_no = client_msg.client_no;
				strcpy(server_msg.msg, client_msg.msg);
				if(client_no > current_connections || client_no < 1){
					server_msg.error_code = CLIENT_NOT_EXIST;
					char *err_msg = "client not exist";
					memcpy(server_msg.error_msg, err_msg, strlen(err_msg) + 2);
					server_msg.flag = 0;
				} else {
					memcpy(buff, &server_msg, sizeof(struct Message));
					n = send(client_fds[client_no - 1], buff, sizeof(server_msg), 0);
					if(n < 0){
						server_msg.flag = 0;
						server_msg.error_code = CLIENT_OFFLINE;
						char *err_msg = "client offline";
						strcpy(server_msg.error_msg, err_msg);
					}
				}
				server_msg.operation = SEND_MESSAGE;
				break;
			default:
				server_msg.error_code = WRONG_OPERATION_CODE;
				char *err_msg = "wrong operation number";
				memcpy(server_msg.error_msg, err_msg, strlen(err_msg) + 2);
				server_msg.flag = 0;
		}
		
		bzero(buff, sizeof(struct Message));
		memcpy(buff, &server_msg, sizeof(struct Message));
		n = send(socketfd, buff, sizeof(server_msg), 0);
		// print_message(server_msg);
		if(n < 0){
			printf("error write to client %d\n", idx + 1);
			// remove client from array
			remove_client(idx);
			break;
		}
	}
	// pthread_exit(NULL);
	return 0;
}

int main(int argc, char *argv[])
{
	// get port No
	if(argc < 2){
		printf("please provide a port number\n");
		exit(0);
	}
	int port_no = atoi(argv[1]);

	// socket file descriptor
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd < 0){
		printf("error opening socket\n");
		exit(0);
	}

	struct sockaddr_in server_addr, client_addr;
	bzero((char *) &server_addr, sizeof(server_addr));

	// server settings
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_no);

	// bind
	int flag = bind(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if(flag < 0){
		printf("error binding socket\n");
		exit(0);
	}

	// listen
	listen(socketfd, 5);

	socklen_t client_len = sizeof(client_addr);

	while(1){
		// accept
		int new_socketfd = accept(socketfd, (struct sockaddr *) &client_addr, &client_len);
		// accept error
		if(new_socketfd < 0){
			printf("error on accept\n");
			continue;
		}

		// create a new thread for this connection
		pthread_t thread_id;
		int flag = pthread_create(&thread_id, NULL, handle_session, (void *)&new_socketfd);

		if(flag == -1){
			printf("error creating thread\n");
			break;
		} else {
			char *ip = inet_ntoa(client_addr.sin_addr);
			printf("client %d connected from %s port %d\n", current_connections + 1, ip, client_addr.sin_port);
			// store the client infomation
			clients[current_connections] = client_addr;
			client_fds[current_connections] = new_socketfd;
			++current_connections;
		}
	}
	
	close(socketfd);

	return 0;
}
