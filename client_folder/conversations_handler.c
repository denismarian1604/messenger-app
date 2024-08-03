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

int get_messages_with_friend(int client_socket, char **config_vars, char *username) {
    char buffer[1025];
    memset(buffer, 0, 1025);
    sprintf(buffer, "REQ/%d\n%s\n%d\n%s\n\n", VIEW_CONVERSATION, logged_user, validation_key, username);

    message_t message;
    memset((void *)&message, 0, sizeof(message_t));
    memcpy(message.content, buffer, MAX_MESSAGE_SIZE);

    int sent_size = server_send(client_socket, buffer, MESSAGE_T_SIZE);
    if (sent_size < 0 || sent_size < strlen(buffer)) {
        return -1;
    }

    message_t response;
    memset(&response, 0, sizeof(message_t));
    int received_size = receive_from_server(client_socket, (void *)&response, MESSAGE_T_SIZE);
    if (received_size < 0) {
        return -1;
    }
    char *token = strtok(response.content + 4, "\n");

    if (atoi(token) == -1 || atoi(token) == -2) {
        return -1;
    } else {
        system("clear");

        parse_conversation(response.content, config_vars);
    }
    return 1;
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

    int rc = get_messages_with_friend(client_socket, config_vars, friend);
    if (rc == -1) {
        printf("%s %s\n", config_vars[64], friend);
    }

    while(rc == 1) {
        sleep(1);
        rc = get_messages_with_friend(client_socket, config_vars, friend);
        if (rc == -1) {
            printf("%s %s\n", config_vars[64], friend);
            break;
        }
    }
    close(client_socket);

    return 0;
}
