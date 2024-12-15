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
char labeled_directories_file[MAX_PATH_LENGTH];


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
    snprintf(labeled_directories_file, MAX_PATH_LENGTH, "%s%s", system_home_path, PRE_LABELED_DIRECTORIES_FILE);
}

void create_cache() {
    struct stat st;
    if (stat(cache_dir, &st) == -1) {
        if (mkdir(cache_dir, 0700) == -1) {
            perror("Failed to create .gogicache directory");
        }
    }
}

void fulfil_labeled_directories_file(char *path, char *description, char *color) {
    FILE *file = fopen(labeled_directories_file, "r");
    char lines[MAX_LABELED_DIRECTORIES][MAX_INPUT_LENGTH];
    int line_count = 0;
    int found = 0;

    char new_entry[MAX_INPUT_LENGTH];
    if (color != NULL) {
        snprintf(new_entry, sizeof(new_entry), "%s:%s:%s\n", path, description, color);
    } else {
        snprintf(new_entry, sizeof(new_entry), "%s:%s\n", path, description);
    }

    if (file == NULL) {
        // If the file doesn't exist, create it and add the new entry
        file = fopen(labeled_directories_file, "w");
        if (file == NULL) {
            perror("Failed to create .labeled_directories_file");
            return;
        }
        fputs(new_entry, file);
        printf("Description and color for '%s' as '%s:%s' added.\n", path, description, color);
        fclose(file);
        return;
    }

    // Read all lines into memory
    while (fgets(lines[line_count], MAX_INPUT_LENGTH, file) && line_count < MAX_LABELED_DIRECTORIES) {
        char current_path[MAX_INPUT_LENGTH], current_description[MAX_INPUT_LENGTH], current_color[MAX_INPUT_LENGTH];

        // Extract path, description, and optional color
        if (sscanf(lines[line_count], "%[^:]:%[^:]:%s", current_path, current_description, current_color) >= 2) {
            // If the path matches, replace the line
            if (strcmp(current_path, path) == 0) {
                snprintf(lines[line_count], MAX_INPUT_LENGTH, "%s", new_entry);
                found = 1;
                printf("Description and color for '%s' updated to '%s:%s'.\n", path, description, color);
                cwd_changed = 1;
            }
        }
        line_count++;
    }
    fclose(file);

    // If the path was not found, add a new entry
    if (!found) {
        if (line_count < MAX_LABELED_DIRECTORIES) {
            snprintf(lines[line_count++], MAX_INPUT_LENGTH, "%s", new_entry);
            printf("Description and color for '%s' as '%s:%s' added.\n", path, description, color);
            cwd_changed = 1;
        } else {
            perror("File contains too many entries");
            return;
        }
    }

    // Write all lines back to the file
    file = fopen(labeled_directories_file, "w");
    if (file == NULL) {
        perror("Failed to open .labeled_directories_file for writing");
        return;
    }
    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], file);
    }
    fclose(file);
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


void print_directory_description(const char *path) {
    // Resolve the absolute path using realpath
    char abs_path[MAX_PATH_LENGTH];
    if (realpath(path, abs_path) == NULL) {
        perror("Failed to resolve absolute path");
        return;
    }

    FILE *file = fopen(labeled_directories_file, "r");
    char line[MAX_INPUT_LENGTH];

    if (file == NULL) {
        return;
    }

    // Search for the directory in the labeled file
    while (fgets(line, sizeof(line), file) != NULL) {
        char current_path[MAX_INPUT_LENGTH];
        char description[MAX_INPUT_LENGTH];

        // Read path and description
        if (sscanf(line, "%[^:]:%[^\n]", current_path, description) == 2) {
            // If the path matches the absolute path, print the description
            if (strcmp(current_path, abs_path) == 0) {
                printf("You are entering '%s'\n", description);
                fclose(file);
                return;
            }
        }
    }
    fclose(file);
}

int get_color_for_directory(const char *cwd) {
    FILE *file = fopen(labeled_directories_file, "r");
    if (file == NULL) {
        return -1;  // Return -1 if the file can't be opened
    }

    char line[MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Trim newline character from the line if present
        line[strcspn(line, "\n")] = '\0';

        // Split the line into path, color name, and any extra parts
        char *path = strtok(line, ":");
        strtok(NULL, ":");  // Skip the second part (irrelevant for now)
        char *color_name = strtok(NULL, ":");  // Get the third part as the color name

        // Check if the current working directory starts with this path
        if (strncmp(cwd, path, strlen(path)) == 0) {
            fclose(file);
            return color_name_to_code(color_name);  // Return the corresponding color code
        }
    }

    fclose(file);
    return -1;  // Return -1 if no color was found
}

// Function to convert a color name to an ANSI escape code
int color_name_to_code(const char *color_name) {
    if (strcmp(color_name, "black") == 0) return 30;
    if (strcmp(color_name, "red") == 0) return 31;
    if (strcmp(color_name, "green") == 0) return 32;
    if (strcmp(color_name, "yellow") == 0) return 33;
    if (strcmp(color_name, "blue") == 0) return 34;
    if (strcmp(color_name, "magenta") == 0) return 35;
    if (strcmp(color_name, "cyan") == 0) return 36;
    if (strcmp(color_name, "white") == 0) return 37;

    // Light colors
    if (strcmp(color_name, "light_black") == 0) return 90;
    if (strcmp(color_name, "light_red") == 0) return 91;
    if (strcmp(color_name, "light_green") == 0) return 92;
    if (strcmp(color_name, "light_yellow") == 0) return 93;
    if (strcmp(color_name, "light_blue") == 0) return 94;
    if (strcmp(color_name, "light_magenta") == 0) return 95;
    if (strcmp(color_name, "light_cyan") == 0) return 96;
    if (strcmp(color_name, "light_white") == 0) return 97;

    // Background colors
    if (strcmp(color_name, "bg_black") == 0) return 40;
    if (strcmp(color_name, "bg_red") == 0) return 41;
    if (strcmp(color_name, "bg_green") == 0) return 42;
    if (strcmp(color_name, "bg_yellow") == 0) return 43;
    if (strcmp(color_name, "bg_blue") == 0) return 44;
    if (strcmp(color_name, "bg_magenta") == 0) return 45;
    if (strcmp(color_name, "bg_cyan") == 0) return 46;
    if (strcmp(color_name, "bg_white") == 0) return 47;

    return -1; // Inappropriate color name
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
