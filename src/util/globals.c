#include "globals.h"

CircularList* processes[3];

CircularList* blocked;
CircularList* stopped;
CircularList* zombied;

CircularList* bg_list;

pcb_t* current = NULL;
pid_t next_pid = 1;

uint64_t job_id = 1;

int logfiledescriptor = 0;

unsigned int tick = 0;
