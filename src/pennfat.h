#ifndef PENNFAT_H
#define PENNFAT_H

#define PROMPT "penn-fat> "
#include <stdbool.h>
#include <stdint.h>

extern uint16_t* fat;

void prompt();
void read_command();
void int_handler(int signo);
int get_block_size(int block_size_config);
int get_fat_size(int block_size, int blocks_in_fat);
int get_num_fat_entries(int block_size, int blocks_in_fat);
int get_data_size(int block_size, int num_fat_entries);
int get_offset_size(int block_size,
                    int blocks_in_fat,
                    int block_num,
                    int offset);

#endif