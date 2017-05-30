/*------------------------------------------------------------------
Ilustrates synchronization between  a server and several clients
using a Queue supported by events and a specific event for each client.
This solution does not correctly support multiple servers (on preemptive environments)
due to the queue implementation (due to the use of an event)!

The clients  puts a request in the queue  together with is specific event
and just waits for the event signal!


SO, 2014
---------------------------------------------------------------------*/

#include "stdafx.h"
#include "Queue0.h"

//#define SYNCH_CLIENTS
#define END_SERVERS

static QUEUE0 requests;
static MUTEX clientMutex;
static EVENT clientsStopped;

static VOID Server(UT_ARGUMENT arg) {
	int id = (int)arg;
	PREQUEST0 request;
	for (;;) {
		// printf("server %d try...\n", id);
		request = QueueGet0(&requests);
		// printf("server %d get!\n", id);
		if (request->end) break;
		request->res = request->n*2;
		EventSet(request->answerEvent);
	}
	//printf("server %d stopped...\n", id);
}


static VOID Client(UT_ARGUMENT arg) {
	CLIENT_ARGS *cargs = (CLIENT_ARGS*)arg;
	LONG *numbers=(LONG *) cargs->numbers;
	EVENT answerEvent;

	REQUEST0 request = { 0 };
	
	static REQUEST0 requestEnd;
	EventInit(&answerEvent, FALSE); 
	
	for (;;) {
		// here canUseServer is  used to guarantee 
		// atomic increment if cargs->pos
		// Note that serves only on a preemptive environment, not in normal Uthreads
#ifdef SYNCH_CLIENTS
		MutexAcquire(&clientMutex);
		int i = cargs->pos;
		cargs->pos = i + 1;
		MutexRelease(&clientMutex);
#else
		LONG i = cargs->pos;
		cargs->pos = i + 1;
#endif
		if (i >= cargs->totalNumbers) break;
		InitializeQueueEntry0(&request, &answerEvent, numbers[i]); 
		QueuePut0(&requests, &request);
		
		EventWait(request.answerEvent);
		numbers[i] = request.res;
	};
	// here canUseServer is  used to guarantee 
	// atomic decrement of cargs->nThreads.
	// Note that serves only on a preemptive environment, not in normal Uthreads
#ifdef SYNCH_CLIENTS
	MutexAcquire(&clientMutex);
	int nt = --cargs->nThreads;
	MutexRelease(&clientMutex);
#else
	int nt = --cargs->nThreads;
#endif
	if (nt == 0) {
		EventSet(&clientsStopped);
	}
}

#define NCLIENTS 4
#define NSERVERS 1

static VOID Manager(UT_ARGUMENT arg) {

	int i;
	
	CLIENT_ARGS *cargs = (CLIENT_ARGS*)arg;
	static REQUEST0 requestEnd;

	/* initialize the queue */
	QueueInit0(&requests);
	EventInit(&clientsStopped, FALSE);
#ifdef SYNCH_CLIENTS
	MutexInit(&clientMutex, FALSE);
#endif

	/* Create client threads */
	for (int i = 0; i < NCLIENTS; ++i)
		UtCreate(Client, (UT_ARGUMENT)cargs);
/* Create server threads */
	for (int i = 0; i < NSERVERS; ++i)
		UtCreate(Server, (UT_ARGUMENT)i);

	// Wait for wall clients termination
	EventWait(&clientsStopped);
	//printf("All clients stopped, queueSize = %d!\n", QueueCount0(&requests));

#ifdef END_SERVERS
	// stop the servers
	// or reasons explained in the queue simplementation,
	// this scenario doesn't really work with more than one server!
	// But the termination synchronization below does!
	for (i = 0; i < NSERVERS; ++i) {
		requestEnd.end = TRUE;
		QueuePut0(&requests, &requestEnd);
	}
#endif
}

// test function
VOID ClientServerQueue0(LONG vals[], INT size) {
	 
	CLIENT_ARGS cargs;
 
	cargs.numbers = vals;
	cargs.totalNumbers =  size;
	cargs.nThreads = NCLIENTS;
	cargs.pos = 0;
	
	// just create the manager thread
	UtCreate(Manager, (UT_ARGUMENT) &cargs);
	 
	UtRun();
	ShowResults("ClientServerQueue0", vals, size);
}




