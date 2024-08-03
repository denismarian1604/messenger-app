#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <time.h>
#include <signal.h>

#define VERSION "v1.0 update 1"
#define MAX_CLIENTS 10
#define MAX_MESSAGE_SIZE 2000
#define MESSAGE_T_SIZE sizeof(struct message)
#define NR_VARS 9

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

#define VIEW_CONVERSATION 70

pthread_mutex_t database_modification_mutex;
pthread_cond_t database_modification_condition_var;

static char *users_database_header = "username,password,firstName,lastName,role,registerTimeStamp,lastSeenTimeStamp";
static char *friend_requests_db_filename = "../databases/friend_requests_db.txt";
static char *conversations_db_filename = "../databases/conversations_db.txt";

typedef struct message {
    char content[MAX_MESSAGE_SIZE + 1];
    int _validation_key; // later used to validate the message
} message_t;

typedef struct info {
    long _timestamp_last_seen;
    long _timestamp_register;

    int _status;

    char *_username, *_password, *_first_name, *_last_name, *_role;
} client_info;

typedef struct client {
    int _client_socket;
    // struct sockaddr_in _client_address; // will be used if needed in the future
    client_info *_info;

    struct client **_friends;
    int _nr_friends;

    // int _nr_waiting_messages;
    // message_t **_waiting_messages; // messages received while the client was offline; to be implemented in v2.0

    int _validation_key; // different from the message_t validation key

    int _FLAGS; // future use for locking accounts, etc.
} client_t;

typedef struct friend_req {
    client_t *sender, *recipient;
} friend_req_t;

typedef struct conversation {
    client_t *_client1, *_client2;
    message_t **_messages;
    int _nr_messages;
} conversation_t;

typedef struct arg {
    client_t ***_clients;
    int *_nr_clients;

    conversation_t ***_conversations;
    int *_nr_conversations;

    int _client_socket;
    void *_variable_variables;
    void **_variable_pointers;
} thread_arg_t;

char **load_config(char *filename);
int init_server(int *server_socket, struct sockaddr_in *server_address);

int load_clients(client_t ***clients, int *nr_clients, char *clients_filename);
int load_friends(client_t **clients, int nr_clients, char *friends_filename);
int load_friend_requests(client_t **clients, int nr_clients, friend_req_t **friend_requests, int *nr_friend_requests);
int load_conversations(client_t **clients, int nr_clients, conversation_t ***conversations, int *nr_conversations);

client_t *find_client(client_t **clients, int nr_clients, char *username);

void new_connection_announcer(int client_fd, struct sockaddr_in *client_addr, FILE *server_log_file);
int send_to_client(int client_fd, void *buffer, int size);

void *handle_client(void *args);
void *database_maintenance(void *args);

void send_message_to_client(int client_socket, char *error_message);

void view_conversation(thread_arg_t *arg, message_t message);
void send_message(thread_arg_t *arg, message_t message);