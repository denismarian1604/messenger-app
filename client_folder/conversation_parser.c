#include "conversations_handler.h"

extern char logged_user[200];
extern char friend[200];

void parse_conversation(char *messages, char **config_vars) {
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
}
