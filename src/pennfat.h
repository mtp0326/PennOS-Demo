#ifndef PROMP
#define PROMPT "penn-fat> "
#endif

#include <stdbool.h>

int main(int argc, char* argv[]);

void prompt();
void read_command();
void int_handler(int signo);