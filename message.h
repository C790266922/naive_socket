#include <time.h>
#include <stdlib.h>
#include <netinet/in.h>

// max connections to server
#define MAX_CONNECTION 20
// max message size
#define MAX_MSG_SIZE 300

// operations
#define GET_TIME  1
#define GET_SERVER_NAME 2
#define GET_CONNECTION_LIST 3
#define SEND_MESSAGE 4
#define CLOSE_SOCKET 5
#define SHUTDOWN_CLIENT 6
#define CONNECT_SERVER 7
#define RECV_MESSAGE 8

// error code
#define CLIENT_OFFLINE 11
#define CLIENT_NOT_EXIST 12
#define WRONG_OPERATION_CODE 13

struct Client{
	int index; // client index
	char ip[20]; // ipv4 address, string, eg. 127.0.0.1
	int port; // port number, 0-65535
};

struct Message
{
	// operation: 1, 2, 3, 4, 5, 6
	int operation;
	// clients connected to server
	int connection_num;
	struct Client clients[MAX_CONNECTION];
	// unix timestamp
	long t;	
	// server name
	char server_name[MAX_MSG_SIZE];
	// message to send
	int client_no;
	char msg[MAX_MSG_SIZE];
	// flag to identify if the operation succeed
	int flag;
	// error code
	int error_code;
	char error_msg[MAX_MSG_SIZE];
};

void print_message(struct Message msg)
{
	printf("\n========================\n");
	printf("message content:\n");
	printf("operation: %d\n", msg.operation);
	printf("flag: %d\n", msg.flag);
	printf("connection_num: %d\n", msg.connection_num);
	printf("connection list:\n");
	int i = 0;
	for( ; i < msg.connection_num; ++i){
		printf("\t%d %s %d\n", msg.clients[i].index, msg.clients[i].ip, msg.clients[i].port);
	}
	printf("time: %ld\n", msg.t);
	printf("server_name: %s\n", msg.server_name);
	printf("client_No: %d\n", msg.client_no);
	printf("message to send: %s\n", msg.msg);
	printf("error_code: %d\n", msg.error_code);
	printf("error_message: %s\n", msg.error_msg);
	printf("\n========================\n");
}
