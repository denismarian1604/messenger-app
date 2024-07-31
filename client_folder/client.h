#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define VERSION "v1.0 update 0"

#define PORT 16048
#define IP "192.168.100.206"

#define MAX_MESSAGE_SIZE 2000
#define MESSAGE_T_SIZE sizeof(struct message)
#define NR_VARS 67

#define NOT_LOGGED_IN 0
#define LOGGED_IN 1
#define IN_FRIEND_REQUESTS 2

#define ONLINE 16
#define OFFLINE 17

#define REGISTER 1
#define LOGIN 2
#define EXIT 3
#define SEND_MESSAGE 4
#define VIEW_MESSAGES 5
#define SEND_FRIEND_REQ 6
#define VIEW_FRIEND_REQS 7
#define VIEW_FRIENDS 8
#define LOGOUT 9
#define ACCEPT_FRIEND_REQ 10
#define DECLINE_FRIEND_REQ 11
#define BACK_FROM_FRIEND_REQS 12

#define VIEW_CONVERSATION 70

typedef struct message {
    char content[MAX_MESSAGE_SIZE + 1];
    int _validation_key; // later used to validate the message
} message_t;

char **load_config_client(char *filename);

void client_init(int *client_socket, struct sockaddr_in *client_addr);
int connect_to_server(int *client_socket, struct sockaddr_in *client_addr);

int server_send(int client_socket, void *buffer, int size);
int receive_from_server(int client_socket, void *buffer, int size);

void display_client_menu(char **config_vars);
void handle_not_logged_in(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars);
void handle_logged_in(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars);
void handle_in_friend_requests(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars);

void parse_conversation(char *messages, char *friend, char **config_vars);