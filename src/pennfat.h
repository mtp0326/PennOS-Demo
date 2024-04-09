#ifndef PENNFAT_H
#define PENNFAT_H

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "parser.h"
#include "util/pennfat_kernel.h"
#ifndef PROMPT
#define PROMPT "penn-fat> "
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

void ls();
#endif