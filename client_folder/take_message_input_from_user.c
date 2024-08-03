#include "conversations_handler.h"

char logged_user[200];
int validation_key;
char friend[200];
char config_file[200];
int client_socket;
char IP[20];
int PORT;

static void sigint_handler(int signum) {
    close(client_socket);
    printf("Caught signal %d\n", signum);
    exit(0);
}

int main() {
    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = sigint_handler;

    char validation_key_string[100];

    system("clear");

    char *home_dir = getenv("HOME");

    char *ptr_tmp;
    char absolute_tmp_file[200];
    memset(absolute_tmp_file, 0, 200);
    ptr_tmp = realpath(strcat(home_dir,"/tmp/information.txt"), absolute_tmp_file);

    FILE *information_file = fopen(absolute_tmp_file, "r");
    if (!information_file) {
        printf("Error opening information file\n");
        exit(-1);
    }

    char *line = (char *)malloc(200 * sizeof(char));
    if (!line) {
        printf("Error allocating memory\n");
        exit(-1);
    }
    size_t line_len = 40;

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    strncpy(logged_user, line, strlen(line));
    logged_user[strlen(logged_user) - 1] = '\0';

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    strncpy(validation_key_string, line, strlen(line));
    validation_key_string[strlen(validation_key_string) - 1] = '\0';
    validation_key = atoi(validation_key_string);

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    strncpy(friend, line, strlen(line));
    friend[strlen(friend) - 1] = '\0';

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    strncpy(config_file, line, strlen(line));
    config_file[strlen(config_file) - 1] = '\0';

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    strncpy(IP, line, strlen(line));
    IP[strlen(IP) - 1] = '\0';

    memset(line, 0, 200);
    getline(&line, &line_len, information_file);
    PORT = atoi(line);

    free(line);

    char **config_vars = load_config_client(config_file);
    if (!config_vars) {
        printf("Error loading config variables\n");
        exit(-1);
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in client_addr;

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(PORT);
    client_addr.sin_addr.s_addr = inet_addr(IP);

    // launch another connection to the server
    if (connect(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        printf("Error connecting to server\n");
        exit(-1);
    }

    char *message_to_send = (char *)malloc(MAX_MESSAGE_SIZE * sizeof(char));
    if (!message_to_send) {
        close(client_socket);
        printf("Error allocating memory\n");
        exit(-1);
    }

    while (1) {
        system("clear");

        write(1, config_vars[65], strlen(config_vars[65]));

        memset(message_to_send, 0, MAX_MESSAGE_SIZE);
        read(0, message_to_send, MAX_MESSAGE_SIZE);

        time_t now;
        now = time(NULL);

        time(&now);

        char buffer[MAX_MESSAGE_SIZE];
        memset(buffer, 0, MAX_MESSAGE_SIZE);
        sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n%ld\n%s\n\n", SEND_MESSAGE, logged_user, validation_key, friend, now, message_to_send);

        message_t message;
        memset((void *)&message, 0, sizeof(message_t));
        memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

        int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
        if (sent_size < 0 || sent_size < strlen(buffer)) {
            printf("%s\n", config_vars[66]);
            return -1;
        }

        message_t response;
        memset(&response, 0, sizeof(message_t));
        int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
        if (received_size < 0) {
            printf("%s\n", config_vars[27]);
            return 0;
        }
        char *token = strtok(response.content + 4, "\n");

        if (atoi(token) == -1) {
            printf("%s\n", config_vars[39]);
            continue;
        } else if (atoi(token) == -2) {
            printf("%s\n", config_vars[61]);
            printf("%s\n", config_vars[62]);
            continue;
        }

        sleep(2);
    }

    close(client_socket);

    return 0;
}