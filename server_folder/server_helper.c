#include "server.h"

int init_server(int *server_socket, struct sockaddr_in *server_address) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Error creating server socket\n");
        return -1;
    }

    (*server_address).sin_family = AF_INET;
    (*server_address).sin_port = htons(PORT);
    (*server_address).sin_addr.s_addr = INADDR_ANY;

    if (bind(*server_socket, (struct sockaddr *)server_address, sizeof(struct sockaddr_in)) < 0) {
        printf("Error binding server socket\n");
        return -1;
    }

    if (listen(*server_socket, MAX_CLIENTS) < 0) {
        printf("Error listening on server socket\n");
        return -1;
    }

    return 0;
}

char *get_string_date() {
    time_t now;
    now = time(NULL);

    struct tm ts = *localtime(&now);
    char *date = (char *)malloc(100 * sizeof(char));
    strftime(date, 100, "%a %Y-%m-%d %H:%M:%S", &ts);

    return date;
}

void new_connection_announcer(int client_fd, struct sockaddr_in *client_addr, FILE *server_log_file) {
    time_t now;
    now = time(NULL);

    fclose(server_log_file);
    fopen("server_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    
    pthread_mutex_lock(&database_modification_mutex);
    fprintf(server_log_file, "[%s] [OK] New connection established on socket %d from IP %s on port %d\n",
                                date, client_fd, inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    fflush(server_log_file);
    pthread_mutex_unlock(&database_modification_mutex);
}

int receive_from_client(int client_fd, void *buffer, int size) {
    int received_so_far = 0;

    while (received_so_far < size) {
        int received = recv(client_fd, buffer + received_so_far, size - received_so_far, 0);
        if (received < 0) {
            return -1;
        }

        received_so_far += received;
    }

    return received_so_far;
}

int send_to_client(int client_fd, void *buffer, int size) {
    int sent_so_far = 0;

    while (sent_so_far < size) {
        int sent = send(client_fd, buffer + sent_so_far, size - sent_so_far, 0);
        if (sent < 0) {
            return -1;
        }

        sent_so_far += sent;
    }

    return sent_so_far;
}

void send_message_to_client(int client_socket, char *error_message) {
    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "%s", error_message);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }
}

void register_client(thread_arg_t *arg, message_t message) {
    char *token = strtok(message.content + 6, "\n");
    if(find_client(*(arg->_clients), *(arg->_nr_clients), token)) {
        send_message_to_client(arg->_client_socket, "RES/-1\nUsername already exists.\n\n");
        return;
    }

    client_info *info = (client_info *)malloc(sizeof(client_info));
    if (!info) {
        printf("Error allocating memory for client info\n");
        return;
    }

    info->_username = (char *)malloc((strlen(token) + 1) * sizeof(char));
    strcpy(info->_username, token);

    token = strtok(NULL, "\n");
    info->_password = (char *)malloc((strlen(token) + 1) * sizeof(char));
    strcpy(info->_password, token);

    token = strtok(NULL, "\n");
    info->_first_name = (char *)malloc((strlen(token) + 1) * sizeof(char));
    strcpy(info->_first_name, token);

    token = strtok(NULL, "\n");
    info->_last_name = (char *)malloc((strlen(token) + 1) * sizeof(char));
    strcpy(info->_last_name, token);

    info->_status = OFFLINE;

    token = strtok(NULL, "\n");
    info->_timestamp_register = atol(token);
    info->_timestamp_last_seen = time(NULL);

    info->_role = (char *)malloc((strlen(token) + 1) * sizeof(char));
    strcpy(info->_role, "client");

    pthread_mutex_lock(&database_modification_mutex);

    client_t *client = (client_t *)malloc(sizeof(client_t));
    if (!client) {
        printf("Error allocating memory for client\n");
        return;
    }

    client->_info = info;
    client->_validation_key = -1;

    client_t ***clients = arg->_clients;
    int *nr_clients = arg->_nr_clients;

    if (*nr_clients == MAX_CLIENTS) {
        printf("Maximum number of clients reached\n");
        return;
    }

    (*clients)[*nr_clients] = client;
    (*nr_clients)++;

    time_t now;
    now = time(NULL);

    FILE *server_log_file = fopen("server_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(server_log_file, "[%s] [OK] New client registered : %s.\n", date, info->_username);
    fflush(server_log_file);

    fclose(server_log_file);

    pthread_mutex_unlock(&database_modification_mutex);
    pthread_cond_signal(&database_modification_condition_var);

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "RES/1\nUsername %s successfully registered.\n\n", token);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
}
    return;
}

void login_client(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nUsername does not exist.\n\n");
        return;
    }

    char *password = strtok(NULL, "\n");

    // check if the passwords match
    if (strncmp(client->_info->_password, password, strlen(client->_info->_password)) != 0) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid password.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed log in attempt caused by wrong password on username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    if (client->_validation_key != -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nClient already connected.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Attempt to login as already connected client : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    pthread_mutex_lock(&database_modification_mutex);

    // update the last seen timestamp
    client->_info->_timestamp_last_seen = time(NULL);
    // update the client status
    client->_info->_status = ONLINE;
    // update the client socket
    client->_client_socket = arg->_client_socket;
    // generate the validation key
    client->_validation_key = rand() % 1000000;

    time_t now;
    now = time(NULL);

    FILE *server_log_file = fopen("server_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(server_log_file, "[%s] [OK] Client(%s) logged in : %s -- %d.\n", date, client->_info->_role, username, client->_validation_key);
    fflush(server_log_file);

    fclose(server_log_file);

    pthread_mutex_unlock(&database_modification_mutex);
    pthread_cond_signal(&database_modification_condition_var);

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "RES/1\n%d\nSuccessfully logged in.\n\n", client->_validation_key);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }
}

void exit_client(thread_arg_t *arg, message_t message) {
    close(arg->_client_socket);
}

void logout_client(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nUsername does not exist.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed log out attempt on inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed log out attempt on username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    pthread_mutex_lock(&database_modification_mutex);

    // update the last seen timestamp
    client->_info->_timestamp_last_seen = time(NULL);
    // update the client status
    client->_info->_status = OFFLINE;
    // update the client socket
    client->_client_socket = -1;
    // reset the validation key
    client->_validation_key = -1;

    time_t now;
    now = time(NULL);

    FILE *server_log_file = fopen("server_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(server_log_file, "[%s] [OK] Client logged out : %s\n", date, username);
    fflush(server_log_file);

    fclose(server_log_file);

    pthread_mutex_unlock(&database_modification_mutex);
    pthread_cond_signal(&database_modification_condition_var);

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "RES/1\n%d\nSuccessfully logged out.\n\n", client->_validation_key);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }
}

void send_friend_request(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed friend request attempt from inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed friend request attempt from username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    client_t *recipient = find_client(*(arg->_clients), *(arg->_nr_clients), strtok(NULL, "\n"));
    if (!recipient) {
        send_message_to_client(arg->_client_socket, "RES/-1\nRecipient username does not exist.\n\n");

        return;
    } else if (strcmp(recipient->_info->_username, client->_info->_username) == 0) {
        send_message_to_client(arg->_client_socket, "RES/-1\nYou cannot send a friend request to yourself.\n\n");

        return;
    }

    // check if the users are already friends
    for (int i = 0; i < client->_nr_friends; i++) {
        if (strncmp(client->_friends[i]->_info->_username, recipient->_info->_username, strlen(recipient->_info->_username)) == 0) {
            send_message_to_client(arg->_client_socket, "RES/2\nYou are already friends with the recipient.\n\n");

            return;
        }
    }
    friend_req_t **friend_requests = (friend_req_t **)(arg->_variable_pointers[1]);
    int *nr_friend_requests = (int *)(arg->_variable_pointers[2]);
    // check if there is already a friend request from the recipient to the current sender
    // in that case, automatically accept the friend request, and make the two friends
    for (int i = 0; i < *nr_friend_requests; i++) {
        char *sender_check = (*friend_requests)[i].sender->_info->_username;
        char *recipient_check = (*friend_requests)[i].recipient->_info->_username;

        // add the two clients as friends
        if (strncmp(sender_check, recipient->_info->_username, strlen(recipient->_info->_username)) == 0 &&
            strncmp(recipient_check, client->_info->_username, strlen(client->_info->_username)) == 0) {
            client->_friends = (client_t **)realloc(client->_friends, (client->_nr_friends + 1) * sizeof(client_t *));
            if (!client->_friends) {
                printf("Error reallocating memory for friends\n");
                return;
            }

            client->_friends[client->_nr_friends] = recipient;
            client->_nr_friends++;

            recipient->_friends = (client_t **)realloc(recipient->_friends, (recipient->_nr_friends + 1) * sizeof(client_t *));
            if (!recipient->_friends) {
                printf("Error reallocating memory for friends\n");
                return;
            }

            recipient->_friends[recipient->_nr_friends] = client;
            recipient->_nr_friends++;

            // remove the friend request
            for (int j = i; j < *nr_friend_requests - 1; j++) {
                (*friend_requests)[j] = (*friend_requests)[j + 1];
            }

            *nr_friend_requests = *nr_friend_requests - 1;
            *friend_requests = (friend_req_t *)realloc(*friend_requests, *nr_friend_requests * sizeof(friend_req_t));
            if (!*friend_requests) {
                printf("Error reallocating memory for friend requests\n");
                return;
            }

            send_message_to_client(arg->_client_socket, "RES/3\nFriend request mutually cancelled with friend request from the recipient.\n\n");

            return;
        }
    }

    // check if the client already made a friend request to the recipient
    for (int i = 0; i < *nr_friend_requests; i++) {
        char *sender_check = (*friend_requests)[i].sender->_info->_username;
        char *recipient_check = (*friend_requests)[i].recipient->_info->_username;

        if (strncmp(sender_check, client->_info->_username, strlen(client->_info->_username)) == 0&&
            strncmp(recipient_check, recipient->_info->_username, strlen(recipient->_info->_username)) == 0) {
            send_message_to_client(arg->_client_socket,
                                        "RES/4\nYou already sent a friend request to the recipient.\n\n");
            return;
        }
    }

    // if not, register the friend request
    *friend_requests = (friend_req_t *)realloc(*friend_requests, (*nr_friend_requests + 1) * sizeof(friend_req_t));
    if (!*friend_requests) {
        printf("Error reallocating memory for friend requests\n");
        return;
    }

    (*friend_requests)[*nr_friend_requests].sender = client;
    (*friend_requests)[*nr_friend_requests].recipient = recipient;

    (*nr_friend_requests)++;

    send_message_to_client(arg->_client_socket, "RES/1\nFriend request sent successfully.\n\n");

    return;
}

void view_friend_requests(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed friend requests review attempt from inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed friend requests review attempt from username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    friend_req_t *friend_requests = *((friend_req_t **)(arg->_variable_pointers[1]));
    int nr_friend_requests = *((int *)(arg->_variable_pointers[2]));

    char **sent_requests = NULL;
    int nr_sent_requests = 0;

    char **received_requests = NULL;
    int nr_received_requests = 0;

    for (int i = 0; i < nr_friend_requests; i++) {
        client_t *sender = friend_requests[i].sender;
        client_t *recipient = friend_requests[i].recipient;

        // check if the client is the sender
        if (strncmp(client->_info->_username, sender->_info->_username, strlen(sender->_info->_username)) == 0) {
            if (!sent_requests) {
                sent_requests = (char **)malloc(sizeof(char *));
            } else {
                sent_requests = (char **)realloc(sent_requests, (nr_sent_requests + 1) * sizeof(char *));
            }

            sent_requests[nr_sent_requests++] = strdup(recipient->_info->_username);
            // check if the client is the recipient
        } else if(strncmp(client->_info->_username, recipient->_info->_username, strlen(recipient->_info->_username)) == 0) {
            if (!received_requests) {
                received_requests = (char **)malloc(sizeof(char *));
            } else {
                received_requests = (char **)realloc(received_requests, (nr_received_requests + 1) * sizeof(char *));
            }

            received_requests[nr_received_requests++] = strdup(sender->_info->_username);
        }
    }

    char format_string[MAX_MESSAGE_SIZE + 1 - 9] = {'\0'};
    strcpy(format_string, "{\"to\":[");

    // attach the sent requests
    for (int i = 0; i < nr_sent_requests; i++) {
        strcat(format_string, "\"");
        strcat(format_string, sent_requests[i]);
        strcat(format_string, "\"");

        if (i < nr_sent_requests - 1) {
            strcat(format_string, ",");
        }
    }

    strcat(format_string, "]}{\"from\":[");

    // attach the received requests
    for (int i = 0; i < nr_received_requests; i++) {
        strcat(format_string, "\"");
        strcat(format_string, received_requests[i]);
        strcat(format_string, "\"");

        if (i < nr_received_requests - 1) {
            strcat(format_string, ",");
        }
    }

    strcat(format_string, "]}");

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "RES/1\n%s\n\n", format_string);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }

    // free the memory allocated for the friend requests
    for (int i = 0; i < nr_sent_requests; i++) {
        free(sent_requests[i]);
    }

    for (int i = 0; i < nr_received_requests; i++) {
        free(received_requests[i]);
    }

    free(sent_requests);
    free(received_requests);

    return;
}

void accept_friend_request(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed friend request acceptance from inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed friend request acceptance from username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    client_t *target_client = find_client(*(arg->_clients), *(arg->_nr_clients), strtok(NULL, "\n"));
    if (!target_client) {
        send_message_to_client(arg->_client_socket, "RES/-1\nTarget username does not exist.\n\n");

        return;
    }

    friend_req_t **friend_requests = (friend_req_t **)(arg->_variable_pointers[1]);
    int *nr_friend_requests = (int *)(arg->_variable_pointers[2]);

    int pos = -1;

    for (int i = 0; i < *nr_friend_requests; i++) {
        char *sender_check = (*friend_requests)[i].sender->_info->_username;
        char *recipient_check = (*friend_requests)[i].recipient->_info->_username;

        if (strncmp(sender_check, target_client->_info->_username, strlen(target_client->_info->_username)) == 0 &&
            strncmp(recipient_check, client->_info->_username, strlen(client->_info->_username)) == 0) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nNo friend request found.\n\n");

        return;
    }

    // add the two clients as friends
    client->_friends = (client_t **)realloc(client->_friends, (client->_nr_friends + 1) * sizeof(client_t *));
    if (!client->_friends) {
        printf("Error reallocating memory for friends\n");
        return;
    }

    client->_friends[client->_nr_friends] = target_client;
    client->_nr_friends++;

    target_client->_friends = (client_t **)realloc(target_client->_friends, (target_client->_nr_friends + 1) * sizeof(client_t *));
    if (!target_client->_friends) {
        printf("Error reallocating memory for friends\n");
        return;
    }

    target_client->_friends[target_client->_nr_friends] = client;
    target_client->_nr_friends++;

    // remove the friend request
    for (int i = pos; i < *nr_friend_requests - 1; i++) {
        (*friend_requests)[i] = (*friend_requests)[i + 1];
    }
    
    // decrease the number of friend requests
    *nr_friend_requests = *nr_friend_requests - 1;
    // realloc the friend requests
    *friend_requests = (friend_req_t *)realloc(*friend_requests, *nr_friend_requests * sizeof(friend_req_t));
    if (!*friend_requests) {
        printf("Error reallocating memory for friend requests\n");
        return;
    }

    send_message_to_client(arg->_client_socket, "RES/1\nFriend request accepted successfully.\n\n");

    return;
}

void decline_friend_request(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed friend request decline from inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed friend request decline from username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    client_t *target_client = find_client(*(arg->_clients), *(arg->_nr_clients), strtok(NULL, "\n"));
    if (!target_client) {
        send_message_to_client(arg->_client_socket, "RES/-1\nTarget username does not exist.\n\n");

        return;
    }

    friend_req_t **friend_requests = (friend_req_t **)(arg->_variable_pointers[1]);
    int *nr_friend_requests = (int *)(arg->_variable_pointers[2]);

    int pos = -1;

    for (int i = 0; i < *nr_friend_requests; i++) {
        char *sender_check = (*friend_requests)[i].sender->_info->_username;
        char *recipient_check = (*friend_requests)[i].recipient->_info->_username;

        if (strncmp(sender_check, target_client->_info->_username, strlen(target_client->_info->_username)) == 0 &&
            strncmp(recipient_check, client->_info->_username, strlen(client->_info->_username)) == 0) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nNo friend request found.\n\n");

        return;
    }

    // remove the friend request
    for (int i = pos; i < *nr_friend_requests - 1; i++) {
        (*friend_requests)[i] = (*friend_requests)[i + 1];
    }

    // decrease the number of friend requests
    *nr_friend_requests = *nr_friend_requests - 1;
    // realloc the friend requests
    *friend_requests = (friend_req_t *)realloc(*friend_requests, *nr_friend_requests * sizeof(friend_req_t));
    if (!*friend_requests) {
        printf("Error reallocating memory for friend requests\n");
        return;
    }

    send_message_to_client(arg->_client_socket, "RES/1\nFriend request declined successfully.\n\n");

    return;
}

void view_friends(thread_arg_t *arg, message_t message) {
    char *username = strtok(message.content + 6, "\n");
    client_t *client;
    // check if the username exists
    if(!(client = find_client(*(arg->_clients), *(arg->_nr_clients), username))) {
        send_message_to_client(arg->_client_socket, "RES/-1\nUsername does not exist.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [WARN] Failed friends review attempt on inexistent username : %s.\n", date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char *validation_key_string = strtok(NULL, "\n");
    int validation_key = atoi(validation_key_string);

    // check if the validation codes match
    if (validation_key != client->_validation_key || client->_validation_key == -1) {
        send_message_to_client(arg->_client_socket, "RES/-1\nInvalid validation key.\n\n");

        pthread_mutex_lock(&database_modification_mutex);

        time_t now;
        now = time(NULL);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char date[80];
        struct tm ts = *localtime(&now);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        fprintf(server_log_file, "[%s] [CRITICAL] Failed friends review attempt on username %s with mistaken validation key. Possible data breach.\n",
                                    date, username);
        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);

        return;
    }

    char format_string[MAX_MESSAGE_SIZE + 1 - 9] = {'\0'};
    strcpy(format_string, "{\"friends\":[");

    client_t **friends = client->_friends;

    for (int i = 0; i < client->_nr_friends; i++) {
        strcat(format_string, "\"");
        strcat(format_string, friends[i]->_info->_username);
        strcat(format_string, "\"");

        if (i < client->_nr_friends - 1) {
            strcat(format_string, ",");
        }
    }

    strcat(format_string, "]}");

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);
    sprintf(buffer, "RES/1\n%s\n\n", format_string);

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }
}

void *handle_client(void *args) {
    thread_arg_t *arg = (thread_arg_t *)args;

    int req_type;

    do {
        message_t message;
        memset(&message, 0, sizeof(message_t));

        int received = receive_from_client(arg->_client_socket, (void *)&message, MESSAGE_T_SIZE);
        if (received < 0 || received != MESSAGE_T_SIZE) {
            close(arg->_client_socket);
            free(args);

            printf("Error receiving message from client\n");

            pthread_join(pthread_self(), NULL);
            return NULL;
        }
        // TODO someday: deal with lazy clients who send only part of the message; add a timer thread to check for this

        if (strncmp(message.content, "REQ", 3) == 0) {
            char copy[MAX_MESSAGE_SIZE + 1];
            memset(copy, 0, MAX_MESSAGE_SIZE + 1);
            memcpy(copy, message.content, MAX_MESSAGE_SIZE + 1);
            char *token = strtok(copy + 4, "\n");
            req_type = atoi(token);

            switch(req_type) {
                case REGISTER:
                    register_client(arg, message);
                    break;
                case LOGIN:
                    login_client(arg, message);
                    break;
                case EXIT:
                    exit_client(arg, message);
                    break;
                case VIEW_CONVERSATION:
                    view_conversation(arg, message);
                    break;
                case SEND_MESSAGE:
                    send_message(arg, message);
                    break;
                case VIEW_MESSAGES:
                    view_conversation(arg, message);
                    break;
                case SEND_FRIEND_REQ:
                    send_friend_request(arg, message);
                    break;
                case VIEW_FRIEND_REQS:
                    view_friend_requests(arg, message);
                    break;
                case VIEW_FRIENDS:
                    view_friends(arg, message);
                    break;
                case ACCEPT_FRIEND_REQ:
                    accept_friend_request(arg, message);
                    break;
                case DECLINE_FRIEND_REQ:
                    decline_friend_request(arg, message);
                    break;
                case LOGOUT:
                    logout_client(arg, message);
                    break;
                default:
                    printf("Invalid request type\n");
                    break;
            }
        }
    } while(req_type != EXIT);

    free(args);
    pthread_join(pthread_self(), NULL);

    return NULL;
}

void *database_maintenance(void *args) {
    thread_arg_t *arg = (thread_arg_t *)args;

    while (1) {
        // once every 15 seconds, propagate any possible changes in the databases
        // on the disk
        sleep(15);

        pthread_mutex_lock(&database_modification_mutex);

        FILE *server_log_file = fopen("server_log.txt", "a");

        char *date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Databases maintenance thread started %d\n", date, *(arg->_nr_clients));
        free(date);

        FILE *users_database_file = fopen((char *)(arg->_variable_pointers[0]), "w");

        if (!users_database_file) {
            printf("Error opening database files\n");

            char *date = get_string_date();
            fprintf(server_log_file, "[%s] [ERROR] Could not open databases. Aborting.%d\n", date, *(arg->_nr_clients));
            free(date);

            pthread_mutex_unlock(&database_modification_mutex);
            pthread_cond_signal(&database_modification_condition_var);

            continue;
        }

        // write the (eventual) changes to the users databases files
        fprintf(users_database_file, "%s\n", users_database_header);
        for (int i = 0; i < *(arg->_nr_clients); i++) {
            client_info *info = (*(arg->_clients))[i]->_info;
            fprintf(users_database_file, "%s,%s,%s,%s,%s,%ld,%ld\n", info->_username, info->_password, info->_first_name, info->_last_name, info->_role, info->_timestamp_register, info->_timestamp_last_seen);
        }

        fflush(users_database_file);
        fclose(users_database_file);

        date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Users database maintenance finished\n", date);
        free(date);

        fflush(server_log_file);

        FILE *friends_database_file = fopen((char *)(arg->_variable_pointers[1]), "w");

        if (!friends_database_file) {
            printf("Error opening database files\n");

            char *date = get_string_date();
            fprintf(server_log_file, "[%s] [ERROR] Could not open databases. Aborting.%d\n", date, *(arg->_nr_clients));
            free(date);

            pthread_mutex_unlock(&database_modification_mutex);
            pthread_cond_signal(&database_modification_condition_var);

            continue;
        }

        // write the (eventual) changes to the friends databases files
        for (int i = 0; i < *(arg->_nr_clients); i++) {
            client_t *client = (*(arg->_clients))[i];

            if (!client->_nr_friends)
                continue;
            fprintf(friends_database_file, "{\"username\":\"%s\",\"friends\":[\"%s\"", client->_info->_username,
                                                                                        client->_friends[0]->_info->_username);

            for (int j = 1; j < client->_nr_friends; j++) {
                fprintf(friends_database_file, ",\"%s\"", client->_friends[j]->_info->_username);
            }

            fprintf(friends_database_file, "]}\n");
        }

        fflush(friends_database_file);
        fclose(friends_database_file);

        date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Friends database maintenance finished\n", date);
        free(date);

        fflush(server_log_file);

        FILE *friend_requests_file = fopen(friend_requests_db_filename, "w");

        if (!friend_requests_file) {
            printf("Error opening database files\n");

            char *date = get_string_date();
            fprintf(server_log_file, "[%s] [ERROR] Could not open databases. Aborting.%d\n", date, *(arg->_nr_clients));
            free(date);

            pthread_mutex_unlock(&database_modification_mutex);
            pthread_cond_signal(&database_modification_condition_var);

            continue;
        }

        friend_req_t *friend_requests = *(friend_req_t **)(arg->_variable_pointers[2]);
        int nr_friend_requests = *((int *)(arg->_variable_pointers[3]));

        // write the (eventual) changes to the friend requests file
        fprintf(friend_requests_file, "sender,recipient\n");
        for (int i = 0; i < nr_friend_requests; i++) {
            fprintf(friend_requests_file, "%s,%s\n", friend_requests[i].sender->_info->_username,
                                                    friend_requests[i].recipient->_info->_username);
        }

        date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Friend requests database maintenance finished\n", date);
        free(date);

        fflush(friend_requests_file);
        fclose(friend_requests_file);

        fflush(server_log_file);

        FILE *conversations_file = fopen(conversations_db_filename, "w");

        if (!conversations_file) {
            printf("Error opening database files\n");

            char *date = get_string_date();
            fprintf(server_log_file, "[%s] [ERROR] Could not open databases. Aborting.%d\n", date, *(arg->_nr_clients));
            free(date);

            pthread_mutex_unlock(&database_modification_mutex);
            pthread_cond_signal(&database_modification_condition_var);

            continue;
        }

        // write the (eventual) changes to the conversations file
        for (int i = 0; i < *(arg->_nr_conversations); i++) {
            char *client1_string = (*(arg->_conversations))[i]->_client1->_info->_username;
            char *client2_string = (*(arg->_conversations))[i]->_client2->_info->_username;
            message_t **messages = (*(arg->_conversations))[i]->_messages;
            
            fprintf(conversations_file, "{\"%s\",\"%s\",[", client1_string, client2_string);

            for (int j = 0; j < (*(arg->_conversations))[i]->_nr_messages; j++) {
                fprintf(conversations_file, "{%s}", messages[j]->content);

                if (j < (*(arg->_conversations))[i]->_nr_messages - 1) {
                    fprintf(conversations_file, ",");
                }
            }

            fprintf(conversations_file, "]}\n");
        }

        fflush(conversations_file);
        fclose(conversations_file);

        date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Conversations database maintenance finished\n", date);
        free(date);

        fflush(server_log_file);

        date = get_string_date();
        fprintf(server_log_file, "[%s] [OK] Databases maintenance finished successfully.\n", date);
        free(date);

        fflush(server_log_file);

        fclose(server_log_file);

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);
    }

    pthread_join(pthread_self(), NULL);
    return NULL;
}
