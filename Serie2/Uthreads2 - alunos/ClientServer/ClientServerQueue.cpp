/*------------------------------------------------------------------
  Ilustrates synchronization between  a server and several clients
  using a Queue and a specific event for ecah client.
  
  The client just puts a request in the queue together is specific event
  and just wait for the event signaling!

  Just the server code is shown.
  You must complete the implementation queue and implement the client!
 
  SO, 2011
---------------------------------------------------------------------*/

#include "stdafx.h"
#include "Queue.h"

#define CLIENTS_NUMBER 6
#define SERVERS_NUMBER 4

/* the request Queue */
static QUEUE requests;

/* used for controlled termination */
static EVENT clientsStopped;

/* used for protecting shared client data (in a preemptive environment) */
static MUTEX clientMutex;


static VOID Client(UT_ARGUMENT arg) {
	CLIENT_ARGS *cargs = (CLIENT_ARGS *) arg;
	LONG *numbers = cargs->numbers;
	/* answer event for the client */
	EVENT answerEvent;
	
	EventInit(&answerEvent, FALSE); 

	for (;;) {
		 MutexAcquire(&clientMutex);
		 int i = cargs->pos++;
		 MutexRelease(&clientMutex);

		 if (i >= cargs->totalNumbers) break;
		 // Use the queue to post a request and wait for the answer  
	     REQUEST request;
         InitializeQueueEntry(&request, &answerEvent, numbers[i]);
         QueuePut(&requests, &request);

		 // Wait answer Event
         EventWait(&answerEvent);

         // read answer  
         numbers[i] = request.res;
    };

	MutexAcquire(&clientMutex);
	int nt = --cargs->nThreads;
	MutexRelease(&clientMutex);
	// last thread sinalize termination
	if (nt == 0) {
		EventSet(&clientsStopped);
	}	 
}

static VOID Server(UT_ARGUMENT arg) {
	int id = (int)arg;
	for(; ; ) {
		/* get a request from queue */
		PREQUEST r = QueueGet(&requests);
		if (r->end) 
			break;
		/* do work */
		r->res = r->n*2;
		/* sinalize answer */
		EventSet(r->answerEvent);
	}
}



static VOID Manager(UT_ARGUMENT arg) {
	 
	int i;
	CLIENT_ARGS *cargs = (CLIENT_ARGS*) arg;
	static REQUEST requestEnd;

	/* initialze the queue */
	QueueInit(&requests);
	EventInit(&clientsStopped, FALSE);
	MutexInit(&clientMutex, FALSE);

	/* Create client threads */
	for (int i = 0; i < CLIENTS_NUMBER; ++i)
		UtCreate(Client, (UT_ARGUMENT)cargs);
	/* Create server threads */
	for (int i = 0; i < SERVERS_NUMBER; ++i)
		UtCreate(Server, (UT_ARGUMENT)i);
	
	// Wait for wall clients termination
	EventWait(&clientsStopped);
	// stop the servers
	for (i = 0; i < SERVERS_NUMBER; ++i) {
		requestEnd.end = TRUE;
		QueuePut(&requests, &requestEnd);
	} 
}

// test function
VOID ClientServerQueue(LONG vals[], INT size) {	
	CLIENT_ARGS cargs;
	 
	cargs.numbers = vals;
	cargs.totalNumbers = size;
	cargs.nThreads = CLIENTS_NUMBER;
	cargs.pos = 0;

	UtCreate(Manager, (UT_ARGUMENT) &cargs);
	
	UtRun();

	ShowResults("ClientServerQueue", vals, size);
}