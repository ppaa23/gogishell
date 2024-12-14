#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pty.h>
#include <ctype.h>

#define MAX_INPUT 4096
#define OUTPUT_FILE "./build/test_output.txt"

void trim_whitespace(char *str) {
    char *end;

    // Trim leading spaces
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';
}

// Function to remove ANSI escape codes from a string
void strip_ansi_codes(char *str) {
    char clean[MAX_INPUT];
    int j = 0;
    int in_escape = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\033' || (str[i] == '^' && str[i + 1] == '[')) { // Start of ESC sequence
            in_escape = 1;
            continue;
        }

        if (in_escape) {
            // Check if this is the end of the ANSI sequence (letters like 'm', 'K', etc.)
            if ((str[i] >= 'A' && str[i] <= 'Z') || (str[i] >= 'a' && str[i] <= 'z')) {
                in_escape = 0; // End the escape sequence
            }
            continue; // Skip the character
        }

        // Copy non-escape characters to the cleaned output
        clean[j++] = str[i];
    }

    clean[j] = '\0';
    strncpy(str, clean, MAX_INPUT); // Copy cleaned string back
}

void compare_output(FILE *file) {
    char line[MAX_INPUT];
    int skipped_welcome = 0;
    int expected_index = 0;

    // Define expected outputs excluding prompts and user inputs
    const char *expected_outputs[] = {
        "Hello, GoGiShell!",
        "1 echo Hello, GoGiShell!",
        "2 abbr",
        "3 history",
        "1 echo Hello, GoGiShell!",
        "2 abbr",
        "3 history",
        "4 history",
        "3 history",
        "4 history",
        "5 history 3",
        "History was successfully cleared.",
        "Abbreviation 'Hello' as 'Hi' added.",
        "Abbreviation file updated successfully.",
        "Hello, GoGiShell!",
        "Hi:Hello",
        "Usage: abbr",
        "1 setabbr Hello Hi",
        "2 echo Hi, GoGiShell!",
        "3 abbr",
        "4 abbr 3",
        "5 history",
        "Hello, GoGiShell! One more time!",
        "Hello, GoGiShell! One more time!",
        "Hi:Hello",
        "No such internal or GoGiShell command: No such file or directory",
        "Usage: home",
        "Hello",
        "Usage: home",
        "Hello GoGiShell",
        "Hello GoGiShell",
        "Hello, GoGiShell",
        "Hello, GoGiShell",
        "2",
        "Thank you for using GoGiShell!"
    };

    int total_expected = sizeof(expected_outputs) / sizeof(expected_outputs[0]);

    // Read output line-by-line
    while (fgets(line, sizeof(line), file)) {
        // Skip the first 6 lines (welcome message)
        if (skipped_welcome < 6) {
            skipped_welcome++;
            continue;
        }

        // Strip ANSI codes and trim whitespace
        strip_ansi_codes(line);
        trim_whitespace(line);

        // Skip prompts, home reliable abbreviations and inputs (lines starting with "GoGiShell:" or empty lines)
        if (strstr(line, "GoGiShell:") == line || strstr(line, "~:") == line || line[0] == '\0') {
            continue;
        }

        // Compare with expected output
        if (expected_index < total_expected) {
            // Copy the expected output and trim whitespace
            char expected_trimmed[MAX_INPUT];
            strncpy(expected_trimmed, expected_outputs[expected_index], sizeof(expected_trimmed));
            trim_whitespace(expected_trimmed);

            // Debug output (only print if there is a mismatch)
            if (strcmp(line, expected_trimmed) != 0) {
                printf("Mismatch at line %d: Expected \"%s\", but got \"%s\"\n",
                       expected_index + 1, expected_trimmed, line);
                break;
            }
            expected_index++;
        } else {
            printf("Unexpected extra output: \"%s\"\n", line);
            break;
        }
    }

    // Check for missing expected lines
    if (expected_index < total_expected) {
        printf("Failure: some of tests were failed\n");
    } else {
        printf("Success: all tests were passed\n");
    }
}

// Function to simulate typing
void simulate_typing(int fd, const char *text) {
    for (size_t i = 0; i < strlen(text); i++) {
        char c = text[i];
        write(fd, &c, 1);
        usleep(20000);  // Simulate typing delay (20ms per character)
    }
}

int main() {
    int master_fd, slave_fd;
    pid_t pid;
    char buffer[MAX_INPUT];
    ssize_t bytes_read;

    printf("Starting GoGiShell testing...\n");

    FILE *file;
    // Open PTY
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
        perror("Internal function openpty failed");
        exit(1);
    }

    pid = fork();
    if (pid == -1) {
        perror("Internal function fork failed");
        exit(1);
    }

    if (pid == 0) {  // Child process (GoGiShell)
        close(master_fd);

        // Redirect stdin, stdout, and stderr to slave end of the PTY
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);

        // Execute GoGiShell
        execlp("./build/GoGiShell", "GoGiShell", NULL);
        perror("Internal function execlp failed");
        exit(1);
    } else {  // Parent process
        close(slave_fd);

        // Commands to send to GoGiShell
        const char *commands[] = {
            "\n",
            "echo Hello, GoGiShell!\n",
            "abbr\n",
            "history\n",
            "h\t\n",
            "history 3\n",
            "history clear\n",
            "setabbr Hello Hi\n",
            "echo Hi, GoGiShell!\n",
            "abbr\n",
            "abbr 3\n", // Error
            "h\t\n",
            "ec\t One more time!\n",
            "\033[A\n",
            "\033[A\033[A\033[A\033[A\033[A\033[A\033[B\n",
            "abc\n",  // Internal error
            "cd build\n",
            "home 2\n",  // Error
            "cd ..\n",
            "echo Hi\b \bello\n",
            "\033[A\033[A\033[A\033[C\n",
            "echo Hello GoGiShell > out.txt\n",
            "cat out.txt\n",
            "echo Hello, GoGiShell >> out.txt\n",
            "cat out.txt\n",
            "grep , < out.txt\n",
            "echo 11 > out.txt\n",
            "echo 12 >> out.txt\n",
            "echo 23 >> out.txt\n",
            "grep 2 < out.txt | wc -l\n",
            "rm out.txt\n",
            "exit\n"
        };

        // Simulate typing commands
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            simulate_typing(master_fd, commands[i]);
        }

        // Open file for output
        file = fopen(OUTPUT_FILE, "w");
        if (!file) {
            perror("Failed opening test_output.txt");
            exit(1);
        }

        // Read from master_fd and write to file
        while ((bytes_read = read(master_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate

            fprintf(file, "%s", buffer);
            fflush(file);
        }

        // Close writing end
        close(master_fd);

        // Wait for child process to finish
        if (wait(NULL) == -1) {
            perror("Internal function wait failed");
            exit(1);
        }

        fclose(file);
        printf("GoGiShell results were written.\n");

        // Reopen output file for comparison
        file = fopen(OUTPUT_FILE, "r");
        if (!file) {
            perror("Failed opening test_output.txt");
            exit(1);
        }

        printf("Starting comparison...\n");
        compare_output(file);

        fclose(file);

        return 0;
    }
}
