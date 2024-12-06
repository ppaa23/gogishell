#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "headers.h"


void sethome(char *args[]) {
    if (args[1] == NULL || args[2] != NULL) {
        printf("Usage: sethome <path>\n\tsethome .\n\tsethome ..\n\tsethome default\n");
    } else if (strcmp(args[1], ".") == 0) {
        // Change to the current directory
        if (getcwd(home_dir, sizeof(home_dir)) == NULL) {
            perror("Internal function getcwd failed");
        }
        fulfil_home_path_file(home_dir);
    } else if (strcmp(args[1], "..") == 0) {
        // Get the current working directory
        char cwd[MAX_PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("Internal function getcwd failed");
            return;
        }
        // If the current directory is not root, change home_dir to its parent (else do nothing)
        if (strcmp(cwd, "/") != 0) {
            char temp_cwd[MAX_PATH_LENGTH];
            strncpy(temp_cwd, cwd, sizeof(temp_cwd));  // Copy cwd into temp_cwd
            fulfil_home_path_file(dirname(temp_cwd));  // Get the parent directory
        } else {
            if (getcwd(home_dir, sizeof(home_dir)) == NULL) {
                perror("Internal function getcwd failed");
            }
            fulfil_home_path_file(home_dir);
        }
    } else if (strcmp(args[1], "~") == 0) {
        // Do not change
    } else if (strcmp(args[1], "default") == 0) {
        // Reset home_dir to the default home directory (from getenv("HOME"))
        const char *default_home = getenv("HOME");
        if (default_home != NULL) {
            strncpy(home_dir, default_home, sizeof(home_dir) - 1);
            home_dir[sizeof(home_dir) - 1] = '\0';  // Null-terminate
        } else {
            perror("Failed to retrieve HOME from environment");
        }
        fulfil_home_path_file(home_dir);
    } else {
        // Change home_dir to the specified path
        strncpy(home_dir, args[1], sizeof(home_dir) - 1);
        home_dir[sizeof(home_dir) - 1] = '\0';

        char simplified_path[MAX_PATH_LENGTH];
        if (realpath(home_dir, simplified_path) == NULL) {
            printf("Usage: sethome <path>\n\tsethome .\n\tsethome ..\n\tsethome default\n");
            simplified_path[0] = '\0';  // Clear simplified_path in case of an error
            return;
        }

        fulfil_home_path_file(simplified_path);
    }
    cwd_changed = 1;
}

void history(char *args[]) {
    // If too many arguments are provided, print usage
    if (args[1] != NULL && args[2] != NULL) {
        printf("Usage: history <number_of_lines>\n\thistory clear\n\thistory\n");
        return;
    }

    // Handle "history clear"
    if (args[1] != NULL && strcmp(args[1], "clear") == 0) {
        FILE *file = fopen(history_file, "w");
        if (file == NULL) {
            perror("Failed to open .history");
            return;
        }
        fclose(file);
        total_commands = 0;
        printf("History was successfully cleared.\n");
        return;
    }

    // Open the history file for reading
    FILE *file = fopen(history_file, "r");
    if (file == NULL) {
        perror("Failed to open .history");
        return;
    }

    // If no arguments are provided, show the entire history
    if (args[1] == NULL) {
        char line[MAX_COMMAND_NUMBER];
        int line_number = 1;

        while (fgets(line, sizeof(line), file) != NULL) {
            printf("%d %s", line_number++, line);
        }

        fclose(file);
        return;
    }

    // Handle "history <number_of_lines>"
    char *endptr;
    long num_lines = strtol(args[1], &endptr, 10);
    if (*endptr != '\0' || num_lines <= 0) {
        printf("Usage: history <number_of_lines>\n\thistory clear\n\thistory\n");
        fclose(file);
        return;
    }

    // Read and count the total number of lines in the file
    char lines[MAX_COMMAND_NUMBER][MAX_COMMAND_NUMBER]; // Temporary storage for lines
    int total_lines = 0;

    while (fgets(lines[total_lines], sizeof(lines[0]), file) != NULL) {
        if (total_lines < MAX_COMMAND_NUMBER - 1) { // Avoid buffer overflow
            total_lines++;
        }
    }
    fclose(file);

    // Print the requested number of lines from the end
    int start_line = (num_lines >= total_lines) ? 0 : total_lines - num_lines;
    for (int i = start_line; i < total_lines; i++) {
        printf("%d %s", i + 1, lines[i]);
    }
}

void home(char *args[]) {
    if (args[1] != NULL) {
        printf("Usage: home\n");
    }
    else {
        printf("Current home directory is: %s\n", home_dir);
    }
}

void setabbr(char *args[]) {
    if (args[1] == NULL) {
        printf("Usage: setabbr <value> <key>\n\tsetabbr clear\n");
        return;
    }

    if (strcmp(args[1], "clear") == 0) {
        // Open the file in write mode to clear its contents
        FILE *file = fopen(abbreviation_file, "w");
        if (file == NULL) {
            perror("Failed to open .abbreviation_file");
            return;
        }
        fclose(file);
        printf("Abbreviations were successfully cleared.\n");

        fulfil_abbreviation_file(home_dir, "~");
        get_total_abbreviations();

        return;
    }

    if (args[2] == NULL) {
        printf("Usage: setabbr <value> <key>\n\tsetabbr clear\n");
        return;
    }

    char value[MAX_INPUT_LENGTH] = ""; // Initialize the result buffer
    int i = 1;

    while (args[i + 1] != NULL) { // Stop before the last argument
        strcat(value, args[i]);
        if (args[i + 2] != NULL) { // Add space if it's not the second-to-last argument
            strcat(value, " ");
        }
        i++;
    }
    char *key = args[i];

    // Validate that neither key nor value contains ':'
    if (strchr(value, ':') != NULL || strchr(key, ':') != NULL) {
        printf("':' is not allowed in value or key.\n");
        return;
    }

    fulfil_abbreviation_file(value, key);
}

void abbr(char *args[]) {
    if (args[1] != NULL) {
        printf("Usage: abbr\n");
    }
    else {
        FILE *file = fopen(abbreviation_file, "r");
        if (file == NULL) {
            perror("Failed to opening .abbreviation");
            return;
        }

        char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 1]; // Buffer to store each line
        while (fgets(line, sizeof(line), file) != NULL) { // Read line by line
            printf("%s", line); // Print each line to the console
        }

        fclose(file);
    }
}

void help(char *args[]) {
    if (args[1] != NULL) {
        printf("Usage: help\n");
    }
    else {
        printf("\n");
        printf("GoGiShell is pseudoshell that provides interaction between user and BASH.\n");
        printf("Except for internal BASH commands and tools, GoGiShell can operate a several own ones:\n");
        printf("\n");
        printf("sethome - change the path to home directory to required, accepts one of following arguments:\n");
        printf("        <path> - path ('.' and '..' in path interpreted)\n");
        printf("        . - current directory\n");
        printf("        .. - parent directory\n");
        printf("        default - internal home directory provided by system\n");
        printf("\n");
        printf("home - print the path to home directory\n");
        printf("\n");
        printf("setabbr <value> <key> - define <key> as <value> and interpret the latter as the first in user's input, or\n");
        printf("        setabbr clear - clean list of abbreviations\n");
        printf("\n");
        printf("abbr - print the list of defined pairs key:value\n");
        printf("\n");
        printf("history - print the whole GoGiShell history of commands without arguments, or:\n");
        printf("        clear - clear the GoGiShell history\n");
        printf("        <number> - print <number> last commands\n");
        printf("\n");
        printf("help - print manual\n");
        printf("\n");
        printf("Furthermore, GoGiShell provides access to commands from history in-line.\n");
        printf("Using UP_ARROW and DOWN_ARROW buttons navigates in history.\n");
        printf("\n");
        printf("Finally, GoGiShell provides autocomleting of the current input in-line.\n");
        printf("Using TAB buttons completes current input to the most used command from history that starts the same way.\n");
        printf("\n");
        printf("Please don't try to launch this pseudoshell or related programs (e.g. Makefile) from itself, it can lead to unknown consequences!\n");
        printf("\n");
    }
}
