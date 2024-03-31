/*
 * Penn-Parser
 * Seungmin Han, 23fa
 * Initial parser hanbangw, 21fa
 */

#ifndef PENN_PARSER_H_
#define PENN_PARSER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Here defines all possible parser success/error codes */
// parser encoutnered a system call error
#define SYSTEM_ERROR (-1)

// parse successful
#define PARSE_SUCCESSFUL 0

// parser encountered an unexpected pipeline token '|'
#define UNEXPECTED_PIPELINE 3

// parser encountered an unexpected ampersand token '&'
#define UNEXPECTED_BACKGROUND 4

// parser didn't find any commands or arguments where it expects one
#define EXPECT_COMMANDS 7

struct parsed_command {
  bool is_background;  // indicates if the '&' symbol exists at the end
  int num_commands;    // number of commands in the pipeline
  char*** commands;    // array for commands and its arguments
  char* str_parse;     // the string behind parsed in
};

// would parse the command return a pointer to it or NULL via the output
// parameter out_cmd
//
// Arguments:
// - str_to_parse: the string to parse. Note: you are not allowed to modify the
// input
// - out_cmd: the output parameter to return the struct parsed_command*
//
// Returns:
// - sets *out_cmd to a pointer to a newly allocated parsed_command struct that
//   contains the parsed output. or sets to NULL on error
// - 0 on success, or the corresponding error code (see above) on error
int parse_command(const char* str_to_parse, struct parsed_command** out_cmd);

// print the parsed command. Used for testing and debugging
// Assumes the cmd is well-formed and comes from parse_command()
void print_parsed_command(const struct parsed_command* cmd);

// free the struct gotten from parse_command.
// Assumes the cmd is well-formed and comes from parse_command()
// returns bool for success/failure
bool free_parsed_command(struct parsed_command* cmd);

int separate_to_commands(char*** cmds, int* n, char* parse_string);

int separate_to_args(char**** cmds2,
                     char* single_command,
                     int command_counter_copy,
                     bool background);

// helper functions to check for input errors
int check_errors(char* str, bool* background);

// this one is for if there are multiple consecutive pipeline commands
int check_error2(char* str, bool* amp);

bool not_whitespace(const char* str, unsigned long index);

#endif
