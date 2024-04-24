#ifndef STRESS_H
#define STRESS_H
#include "sys_call.h"
#include "globals.h"
void* hang(void*);
void* nohang(void*);
void* recur(void*);

#endif
