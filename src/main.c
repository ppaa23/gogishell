#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "headers.h"

char home_dir[MAX_PATH_LENGTH];
int cwd_changed = 1;
int total_commands = 0;
int total_abbreviations = 0;


void expand_abbreviations_in_input(char *input) {
    char expanded[MAX_INPUT_LENGTH] = {0};
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int i = 0, j = 0;

    // Open the file containing abbreviations
    FILE *file = fopen(abbreviation_file, "r");
    if (!file) {
        perror("Failed to open .abbreviation");
        return;
    }

    // Read each line from the abbreviation file
    char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 1];
    while (fgets(line, sizeof(line), file)) {
        char *colon_pos = strchr(line, ':');
        if (colon_pos) {
            *colon_pos = '\0'; // Null-terminate key and value
            strncpy(key, line, MAX_KEY_LENGTH - 1);
            key[MAX_KEY_LENGTH - 1] = '\0'; // Ensure null-termination
            strncpy(value, colon_pos + 1, MAX_VALUE_LENGTH - 1);
            value[MAX_VALUE_LENGTH - 1] = '\0'; // Ensure null-termination

            // Remove newline character from value
            value[strcspn(value, "\n")] = '\0';

            // Process input to expand the abbreviation
            i = 0; j = 0; // Reset indices for processing
            while (input[i] != '\0' && j < MAX_INPUT_LENGTH - 1) {
                if (strncmp(&input[i], key, strlen(key)) == 0) {
                    // Match found; check if the expanded output can fit
                    int value_len = strlen(value);
                    if (j + value_len < MAX_INPUT_LENGTH - 1) {
                        strcpy(&expanded[j], value);
                        j += value_len;    // Move the output index
                        i += strlen(key);  // Move the input index past the key
                    } else {
                        // Truncate and safely null-terminate if value is too long
                        perror("Expanded value exceeds maximum input length");
                        expanded[MAX_INPUT_LENGTH - 1] = '\0';
                        break;
                    }
                } else {
                    // No match; copy the current character if space permits
                    if (j < MAX_INPUT_LENGTH - 1) {
                        expanded[j++] = input[i++];
                    } else {
                        perror("Expanded output exceeds maximum input length");
                        expanded[MAX_INPUT_LENGTH - 1] = '\0';
                        break;
                    }
                }
            }

            // Null-terminate the expanded string
            expanded[j] = '\0';

            // Copy expanded result back into input for further abbreviation checks
            strncpy(input, expanded, MAX_INPUT_LENGTH - 1);
            input[MAX_INPUT_LENGTH - 1] = '\0'; // Ensure null-termination
        }
    }

    fclose(file); // Close the file
}

void parse_input(char *input, char *args[]) {
    char *token;
    int i = 0;

    token = strtok(input, " \n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL;
}

void execute_command(char *cmd, char *args[]) {
    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(cmd, args) == -1) {
            perror("No such internal or GoGiShell command");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Internal function waitpid failed");
        }
    } else {
        perror("Internal function fork failed");
        exit(EXIT_FAILURE);
    }
}

void process_input(char *input) {
    char *args[MAX_ARGS];
    fulfil_history_file(input);
    expand_abbreviations_in_input(input);
    parse_input(input, args);

    struct Command GoGi_commands[] = {
        {"sethome", sethome},
        {"history", history},
        {"home", home},
        {"setabbr", setabbr},
        {"abbr", abbr},
        {"help", help}
    };

    if (args[0] == NULL) {
        printf("No command provided.\n");
        return;
    }

    size_t num_commands = sizeof(GoGi_commands) / sizeof(GoGi_commands[0]);

    for (size_t i = 0; i < num_commands; i++) {
        if (strcmp(args[0], GoGi_commands[i].command) == 0) {
            GoGi_commands[i].function(args);
            return;
        }
    }
    
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(home_dir);
            cwd_changed = 1;
        } else {
            if (chdir(args[1]) == -1) {
                perror("BASH command cd failed");
            } else {
                cwd_changed = 1;
            }
        }
    } else {
        execute_command(args[0], args);
    }
}

void handle_up_arrow(char *input, int *command_index, int *i) {
    if (*command_index > 1) {
        while (*i > 0) {
            printf("\b \b");
            (*i)--;
        }

        (*command_index)--;
        char *command = get_command_from_history(*command_index);

        strcpy(input, command);
        printf("%s", command);
        *i = strlen(input);

        free(command);
    }
}

void handle_down_arrow(char *input, int *command_index, int *i) {
    if (*command_index < total_commands) {
        while (*i > 0) {
            printf("\b \b");
            (*i)--;
        }

        (*command_index)++;
        char *command = get_command_from_history(*command_index);

        strcpy(input, command);
        printf("%s", command);
        *i = strlen(input);

        free(command);
    } else if (*command_index == total_commands) {
        while (*i > 0) {
            printf("\b \b");
            (*i)--;
        }
        (*command_index)++;
    }
}

void handle_tab(char *input, int *i) {
    input[*i] = '\0';
    char *suggestion = get_most_used_command(input);

    if (suggestion) {
        while (*i > 0) {
            printf("\b \b");
            (*i)--;
        }

        strcpy(input, suggestion);
        printf("%s", suggestion);
        *i = strlen(suggestion);

        free(suggestion);
    }
}

void get_prompt(char *cwd, char *home_dir, char *display_cwd) {
    if (strncmp(cwd, home_dir, strlen(home_dir)) == 0) {
        snprintf(display_cwd, MAX_PATH_LENGTH, "~%s", cwd + strlen(home_dir));
    } else {
        strncpy(display_cwd, cwd, MAX_PATH_LENGTH);
        display_cwd[MAX_PATH_LENGTH - 1] = '\0';
    }
}

int main() {
    char input[MAX_INPUT_LENGTH];
    char cwd[MAX_PATH_LENGTH];
    char display_cwd[MAX_PATH_LENGTH];
    struct termios original_termios;
    int i, ch, command_index = 0;

    printf("Welcome to GoGiShell!\n");
    printf("Please read the GoGiShell manual by printing 'help'\n");
    printf("\n");

    initialize_paths();

    create_cache();

    strncpy(home_dir, getenv("HOME"), MAX_PATH_LENGTH - 1);
    home_dir[MAX_PATH_LENGTH - 1] = '\0';
    fulfil_home_path_file(home_dir);
    get_home_dir();

    fulfil_history_file("");
    get_total_commands();

    fulfil_abbreviation_file(home_dir, "~");
    get_total_abbreviations();

    enable_noncanonical_mode(&original_termios);

    while (1) {
        if (cwd_changed) {
            get_home_dir();
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                perror("Internal function getcwd failed");
                continue;
            }

            get_prompt(cwd, home_dir, display_cwd);

            cwd_changed = 0;
        }

        printf("\033[1;34mGoGiShell:\033[37m%s$ ", display_cwd);
        fflush(stdout);

        memset(input, 0, MAX_INPUT_LENGTH);
        i = 0;
        command_index = total_commands + 1;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            if (ch == 27) { // Escape character (ASCII 27)
                if ((ch = getchar()) == '[') {
                    ch = getchar();
                    if (ch == 'A') { // UP Arrow
                        handle_up_arrow(input, &command_index, &i);
                    } else if (ch == 'B') { // DOWN Arrow
                        handle_down_arrow(input, &command_index, &i);
                    }
                }
            } else if ((ch == 8) || (ch == 127)) {  // Backspace (ASCII 8 или 127)
                if (i > 0) {
                    printf("\b \b");
                    i--;
                }
            } else if (ch == '\t') {  // TAB
                handle_tab(input, &i);
            } else {
                input[i++] = ch;
                putchar(ch);
            }
        }

        putchar('\n');
        input[i] = '\n';
        i++;
        input[i] = '\0';

        if (strcmp(input, "\n") == 0) {
            continue;
        }

        if (strcmp(input, "exit\n") == 0) {
            break;
        }

        process_input(input);
    }

    disable_noncanonical_mode(&original_termios);

    printf("Thank you for using GoGiShell!\n");
    return 0;
}
