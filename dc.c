#pragma once
#include <stdlib.h>

#include "os345.h"
#include "dc.h"

Memo* newMemo(int cost, Semaphore* task) 
{
	Memo* newMemo = malloc(sizeof(Memo));
	newMemo->cost = cost;
	newMemo->event = task;
	newMemo->next = 0;
	return newMemo;
}

void tick(Memo** clock) 
{
	if (*clock == 0) return;

	Memo* memo = *clock;

	if (memo) memo->cost--;

	while (memo && memo->cost < 1) 
	{
		semSignal(memo->event);
		Memo* deleteMe = memo;
		memo = memo->next;
		free(deleteMe);
	}

	(*clock) = memo;
}

void insert(Memo** clock, Memo* new) 
{
	Memo** next = clock;

	while ((*next) && (*next)->cost <= new->cost) 
	{
		new->cost -= (*next)->cost;
		next = &((*next)->next);
	}

	if (*next) 
	{
		(*next)->cost -= new->cost;
		new->next = (*next);
	}

	*next = new;
}