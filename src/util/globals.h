#ifndef GLOBALS_H
#define GLOBALS_H

#include "clinkedlist.h"
#include "kernel.h"
#include "prioritylist.h"

extern CircularList* processes[3];

extern CircularList* blocked;

extern CircularList* stopped;

extern CircularList* zombied;

extern pcb_t* current;

extern pid_t next_pid;  // idk, i guess redefine this if you want to reuse pcbs?
// i have no strong urge to write that function

extern int logfiledescriptor;

extern unsigned int tick;

#endif