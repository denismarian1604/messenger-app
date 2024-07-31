#include "server.h"

void view_conversation(thread_arg_t *arg, message_t message) {
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

        fprintf(server_log_file, "[%s] [WARN] Failed conversation review attempt from inexistent username : %s.\n", date, username);
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

        fprintf(server_log_file, "[%s] [CRITICAL] Failed conversation review attempt from username %s with mistaken validation key. Possible data breach.\n",
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
        send_message_to_client(arg->_client_socket, "RES/-1\nYou cannot send a message to yourself.\n\n");

        return;
    }

    int found = 0;
    // check if the users are friends
    for (int i = 0; i < client->_nr_friends; i++) {
        if (strncmp(client->_friends[i]->_info->_username, recipient->_info->_username, strlen(recipient->_info->_username)) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        send_message_to_client(arg->_client_socket, "RES/-2\nYou are not friends with the recipient.\n\n");

        return;
    }

    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE + 1);

    strcpy(buffer, "RES/1\n");
    strcat(buffer, (recipient->_info->_status == ONLINE) ? "16" : "17");
    strcat(buffer, "\n");

    conversation_t **conversations = *(arg->_conversations);
    int nr_conversations = *(arg->_nr_conversations);
    int found_conv = 0;

    for (int i = 0; i < nr_conversations; i++) {
        if ((strcmp(conversations[i]->_client1->_info->_username, client->_info->_username) == 0 &&
            strcmp(conversations[i]->_client2->_info->_username, recipient->_info->_username) == 0) ||
            (strcmp(conversations[i]->_client1->_info->_username, recipient->_info->_username) == 0 &&
            strcmp(conversations[i]->_client2->_info->_username, client->_info->_username) == 0)) {

            found_conv = 1;
            
            for (int j = 0; j < conversations[i]->_nr_messages; j++) {
                strcat(buffer, "{");
                strcat(buffer, conversations[i]->_messages[j]->content);
                strcat(buffer, "}");

                if (j < conversations[i]->_nr_messages - 1)
                    strcat(buffer, ",");
            }

            break;
        }
    }

    if (!found_conv) {
        strcat(buffer, "{}");
    }

    strcat(buffer, "\n\n");

    message_t response;
    memset(&response, 0, sizeof(message_t));
    memcpy(response.content, buffer, MAX_MESSAGE_SIZE + 1);

    int sent_size = send_to_client(arg->_client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (sent_size < 0) {
        printf("Error sending response to client\n");
        return;
    }
}

void send_message(thread_arg_t *arg, message_t message) {
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

        fprintf(server_log_file, "[%s] [WARN] Failed message sending attempt from inexistent username : %s.\n", date, username);
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

        fprintf(server_log_file, "[%s] [CRITICAL] Failed message sending attempt from username %s with mistaken validation key. Possible data breach.\n",
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
        send_message_to_client(arg->_client_socket, "RES/-1\nYou cannot send a message to yourself.\n\n");

        return;
    }

    int found = 0;
    // check if the users are friends
    for (int i = 0; i < client->_nr_friends; i++) {
        if (strncmp(client->_friends[i]->_info->_username, recipient->_info->_username, strlen(recipient->_info->_username)) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        send_message_to_client(arg->_client_socket, "RES/-2\nYou are not friends with the recipient.\n\n");

        return;
    }

    char *timestamp_string = strdup(strtok(NULL, "\n"));
    int timestamp = atoi(timestamp_string);

    char *sent_message = strtok(NULL, "\n");

    conversation_t ***conversations = arg->_conversations;
    int *nr_conversations = arg->_nr_conversations;

    // check if the converstion between the two users, client and recipient, exists
    int found_conversation = 0;
    for (int i = 0; i < *nr_conversations; i++) {
        if ((strcmp((*conversations)[i]->_client1->_info->_username, client->_info->_username) == 0 &&
            strcmp((*conversations)[i]->_client2->_info->_username, recipient->_info->_username) == 0) ||
            (strcmp((*conversations)[i]->_client1->_info->_username, recipient->_info->_username) == 0 &&
            strcmp((*conversations)[i]->_client2->_info->_username, client->_info->_username) == 0)) {
            
            found_conversation = 1;
            message_t *new_message = (message_t *)malloc(sizeof(message_t));
            if (!new_message) {
                send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

                return;
            }

            memset(new_message, 0, sizeof(message_t));
            sprintf(new_message->content, "\"from\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%d\"", client->_info->_username, sent_message, timestamp);

            // if the nr of messages exceeds the limit(15), delete the oldest message
            if ((*conversations)[i]->_nr_messages == 15) {
                free((*conversations)[i]->_messages[0]);
                for (int j = 0; j < (*conversations)[i]->_nr_messages - 1; j++) {
                    (*conversations)[i]->_messages[j] = (*conversations)[i]->_messages[j + 1];
                }

                (*conversations)[i]->_nr_messages--;
            }

            pthread_mutex_lock(&database_modification_mutex);

            (*conversations)[i]->_messages = (message_t **)realloc((*conversations)[i]->_messages, ((*conversations)[i]->_nr_messages + 1) * sizeof(message_t *));
            if (!(*conversations)[i]->_messages) {
                send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

                return;
            }

            (*conversations)[i]->_messages[(*conversations)[i]->_nr_messages] = new_message;
            (*conversations)[i]->_nr_messages++;

            pthread_mutex_unlock(&database_modification_mutex);
            pthread_cond_signal(&database_modification_condition_var);

            found_conversation = 1;
            break;
        }
    }

    // if the conversation does not exist, create it
    if (!found_conversation) {
        conversation_t *new_conversation = (conversation_t *)malloc(sizeof(conversation_t));
        if (!new_conversation) {
            send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

            return;
        }

        memset(new_conversation, 0, sizeof(conversation_t));
        new_conversation->_client1 = client;
        new_conversation->_client2 = recipient;

        message_t *new_message = (message_t *)malloc(sizeof(message_t));
        if (!new_message) {
            send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

            return;
        }

        memset(new_message, 0, sizeof(message_t));
        sprintf(new_message->content, "\"from\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%d\"", client->_info->_username, sent_message, timestamp);

        new_conversation->_messages = (message_t **)malloc(sizeof(message_t *));
        if (!new_conversation->_messages) {
            send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

            return;
        }

        new_conversation->_messages[0] = new_message;
        new_conversation->_nr_messages = 1;

        pthread_mutex_lock(&database_modification_mutex);

        *nr_conversations += 1;
        *conversations = (conversation_t **)realloc(*conversations, *nr_conversations * sizeof(conversation_t *));
        if (!*conversations) {
            send_message_to_client(arg->_client_socket, "RES/-1\nAn error has been encountered.\n\n");

            return;
        }

        (*conversations)[*nr_conversations - 1] = new_conversation;

        pthread_mutex_unlock(&database_modification_mutex);
        pthread_cond_signal(&database_modification_condition_var);
    }

    send_message_to_client(arg->_client_socket, "RES/1\nMessage sent successfully.\n\n");

    return;
}
