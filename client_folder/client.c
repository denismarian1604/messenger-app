#include "client.h"

int client_status = NOT_LOGGED_IN;
char logged_user[100];
int validation_key = -1;
char config_file[100];
int client_socket;

static void sigint_handler(int signum) {
    close(client_socket);
    printf("Caught signal %d\n", signum);
    exit(0);
}

int main(int argc, char *argv[]) {
    // clear the screen beforehand
    system("clear");

    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = sigint_handler;

    if (argc != 2) {
        printf("Usage: ./client <desired_language>\nCurrently supported languages: ro, en\n");
        exit(-1);
    }

    memset(config_file, 0, 100);

    sprintf(config_file, "../configs/client_config_%s.txt", argv[1]);

    FILE *client_log_file = fopen("client_log.txt", "w");
    if (!client_log_file) {
        printf("Error opening client log file\n");
        exit(-1);
    }

    fprintf(client_log_file, "Client %s log\n", VERSION);

    char **config_vars = load_config_client(config_file);

    time_t now;
    struct tm ts;
    now = time(NULL);

    time(&now);
    char date[80];

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(client_log_file, "[%s] Finished loading config variables.\n Config vars:\n", date);
    for (int i = 0; i < NR_VARS; i++) {
        fprintf(client_log_file, "%s\n", config_vars[i]);
    }

    struct sockaddr_in client_addr;

    client_init(&client_socket, &client_addr);
    int rc = connect_to_server(&client_socket, &client_addr);
    if (rc < 0) {
        printf("Error connecting to server\n");
        exit(1);
    }

    ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", &ts);
    fprintf(client_log_file, "[%s] Established connection to server.\n", date);

    fflush(client_log_file);

    int command;

    do {
        display_client_menu(config_vars);
        printf("Enter desired action: ");
        scanf("%d", &command);

        if (client_status == LOGGED_IN)
            command += 3;
        else if(client_status == IN_FRIEND_REQUESTS)
            command += 9;

        switch(client_status) {
            case NOT_LOGGED_IN:
                handle_not_logged_in(client_socket, &client_addr, command, config_vars);
                break;
            case LOGGED_IN:
                handle_logged_in(client_socket, &client_addr, command, config_vars);
                break;
            case IN_FRIEND_REQUESTS:
                handle_in_friend_requests(client_socket, &client_addr, command, config_vars);
                break;
        }

        getchar();
        if (command != EXIT && client_status != IN_FRIEND_REQUESTS)
            system("clear");
    } while(command != EXIT);

    fclose(client_log_file);
    close(client_socket);
    
    // free the config variables
    for (int i = 0; i < NR_VARS; i++) {
        free(config_vars[i]);
    }
    free(config_vars);

    return 0;
}