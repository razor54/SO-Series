/*------------------------------------------------------------------
  Ilustrates synchronization between a single client and a server
  using events.
  Using events, we can passively wait for a binary condition,
  saving CPU cycles.
  
  SO, 2014
---------------------------------------------------------------------*/

#include "stdafx.h"

//#define END_SERVER

/* events used in synchronization */
static EVENT haveWork, haveAnswer;

static BOOL end;

/* shared state between client and server */
static LONG n, res;

#ifdef END_SERVER
/* used in server termination */
static BOOL end;
#endif

static VOID Client(UT_ARGUMENT arg) {
	CLIENT_ARGS *cargs = (CLIENT_ARGS *) arg;
	LONG *numbers= cargs->numbers;
	 
	for (int i=0; i < cargs->totalNumbers; ++i) {
		n = numbers[i];
		/* sinalize work */
		EventSet(&haveWork);

		/*wait for answer */
		EventWait(&haveAnswer);
		numbers[i] = res;
	};
	end = TRUE;
	EventSet(&haveWork);
#ifdef END_SERVER
	end = true;
	EventSet(&haveWork);
#endif
}

static VOID Server(UT_ARGUMENT arg) {
	while (true) {
		/* passivelly waiting for work */
		EventWait(&haveWork);
#ifdef END_SERVER
		/* check if to terminate */
		if (end) break;
#endif
		if (end) break;
		/* do work */
		res = n*2;
		/* sinalize answer */
		EventSet(&haveAnswer);
	}
}

/* test function */
VOID ClientServer(LONG vals[], INT size) {
	CLIENT_ARGS cargs;

	cargs.numbers = vals;
	cargs.totalNumbers = size;

	 
	EventInit(&haveWork, FALSE);
	EventInit(&haveAnswer, FALSE);
		
	UtCreate(Client, (UT_ARGUMENT) &cargs);
	UtCreate(Server, NULL);

	UtRun();

	
}