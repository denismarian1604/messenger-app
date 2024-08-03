#include "client.h"

extern int client_status;
extern char logged_user[100];
extern int validation_key;
extern char config_file[100];
extern char IP[20];
extern int PORT;

void client_init(int *client_socket, struct sockaddr_in *client_addr) {
    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_socket < 0) {
        printf("Error creating client socket\n");
        exit(1);
    }

    memset(client_addr, 0, sizeof(struct sockaddr_in));

    (*client_addr).sin_family = AF_INET;
    (*client_addr).sin_port = htons(PORT);
    (*client_addr).sin_addr.s_addr = inet_addr(IP);
}

int connect_to_server(int *client_socket, struct sockaddr_in *client_addr) {
    if (connect(*client_socket, (struct sockaddr *)client_addr, sizeof(struct sockaddr_in)) < 0) {
        return -1;
    }

    return 0;
}

void display_client_menu(char **config_vars) {
    switch(client_status) {
        case NOT_LOGGED_IN:
            printf("1. %s\n", config_vars[9]);
            printf("2. %s\n", config_vars[10]);
            printf("3. %s\n", config_vars[11]);
            break;
        case LOGGED_IN:
            printf("1. %s\n", config_vars[12]);
            printf("2. %s\n", config_vars[13]);
            printf("3. %s\n", config_vars[14]);
            printf("4. %s\n", config_vars[15]);
            printf("5. %s\n", config_vars[16]);
            printf("6. %s\n", config_vars[17]);
            break;
        case IN_FRIEND_REQUESTS:
            printf("1. %s\n", config_vars[18]);
            printf("2. %s\n", config_vars[19]);
            printf("3. %s\n", config_vars[20]);
            break;
    }
}

int server_send(int client_socket, void *buffer, int size) {
    int sent_so_far = 0;

    while (sent_so_far < size) {
        int sent = send(client_socket, buffer + sent_so_far, size - sent_so_far, 0);
        if (sent < 0) {
            return -1;
        }

        sent_so_far += sent;
    }

    return sent_so_far;
}

int receive_from_server(int client_socket, void *buffer, int size) {
    int received_so_far = 0;

    while (received_so_far < size) {
        int received = recv(client_socket, buffer + received_so_far, size - received_so_far, 0);
        if (received < 0) {
            return -1;
        }

        received_so_far += received;
    }

    return received_so_far;
}

void register_client(int client_socket, char **config_vars) {
    char* username = (char*)malloc(100 * sizeof(char));
    char* password = (char*)malloc(100 * sizeof(char));
    char* first_name = (char*)malloc(100 * sizeof(char));
    char* last_name = (char*)malloc(100 * sizeof(char));

    write(1, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);
    write(1, config_vars[23], strlen(config_vars[23]));
    read(0, password, 100);
    write(1, config_vars[24], strlen(config_vars[24]));
    read(0, first_name, 100);
    write(1, config_vars[25], strlen(config_vars[25]));
    read(0, last_name, 100);

    username[strlen(username) - 1] = '\0';
    password[strlen(password) - 1] = '\0';
    first_name[strlen(first_name) - 1] = '\0';
    last_name[strlen(last_name) - 1] = '\0';

    time_t now;
    now = time(NULL);

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%s\n%s\n%s\n%lu\n\n", REGISTER, username, password, first_name, last_name, (unsigned long)now);

    free(username);
    free(password);
    free(first_name);
    free(last_name);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[26]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }

    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[28]);
    } else {
        printf("%s\n", config_vars[29]);
    }

    sleep(2);
}

void login_client(int client_socket, char **config_vars) {
    char* username = (char*)malloc(100 * sizeof(char));
    char* password = (char*)malloc(100 * sizeof(char));

    write(1, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);
    write(1, config_vars[23], strlen(config_vars[23]));
    read(0, password, 100);

    username[strlen(username) - 1] = '\0';
    password[strlen(password) - 1] = '\0';

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%s\n\n", LOGIN, username, password);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[30]);

        free(username);
        free(password);

        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);

        free(username);
        free(password);

        return;
    }
    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[31]);

        sleep(2);

        free(username);
        free(password);

        return;
    } else {
        printf("%s\n", config_vars[32]);
        strncpy(logged_user, username, strlen(username));
        char *token = strtok(response.content + 6, "\n");
        validation_key = atoi(token);
        client_status = LOGGED_IN;
    }

    time_t now;
    now = time(NULL);

    FILE *client_log_file = fopen("client_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(client_log_file, "[%s] [OK] Successfully logged in : %s with validation code %d.\n",
                                date, logged_user, validation_key);
    fflush(client_log_file);

    fclose(client_log_file);

    free(username);
    free(password);

    sleep(2);
}

void exit_client(int client_socket, char **config_vars) {
    printf("%s\n", config_vars[33]);

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n\n", EXIT);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[34]);
        return;
    }

    close(client_socket);
}

void logout_client(int client_socket, char **config_vars) {
    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n", LOGOUT, logged_user, validation_key);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[35]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char *token = strtok(response.content + 4, "\n");
    int val = atoi(token);

    if (val == -1) {
        printf("%s\n", config_vars[36]);

        sleep(2);

        return;
    } else {
        printf("%s\n", config_vars[37]);
        client_status = NOT_LOGGED_IN;
    }

    time_t now;
    now = time(NULL);

    FILE *client_log_file = fopen("client_log.txt", "a");

    char date[80];
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

    fprintf(client_log_file, "[%s] [OK] Successfully logged out.\n", date);
    fflush(client_log_file);

    fclose(client_log_file);

    memset(logged_user, 0, 100);
    validation_key = -1;

    sleep(2);
}

void send_friend_req(int client_socket, char **config_vars) {
    char* username = (char*)malloc(100 * sizeof(char));

    write(1, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);

    username[strlen(username) - 1] = '\0';

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n\n", SEND_FRIEND_REQ, logged_user, validation_key, username);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    free(username);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[38]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[39]);
    } else if (atoi(token) == 2) {
        printf("%s\n", config_vars[40]);
    } else if (atoi(token) == 3) {
        printf("%s\n", config_vars[41]);
    } else if (atoi(token) == 4) {
        printf("%s\n", config_vars[42]);
    } else {
        printf("%s\n", config_vars[43]);
    }

    sleep(2);
    return;
}

void view_friend_reqs(int client_socket, char **config_vars) {
    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n\n", VIEW_FRIEND_REQS, logged_user, validation_key);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[44]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char message_content_copy[MAX_MESSAGE_SIZE];
    memset(message_content_copy, 0, MAX_MESSAGE_SIZE);
    strncpy(message_content_copy, response.content, MAX_MESSAGE_SIZE);

    char *return_code = strtok(response.content + 4, "\n");

    if (atoi(return_code) == -1) {
        printf("%s\n", config_vars[39]);
    } else {
        system("clear");

        client_status = IN_FRIEND_REQUESTS;

        char *token = strtok(message_content_copy + 6, "\n");
        char *backup_token = strdup(token);

        char *sent_requests = strtok(token + 7, "}]");

        char *sent_token = strtok(sent_requests, ",\"");

        int printed = 0;

        printf("%s\n", config_vars[45]);

        while (sent_token) {
            if (sent_token[0] == '{')
                break;
            printed = 1;
            printf("%s\n", sent_token);
            sent_token = strtok(NULL, ",\"");
        }

        if (!printed) {
            printf("%s\n", config_vars[46]);
        }

        char *received_requests = strtok(strstr(backup_token, "\"from\":") + strlen("\"from\":") + 1, "[]");

        char *received_token = strtok(received_requests, ",\"}");

        printed = 0;

        printf("%s\n", config_vars[47]);

        while (received_token) {
            printed = 1;
            printf("%s\n", received_token);
            received_token = strtok(NULL, ",\"}");
        }

        if (!printed) {
            printf("%s\n", config_vars[48]);
        }

        printf("\n");
    }

    return;
}

void accept_friend_req(int client_socket, char **config_vars) {
    char* username = (char*)malloc(100 * sizeof(char));

    write(0, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);

    username[strlen(username) - 1] = '\0';

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n\n", ACCEPT_FRIEND_REQ, logged_user, validation_key, username);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    free(username);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[49]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[39]);
    } else {
        printf("%s\n", config_vars[50]);
    }

    sleep(2);

    system("clear");

    view_friend_reqs(client_socket, config_vars);

    return;
}

void decline_friend_req(int client_socket, char **config_vars) {
    char* username = (char*)malloc(100 * sizeof(char));

    write(0, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);

    username[strlen(username) - 1] = '\0';

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n\n", DECLINE_FRIEND_REQ, logged_user, validation_key, username);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    free(username);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[51]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[39]);
    } else {
        printf("%s\n", config_vars[52]);
    }

    sleep(2);

    system("clear");

    view_friend_reqs(client_socket, config_vars);

    return;
}

void view_friends(int client_socket, char **config_vars) {
    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n\n", VIEW_FRIENDS, logged_user, validation_key);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[53]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char *return_code = strtok(response.content + 4, "\n");

    if (atoi(return_code) == -1) {
        printf("%s\n", config_vars[39]);
    } else {
        system("clear");

        char *friends = strtok(response.content + 6, "\n");

        char *token = strtok(friends + 12, ",]}\"");

        int printed = 0;

        printf("%s\n", config_vars[54]);

        while (token) {
            printed = 1;
            printf("%s\n", token);
            token = strtok(NULL, ",]}\"");
        }

        if (!printed) {
            printf("%s\n", config_vars[55]);
        }

        printf("\n");
    }

    printf("%s\n", config_vars[56]);
    getchar();

    return;
}

// send message and conversation with a friend moved to a separate file

void send_message_handler(int client_socket, char **config_vars) {
    // read the username of the friend, then
    // fork the current process and launch a conversations_handler instance
    char* username = (char*)malloc(100 * sizeof(char));
    memset(username, 0, 100);

    write(1, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);
    username[strlen(username) - 1] = '\0';

    char *validation_key_string = (char *)malloc(100 * sizeof(char));
    if (!validation_key_string) {
        printf("Error allocating memory\n");
        exit(-1);
    }
    sprintf(validation_key_string, "%d", validation_key);

    char *ptr;
    char absolute_config_file[200];
    memset(absolute_config_file, 0, 200);
    printf("%s\n", config_file);
    ptr = realpath(config_file, absolute_config_file);
    printf("%s\n", absolute_config_file);

    char *home_dir = getenv("HOME");

    char *ptr_tmp;
    char absolute_tmp_file[200];
    memset(absolute_tmp_file, 0, 200);

    char *final_tmp_path = (char *)malloc(200 * sizeof(char));
    strcpy(final_tmp_path, home_dir);
    strcat(final_tmp_path, "/tmp/information.txt"); // (home_folder_user)/tmp folder REQUIRED!
    ptr_tmp = realpath(final_tmp_path, absolute_tmp_file);

    FILE *information_file = fopen(absolute_tmp_file, "w");
    fprintf(information_file, "%s\n%s\n%s\n%s\n%s\n%d\n", logged_user, validation_key_string, username, absolute_config_file, IP, PORT);
    fclose(information_file);


    // fork the current process and launch a conversations_handler instance
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/open", "open", "-a", "Terminal.app","./conversations_handler", NULL);
    }
    wait(NULL);

    // fork the current process and launch a take_message_input_from_user instance
    pid = fork();
    if (pid == 0) {
        execl("/usr/bin/open", "open", "-a", "Terminal.app","./take_message_input_from_user", NULL);
    }
    wait(NULL);

    free(validation_key_string);
    free(username);

    sleep(2);

    remove("/tmp/information.txt");

    return;
}

void parse_conversation(char *messages, char *friend, char **config_vars) {
    system("clear");

    char *client_status = strtok(messages + 6, "\n");

    int status;
    if (atoi(client_status) == -1) {
        printf("%s\n", config_vars[39]); // shouldn't reach here
        exit(-1);
    } else {
        status = atoi(client_status);
    }
    
    // display the conversation header
    for (int i = 0; i < 60; i++) {
        printf("-");
    }
    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf("|");
        if (i == 1) {
            printf(" %s %s", friend, (status == ONLINE) ? config_vars[3] : config_vars[2]);
            for (int i = 5 + strlen(friend) + ((status == ONLINE) ? 5 : 7); i < 58; i++) {
                printf(" ");
            }
        } else {
            for (int i = 0; i < 58; i++) {
                printf(" ");
            }
        }
        printf("|\n");
    }
    for (int i = 0; i < 60; i++) {
        printf("-");
    }
    printf("\n");

    char *message = strtok(NULL, "{}\n");

    if (!message) {
        printf("%s %s.\n", config_vars[57], friend);
        return;
    }
    
    while (message) {
        char sender[100];
        memset(sender, 0, 100);

        strncpy(sender, message + 8, strstr(message, "\",") - (message + 8));

        char effective_message[MAX_MESSAGE_SIZE + 1];
        memset(effective_message, 0, MAX_MESSAGE_SIZE + 1);
        
        strncpy(effective_message, message + (8 + strlen(sender) + 13), strstr(message + (8 + strlen(sender) + 13), "\",") - (message + (8 + strlen(sender) + 13)));
        
        // skip the sender and the message
        char *timestamp = message + (8 + strlen(sender) + 13) + (strlen(effective_message) + 15);
        timestamp[strlen(timestamp) - 1] = '\0';

        time_t now = atol(timestamp);
        struct tm ts;

        char date[80];
        memset(date, 0, 80);

        (void) localtime_r(&now, &ts);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);

        if (strcmp(sender, logged_user) == 0) {
            printf("%s %s %s:\n", config_vars[58], config_vars[59], date);
        } else {
            printf("%s %s %s:\n", sender, config_vars[59], date);
        }

        printf("%s\n\n", effective_message);
        
        // skip the comma
        message = strtok(NULL, "{}");
        if (!message)
            break;
        message = strtok(NULL, "{}");
    }

    getchar();
}

void view_messages_handler(int client_socket, char **config_vars) {
    // read the username of the friend, then
    // fork the current process and launch a conversations_handler instance
    char* username = (char*)malloc(100 * sizeof(char));
    memset(username, 0, 100);

    write(1, config_vars[22], strlen(config_vars[22]));
    read(0, username, 100);
    username[strlen(username) - 1] = '\0';

    char *validation_key_string = (char *)malloc(100 * sizeof(char));
    if (!validation_key_string) {
        printf("Error allocating memory\n");
        exit(-1);
    }
    sprintf(validation_key_string, "%d", validation_key);

    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, MAX_MESSAGE_SIZE);
    sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n\n", VIEW_MESSAGES, logged_user, validation_key, username);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    free(username);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        printf("%s\n", config_vars[60]);
        return;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        printf("%s\n", config_vars[27]);
        return;
    }
    char content_copy[MAX_MESSAGE_SIZE];
    memset(content_copy, 0, MAX_MESSAGE_SIZE);
    strncpy(content_copy, response.content, MAX_MESSAGE_SIZE);

    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1) {
        printf("%s\n", config_vars[39]);
        sleep(2);
        return;
    } else if (atoi(token) == -2) {
        printf("%s\n", config_vars[61]);
        printf("%s\n", config_vars[62]);
        sleep(2);
        return;
    }

    parse_conversation(response.content, username, config_vars);

    return;
}

void handle_not_logged_in(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars) {
    switch(command) {
        case REGISTER:
            register_client(client_socket, config_vars);
            break;
        case LOGIN:
            login_client(client_socket, config_vars);
            break;
        case EXIT:
            exit_client(client_socket, config_vars);
            break;
        default:
            printf("%s\n", config_vars[63]);
            break;
    }
}

void handle_logged_in(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars) {
    switch(command) {
        case SEND_MESSAGE:
            send_message_handler(client_socket, config_vars);
            break;
        case VIEW_MESSAGES:
            view_messages_handler(client_socket, config_vars);
            break;
        case SEND_FRIEND_REQ:
            send_friend_req(client_socket, config_vars);
            break;
        case VIEW_FRIEND_REQS:
            view_friend_reqs(client_socket, config_vars);
            break;
        case VIEW_FRIENDS:
            view_friends(client_socket, config_vars);
            break;
        case LOGOUT:
            logout_client(client_socket, config_vars);
            break;
        default:
            printf("%s\n", config_vars[63]);
            break;
    }
}

void handle_in_friend_requests(int client_socket, struct sockaddr_in *client_addr, int command, char **config_vars) {
    system("clear");
    view_friend_reqs(client_socket, config_vars);

    switch(command) {
        case ACCEPT_FRIEND_REQ:
            accept_friend_req(client_socket, config_vars);
            break;
        case DECLINE_FRIEND_REQ:
            decline_friend_req(client_socket, config_vars);
            break;
        case BACK_FROM_FRIEND_REQS:
            client_status = LOGGED_IN;
            break;
        default:
            printf("%s\n", config_vars[63]);
            break;
    }
}
