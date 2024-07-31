#include "server.h"

char **load_config(char *filename) {
    FILE *config_file = fopen(filename, "r");
    if (!config_file) {
        printf("Error opening file %s\n", filename);
        exit(-1);
    }

    char **config_vars = (char **)malloc(NR_VARS * sizeof(char *));

    for (int i = 0; i < NR_VARS; i++) {
        // read the line
        char *line = NULL;
        size_t len = 0;
        ssize_t read = getline(&line, &len, config_file);
        if (read == -1) {
            printf("Error reading line %d\n", i);
            exit(-1);
        }

        // remove the newline character
        line[strlen(line) - 1] = '\0';

        // allocate memory for config_vars[i]
        config_vars[i] = (char *)malloc(100 * sizeof(char));
        if (!config_vars[i]) {
            printf("Error allocating memory for config_vars[%d]\n", i);
            exit(-1);
        }

        // copy the line to the variable
        strcpy(config_vars[i], line);
    }

    FILE *config_file_out = fopen("config_file_out.txt", "w");

    for (int i = 0; i < NR_VARS; i++) {
        fprintf(config_file_out, "%s\n", config_vars[i]);
    }

    return config_vars;
}

int load_clients(client_t ***clients, int *nr_clients, char *clients_filename) {
    FILE *filename = fopen(clients_filename, "r");
    if (!filename) {
        printf("Error opening file %s\n", clients_filename);
        return -1;
    }

    // read header
    char *line = (char *)malloc(150 * sizeof(char));
    if (!line) {
        printf("Error allocating memory for line\n");
        return -1;
    }
    size_t len = 150;
    ssize_t read = getline(&line, &len, filename);
    if (read == -1) {
        printf("Error encountered while reading client info; first line\n");
        return -1;
    }

    read = getline(&line, &len, filename);
    if (read == -1) {
        printf("Error encountered while reading client info; first client\n");
    }

    int found_clients = 0;

    while (read > 0) {
        found_clients++;
        *nr_clients = found_clients;

        if (found_clients > MAX_CLIENTS) {
            // *clients = (client_t **)realloc(*clients, found_clients * sizeof(client_t *));

            // if (!*clients) {
            //     printf("Error reallocating clients\n");
            //     return -1;
            // }
            printf("Too many clients; Last client loaded : %s\n", (*clients)[found_clients - 1]->_info->_username);
            break;
        }

        (*clients)[found_clients - 1] = (client_t *)malloc(sizeof(client_t));
        if (!(*clients)[found_clients - 1]) {
            printf("Error allocating client\n");
            return -1;
        }

        (*clients)[found_clients - 1]->_client_socket = -1;
        // (*clients)[found_clients - 1]->_nr_waiting_messages = 0;
        // (*clients)[found_clients - 1]->_waiting_messages = NULL;

        (*clients)[found_clients - 1]->_info = (client_info *)malloc(sizeof(client_info));

        (*clients)[found_clients - 1]->_info->_username = (char *)malloc(100 * sizeof(char));
        (*clients)[found_clients - 1]->_info->_password = (char *)malloc(100 * sizeof(char));
        (*clients)[found_clients - 1]->_info->_first_name = (char *)malloc(100 * sizeof(char));
        (*clients)[found_clients - 1]->_info->_last_name = (char *)malloc(100 * sizeof(char));
        (*clients)[found_clients - 1]->_info->_role = (char *)malloc(100 * sizeof(char));

        (*clients)[found_clients - 1]->_info->_timestamp_register = 0;
        (*clients)[found_clients - 1]->_info->_timestamp_last_seen = 0;

        (*clients)[found_clients - 1]->_info->_status = OFFLINE;
        (*clients)[found_clients - 1]->_friends = (client_t **)malloc(100 * sizeof(client_t *));
        (*clients)[found_clients - 1]->_nr_friends = 0;
        (*clients)[found_clients - 1]->_validation_key = -1;

        (*clients)[found_clients - 1]->_info->_username = strdup(strtok(line, ","));
        (*clients)[found_clients - 1]->_info->_password = strdup(strtok(NULL, ","));
        (*clients)[found_clients - 1]->_info->_first_name = strdup(strtok(NULL, ","));
        (*clients)[found_clients - 1]->_info->_last_name = strdup(strtok(NULL, ","));
        (*clients)[found_clients - 1]->_info->_role = strdup(strtok(NULL, ","));
        (*clients)[found_clients - 1]->_info->_timestamp_register = atol(strtok(NULL, ","));
        (*clients)[found_clients - 1]->_info->_timestamp_last_seen = atol(strtok(NULL, ","));

        read = getline(&line, &len, filename);
        if (read == 0) {
            break;
        } else if (read == -1) {
            // eof
            break;
        }
    }

    fclose(filename);

    // free memory
    free(line);

    return 0;
}

client_t *find_client(client_t **clients, int nr_clients, char *username) {
    for (int i = 0; i < nr_clients; i++) {
        if (strcmp(clients[i]->_info->_username, username) == 0) {
            return clients[i];
        }
    }

    return NULL;
}

int load_friends(client_t **clients, int nr_clients, char *friends_filename) {
    FILE *filename = fopen(friends_filename, "r");
    if (!filename) {
        printf("Error opening file %s\n", friends_filename);
        return -1;
    }

    // read friend relationships
    char *line = (char *)malloc(150 * sizeof(char));
    if (!line) {
        printf("Error allocating memory for line\n");
        return -1;
    }
    size_t len = 150;
    ssize_t read;

    read = getline(&line, &len, filename);
    if (read <= 0) {
        printf("Error encountered while reading friend info; first client. Probably empty file.\n");
        return 0;
    }

    while (read > 0) {

        // extract username and friends
        char *username = strdup(strtok(line, ",{} ") + 12);
        username[strlen(username) - 1] = '\0';

        // comma is no longer delimiter
        char *friends = strdup(strtok(NULL, "{} ") + 11);
        friends[strlen(friends) - 1] = '\0';

        // find the client
        client_t *client = find_client(clients, nr_clients, username);
        if (!client) {
            printf("Client %s not found\n", username);
            continue;
        }

        // find the friends
        char *friend = strtok(friends, ",\"");
        while (friend) {
            client_t *friend_client = find_client(clients, nr_clients, friend);
            if (!friend_client) {
                printf("Friend %s not found\n", friend);
                continue;
            }

            client->_friends[client->_nr_friends++] = friend_client;

            friend = strtok(NULL, ",\"");
        }

        read = getline(&line, &len, filename);
    }

    fclose(filename);

    // free memory
    free(line);

    return 0;
}

int load_friend_requests(client_t **clients, int nr_clients, friend_req_t **friend_requests, int *nr_friend_requests) {
    FILE *filename = fopen(friend_requests_db_filename, "r");
    if (!filename) {
        printf("Error opening file %s\n", friend_requests_db_filename);
        return -1;
    }

    // read header
    char *line = (char *)malloc(150 * sizeof(char));
    if (!line) {
        printf("Error allocating memory for line\n");
        return -1;
    }
    size_t len = 150;
    ssize_t read = getline(&line, &len, filename);
    if (read == -1) {
        printf("Error encountered while reading friend requests info; first line\n");
        return -1;
    }

    read = getline(&line, &len, filename);
    if (read <= 0) {
        printf("No friend requests found\n");

        return 0;
    }

    while (read > 0) {
        // extract username and friends
        char *token = strtok(line, ",");
        char *sender = strdup(token);
        
        token = strtok(NULL, ",");
        char *recipient = strdup(token);
        // remove newline character
        if (recipient[strlen(recipient) - 1] == '\n')
            recipient[strlen(recipient) - 1] = '\0';

        client_t *sender_client = find_client(clients, nr_clients, sender);
        client_t *recipient_client = find_client(clients, nr_clients, recipient);

        (*nr_friend_requests)++;
        if (*friend_requests == NULL) {
            *friend_requests = (friend_req_t *)malloc(sizeof(friend_req_t));
        } else {
            *friend_requests = (friend_req_t *)realloc(*friend_requests, *nr_friend_requests * sizeof(friend_req_t));
        }

        (*friend_requests)[*nr_friend_requests - 1].sender = sender_client;
        (*friend_requests)[*nr_friend_requests - 1].recipient = recipient_client;

        read = getline(&line, &len, filename);
    }

    fclose(filename);

    // free memory
    free(line);

    return 0;
}

int load_conversations(client_t **clients, int nr_clients, conversation_t ***conversations, int *nr_conversations) {
     FILE *filename = fopen(conversations_db_filename, "r");
    if (!filename) {
        printf("Error opening file %s\n", conversations_db_filename);
        return -1;
    }

    // read each conversation
    char *line = (char *)malloc(3000 * sizeof(char));
    if (!line) {
        printf("Error allocating memory for line\n");
        return -1;
    }
    size_t len = 150;
    ssize_t read;

    read = getline(&line, &len, filename);
    if (read <= 0) {
        printf("Error encountered while reading conversation info; first conversation. Probably empty file.\n");
        return 0;
    }

    while (read > 0) {
        // extract the two clients involved in the conversation
        char *client1_string = strdup(strtok(line, "{,\""));
        char *client2_string = strdup(strtok(NULL, "{,\""));
        
        // find the clients references
        client_t *client1 = find_client(clients, nr_clients, client1_string);
        client_t *client2 = find_client(clients, nr_clients, client2_string);
        free(client1_string);
        free(client2_string);

        // create the conversation
        if (*conversations == NULL) {
            *conversations = (conversation_t **)malloc(sizeof(conversation_t *));
            if (!*conversations) {
                printf("Error allocating memory for conversations\n");
                return -1;
            }
        } else {
            *conversations = (conversation_t **)realloc(*conversations, (*nr_conversations + 1) * sizeof(conversation_t *));
            if (!*conversations) {
                printf("Error reallocating memory for conversations\n");
                return -1;
            }
        }

        (*conversations)[*nr_conversations] = (conversation_t *)malloc(sizeof(conversation_t));
        if (!(*conversations)[*nr_conversations]) {
            printf("Error allocating memory for conversation\n");
            return -1;
        }
        (*conversations)[*nr_conversations]->_client1 = client1;
        (*conversations)[*nr_conversations]->_client2 = client2;
        (*conversations)[*nr_conversations]->_nr_messages = 0;
        // allocate memory for the messages
        // (limited at 15. when the number of messages live exceeds 15, only the most recent 15 will be kept)
        (*conversations)[*nr_conversations]->_messages = (message_t **)malloc(15 * sizeof(message_t *));
        if (!(*conversations)[*nr_conversations]->_messages) {
            printf("Error allocating memory for messages\n");
            return -1;
        }

        // get the messages in the conversation
        char *conversation_messages = strdup(strtok(NULL, "") + 1);
        conversation_messages[strlen(conversation_messages) - 1] = '\0';

        char *conversation_message = strtok(conversation_messages, "[]{}");
    
        while (conversation_message) {
            (*conversations)[*nr_conversations]->_nr_messages++;
            (*conversations)[*nr_conversations]->_messages[(*conversations)[*nr_conversations]->_nr_messages - 1] = (message_t *)malloc(sizeof(message_t));
            if (!(*conversations)[*nr_conversations]->_messages[(*conversations)[*nr_conversations]->_nr_messages - 1]) {
                printf("Error allocating memory for message\n");
                return -1;
            }

            message_t *current_message = (*conversations)[*nr_conversations]->_messages[(*conversations)[*nr_conversations]->_nr_messages - 1];
            memset(current_message->content, 0, MAX_MESSAGE_SIZE + 1);
            strncpy(current_message->content, conversation_message, strlen(conversation_message));
            
            // skip the comma
            conversation_message = strtok(NULL, "[]{}");
            // if there is no following comma, there is no following message
            // therefore break out of the loop
            if (!conversation_message)
                break;
            // get the next message
            conversation_message = strtok(NULL, "[]{}");
        }

        // increment the number of conversations
        (*nr_conversations)++;

        read = getline(&line, &len, filename);
    }

    fclose(filename);

    // free memory
    free(line);

    return 0;
}
