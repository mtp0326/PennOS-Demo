#ifndef PENNFAT_H
#define PENNFAT_H

#define PROMPT "penn-fat> "
#include <stdbool.h>
#include <stdint.h>

extern uint16_t* fat;

int main(int argc, char* argv[]);

void prompt();
void read_command();
void int_handler(int signo);
int get_block_size(int block_size_config);

#endif