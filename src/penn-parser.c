#include "penn-parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_command(const char* const str_to_parse,
                  struct parsed_command** const out_cmd) {
  // if input string is just & then return 7 (EXPECTED_COMMANDS)
  if (strcmp(str_to_parse, "&") == 0) {
    return EXPECT_COMMANDS;
  }
  int num = 0;
  bool background = false;
  char* s_cpy = strdup(str_to_parse);
  char* s_cpy2 = strdup(str_to_parse);
  int err = check_errors(s_cpy2, &background);
  free(s_cpy2);
  if (err != 0) {
    free(s_cpy);
    return err;
  }

  struct parsed_command* pcptr = malloc(sizeof(struct parsed_command));

  pcptr->is_background = background;
  char** cmds = NULL;
  int ret = separate_to_commands(&cmds, &num, s_cpy);
  if (ret != 0) {
    free(s_cpy);
    free(pcptr);
    return ret;
  }
  char*** args = NULL;
  for (int i = 0; i < num; i++) {
    int command_counter_copy = i + 1;
    int ret2 =
        separate_to_args(&args, cmds[i], command_counter_copy, background);
    if (ret2 != 0) {
      free(s_cpy);
      free(pcptr);
      free(cmds);
      return ret2;
    }
  }
  free(cmds);
  pcptr->num_commands = num;
  pcptr->commands = args;
  pcptr->str_parse = s_cpy;
  *out_cmd = pcptr;
  return PARSE_SUCCESSFUL;
}

int separate_to_commands(char*** cmds, int* n, char* parse_string) {
  char** cmds_temp = NULL;
  int command_counter = 0;
  int command_size_counter = 0;
  for (char* str = strtok(parse_string, "|"); str != NULL;
       str = strtok(NULL, "|")) {
    if (strcmp(str, " ") == 0) {
      free(cmds_temp);
      return UNEXPECTED_PIPELINE;
    }
    command_size_counter += sizeof(char*);
    char** new_cmds = realloc(cmds_temp, command_size_counter);
    if (new_cmds == NULL) {
      free(cmds_temp);
      return SYSTEM_ERROR;
    }
    cmds_temp = new_cmds;
    cmds_temp[command_counter] = str;
    command_counter++;
  }
  *n = command_counter;
  *cmds = cmds_temp;
  return PARSE_SUCCESSFUL;
}

int separate_to_args(char**** cmds2,
                     char* single_command,
                     int command_counter_copy,
                     bool background) {
  int arg_counter = 1;
  int arg_size_counter = 0;
  char*** cmds2_temp = *cmds2;
  char*** cmds2_new =
      realloc(cmds2_temp, sizeof(char**) * command_counter_copy);
  if (cmds2_new == NULL) {
    free(cmds2_temp);
    return SYSTEM_ERROR;
  }
  cmds2_temp = cmds2_new;
  cmds2_temp[command_counter_copy - 1] = NULL;
  for (char* str3 = strtok(single_command, " \n\t\r"); str3 != NULL;
       str3 = strtok(NULL, " \n\t\r")) {
    if (strcmp(str3, "&") != 0) {
      arg_size_counter += sizeof(char*);
      char** new_args =
          realloc(cmds2_temp[command_counter_copy - 1], arg_size_counter);
      if (new_args == NULL) {
        free(cmds2_temp);
        return SYSTEM_ERROR;
      }
      cmds2_temp[command_counter_copy - 1] = new_args;
      cmds2_temp[command_counter_copy - 1][arg_counter - 1] = str3;
      arg_counter++;
    } else if (!background) {
      free(cmds2_temp[command_counter_copy - 1]);
      free(cmds2_temp);
      return UNEXPECTED_BACKGROUND;
    }
  }
  char** args_null = realloc(cmds2_temp[command_counter_copy - 1],
                             arg_size_counter + sizeof(char*));
  if (args_null == NULL) {
    free(cmds2_temp);
    return SYSTEM_ERROR;
  }
  cmds2_temp[command_counter_copy - 1] = args_null;
  cmds2_temp[command_counter_copy - 1][arg_counter - 1] = NULL;
  *cmds2 = cmds2_temp;
  return PARSE_SUCCESSFUL;
}

int check_errors(char* str, bool* background) {
  bool found_amp = false;
  bool all_whitespace_front = true;
  bool amp = false;
  for (int i = 0; i < strlen(str); i++) {
    // if found multiple &
    if (str[i] == '&' && found_amp) {
      return UNEXPECTED_BACKGROUND;
    }
    // check if non white-space found before |
    if (not_whitespace(str, i) && all_whitespace_front) {
      all_whitespace_front = false;
      if (str[i] == '|') {
        return UNEXPECTED_PIPELINE;
      }
      if (str[i] == '&') {
        return UNEXPECTED_BACKGROUND;
      }
    }

    // found a &
    if (str[i] == '&') {
      found_amp = true;
    }
  }

  int ret = check_error2(str, &amp);
  if (ret != 0) {
    return ret;
  }
  *background = amp;
  return PARSE_SUCCESSFUL;
}

int check_error2(char* str, bool* amp) {
  bool found_pipeline = false;
  int counter = 0;
  bool all_whitespace_back = true;
  bool amp_last = false;

  for (int i = 0; i < strlen(str); i++) {
    if (not_whitespace(str, strlen(str) - 1 - i) && all_whitespace_back) {
      all_whitespace_back = false;
      if (str[strlen(str) - 1 - i] == '|') {
        return EXPECT_COMMANDS;
      }
      if (str[strlen(str) - 1 - i] == '&') {
        amp_last = true;
      }
    }
  }
  // check for consecutive pipelines
  for (char* str2 = strtok(str, " \n\t\r"); str2 != NULL;
       str2 = strtok(NULL, " \n\t\r")) {
    if (strcmp(str2, "|") == 0 && found_pipeline) {
      return UNEXPECTED_PIPELINE;
    }
    found_pipeline = (strcmp(str2, "|") == 0);
    counter++;
  }
  *amp = amp_last;
  return PARSE_SUCCESSFUL;
}

bool not_whitespace(const char* str, unsigned long index) {
  return (str[index] != ' ' && str[index] != '\t' && str[index] != '\r' &&
          str[index] != '\n');
}

void print_parsed_command(const struct parsed_command* const cmd) {
  for (size_t i = 0; i < cmd->num_commands; ++i) {
    for (char** arguments = cmd->commands[i]; *arguments != NULL; ++arguments) {
      fprintf(stderr, "%s ", *arguments);
    }

    if (i != cmd->num_commands - 1) {
      fprintf(stderr, "| ");
    }
  }
  fprintf(stderr, cmd->is_background ? "&" : "");
}

bool free_parsed_command(struct parsed_command* const cmd) {
  for (int i = 0; i < cmd->num_commands; ++i) {
    free(cmd->commands[i]);
  }
  free(cmd->commands);
  free(cmd->str_parse);
  free(cmd);
  return false;
}