#pragma once

#ifndef OS345UTILS_H_
#define OS345UTILS_H_

#include "os345.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

const int maxRecallStackSize = 10;

extern TCB tcb[];
extern DeltaClock* dcFront;
extern Semaphore* dcMutex;

char* popStringStack(StringStack* ss) {
	if (ss->top == -1)
		return 0;
	char* tempptr = ss->stackVals[ss->top];
	ss->top -= 1;
	return tempptr;


}

int pushStringStack(StringStack* ss, char* chp) {

	//remake the stack
	if ((ss->top + 1) == maxRecallStackSize) {
		int i;
		char** nArr = (char**)malloc(sizeof(char*) * maxRecallStackSize);
		for (i = 1; i < maxRecallStackSize; i++) {
			char* tempptr = (char*)malloc(((strlen(ss->stackVals[i])) + 1) * sizeof(char));
			strcpy(tempptr, ss->stackVals[i]);
			nArr[i - 1] = tempptr;
		}
		//clean up after ourselves
		for (i = 0; i < maxRecallStackSize - 1; i++) {
			free(ss->stackVals[i]);
		}
		char* tempptr = (char*)malloc((sizeof(ss->stackVals[i]) + 1) * sizeof(char));
		strcpy(tempptr, chp);
		nArr[maxRecallStackSize - 1] = tempptr;
		ss->stackVals = nArr;
	}
	else {
		ss->top += 1;
		strcpy(ss->stackVals[ss->top] = (char*)malloc(((strlen(chp) + 1) * sizeof(char))), chp);
	}




	return 0;
}

StringStack* initStringStack() {
	StringStack* ss = (StringStack*)malloc(sizeof(StringStack));
	ss->top = -1;
	ss->stackVals = (char**)malloc(sizeof(char*) * maxRecallStackSize);
	return ss;
}

int destroyStringStack(StringStack* ss) {
	int i;
	for (i = 0; i < maxRecallStackSize; i++) {
		if (ss->stackVals[i] != 0)
			free(ss->stackVals[i]);
	}
	free(ss->stackVals);
	ss = 0;
	return 0;
}

int enQ(PQueue q, Tid tid, Priority p) {
	//if there are more tasks than we can handle, say so!
	if (q[0] >= MAX_TASKS) {
		return -1;
	}
	int i;
	//cover the case that there's not anything in the queue
	if (q[0] == 0) {
		q[1] = tid;
		q[0]++;
		return 1;
	}

	for (i = 1; i <= q[0] + 1; i++) {

		//cover the case that this task is lower priority than any of the
				//others in the queue
		if (i == q[0] + 1) {
			q[i] = tid;
			q[0]++;
			return i;
		}
		else if (tcb[q[i]].priority < p) {
			int j;
			//move all of the lower priority items back
			for (j = (q[0] + 1); j > (i); j--) {
				q[j] = q[j - 1];
			}
			q[i] = tid;
			q[0]++;
			return i;
		}

	}

	return -1;
}

int deQ(PQueue q, Tid tid) {
	int i;

	//if the queue is empty, signal it!
	if (q[0] == 0) {
		return -1;
	}
	else if (tid == -1) {
		int retInt = q[1];
		//move all of the tasks in the queue one down
		for (i = 1; i <= (q[0] - 1); i++) {
			q[i] = q[i + 1];
		}
		q[q[0]] = 0;
		q[0]--;
		return retInt;
	}
	else {
		for (i = 1; i <= q[0]; i++) {
			//if the specified task is found, move all the others down
			if (q[i] == tid) {
				int retInt = q[i];
				int j;

				for (j = i; j <= (q[0] - 1); j++) {
					q[j] = q[j + 1];
				}
				q[q[0]] = 0;
				q[0]--;

				return retInt;
			}
		}
	}
	//if the task was not found in the queue
	return -1;
}

PQueue initQ() {
	PQueue nq;
	nq = (int*)malloc(MAX_TASKS * sizeof(int));
	nq[0] = 0;
	return nq;
}

int destroyQ(PQueue q) {
	free(q);
	return 0;
}


int destroyDC(DeltaClock* dc) {

	while (dc) {
		DeltaClock* dcBefore = dc;
		dc = dc->next;
		free(dcBefore);
	}

	return 1;
}

int incrementDC(int tics) {
	int rem = tics;
	DeltaClock* currClk = dcFront;
	int finished = 0;
	while (!finished) {

		if (currClk == 0) {
			break;
		}

		currClk->tics = currClk->tics - rem;
		if (currClk->tics <= 0) {
			//there is no time left on this clock.
			//free this clock and decrement the next one
			rem = currClk->tics * -1;
			DeltaClock* prevClk = currClk;

			currClk = currClk->next;

			//advance the front of the clock
			dcFront = currClk;
			semSignal(prevClk->sem);
			free(prevClk);

		}
		else {
			//there is still time on this clock; keep it
			finished = 1;
		}
	}
	return 1;

}

int insertDC(int tics, Semaphore* sem) {
	semWait(dcMutex);
	DeltaClock* dc = dcFront; SWAP;


	DeltaClock* dcNew = (DeltaClock*)malloc(sizeof(DeltaClock)); SWAP;
	dcNew->sem = sem; SWAP;
	dcNew->tics = tics; SWAP;


	if (!dc) {
		SWAP;
		dcNew->next = dcFront; SWAP;
		//Set front
		dcFront = dcNew; SWAP;
		semSignal(dcMutex); SWAP;
		return 1; SWAP;
	}

	DeltaClock* dcBefore = 0; SWAP;

	while (dc) {
		SWAP;
		if (dcNew->tics < dc->tics) {
			SWAP;
			dcNew->next = dc; SWAP;
			if (dcBefore) dcBefore->next = dcNew;
			else dcFront = dcNew; SWAP;

			//update the count on each dc node after this one
			dc->tics = dc->tics - dcNew->tics; SWAP;
			dc = dc->next; SWAP;

			semSignal(dcMutex); SWAP;
			return 1; SWAP;
		}
		else {
			dcNew->tics = dcNew->tics - dc->tics; SWAP;
			dcBefore = dc; SWAP;
			dc = dc->next; SWAP;
		}
	}

	dcNew->next = 0; SWAP;
	if (dcBefore != 0) dcBefore->next = dcNew; SWAP;

	semSignal(dcMutex); SWAP;

	return 1;
}



#endif /* OS345UTILS_H_ */
const int maxRecallStackSize = 10;