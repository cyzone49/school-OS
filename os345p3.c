// os345p3.c - Jurassic Park
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>


#include "os345.h"
#include "os345park.h"
#include "dc.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over


// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);

void sendLock(Semaphore* lock, Semaphore* sem);
Semaphore* getLock(Semaphore* lock);

void vWaitPark();
void vEnterPark();
void vWaitMuseum();
void vEnterMuseum();
void vWaitRide();
void vEnterRide();
void vWaitGiftShop();
void vEnterGiftShop();
void vLeavePark();

void driverSellTicket(int id);
void driverSleep(int id);
void driverDrive(int id, int carId);

int driver(int argc, char* argv[]);
int visitor(int argc, char* argv[]);
int car(int argc, char* argv[]);

int printPark(int argc, char* argv[]);
//void pnprintf(const char* fmt, ...);
//void ptprintf(const char* fmt, ...);

void waitRandom(int max, Semaphore* lock);

// ***********************************************************************
// ***********************************************************************
// project3 semaphores
Semaphore* mailMutex;				// mailbox protection
Semaphore* mailSent;				// mailbox sent flag
Semaphore* mailAcquired;			// mailbox received flag
Semaphore* mail;					// mailbox contents

Semaphore* parkOccupancy;			// park occupancy resource
Semaphore* museumOccupancy;			// museum occupancy resource
Semaphore* rideTicket;				// ride ticket resource
Semaphore* giftShopOccupancy;		// gift shop occupancy resource

Semaphore* rideTicketBoothMutex;	// mutex - ticket booth 
Semaphore* workerNeeded;			// flag - worker needed
Semaphore* rideTicketSellerNeeded;	// flag - ticket seller needed
Semaphore* rideTicketSold;			// flag - ticket sold
Semaphore* rideTicketBought;		// flag - ticket bought
Semaphore* rideDriverNeeded;		// flag - driver needed
Semaphore* rideSeatTaken;			// flag - ride seat taken 
Semaphore* rideSeatOpen;			// flag - ride seat open 
Semaphore* rideDriverSeatTaken;		// flag - driver seat taken 

Semaphore* departingCar;			// lock for mailbox
Semaphore* departingCarDriver;		// lock for mailbox

Semaphore* randSem;					// Extra random semaphore

extern Semaphore* tics10thsec;

Memo* Dclock;



// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf[32];													SWAP;
	char buf2[32];													SWAP;
	char* newArgv[2];												SWAP;

	// start park
	sprintf(buf, "jurassicPark");									SWAP;
	newArgv[0] = buf;												SWAP;
	createTask(buf,														// task name
		jurassicTask,													// task
		MED_PRIORITY,													// task priority
		1,																// task count
		newArgv);													SWAP;	// task argument				

	// wait for park to get initialized...
	//while (!parkMutex)				SWAP;
	printf("\nStart Jurassic Park...");


	//?? create car, driver, and visitor tasks here

	// create necessary semaphores	
	randSem = createSemaphore("Random Semaphore", COUNTING, 0);										SWAP;
	mailMutex = createSemaphore("Mail Mutex", BINARY, 1);											SWAP;
	mailSent = createSemaphore("Mail Sent Flag", BINARY, 0);										SWAP;
	mailAcquired = createSemaphore("Mail Acquired Flag", BINARY, 0);								SWAP;

	parkOccupancy = createSemaphore("Park Occupancy Resource", COUNTING, MAX_IN_PARK);				SWAP;
	museumOccupancy = createSemaphore("Museum Occupancy Resource", COUNTING, MAX_IN_MUSEUM);		SWAP;
	rideTicket = createSemaphore("Ride Ticket Resource", COUNTING, MAX_TICKETS); SWAP;
	giftShopOccupancy = createSemaphore("Gift Shop Occupancy Resource", COUNTING, MAX_IN_GIFTSHOP); SWAP;

	workerNeeded = createSemaphore("Worker Needed Flag", COUNTING, 0);								SWAP;
	rideTicketBoothMutex = createSemaphore("Ticket Booth Mutex", BINARY, 1);						SWAP;
	rideTicketSellerNeeded = createSemaphore("Ride Ticket Seller Needed Flag", COUNTING, 0);		SWAP;
	rideTicketBought = createSemaphore("Ride Ticket Bought Flag", COUNTING, 0);						SWAP;
	rideTicketSold = createSemaphore("Ride Ticket Sold Flag", COUNTING, 0);							SWAP;
	rideSeatOpen = createSemaphore("Ride Seat Open Resource", COUNTING, 0);							SWAP;
	rideSeatTaken = createSemaphore("Ride Seat Taken", COUNTING, 0);								SWAP;
	rideDriverNeeded = createSemaphore("Ride Driver Needed Flag", COUNTING, 0);						SWAP;
	rideDriverSeatTaken = createSemaphore("Ride Driver Seat Taken", COUNTING, 0);					SWAP;

	departingCar = createSemaphore("MBL Departing Car", COUNTING, 0);								SWAP;
	departingCarDriver = createSemaphore("MBL Departing Car Driver", COUNTING, 0);					SWAP;

	while (!parkMutex) SWAP;																		SWAP;

	printf("\nStart Jurassic Park...");																SWAP;

	// create car tasks
	for (int i = 0; i < NUM_CARS; i++)
	{
		sprintf(buf, "Car%i", i);									SWAP;
		newArgv[0] = buf;											SWAP;
		sprintf(buf2, "%i", i);										SWAP;
		newArgv[1] = buf2;											SWAP;
		createTask(buf, car, MED_PRIORITY, 2, newArgv);				SWAP;
	}

	// create driver tasks
	for (int i = 0; i < NUM_DRIVERS; i++)
	{
		sprintf(buf, "Driver%i", i);								SWAP;
		newArgv[0] = buf;											SWAP;
		sprintf(buf2, "%i", i);										SWAP;
		newArgv[1] = buf2;											SWAP;
		createTask(buf, driver, MED_PRIORITY, 2, newArgv);			SWAP;
	}

	// create visitor tasks
	for (int i = 0; i < NUM_VISITORS; i++)
	{
		sprintf(buf, "Visitor%i", i);								SWAP;
		newArgv[0] = buf;											SWAP;
		sprintf(buf2, "%i", i);										SWAP;
		newArgv[1] = buf2;											SWAP;
		createTask(buf, visitor, MED_PRIORITY, 2, newArgv);			SWAP;
	}

	// create delta clock task
	sprintf(buf, "Delta Clock");									SWAP;
	newArgv[0] = buf;												SWAP;
	createTask(buf, P3_dc, MED_PRIORITY, 1, newArgv);				SWAP;

	return 0;
} // end project3


// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents
	while (1)
	{
		semWait(tics10thsec);										SWAP;
		tick(&Dclock);												SWAP;
	}
	return 1;
} // end CL3_dc



// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
	printf("\n******* Delta Clock *******");						SWAP;
	Memo* next = Dclock;											SWAP;
	while (next)
	{
		printf("/n  %s", next->event->name);						SWAP;
		next = next->next;											SWAP;
	}
	return;
}

// ***********************************************************************
// ***********************************************************************
// car task
int car(int argc, char* argv[])
{
	char buf[32];													SWAP;

	int carId = atoi(argv[1]);										SWAP;
	sprintf(buf, "Car%i Seat Belt", carId);							SWAP;
	Semaphore* seatBelt = createSemaphore(buf, COUNTING, 0);		SWAP;

	sprintf(buf, "Car%i Seat Belt Driver", carId);					SWAP;
	Semaphore* driverSeatBelt = createSemaphore(buf, COUNTING, 0);	SWAP;


	while (1)
	{
		// acquire visitors
		for (int i = 0; i < NUM_SEATS; i++)
		{
			// wait for the seat
			semWait(fillSeat[carId]);								SWAP;		// seat ready to fill

			// signal for a visitor
			semSignal(rideSeatOpen);								SWAP;		// ride seat iopen for visitor

			// wait for the visitor to reply
			semWait(rideSeatTaken);									SWAP;		// ride seat taken 			
			semSignal(seatFilled[carId]);							SWAP;		// seat filled with visitor
		}

		// acquire a driver		
		semSignal(rideDriverNeeded);								SWAP;		// ride driver needed		
		semSignal(workerNeeded);									SWAP;		// worker needed		
		semWait(rideDriverSeatTaken);								SWAP;		// ride driver seat is taken

		// start ride -> ride over		
		sendLock(departingCar, seatBelt);							SWAP;		// 1st visitor ready to depart		
		sendLock(departingCar, seatBelt);							SWAP;		// 2nd visitor ready to depart		
		sendLock(departingCar, seatBelt);							SWAP;		// 3rd visitor ready to depart		
		sendLock(departingCarDriver, driverSeatBelt);				SWAP;		// ride driver ready to depart		
		semWait(rideOver[carId]);									SWAP;		// wait until ride is over

		// ride over. release driver/visitors		
		semSignal(driverSeatBelt);									SWAP;		// ride driver released		
		semSignal(seatBelt);										SWAP;		// 1st visitor released		
		semSignal(seatBelt);										SWAP;		// 2nd visitor released		
		semSignal(seatBelt);										SWAP;		// 3rd visitor released									SWAP;
	}
	return 0;
} // end car task

// driver task
int driver(int argc, char* argv[])
{
	int id = atoi(argv[1]);											SWAP;
	while (1)
	{
		// sleep. wait for work
		driverSleep(id);											SWAP;
		//ptprintf("\n%10s -| workerNeeded", argv[0]);				SWAP;
		semWait(workerNeeded);										SWAP;

		if (semTryLock(rideTicketSellerNeeded))						// ticket seller needed
		{
			semWait(rideTicketBoothMutex);							SWAP;
			driverSellTicket(id);									SWAP;
			//ptprintf("\n%10s -> rideTicketSold", argv[0]);			SWAP;
			semSignal(rideTicketSold);								SWAP;
			//ptprintf("\n%10s -| rideTicketBought", argv[0]);		SWAP;
			semWait(rideTicketBought);								SWAP;
			semSignal(rideTicketBoothMutex);						SWAP;
		}
		else if (semTryLock(rideDriverNeeded))						// driver needed
		{
			//ptprintf("\n%10s -> rideDriverSeatTaken", argv[0]);		SWAP;
			semSignal(rideDriverSeatTaken);							SWAP;
			Semaphore* seatbelt = getLock(departingCarDriver);		SWAP;

			// pull car id from the semaphore name
			driverDrive(id, atoi(seatbelt->name + 3));				SWAP;

			//ptprintf("\n%10s -| seatbelt", argv[0]);				SWAP;
			semWait(seatbelt);										SWAP;
		}
		else														// you're fired ?
		{
			//ptprintf("\n%10s *** FIRED! ***", argv[0]);				SWAP;
		}
	}
	return 0;
} // end driver task


// ***********************************************************************
// ***********************************************************************
// visitor task
int visitor(int argc, char* argv[])
{
	// visitor wait at the park gate
	waitRandom(50, randSem);
	vWaitPark();												SWAP;
	//ptprintf("\n%10s -| parkOccupancy", argv[0]);				SWAP;
	semWait(parkOccupancy);										SWAP;

	// visitor enter park
	vEnterPark();												SWAP;
	waitRandom(30, randSem);
	//ptprintf("\n%10s -| rideTicket", argv[0]);					SWAP;
	semWait(rideTicket);										SWAP;
	//ptprintf("\n%10s -> rideTicketSellerNeeded", argv[0]);		SWAP;
	semSignal(rideTicketSellerNeeded);							SWAP;
	//ptprintf("\n%10s -> workerNeeded", argv[0]);				SWAP;
	semSignal(workerNeeded);									SWAP;
	//ptprintf("\n%10s -| rideTicketSold", argv[0]);				SWAP;
	semWait(rideTicketSold);									SWAP;
	//ptprintf("\n%10s -> rideTicketBought", argv[0]);			SWAP;
	semSignal(rideTicketBought);								SWAP;

	// visitor wait for museum then enter
	waitRandom(30, randSem);
	vWaitMuseum();												SWAP;
	//ptprintf("\n%10s -| museumOccupancy", argv[0]);				SWAP;
	semWait(museumOccupancy);									SWAP;
	vEnterMuseum();												SWAP;

	// visitor wait for ride then enter
	waitRandom(30, randSem);
	vWaitRide();												SWAP;
	//ptprintf("\n%10s -> museumOccupancy", argv[0]);				SWAP;
	semSignal(museumOccupancy);									SWAP;
	//ptprintf("\n%10s -| rideSeatOpen", argv[0]);				SWAP;
	semWait(rideSeatOpen);										SWAP;
	vEnterRide();												SWAP;
	//ptprintf("\n%10s -> rideTicket", argv[0]);					SWAP;
	semSignal(rideTicket);										SWAP;
	//ptprintf("\n%10s -> rideSeatTaken", argv[0]);				SWAP;
	semSignal(rideSeatTaken);									SWAP;
	//ptprintf("\n%10s -| departingCar", argv[0]);				SWAP;
	semWait(getLock(departingCar));								SWAP;

	// visitor wait for gift shop, then enter
	waitRandom(30, randSem);
	vWaitGiftShop();											SWAP;
	//ptprintf("\n%10s -| giftShopOccupancy", argv[0]);			SWAP;
	semWait(giftShopOccupancy);									SWAP;
	vEnterGiftShop();											SWAP;

	// visitor leave the park
	waitRandom(30, randSem);
	vLeavePark();												SWAP;
	//ptprintf("\n%10s -> giftShopOccupancy", argv[0]);			SWAP;
	semSignal(giftShopOccupancy);								SWAP;
	//ptprintf("\n%10s -> parkOccupancy", argv[0]);				SWAP;
	semSignal(parkOccupancy);									SWAP;

	return 0;
} // end visitor task

// ***********************************************************************
// ***********************************************************************
// tasks functions
void vWaitPark()
{
	semWait(parkMutex);														SWAP;
	
	myPark.numOutsidePark++;												SWAP;
	semSignal(parkMutex);													SWAP;
}
void vEnterPark()
{
	semWait(parkMutex);														SWAP;	
	myPark.numOutsidePark--;												SWAP;
	myPark.numInPark++;														SWAP;
	myPark.numInTicketLine++;												SWAP;
	semSignal(parkMutex);													SWAP;
}
void vWaitMuseum()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInTicketLine--;												SWAP;
	myPark.numInMuseumLine++;												SWAP;
	semSignal(parkMutex);													SWAP;
}
void vEnterMuseum()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInMuseumLine--;												SWAP;
	myPark.numInMuseum++;													SWAP;
	semSignal(parkMutex);													SWAP;
}
void vWaitRide()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInMuseum--;													SWAP;
	myPark.numInCarLine++;													SWAP;
	semSignal(parkMutex);													SWAP;
}
void vEnterRide()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInCarLine--;													SWAP;
	myPark.numInCars++;														SWAP;
	semSignal(parkMutex);													SWAP;
}
void vWaitGiftShop()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInCars--;														SWAP;
	myPark.numRidesTaken++;													SWAP;
	myPark.numInGiftLine++;													SWAP;
	semSignal(parkMutex);													SWAP;
}
void vEnterGiftShop()
{
	semWait(parkMutex);														SWAP;	
	myPark.numInGiftLine--;													SWAP;
	myPark.numInGiftShop++;													SWAP;
	semSignal(parkMutex);													SWAP;
}
void vLeavePark() {
	semWait(parkMutex);														SWAP;	
	myPark.numInGiftShop--;													SWAP;
	myPark.numInPark--;														SWAP;
	myPark.numExitedPark++;													SWAP;
	semSignal(parkMutex);													SWAP;
}

void driverSellTicket(int id)
{
	semWait(parkMutex);														SWAP;	
	myPark.drivers[id] = -1;												SWAP;
	semSignal(parkMutex);													SWAP;
}
void driverSleep(int id)
{
	semWait(parkMutex);														SWAP;	
	myPark.drivers[id] = 0;													SWAP;
	semSignal(parkMutex);													SWAP;
}
void driverDrive(int id, int carId)
{
	semWait(parkMutex);														SWAP;	
	myPark.drivers[id] = carId + 1;											SWAP;
	semSignal(parkMutex);													SWAP;
}


// ***********************************************************************
// ***********************************************************************
// peripherals
void sendLock(Semaphore* lock, Semaphore* sem)
{
	// pass semaphore to car (1 at a time)	
	semWait(mailMutex);													SWAP;		// wait for mailbox	
	semWait(lock);														SWAP;		// wait for passenger request	
	mail = sem;															SWAP;		// put semaphore in mailbox	
	semSignal(mailSent);												SWAP;		// raise the mailbox flag	
	semWait(mailAcquired);												SWAP;		// wait for delivery	
	semSignal(mailMutex);												SWAP;		// release mailbox
}

Semaphore* getLock(Semaphore* lock)
{
	// get passenger semaphore	
	semSignal(lock);													SWAP;	
	semWait(mailSent);													SWAP;		// wait for mail	
	Semaphore* ret = mail;												SWAP;		// get mail	
	semSignal(mailAcquired);											SWAP;		// put flag down
	return ret;
}
//
//void pnprintf(const char* fmt, ...)
//{
//	va_list args;								SWAP;
//	va_start(args, fmt);						SWAP;
//	//vprintf(fmt, args); SWAP;
//	fflush(stdout);								SWAP;
//	va_end(args);								SWAP;
//}
//void ptprintf(const char* fmt, ...)
//{
//	va_list args;								SWAP;
//	va_start(args, fmt);						SWAP;
//	//vprintf(fmt, args); SWAP;
//	fflush(stdout);								SWAP;
//	va_end(args);								SWAP;
//}

void waitRandom(int max, Semaphore* lock)
{
	insert(&Dclock, newMemo(rand() % max, lock));
	semWait(lock);
}

// ***********************************************************************
// ***********************************************************************
// print park
int printPark(int argc, char* argv[])
{
	semWait(parkMutex);							SWAP;
	semSignal(parkMutex);						SWAP;
	return 0;
} // end print park

extern Semaphore* tics1sec;

// ****************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec)
			printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask


//// ***********************************************************************
//// monitor the delta clock task
//int dcMonitorTask(int argc, char* argv[])
//{
//	int i, flg;
//	char buf[32];
//	// create some test times for event[0-9]
//	int ttime[10] = {
//		90, 300, 50, 170, 340, 300, 50, 300, 40, 110 };
//
//	for (i = 0; i < 10; i++)
//	{
//		sprintf(buf, "event[%d]", i);
//		event[i] = createSemaphore(buf, BINARY, 0);
//		insertDeltaClock(ttime[i], event[i]);
//	}
//	printDeltaClock();
//
//	while (numDeltaClock > 0)
//	{
//		SEM_WAIT(dcChange)
//			flg = 0;
//		for (i = 0; i < 10; i++)
//		{
//			if (event[i]->state == 1) {
//				printf("\n  event[%d] signaled", i);
//				event[i]->state = 0;
//				flg = 1;
//			}
//		}
//		if (flg) printDeltaClock();
//	}
//	printf("\nNo more events in Delta Clock");
//
//	// kill dcMonitorTask
//	tcb[timeTaskID].state = S_EXIT;
//	return 0;
//} // end dcMonitorTask



// ***********************************************************************
// test delta clock
//int P3_tdc(int argc, char* argv[])
//{
//	createTask( "DC Test",			// task name
//		dcMonitorTask,		// task
//		10,					// task priority
//		argc,					// task arguments
//		argv);
//
//	timeTaskID = createTask( "Time",		// task name
//		timeTask,	// task
//		10,			// task priority
//		argc,			// task arguments
//		argv);
//	return 0;
//} // end P3_tdc
