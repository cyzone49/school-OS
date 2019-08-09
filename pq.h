#ifndef PQ_H_
#define PQ_H_

#include <stdarg.h>
#include "os345.h"

#define CAP_INCREASE 5

typedef int Tid;

typedef struct priorityQueue {
	int size;
	int cap;
	Tid* content;
	char* name;
} PQ;

// new
PQ* newPqueue(int cap, char* name);

// main functions
Tid next(PQ* q);
Tid pull(PQ* q, int tid);
void put(PQ* q, Tid tid);
Tid pop(PQ* q);
Tid ready(PQ* src, PQ* dst);
Tid block(PQ* src, PQ* dst, Tid tid);

// peripherals
void printPqueue(PQ* q);
void pqprintf(char* fmt, ...);

#endif /* PQ_H_ */