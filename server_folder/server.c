#include "server.h"

int PORT = -1;

void free_client(client_t *client) {
    if (client->_info->_username != NULL) {
        free(client->_info->_username);
    }
    if (client->_info->_password != NULL) {
        free(client->_info->_password);
    }
    if (client->_info->_first_name != NULL) {
        free(client->_info->_first_name);
    }
    if (client->_info->_last_name != NULL) {
        free(client->_info->_last_name);
    }
    if (client->_info->_role != NULL) {
        free(client->_info->_role);
    }
    if (client->_info != NULL) {
        free(client->_info);
    }
    if (client->_friends != NULL) {
        free(client->_friends);
    }
}

static void sigint_handler(int signum) {
    printf("Caught signal %d\n", signum);
    exit(0);
}

int main(int argc, char *argv[]) {
    // clear the screen beforehand
    system("clear");

    // seed the random number generator
    srand(time(NULL));

    if (argc != 5) {
        printf("Usage: ./server <config_file> <clients_database_file> <clients_friends_database> <server_port>\n");
        exit(-1);
    }

    PORT = atoi(argv[4]);

    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = sigint_handler;

    pthread_mutex_init(&database_modification_mutex, NULL);
    pthread_cond_init(&database_modification_condition_var, NULL);

    FILE *server_log_file = fopen("server_log.txt", "w");
    fprintf(server_log_file, "Server %s log\n", VERSION);

    time_t now;
    struct tm ts;
    now = time(NULL);

    time(&now);
    char date[80];

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    if (sigaction (SIGINT, &sa, NULL) == -1)
        fprintf (server_log_file, "[%s] Failed to overwrite SIGINT.\n", date);
    else
        fprintf (server_log_file, "[%s] Successfully overwritten SIGINT.\n", date);

    char **config_vars = load_config(argv[1]);

    char *loading = config_vars[0];
    char *typing = config_vars[1];
    char *offline = config_vars[2];
    char *online = config_vars[3];
    char *last_activity = config_vars[4];

    char *not_friends = config_vars[5];
    char *friend_req_sent_succ = config_vars[6];
    char *friend_req_sent_fail = config_vars[7];
    char *friend_req_await = config_vars[8];

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(server_log_file, "[%s] Finished loading config variables.\n", date);
    fflush(server_log_file);

    int server_socket;
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    int rc = init_server(&server_socket, &server_address);
    if (rc == -1)
        goto end;

    client_t **clients = (client_t **)malloc(MAX_CLIENTS * sizeof(client_t *));
    int nr_clients = 0;
    if (!clients) {
        printf("Error allocating memory for clients\n");
        goto end;
    }

    rc = load_clients(&clients, &nr_clients, argv[2]);
    if (rc == -1)
        goto end_if_clients;

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(server_log_file, "[%s] Finished loading clients.(%d)\n", date, nr_clients);
    fflush(server_log_file);

    rc = load_friends(clients, nr_clients, argv[3]);
    if (rc == -1)
        goto end_if_clients;

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(server_log_file, "[%s] Finished loading friends.\n", date);
    fflush(server_log_file);

    int nr_friend_requests = 0;
    friend_req_t *friend_requests = NULL;

    rc = load_friend_requests(clients, nr_clients, &friend_requests, &nr_friend_requests);
    if (rc == -1)
        goto end_if_clients;
    
    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(server_log_file, "[%s] [OK] Finished loading friend requests(%d)\n", date, nr_friend_requests);
    fflush(server_log_file);

    int nr_conversations = 0;
    conversation_t **conversations = NULL;

    rc = load_conversations(clients, nr_clients, &conversations, &nr_conversations);
    if (rc == -1)
        goto end_if_friend_requests;
    
    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(server_log_file, "[%s] [OK] Finished loading the conversations(%d)\n", date, nr_conversations);
    fflush(server_log_file);

    fflush(server_log_file);

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(server_log_file, "[%s] [OK] Finished initializing server. Running on socket %d and on port %d\n", date, server_socket, PORT);

    fflush(server_log_file);

    // create the thread that updates each of the databases live(every 15 seconds)
    thread_arg_t *maintenance_args = (thread_arg_t *)malloc(sizeof(thread_arg_t));
    if (!maintenance_args) {
        printf("Error allocating memory for thread arguments\n");
        goto end_if_conversations;
    }

    maintenance_args->_client_socket = -1;

    maintenance_args->_clients = &clients;
    maintenance_args->_nr_clients = &nr_clients;

    maintenance_args->_conversations = &conversations;
    maintenance_args->_nr_conversations = &nr_conversations;

    maintenance_args->_variable_variables = NULL;
    maintenance_args->_variable_pointers = (void **)malloc(4 * sizeof(void *));
    if (!maintenance_args->_variable_pointers) {
        printf("Error allocating memory for thread arguments\n");
        goto end_if_conversations;
    }
    maintenance_args->_variable_pointers[0] = (void *)argv[2];
    maintenance_args->_variable_pointers[1] = (void *)argv[3];
    maintenance_args->_variable_pointers[2] = (void *)(&friend_requests);
    maintenance_args->_variable_pointers[3] = (void *)(&nr_friend_requests);

    pthread_t database_thread;
    pthread_create(&database_thread, NULL, &database_maintenance, (void *)maintenance_args);

    int client_fd;
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = (socklen_t)sizeof(client_addr);

    // accept new connections
    while((client_fd = accept(server_socket, (struct sockaddr *)&client_addr, &len)) > 0) {
        new_connection_announcer(client_fd, &client_addr, server_log_file);
        // create a new thread and handle the client
        pthread_t client_thread;
        
        // create the argument of the function for the thread
        thread_arg_t *args = (thread_arg_t *)malloc(sizeof(thread_arg_t));
        if (!args) {
            printf("Error allocating memory for thread arguments\n");
            goto end_if_conversations;
        }

        args->_client_socket = client_fd;

        args->_clients = &clients;
        args->_nr_clients = &nr_clients;

        args->_conversations = &conversations;
        args->_nr_conversations = &nr_conversations;

        args->_variable_variables = NULL;
        args->_variable_pointers = (void **)malloc(3 * sizeof(void *));
        if (!args->_variable_pointers) {
            printf("Error allocating memory for thread arguments\n");
            goto end_if_conversations;
        }
        args->_variable_pointers[0] = (void *)argv[2];
        args->_variable_pointers[1] = (void *)(&friend_requests);
        args->_variable_pointers[2] = (void *)(&nr_friend_requests);

        pthread_create(&client_thread, NULL, &handle_client, (void *)args);
    }

end_if_conversations:
    for (int i = 0; i < nr_conversations; i++) {
        if (conversations[i] == NULL)
            continue;
        free(conversations[i]);
    }
    free(conversations);

end_if_friend_requests:
    if (friend_requests != NULL) {
        free(friend_requests);
    }

end_if_clients:
    for (int i = 0; i < nr_clients; i++) {
        if (clients[i] == NULL)
            continue;
        free_client(clients[i]);
        free(clients[i]);
        close(clients[i]->_client_socket);
    }
    free(clients);

end:
    for (int i = 0; i < NR_VARS; i++) {
            free(config_vars[i]);
        }
    free(config_vars);
    close(server_socket);
    fclose(server_log_file);

    return 0;
}