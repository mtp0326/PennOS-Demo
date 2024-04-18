#ifndef PENNFAT_H
#define PENNFAT_H

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "parser.h"
#include "util/pennfat_kernel.h"
#ifndef PROMPT_PENN_FAT
#define PROMPT_PENN_FAT "penn-fat> "
#endif
#define MAX_LEN 4096

void mkfs(const char* fs_name, int blocks_in_fat, int block_size_config);
int mount(const char* fs_name);
int unmount();

void prompt();
void read_command();
void int_handler(int signo);
int get_block_size(int block_size_config);
int get_fat_size(int block_size, int blocks_in_fat);
int get_num_fat_entries(int block_size, int blocks_in_fat);
int get_data_size(int block_size, int num_fat_entries);
int get_offset_size(int block_num, int offset);

void initialize_global_fd_table();

void touch(char** args);
void rm(char** args);
void mv(char** args);
void chmod(char** args);
void cat_file_wa(char** args);

void ls();
void cp_within_fat(char* source, char* dest);
void cp_to_host(char* source, char* host_dest);
void cp_from_host(char* host_source, char* dest);

void cat_w(char* output);
void cat_a(char* output);

#endif