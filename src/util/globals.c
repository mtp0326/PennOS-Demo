#include "globals.h"

CircularList* processes[3];

CircularList* blocked;
CircularList* stopped;
CircularList* zombied;

pcb_t* current = NULL;
pid_t next_pid = 1;
