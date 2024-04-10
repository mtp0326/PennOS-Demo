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

#endif