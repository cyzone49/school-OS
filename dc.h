#pragma once

#ifndef DC_H_
#define DC_H_

#include "os345.h"

typedef struct memo {
	int cost;
	Semaphore* event;
	struct memo* next;
} Memo;

Memo* newMemo(int cost, Semaphore* task);
void tick(Memo** dc);
void insert(Memo** dc, Memo* new);

#endif /* DC_H_ */