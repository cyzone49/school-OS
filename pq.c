#include <stdio.h>
#include <stdlib.h>

#include "os345.h"
#include "pq.h"

extern PQ* rQueue;						// task ready queue

// create new Priority Queue
PQ* newPqueue(int cap, char* name) 
{	
	PQ* newQueue = malloc(sizeof(PQ));
	newQueue->size = 0;
	newQueue->name = name;
	if (cap < 1) 
	{
		newQueue->cap = 0;
		newQueue->content = 0;
	}
	else 
	{
		newQueue->cap = cap;
		//newQueue->content = malloc(sizeof(PQ) * (cap));
		newQueue->content = malloc(sizeof(Tid) * (cap));
	}
	return newQueue;
}

// Return Tid of next highest (lowest numarically) priority task in pQueue
// tasks are ordered in highest->lowest priority fashion
Tid next(PQ* q) 
{	
	Tid tid = -1;
	if (q->size < 1) return -1;

	int pos = q->size - 1;
	tid = q->content[pos];
	int priority = taskPriority(tid);

	while (pos > 0) 
	{
		if (priority > taskPriority(q->content[pos - 1])) break;
		q->content[pos] = q->content[pos - 1];
		pos--;
	}
	q->content[pos] = tid;
	
	return tid;
}

Tid nextSlices(PQ* q) {	

	//printPqueue(q);

	if (q->size < 1) return -1;

	Tid tid = -1;

	for (int i = 0; i < q->size; i++) 
	{
		if (getSlices(q->content[i]) > 0) 
		{
			tid = q->content[i];
			break;
		}
	}
	
	return tid;
}

// pop a task from pQueue
Tid pop(PQ* q) 
{
	Tid tid;
	if (q->size < 1) 
	{
		tid = -1; // return -1 if empty queue 
	}
	else // pop last task in queue, set content there -> -1
	{
		tid = q->content[q->size - 1];
		q->content[q->size--] = -1;
	}

	return tid;
}

Tid pull(PQ* q, Tid tid) 
{	
	Tid rtid = -1;
	int pos;

	for (pos = 0; pos < q->size; pos++) 
	{
		if (q->content[pos] == tid) 
		{
			rtid = q->content[pos];
			q->size--;
			break;
		}
	}
	while (pos < q->size) 
	{
		q->content[pos] = q->content[pos + 1];
		pos++;
	}
	
	return rtid;
}

// put new task into input queue 
// increase the queue's cap if necessary
void put(PQ* q, Tid tid) 
{
	int priority = taskPriority(tid);
	int pos = q->size;

	// if queue size has already reach its cap
	// increase the queue size by making a new one and point to it
	if (q->size == q->cap) 
	{
		Tid* newContent = malloc(sizeof(Tid) * (q->cap + CAP_INCREASE));
		while (pos > -1) 
		{
			if (pos > 0) 
			{
				// if input task has lower priority 
				// add input task to new queue at current pos slot
				// then copy all tasks from old queue's content to new queue
				if (priority > taskPriority(q->content[pos - 1])) 
				{
					newContent[pos] = tid;
					for (pos--; pos > -1; pos--) 
					{
						newContent[pos] = q->content[pos];
					}
					break;
				}
				// else move current task in queue 1 step to the end
				// and vacate that slot
				else 
				{
					newContent[pos] = q->content[pos - 1];
					q->content[pos - 1] = 0;
				}
			}
			else 
			{
				newContent[pos] = tid;
			}
			pos--;
		}
		// free old queue version, set up new one that includes input task in order
		if (q->content != 0) free(q->content);		
		q->content = newContent;
		q->cap += CAP_INCREASE;
	}
	else 
	{
		while (pos > -1) 
		{
			if (pos > 0) 
			{
				// if input task has lower priority just
				// add input task to current position in queue
				if (priority > taskPriority(q->content[pos - 1])) 
				{
					q->content[pos] = tid;
					break;
				}
				// else move current task in queue 1 step to the end
				// and vacate that slot
				else 
				{
					q->content[pos] = q->content[pos - 1];
					q->content[pos - 1] = 0;
				}
			}
			else  // input task has higest priority
			{
				q->content[pos] = tid;
			}
			pos--;
		}
	}
	q->size++;
}

// ready task
// by popping next waiting task from src to dst 
Tid ready(PQ* src, PQ* dst) 
{	
	Tid tid = pop(src);
	if (tid > -1) put(dst, tid);	
	return tid;
}

// block task by moving from src (rQueue) -> dst (s->pq)
Tid block(PQ* src, PQ* dst, Tid tid) 
{	
	Tid rtid = pull(src, tid);
	if (rtid > -1) put(dst, rtid);
	return rtid;
}

void pqprintf(char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	fflush(stdout);
	va_end(args);
}

void printPqueue(PQ* q) {
	pqprintf("********* Priority Queue - %s *********\n", q->name);
	pqprintf("/tcap:  %i\n", q->cap);
	pqprintf("/tsize: %i\n", q->size);
	for (int i = q->size - 1; i > -1; i--) {
		Tid tid = q->content[i];
		pqprintf("PQ[%i]: t: %i  p: %i\n", q->size - i - 1, tid, taskPriority(tid));
	}
	pqprintf("****************************************\n");
}