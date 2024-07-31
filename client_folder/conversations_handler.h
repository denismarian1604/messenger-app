#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 16048
#define IP "192.168.100.206"

#define NR_VARS 67

#define ONLINE 16
#define OFFLINE 17

#define SEND_MESSAGE 4

#define MAX_MESSAGE_SIZE 2000
#define MESSAGE_T_SIZE sizeof(struct message)

#define VIEW_CONVERSATION 70

typedef struct message {
    char content[MAX_MESSAGE_SIZE + 1];
    int _validation_key; // later used to validate the message
} message_t;

char **load_config_client(char *filename);

void parse_conversation(char *messages, char **config_vars);

int server_send(int client_socket, void *buffer, int size);
int receive_from_server(int client_socket, void *buffer, int size);
