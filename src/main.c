#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

void process_input(char *input) {
    char *commands[MAX_ARGS];
    int pipeline_count;

    fulfil_history_file(input);
    expand_abbreviations_in_input(input);

    // Split input into pipeline segments
    parse_pipeline(input, commands, &pipeline_count);

    if (pipeline_count > 1) {
        // If there are multiple commands separated by '|', handle as a pipeline
        execute_pipeline(commands, pipeline_count);
    } else {
        // Handle single commands (no pipeline)
        char *args[MAX_ARGS];
        parse_input(input, args);

        struct Command GoGi_commands[] = {
            {"sethome", sethome},
            {"history", history},
            {"home", home},
            {"setabbr", setabbr},
            {"abbr", abbr},
            {"help", help},
            {"ldir", ldir}
        };

        if (args[0] == NULL) {
            printf("No command provided.\n");
            return;
        }

        size_t num_gogi_commands = sizeof(GoGi_commands) / sizeof(GoGi_commands[0]);

        // Check if the command is a custom GoGiShell command
        for (size_t i = 0; i < num_gogi_commands; i++) {
            if (strcmp(args[0], GoGi_commands[i].command) == 0) {
                GoGi_commands[i].function(args);
                return;
            }
        }

        // Handle `cd` command
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                chdir(home_dir);
                cwd_changed = 1;
            } else {
                print_directory_description(args[1]);

                if (chdir(args[1]) == -1) {
                    perror("BASH command cd failed");
                } else {
                    cwd_changed = 1;
                }
            }
        } else {
            // External command execution
            pid_t pid = fork();
            if (pid == 0) {
                // Handle redirection for this single command
                handle_redirection(args);

                if (execvp(args[0], args) == -1) {
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
    }
}

void parse_pipeline(char *input, char *commands[], int *num_commands) {
    char *token;
    *num_commands = 0;

    token = strtok(input, "|");
    while (token != NULL && *num_commands < MAX_ARGS - 1) {
        commands[(*num_commands)++] = token;
        token = strtok(NULL, "|");
    }
    commands[*num_commands] = NULL;
}

void execute_pipeline(char *commands[], int num_commands) {
    int pipe_fds[2];
    pid_t pid;
    int fd_in = 0; // Input for the first command is STDIN

    for (int i = 0; i < num_commands; i++) {
        pipe(pipe_fds);

        pid = fork();
        if (pid == 0) {
            dup2(fd_in, STDIN_FILENO); // Set input to fd_in
            if (i < num_commands - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO); // Redirect output to the pipe
            }
            close(pipe_fds[0]);
            close(pipe_fds[1]);

            // Parse and handle redirection for the current command
            char *args[MAX_ARGS];
            parse_input(commands[i], args);
            handle_redirection(args);

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror("Pipeline command failed");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            wait(NULL); // Wait for the child process
            close(pipe_fds[1]);
            fd_in = pipe_fds[0]; // Set the input for the next command
        } else {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }
}

void handle_redirection(char *args[]) {
    int i = 0;

    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            // Output redirection
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd == -1) {
                perror("Failed to open file for output redirection");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
            close(fd);
            args[i] = NULL; // End the argument list before the redirection symbol
            break;
        } else if (strcmp(args[i], ">>") == 0) {
            // Append output redirection
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd == -1) {
                perror("Failed to open file for append output redirection");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
            close(fd);
            args[i] = NULL; // End the argument list before the redirection symbol
            break;
        } else if (strcmp(args[i], "<") == 0) {
            // Input redirection
            int fd = open(args[i + 1], O_RDONLY);
            if (fd == -1) {
                perror("Failed to open file for input redirection");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO); // Redirect stdin to the file
            close(fd);
            args[i] = NULL; // End the argument list before the redirection symbol
            break;
        }
        i++;
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
    // If the current directory is inside the home directory, shorten it with '~'
    if (strncmp(cwd, home_dir, strlen(home_dir)) == 0) {
        snprintf(display_cwd, MAX_PATH_LENGTH, "~%s", cwd + strlen(home_dir));
    } else {
        strncpy(display_cwd, cwd, MAX_PATH_LENGTH);
        display_cwd[MAX_PATH_LENGTH - 1] = '\0';
    }

    // Get color for the current directory
    int color_code = get_color_for_directory(cwd);
    if (color_code != -1) {
        // Apply color if a valid color code is returned
        char colored_prompt[MAX_PATH_LENGTH];
        snprintf(colored_prompt, MAX_PATH_LENGTH, "\033[%dm%s\033[0m", color_code, display_cwd);
        strncpy(display_cwd, colored_prompt, MAX_PATH_LENGTH);
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
