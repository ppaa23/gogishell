#ifndef HEADERS_H
#define HEADERS_H

#include <termios.h> // For declaration of enable/diable_noncanonical_mode()

#define MAX_INPUT_LENGTH 4096
#define MAX_ARGS 32
#define MAX_PATH_LENGTH 1024
#define MAX_COMMAND_NUMBER 1024
#define MAX_COMMAND_LENGTH 64
#define MAX_ARG_LENGTH 64
#define MAX_ABBREVIATIONS 64
#define MAX_KEY_LENGTH 16
#define MAX_VALUE_LENGTH 64

#define PRE_CACHE_DIR "/.gogicache"
#define PRE_HOME_PATH_FILE "/.gogicache/.home_path"
#define PRE_HISTORY_FILE "/.gogicache/.history"
#define PRE_ABBREVIATION_FILE "/.gogicache/.abbreviation"
#define PRE_SORTED_HISTORY_FILE "/.gogicache/.sorted_history"

struct Command {
    const char *command;
    void (*function)(char *args[]);
};

extern char home_dir[MAX_PATH_LENGTH];
extern int cwd_changed;
extern int total_commands;
extern int total_abbreviations;

extern char cache_dir[MAX_PATH_LENGTH];
extern char home_path_file[MAX_PATH_LENGTH];
extern char history_file[MAX_PATH_LENGTH];
extern char abbreviation_file[MAX_PATH_LENGTH];
extern char sorted_history_file[MAX_PATH_LENGTH];

// Functions updating cache files from variables
void initialize_paths();
void create_cache();
void fulfil_home_path_file(const char *home_dir);
void fulfil_history_file(char *input);
void fulfil_abbreviation_file(char *value, char *key);
void fulfil_sorted_history_file(char *input);

// Comparison for fulfil_sorted_history_file()
int compare(const void *a, const void *b);

// Functions handling noncanonical mode
void enable_noncanonical_mode(struct termios *original_termios);
void disable_noncanonical_mode(struct termios *original_termios);

// Main group of functions interpreting input
void process_input(char *input);
void expand_abbreviations_in_input(char *input);
void parse_input(char *input, char *args[]);
void execute_command(char *cmd, char *args[]);

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

#endif
