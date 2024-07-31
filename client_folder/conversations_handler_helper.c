#include "conversations_handler.h"

char **load_config_client(char *filename) {
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

        // allocate memory for the variable
        config_vars[i] = (char *)malloc((strlen(line) + 1) * sizeof(char));
        if (!config_vars[i]) {
            printf("Error allocating memory for variable %d\n", i);
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
