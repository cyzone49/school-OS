#pragma once
#include <stdlib.h>

#include "os345.h"
#include "dc.h"

Memo* newMemo(int cost, Semaphore* task) 
{
	Memo* retMemo = malloc(sizeof(Memo));
	retMemo->cost = cost;
	retMemo->event = task;
	retMemo->next = 0;
	return retMemo;
}


void tick(Memo** dclock) {

	if (*dclock == 0) 
	{
		return;
	}

	Memo* memo = *dclock;
	if (memo)
		memo->cost--;

	while (memo && memo->cost < 1) 
	{
		semSignal(memo->event);
		Memo* deleteMe = memo;
		memo = memo->next;
		free(deleteMe);
	}

	(*dclock) = memo;
}

void insert(Memo** dclock, Memo* new)
{
	Memo** next = dclock;

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