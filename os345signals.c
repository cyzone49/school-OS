// os345signal.c - signals
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345signals.h"
#include "pq.h"

extern TCB tcb[];							// task control block
extern int curTask;							// current task #

// ***********************************************************************
// ***********************************************************************
//	Call all pending task signal handlers
//
//	return 1 if task is NOT to be scheduled.
//
int signals(void) {
	
	int retVal = 0;
	if (tcb[getCurTask()].signal) 
	{
		if (tcb[getCurTask()].signal & mySIGCONT) 
		{
			tcb[getCurTask()].signal &= ~mySIGCONT;
			(*tcb[getCurTask()].sigContHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGINT) 
		{
			tcb[getCurTask()].signal &= ~mySIGINT;
			(*tcb[getCurTask()].sigIntHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGKILL) 
		{
			tcb[getCurTask()].signal &= ~mySIGKILL;
			(*tcb[getCurTask()].sigKillHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGTERM) 
		{
			tcb[getCurTask()].signal &= ~mySIGTERM;
			(*tcb[getCurTask()].sigTermHandler)();
			retVal = 1;
		}
		if (tcb[getCurTask()].signal & mySIGTSTP) 
		{
			tcb[getCurTask()].signal &= ~mySIGTSTP;
			(*tcb[getCurTask()].sigTstpHandler)();
			retVal = 1;
		}
	}

	return retVal;
}

// **********************************************************************
// **********************************************************************
//	Register task signal handlers
//
int sigAction(void (*sigHandler)(void), int sig)
{
	switch (sig)
	{
		case mySIGCONT:
		{
			tcb[curTask].sigContHandler = sigHandler;		// mySIGCONT handler
			return 0;
		}
		case mySIGINT:
		{
			tcb[curTask].sigIntHandler = sigHandler;		// mySIGINT handler
			return 0;
		}
		case mySIGKILL:
		{
			tcb[curTask].sigKillHandler = sigHandler;		// mySIGKILL handler
			return 0;
		}
		case mySIGTERM:
		{
			tcb[curTask].sigTermHandler = sigHandler;		// mySIGTERM handler
			return 0;
		}
		case mySIGTSTP:
		{
			tcb[curTask].sigTstpHandler = sigHandler;		// mySIGTSTP handler
			return 0;
		}
	}
	return 1;
}


// **********************************************************************
//	sigSignal - send signal to task(s)
//
//	taskId = task (-1 = all tasks)
//	sig = signal
//
int sigSignal(int taskId, int sig)
{
	// check for task
	if ((taskId >= 0) && tcb[taskId].name)
	{	
		tcb[taskId].signal |= sig;
		return 0;
	}
	else if (taskId == -1)
	{
		
		for (taskId=0; taskId<MAX_TASKS; taskId++)
		{
			sigSignal(taskId, sig);
		}
		return 0;
	}
	
	return 1;								// error
}

//	clearSignal - clear signal for task(s)
//	by (taskId) or (-1 = all tasks)
int clearSignal(int taskId, int sig) {
	// check for task
	if ((taskId >= 0) && tcb[taskId].name) {		
		tcb[taskId].signal &= ~sig;
		return 0;
	}
	else if (taskId == -1)
	{
		for (taskId = 0; taskId < MAX_TASKS; taskId++) 
		{
			clearSignal(taskId, sig);
		}
		return 0;
	}
	// error
	return 1;
}


// **********************************************************************
// **********************************************************************
//	Default signal handlers
//

void defaultSigContHandler(void)			// task mySIGCONT handler
{
	return;
}

void defaultSigIntHandler(void)				// task mySIGINT handler
{
	sigSignal(-1, mySIGTERM);
	return;
}

void defaultSigKillHandler(void)			// task mySIGKILL handler
{
	return;
}

void defaultSigTermHandler(void)			// task mySIGTERM handler
{
	killTask(getCurTask());
	return;
}

void defaultSigTstpHandler(void)			// task mySIGTSTP handler
{
	sigSignal(-1, mySIGSTOP);
	return;
}

void createTaskSigHandlers(int tid) {

	tcb[tid].signal = 0;
	if (tid) 
	{
		// inherit parent signal handlers
		tcb[tid].sigContHandler = tcb[getCurTask()].sigContHandler;// mySIGCONT handler
		tcb[tid].sigIntHandler = tcb[getCurTask()].sigIntHandler;// mySIGINT handler
		tcb[tid].sigKillHandler = tcb[getCurTask()].sigKillHandler;// mySIGKILL handler
		tcb[tid].sigTermHandler = tcb[getCurTask()].sigTermHandler;// mySIGTERM handler
		tcb[tid].sigTstpHandler = tcb[getCurTask()].sigTstpHandler;// mySIGTSTP handler

	}
	else 
	{
		// otherwise use defaults
		tcb[tid].sigContHandler = defaultSigContHandler;// task mySIGCONT handler
		tcb[tid].sigIntHandler = defaultSigIntHandler;	// task mySIGINT handler
		tcb[tid].sigKillHandler = defaultSigKillHandler;// task mySIGKILL handler
		tcb[tid].sigTstpHandler = defaultSigTstpHandler;// task mySIGTSTP handler
		tcb[tid].sigTermHandler = defaultSigTermHandler;// task mySIGTERM handler
	}
}
