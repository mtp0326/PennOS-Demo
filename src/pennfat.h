#ifndef PROMP
#define PROMPT "penn-fat> "
#endif

#include <stdbool.h>

int main(int argc, char* argv[]);

void prompt();
void read_command();
void int_handler(int signo);
int get_fat_size(int blocks_in_fat, int block_size_config);