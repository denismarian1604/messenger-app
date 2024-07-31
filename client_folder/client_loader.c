#include "client.h"

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

    fclose(config_file);
    fclose(config_file_out);

    return config_vars;
}
