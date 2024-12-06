#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>

#include "headers.h"

char cache_dir[MAX_PATH_LENGTH];
char home_path_file[MAX_PATH_LENGTH];
char history_file[MAX_PATH_LENGTH];
char abbreviation_file[MAX_PATH_LENGTH];
char sorted_history_file[MAX_PATH_LENGTH];


void initialize_paths() {
    // Get the home directory from the environment variable
    const char *system_home_path = getenv("HOME");
    if (system_home_path == NULL) {
        perror("HOME environment variable not set");
        exit(EXIT_FAILURE);
    }

    snprintf(cache_dir, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_CACHE_DIR);
    snprintf(home_path_file, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_HOME_PATH_FILE);
    snprintf(history_file, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_HISTORY_FILE);
    snprintf(abbreviation_file, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_ABBREVIATION_FILE);
    snprintf(sorted_history_file, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_SORTED_HISTORY_FILE);
}

void create_cache() {
    struct stat st;
    if (stat(cache_dir, &st) == -1) {
        if (mkdir(cache_dir, 0700) == -1) {
            perror("Failed to create .gogicache directory");
        }
    }
}

void fulfil_home_path_file(const char *home_dir) {
    FILE *file = fopen(home_path_file, "w");
    if (file == NULL) {
        perror("Failed to create or open .home_path");
        return;
    }

    if (fprintf(file, "%s\n", home_dir) < 0) {
        perror("Failed to write to .home_path");
    }

    fclose(file);
    printf("Home directory is recorded and set as: %s\n", home_dir);
    get_home_dir();
}

void fulfil_history_file(char *input) {
    FILE *file = fopen(history_file, "a");
    if (file == NULL) {
        perror("Failed to create or open .history");
        return;
    }

    if (strcmp(input, "\n") == 0) {
        // Do not write empty input to history
        return;
    }

    // Write the input command to the file followed by a newline
    if (fprintf(file, "%s", input) < 0) {
        perror("Failed to write to .history");
    } else {
        total_commands++;
        fulfil_sorted_history_file(input);
    }

    fclose(file);
}

void fulfil_abbreviation_file(char *value, char *key) {
    FILE *file = fopen(abbreviation_file, "r");
    char lines[MAX_ABBREVIATIONS][MAX_INPUT_LENGTH];  // Array to hold file lines
    int line_count = 0;
    int found = 0;

    if (file == NULL) {
        // If the file doesn't exist, create it and add the new line
        file = fopen(abbreviation_file, "w");
        if (file == NULL) {
            perror("Failed to create .abbreviation");
            return;
        }
        fprintf(file, "%s:%s\n", key, value);
        fclose(file);
        printf("Abbreviation '%s' as '%s' recorded in a new file.\n", value, key);
        return;
    }

    // Read all lines into memory
    while (fgets(lines[line_count], MAX_INPUT_LENGTH, file) && line_count < MAX_ABBREVIATIONS) {
        char current_key[MAX_INPUT_LENGTH], current_value[MAX_INPUT_LENGTH];

        // Extract key and value
        if (sscanf(lines[line_count], "%[^:]:%s", current_key, current_value) == 2) {
            // If the key matches, replace the line
            if (strcmp(current_key, key) == 0) {
                snprintf(lines[line_count], MAX_INPUT_LENGTH, "%s:%s\n", key, value);
                found = 1;
            }
        }
        line_count++;
    }
    fclose(file);

    // If the key was not found, add a new line
    if (!found) {
        if (line_count < MAX_ABBREVIATIONS) {
            snprintf(lines[line_count++], MAX_INPUT_LENGTH, "%s:%s\n", key, value);
            printf("Abbreviation '%s' as '%s' added.\n", value, key);
            total_abbreviations++;
        } else {
            printf("Error: File contains too many lines. Cannot add abbreviation.\n");
            return;
        }
    } else {
        printf("Abbreviation for '%s' updated to '%s'.\n", key, value);
    }

    // Write all lines back to the file
    file = fopen(abbreviation_file, "w");
    if (file == NULL) {
        perror("Failed to open abbreviation file for writing");
        return;
    }
    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], file);
    }
    fclose(file);
    printf("Abbreviation file updated successfully.\n");
}

// Comparator function for sorting in descending order of usage count
int compare(const void *a, const void *b) {
    int count_a = ((int *)a)[0];
    int count_b = ((int *)b)[0];
    return count_b - count_a; // Descending order
}

void fulfil_sorted_history_file(char *input) {
    FILE *file = fopen(sorted_history_file, "r+"); // Open for reading and writing
    if (file == NULL) {
        file = fopen(sorted_history_file, "w+");
        if (file == NULL) {
            perror("Failed to create or open sorted_history_file");
            return;
        }
    }

    char lines[MAX_COMMAND_NUMBER][MAX_INPUT_LENGTH]; // Temporary storage for input strings
    int usage_counts[MAX_COMMAND_NUMBER] = {0};       // Corresponding usage counts
    int line_count = 0;                               // Number of lines in the file
    int found = 0;                                    // Flag to indicate if input is found

    // Read lines from file and parse them
    char buffer[MAX_INPUT_LENGTH];
    while (fgets(buffer, MAX_INPUT_LENGTH, file) != NULL) {
        int usage;
        char command[MAX_INPUT_LENGTH];

        // Parse usage count and command
        if (sscanf(buffer, "%d %[^\n]", &usage, command) == 2) {
            // Remove trailing newline from the input command (if present)
            command[strcspn(command, "\n")] = '\0';

            // Check if the input command matches
            int is_duplicate = 0;
            for (int i = 0; i < line_count; i++) {
                if (strcmp(lines[i], command) == 0) {
                    usage_counts[i] += usage; // Consolidate usage count
                    if (strcmp(command, input) == 0) {
                        usage_counts[i]++; // Increment for the input command
                        found = 1;
                    }
                    is_duplicate = 1;
                    break;
                }
            }

            // Add the command to the list if it's not a duplicate
            if (!is_duplicate) {
                strcpy(lines[line_count], command);
                usage_counts[line_count] = usage;
                if (strcmp(command, input) == 0) {
                    usage_counts[line_count]++;
                    found = 1;
                }
                line_count++;
            }
        }
    }

    // If input is not found, add it
    if (!found && line_count < MAX_COMMAND_NUMBER) {
        strcpy(lines[line_count], input);
        usage_counts[line_count] = 1;
        line_count++;
    }

    // Combine usage counts and corresponding commands into a sortable array
    int combined[line_count][2]; // [0] = usage count, [1] = line index
    for (int i = 0; i < line_count; i++) {
        combined[i][0] = usage_counts[i];
        combined[i][1] = i;
    }

    // Sort in descending order of usage counts
    qsort(combined, line_count, sizeof(combined[0]), compare);

    // Close the file and reopen for writing
    fclose(file);
    file = fopen(sorted_history_file, "w");
    if (file == NULL) {
        perror("Failed to reopen .sorted_history_file for writing");
        return;
    }

    // Write sorted content to the file
    for (int i = 0; i < line_count; i++) {
        int index = combined[i][1];
        fprintf(file, "%d %s\n", combined[i][0], lines[index]);
    }

    fclose(file);
}

void enable_noncanonical_mode(struct termios *original_termios) {
    struct termios new_termios;

    // Get current terminal settings
    if (tcgetattr(STDIN_FILENO, original_termios) == -1) {
        perror("Internal function tcgetattr failed");
        exit(EXIT_FAILURE);
    }

    // Copy the current settings to modify them
    new_termios = *original_termios;

    // Disable canonical mode and echoing
    new_termios.c_lflag &= ~(ICANON | ECHO);

    // Apply new settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1) {
        perror("Internal function tcsetattr failed");
        exit(EXIT_FAILURE);
    }
}

void disable_noncanonical_mode(struct termios *original_termios) {
    // Restore the original terminal settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, original_termios) == -1) {
        perror("Internal function tcsetattr failed");
        exit(EXIT_FAILURE);
    }
}

void get_home_dir() {
    FILE *file = fopen(home_path_file, "r");
    if (file == NULL) {
        perror("Failed to open .home_path");
        return;
    }
    if (fgets(home_dir, sizeof(home_dir), file) == NULL) {
        perror("Failed to read home directory from .home_path");
    }
    fclose(file);
    // Remove trailing newline if present
    home_dir[strcspn(home_dir, "\n")] = '\0';
}

void get_total_commands() {
    FILE *file = fopen(history_file, "r");
    if (file == NULL) {
        perror("Failed to open .history");
        return;
    }
    int line_count = 0;
    char ch;

    // Read the file character by character
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            line_count++;  // Increment the line count on every newline character
        }
    }

    fclose(file);
    total_commands = line_count;
}

void get_total_abbreviations() {
    FILE *file = fopen(abbreviation_file, "r");
    if (file == NULL) {
        perror("Failed to open .abbreviation");
        return;
    }
    int line_count = 0;
    char ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            line_count++;
        }
    }

    fclose(file);
    total_abbreviations = line_count;
}

char* get_command_from_history(int command_index) {
    FILE *file = fopen(history_file, "r");
    if (file == NULL) {
        perror("Failed to open .history");
        return NULL;
    }
    char *line = malloc(MAX_INPUT_LENGTH);
    if (line == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    int current_line = 1;
    while (fgets(line, MAX_INPUT_LENGTH, file)) {
        if (current_line == command_index) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            fclose(file);
            return line;  // Return the found line (caller must free it)
        }
        current_line++;
    }

    // If line_number is beyond the number of lines in the file
    free(line);
    fclose(file);
    return NULL;
}

char* get_most_used_command(char *input) {
    FILE *file = fopen(sorted_history_file, "r"); // Open the file for reading
    if (file == NULL) {
        perror("Failed to open .sorted_history_file");
        return NULL;
    }

    char buffer[MAX_INPUT_LENGTH];
    int usage;
    char command[MAX_INPUT_LENGTH];

    // Iterate through the file line by line
    while (fgets(buffer, MAX_INPUT_LENGTH, file) != NULL) {
        if (sscanf(buffer, "%d %[^\n]", &usage, command) == 2) {
            // Check if the command starts with the input prefix
            if (strncmp(command, input, strlen(input)) == 0) {
                fclose(file);
                // Allocate memory for the result and return it
                char *result = (char *)malloc(strlen(command) + 1);
                if (result == NULL) {
                    perror("Failed to allocate memory");
                    return NULL;
                }
                strcpy(result, command);
                return result;
            }
        }
    }

    fclose(file);
    return NULL; // No matching command found
}
