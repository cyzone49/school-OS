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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "dc.h"
#include "os345.h"
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;				// protect park access
extern Semaphore* fillSeat[NUM_CARS];		// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];		// (signal) ride over

void visWaitPark();
void visEnterPark();
void visWaitMuseum();
void visEnterMuseum();
void visWaitRide();
void visEnterRide();
void visWaitGiftShop();
void visEnterGiftShop();
void visLeavePark();

void driSellTicket(int id);
void driSleep(int id);
void driDrive(int id, int carId);

void sendLock(Semaphore* lock, Semaphore* sem);
Semaphore* getLock(Semaphore* lock);

int driver(int argc, char* argv[]);
int visitor(int argc, char* argv[]);
int car(int argc, char* argv[]);
void waitRandom(int max, Semaphore* lock);

int printPark(int argc, char* argv[]);

void pnprintf(const char* fmt, ...);
void ptprintf(const char* fmt, ...);

Semaphore* randSem;					// Extra random semaphore

Semaphore* mailMutex;				// mailbox protection
Semaphore* mailSent;				// mailbox sent flag
Semaphore* mailAcquired;			// mailbox received flag
Semaphore* mail;					// mailbox contents

Semaphore* parkOccupancy;			// park occupancy resource
Semaphore* museumOccupancy;			// museum occupancy resource
Semaphore* rideTicket;				// ride ticket resource
Semaphore* giftShopOccupancy;		// gift shop occupancy resource

Semaphore* workerNeeded;			// worker needed flag
Semaphore* rideTicketBoothMutex;	// ticket booth mutex
Semaphore* rideTicketSellerNeeded;	// ticket seller needed flag
Semaphore* rideTicketSold;			// ticket sold flag
Semaphore* rideTicketBought;		// ticket bought flag
Semaphore* rideDriverNeeded;		// driver needed flag
Semaphore* rideSeatTaken;			// ride seat taken flag
Semaphore* rideSeatOpen;			// ride seat open flag
Semaphore* rideDriverSeatTaken;		// driver seat taken flag

Semaphore* departingCar;			// lock for mailbox
Semaphore* departingCarDriver;		// lock for mailbox

extern Semaphore* tics10thsec;

Memo* Dclock;

// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[]) {
	char buf[32]; SWAP;
	char buf2[32]; SWAP;
	char* newArgv[2]; SWAP;

	// start park
	sprintf(buf, "jurassicPark"); SWAP;
	newArgv[0] = buf; SWAP;
	createTask(buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,							// task count
		newArgv); SWAP;					// task argument

// wait for park to get initialized...

	randSem = createSemaphore("Random Semaphore", COUNTING, 0); SWAP;

	mailMutex = createSemaphore("Mail Mutex", BINARY, 1); SWAP;
	mailSent = createSemaphore("Mail Sent Flag", BINARY, 0); SWAP;
	mailAcquired = createSemaphore("Mail Acquired Flag", BINARY, 0); SWAP;

	parkOccupancy = createSemaphore("Park Occupancy Resource", COUNTING, MAX_IN_PARK); SWAP;
	museumOccupancy = createSemaphore("Museum Occupancy Resource", COUNTING, MAX_IN_MUSEUM); SWAP;
	rideTicket = createSemaphore("Ride Ticket Resource", COUNTING, MAX_TICKETS); SWAP;
	giftShopOccupancy = createSemaphore("Gift Shop Occupancy Resource", COUNTING, MAX_IN_GIFTSHOP); SWAP;

	workerNeeded = createSemaphore("Worker Needed Flag", COUNTING, 0); SWAP;
	rideTicketBoothMutex = createSemaphore("Ticket Booth Mutex", BINARY, 1); SWAP;
	rideTicketSellerNeeded = createSemaphore("Ride Ticket Seller Needed Flag", COUNTING, 0); SWAP;
	rideTicketBought = createSemaphore("Ride Ticket Bought Flag", COUNTING, 0); SWAP;
	rideTicketSold = createSemaphore("Ride Ticket Sold Flag", COUNTING, 0); SWAP;
	rideSeatOpen = createSemaphore("Ride Seat Open Resource", COUNTING, 0); SWAP;
	rideSeatTaken = createSemaphore("Ride Seat Taken", COUNTING, 0); SWAP;
	rideDriverNeeded = createSemaphore("Ride Driver Needed Flag", COUNTING, 0); SWAP;
	rideDriverSeatTaken = createSemaphore("Ride Driver Seat Taken", COUNTING, 0); SWAP;

	departingCar = createSemaphore("MBL Departing Car", COUNTING, 0); SWAP;
	departingCarDriver = createSemaphore("MBL Departing Car Driver", COUNTING, 0); SWAP;

	while (!parkMutex) SWAP; SWAP;

	printf("\nStart Jurassic Park..."); SWAP;

	for (int i = 0; i < NUM_CARS; i++) {
		sprintf(buf, "Car%i", i); SWAP;
		newArgv[0] = buf; SWAP;
		sprintf(buf2, "%i", i); SWAP;
		newArgv[1] = buf2; SWAP;
		createTask(buf, car, MED_PRIORITY, 2, newArgv); SWAP;
	}

	for (int i = 0; i < NUM_DRIVERS; i++) {
		sprintf(buf, "Driver%i", i); SWAP;
		newArgv[0] = buf; SWAP;
		sprintf(buf2, "%i", i); SWAP;
		newArgv[1] = buf2; SWAP;
		createTask(buf, driver, MED_PRIORITY, 2, newArgv); SWAP;
	}

	for (int i = 0; i < NUM_VISITORS; i++) {
		sprintf(buf, "Visitor%i", i); SWAP;
		newArgv[0] = buf; SWAP;
		sprintf(buf2, "%i", i); SWAP;
		newArgv[1] = buf2; SWAP;
		createTask(buf, visitor, MED_PRIORITY, 2, newArgv); SWAP;
	}

	sprintf(buf, "Delta Clock"); SWAP;
	newArgv[0] = buf; SWAP;
	createTask(buf, P3_dc, MED_PRIORITY, 1, newArgv); SWAP;

	return 0;
} // end project3

// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[]) {
	while (1) {
		semWait(tics10thsec); SWAP;
		tick(&Dclock); SWAP;
	}
	return 1;
} // end CL3_dc

// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void) {
	printf("\n*** Delta Clock ***"); SWAP;
	Memo* next = Dclock; SWAP;
	while (next) {
		printf("/n  %s", next->event->name); SWAP;
		next = next->next; SWAP;
	}
	return;
}

// ***********************************************************************
// driver task
int driver(int argc, char* argv[]) {
	int id = atoi(argv[1]); SWAP;
	while (1) {
		driSleep(id); SWAP;
		ptprintf("\n%10s -| workerNeeded", argv[0]); SWAP;
		semWait(workerNeeded); SWAP;

		if (semTryLock(rideTicketSellerNeeded)) {
			semWait(rideTicketBoothMutex); SWAP;
			driSellTicket(id); SWAP;
			ptprintf("\n%10s -> rideTicketSold", argv[0]); SWAP;
			semSignal(rideTicketSold); SWAP;
			ptprintf("\n%10s -| rideTicketBought", argv[0]); SWAP;
			semWait(rideTicketBought); SWAP;
			semSignal(rideTicketBoothMutex); SWAP;
		}
		else if (semTryLock(rideDriverNeeded)) {
			ptprintf("\n%10s -> rideDriverSeatTaken", argv[0]); SWAP;
			semSignal(rideDriverSeatTaken); SWAP;
			Semaphore* seatbelt = getLock(departingCarDriver); SWAP;
			driDrive(id, atoi(seatbelt->name + 3)); SWAP;	// hacky-hack, pulling the car id from the semaphore name
			ptprintf("\n%10s -| seatbelt", argv[0]); SWAP;
			semWait(seatbelt); SWAP;
		}
		else {
			ptprintf("\n%10s *** PISSED ***", argv[0]); SWAP;
		}
	}
	return 0;
} // end driver task

// ***********************************************************************
// visitor task
int visitor(int argc, char* argv[]) {
	waitRandom(50, randSem);
	visWaitPark(); SWAP;
	ptprintf("\n%10s -| parkOccupancy", argv[0]); SWAP;
	semWait(parkOccupancy); SWAP;

	visEnterPark(); SWAP;
	waitRandom(30, randSem);
	ptprintf("\n%10s -| rideTicket", argv[0]); SWAP;
	semWait(rideTicket); SWAP;
	ptprintf("\n%10s -> rideTicketSellerNeeded", argv[0]); SWAP;
	semSignal(rideTicketSellerNeeded); SWAP;
	ptprintf("\n%10s -> workerNeeded", argv[0]); SWAP;
	semSignal(workerNeeded); SWAP;
	ptprintf("\n%10s -| rideTicketSold", argv[0]); SWAP;
	semWait(rideTicketSold); SWAP;
	ptprintf("\n%10s -> rideTicketBought", argv[0]); SWAP;
	semSignal(rideTicketBought); SWAP;

	waitRandom(30, randSem);
	visWaitMuseum(); SWAP;
	ptprintf("\n%10s -| museumOccupancy", argv[0]); SWAP;
	semWait(museumOccupancy); SWAP;
	visEnterMuseum(); SWAP;

	waitRandom(30, randSem);
	visWaitRide(); SWAP;
	ptprintf("\n%10s -> museumOccupancy", argv[0]); SWAP;
	semSignal(museumOccupancy); SWAP;
	ptprintf("\n%10s -| rideSeatOpen", argv[0]); SWAP;
	semWait(rideSeatOpen); SWAP;
	visEnterRide(); SWAP;
	ptprintf("\n%10s -> rideTicket", argv[0]); SWAP;
	semSignal(rideTicket); SWAP;
	ptprintf("\n%10s -> rideSeatTaken", argv[0]); SWAP;
	semSignal(rideSeatTaken); SWAP;
	ptprintf("\n%10s -| departingCar", argv[0]); SWAP;
	semWait(getLock(departingCar)); SWAP;

	waitRandom(30, randSem);
	visWaitGiftShop(); SWAP;
	ptprintf("\n%10s -| giftShopOccupancy", argv[0]); SWAP;
	semWait(giftShopOccupancy); SWAP;
	visEnterGiftShop(); SWAP;

	waitRandom(30, randSem);
	visLeavePark(); SWAP;
	ptprintf("\n%10s -> giftShopOccupancy", argv[0]); SWAP;
	semSignal(giftShopOccupancy); SWAP;
	ptprintf("\n%10s -> parkOccupancy", argv[0]); SWAP;
	semSignal(parkOccupancy); SWAP;

	return 0;
} // end visitor task

// ***********************************************************************
// car task
int car(int argc, char* argv[]) {
	char buf[32]; SWAP;
	int carId = atoi(argv[1]); SWAP;
	sprintf(buf, "Car%i Seat Belt", carId); SWAP;
	Semaphore* seatBelt = createSemaphore(buf, COUNTING, 0); SWAP;
	sprintf(buf, "Car%i Seat Belt Driver", carId); SWAP;
	Semaphore* driverSeatBelt = createSemaphore(buf, COUNTING, 0); SWAP;
	while (1) {
		for (int i = 0; i < NUM_SEATS; i++) {
			ptprintf("\n%10s -| carSeat[%i]", argv[0], i); SWAP;
			semWait(fillSeat[carId]); SWAP;		// wait for available seat
			ptprintf("\n%10s -> rideSeatOpen", argv[0]); SWAP;
			semSignal(rideSeatOpen); SWAP;			// signal for visitor
			ptprintf("\n%10s -| rideSeatTaken", argv[0]); SWAP;
			semWait(rideSeatTaken); SWAP;				// wait for visitor to reply
			ptprintf("\n%10s -> seatFilled", argv[0]); SWAP;
			semSignal(seatFilled[carId]); SWAP;
		}
		ptprintf("\n%10s -> rideDriverNeeded", argv[0]); SWAP;
		semSignal(rideDriverNeeded); SWAP;
		ptprintf("\n%10s -> workerNeeded", argv[0]); SWAP;
		semSignal(workerNeeded); SWAP;
		ptprintf("\n%10s -| rideDriverSeatTaken", argv[0]); SWAP;
		semWait(rideDriverSeatTaken); SWAP;

		ptprintf("\n%10s sending seatbelt", argv[0]); SWAP;
		sendLock(departingCar, seatBelt); SWAP;
		ptprintf("\n%10s sending seatbelt", argv[0]); SWAP;
		sendLock(departingCar, seatBelt); SWAP;
		ptprintf("\n%10s sending seatbelt", argv[0]); SWAP;
		sendLock(departingCar, seatBelt); SWAP;
		ptprintf("\n%10s sending Driver seatbelt", argv[0]); SWAP;
		sendLock(departingCarDriver, driverSeatBelt); SWAP;
		ptprintf("\n%10s -| rideOver[%i]", argv[0], carId); SWAP;
		semWait(rideOver[carId]); SWAP;
		ptprintf("\n%10s -> driverSeatBelt", argv[0]); SWAP;
		semSignal(driverSeatBelt); SWAP;
		ptprintf("\n%10s -> seatBelt", argv[0]); SWAP;
		semSignal(seatBelt); SWAP;
		ptprintf("\n%10s -> seatBelt", argv[0]); SWAP;
		semSignal(seatBelt); SWAP;
		ptprintf("\n%10s -> seatBelt", argv[0]); SWAP;
		semSignal(seatBelt); SWAP;
	}
	return 0;
} // end car task

void sendLock(Semaphore* lock, Semaphore* sem) {
	// pass semaphore to car (1 at a time)
	ptprintf("\n%10s mailbox fetch", taskName(getCurTask())); SWAP;
	semWait(mailMutex); SWAP;				// wait for mailbox
	ptprintf("\n%10s mailbox wait on lock", taskName(getCurTask())); SWAP;
	semWait(lock); SWAP;				// wait for passenger request
	ptprintf("\n%10s mailbox fill", taskName(getCurTask())); SWAP;
	mail = sem; SWAP;					// put semaphore in mailbox
	ptprintf("\n%10s mailbox signal sent", taskName(getCurTask())); SWAP;
	semSignal(mailSent); SWAP;			// raise the mailbox flag
	ptprintf("\n%10s mailbox wait recieved", taskName(getCurTask())); SWAP;
	semWait(mailAcquired); SWAP;			// wait for delivery
	ptprintf("\n%10s mailbox release", taskName(getCurTask())); SWAP;
	semSignal(mailMutex); SWAP;			// release mailbox
}

Semaphore* getLock(Semaphore* lock) {
	// get passenger semaphore
	ptprintf("\n%10s mailbox signal lock", taskName(getCurTask())); SWAP;
	semSignal(lock); SWAP;
	ptprintf("\n%10s mailbox wait on mail", taskName(getCurTask())); SWAP;
	semWait(mailSent); SWAP;				// wait for mail
	ptprintf("\n%10s mailbox receive mail", taskName(getCurTask())); SWAP;
	Semaphore* ret = mail; SWAP;		// get mail
	ptprintf("\n%10s mailbox signal acquired", taskName(getCurTask())); SWAP;
	semSignal(mailAcquired); SWAP;		// put flag down
	return ret;
}

void visWaitPark() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (0) waiting at park", taskName(getCurTask())); SWAP;
	myPark.numOutsidePark++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visEnterPark() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (1) waiting at ticket booth", taskName(getCurTask())); SWAP;
	myPark.numOutsidePark--; SWAP;
	myPark.numInPark++; SWAP;
	myPark.numInTicketLine++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visWaitMuseum() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (2) waiting at museum", taskName(getCurTask())); SWAP;
	myPark.numInTicketLine--; SWAP;
	myPark.numInMuseumLine++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visEnterMuseum() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (3) in museum", taskName(getCurTask())); SWAP;
	myPark.numInMuseumLine--; SWAP;
	myPark.numInMuseum++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visWaitRide() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (4) waiting at ride", taskName(getCurTask())); SWAP;
	myPark.numInMuseum--; SWAP;
	myPark.numInCarLine++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visEnterRide() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (5) on ride", taskName(getCurTask())); SWAP;
	myPark.numInCarLine--; SWAP;
	myPark.numInCars++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visWaitGiftShop() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (6) waiting at gift shop", taskName(getCurTask())); SWAP;
	myPark.numInCars--; SWAP;
	myPark.numRidesTaken++; SWAP;
	myPark.numInGiftLine++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visEnterGiftShop() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (7) in gift shop", taskName(getCurTask())); SWAP;
	myPark.numInGiftLine--; SWAP;
	myPark.numInGiftShop++; SWAP;
	semSignal(parkMutex); SWAP;
}
void visLeavePark() {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s (8) left park", taskName(getCurTask())); SWAP;
	myPark.numInGiftShop--; SWAP;
	myPark.numInPark--; SWAP;
	myPark.numExitedPark++; SWAP;
	semSignal(parkMutex); SWAP;
}

void driSellTicket(int id) {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s selling ticket", taskName(getCurTask())); SWAP;
	myPark.drivers[id] = -1; SWAP;
	semSignal(parkMutex); SWAP;
}
void driSleep(int id) {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s sleeping", taskName(getCurTask())); SWAP;
	myPark.drivers[id] = 0; SWAP;
	semSignal(parkMutex); SWAP;
}
void driDrive(int id, int carId) {
	semWait(parkMutex); SWAP;
	pnprintf("\n%10s driving car %i", taskName(getCurTask()), carId); SWAP;
	myPark.drivers[id] = carId + 1; SWAP;
	semSignal(parkMutex); SWAP;
}

void pnprintf(const char* fmt, ...) {
	va_list args; SWAP;
	va_start(args, fmt); SWAP;
	//vprintf(fmt, args); SWAP;
	fflush(stdout); SWAP;
	va_end(args); SWAP;
}
void ptprintf(const char* fmt, ...) {
	va_list args; SWAP;
	va_start(args, fmt); SWAP;
	//vprintf(fmt, args); SWAP;
	fflush(stdout); SWAP;
	va_end(args); SWAP;
}

void waitRandom(int max, Semaphore* lock) {
	insert(&Dclock, newMemo(rand() % max, lock));
	semWait(lock);
}

// ***********************************************************************
// driver task
int printPark(int argc, char* argv[]) {
	semWait(parkMutex); SWAP;
	semSignal(parkMutex); SWAP;
	return 0;
} // end driver task