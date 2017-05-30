/*------------------------------------------------------------------
  Ilustrates synchronization between  a server and several clients
  using events.
  Using events, we can  passively wait for a binary condition,
  saving CPU cycles.
  
  In this implementation we use a Mutex(canUseServer)
  to guarantee exclusive access to server.

 
  SO, 2011
---------------------------------------------------------------------*/

#include "stdafx.h"

#define END_SERVER
#define SYNCH_CLIENTS

#define CLIENTS_NUMBER 30


/* shared state between client and server */
static LONG n, res;

#ifdef END_SERVER
/* to promote a controlled server termination */
static BOOL end;
#endif


/* use to access server exclusivelly (syncronization between clients) */
static MUTEX canUseServer;


/* events used in synchronization between the client who gains access to the server
   and the server itself
*/
static EVENT haveWork, haveAnswer;


static VOID Client(UT_ARGUMENT arg) {
	CLIENT_ARGS *cargs = (CLIENT_ARGS *) arg;
  		
	for (;;) {

#ifdef SYNCH_CLIENTS
		// here canUseServer is  used to guarantee 
		// atomic increment if cargs->pos
		// Note that serves only on a preemptive environment, not in normal Uthreads
		MutexAcquire(&canUseServer);
		int i = cargs->pos ;
		cargs->pos = i + 1;
		MutexRelease(&canUseServer);
#else
		int i = cargs->pos ;
		cargs->pos = i + 1;
#endif
		if (i >= cargs->totalNumbers) {
			break;
		}

		MutexAcquire(&canUseServer); 	// Waiting for exclusive access to server
		n = cargs->numbers[i];			// Set data
		EventSet(&haveWork);			// sinalize work  
		EventWait(&haveAnswer);			// Wait for answer 
		cargs->numbers[i] = res;			// Save result
		MutexRelease(&canUseServer);	// Grant access to server 


	};	

#ifdef END_SERVER
	// last thread sinalize termination
	// here canUseServer is  used to guarantee 
	// atomic decrement of cargs->nThreads.
	// Note that serves only on a preemptive environment, not in normal Uthreads
#ifdef SYNCH_CLIENTS
	MutexAcquire(&canUseServer);
	int currThread = --cargs->nThreads;
	MutexRelease(&canUseServer);
#else
	int currThread = --cargs->nThreads;
#endif

	if (currThread == 0) { // last thread is responsible to terminate server
		end = true;
		EventSet(&haveWork);
	}
#endif
}

static VOID Server(UT_ARGUMENT arg) {
	for(; ; ) {
		EventWait(&haveWork);	// wait for work  
#ifdef END_SERVER
		// if to terminate
		if (end) {
			break;
		}
#endif
		res = n*2;				// do work  
		EventSet(&haveAnswer);	// sinalize client  
	}
}


// test function
VOID ClientServerMultiClients0(LONG vals[], INT size) {
	CLIENT_ARGS cargs;
	
	cargs.numbers = vals;
	cargs.totalNumbers = size;
	cargs.nThreads = CLIENTS_NUMBER;
	cargs.pos = 0;

	EventInit(&haveWork, FALSE);
	EventInit(&haveAnswer, FALSE);

	MutexInit(&canUseServer, FALSE);

	
	for(int i=0; i < CLIENTS_NUMBER; ++i)
		UtCreate(Client, (UT_ARGUMENT) &cargs);
	UtCreate(Server, NULL);

	UtRun();
	ShowResults("ClientServerMultiClients0", vals, size);
}
