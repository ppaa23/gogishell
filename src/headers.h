#ifndef HEADERS_H
#define HEADERS_H

#include <termios.h> // For declaration of enable/disable_noncanonical_mode()

#define MAX_INPUT_LENGTH 4096
#define MAX_ARGS 32
#define MAX_PATH_LENGTH 1024
#define MAX_COMMAND_NUMBER 1024
#define MAX_COMMAND_LENGTH 64
#define MAX_ARG_LENGTH 64
#define MAX_ABBREVIATIONS 64
#define MAX_KEY_LENGTH 16
#define MAX_VALUE_LENGTH 64
#define MAX_LABELED_DIRECTORIES 32
#define MAX_COLOR_NAME_LENGTH 16

#define PRE_CACHE_DIR "/.gogicache"
#define PRE_HOME_PATH_FILE "/.gogicache/.home_path"
#define PRE_HISTORY_FILE "/.gogicache/.history"
#define PRE_ABBREVIATION_FILE "/.gogicache/.abbreviation"
#define PRE_SORTED_HISTORY_FILE "/.gogicache/.sorted_history"
#define PRE_LABELED_DIRECTORIES_FILE "/.gogicache/.labeled_directories"

struct Command {
    const char *command;
    void (*function)(char *args[]);
};

extern char home_dir[MAX_PATH_LENGTH];
extern int cwd_changed;
extern int total_commands;
extern int total_abbreviations;
extern int total_labeled_directories;

extern char cache_dir[MAX_PATH_LENGTH];
extern char home_path_file[MAX_PATH_LENGTH];
extern char history_file[MAX_PATH_LENGTH];
extern char abbreviation_file[MAX_PATH_LENGTH];
extern char sorted_history_file[MAX_PATH_LENGTH];
extern char labeled_directories_file[MAX_PATH_LENGTH];

// Functions updating cache files from variables
void initialize_paths();
void create_cache();
void fulfil_home_path_file(const char *home_dir);
void fulfil_history_file(char *input);
void fulfil_abbreviation_file(char *value, char *key);
void fulfil_sorted_history_file(char *input);
void fulfil_labeled_directories_file(char *path, char *description, char *color);

// Comparison for fulfil_sorted_history_file()
int compare(const void *a, const void *b);

// Functions handling non-canonical mode
void enable_noncanonical_mode(struct termios *original_termios);
void disable_noncanonical_mode(struct termios *original_termios);

// Main group of functions interpreting input
void process_input(char *input);
void expand_abbreviations_in_input(char *input);
void parse_input(char *input, char *args[]);

// Functions updating variables from cache files
void get_home_dir();
void get_total_commands();
void get_total_abbreviations();

// Commands
void sethome(char *args[]);
void history(char *args[]);
void home(char *args[]);
void setabbr(char *args[]);
void abbr(char *args[]);
void ldir(char *args[]);
void help(char *args[]);

// Functions completing input
char* get_command_from_history(int command_index);
char* get_most_used_command(char *input);

// Functions handling escaping sequences
void handle_up_arrow(char *input, int *command_index, int *i);
void handle_down_arrow(char *input, int *command_index, int *i);
void handle_tab(char *input, int *i);

// Updating prompt
void get_prompt(char *cwd, char *home_dir, char *display_cwd);

// Printing the description of entering directory if labeled
void print_directory_description(const char *path);

int get_color_for_directory(const char *cwd);
int color_name_to_code(const char *color_name);

// Handling redirection operators
void handle_redirection(char *args[]);

// Handling pipelines
void parse_pipeline(char *input, char *commands[], int *num_commands);
void execute_pipeline(char *commands[], int num_commands);

#endif
