/*------------------------------------------------------------------
  Ilustrates synchronization between a single client and a server
  using active wait.
  Active wait is considered not good in most scenarios since it
  spends CPU cycles just for nothing

  SO, 2014
---------------------------------------------------------------------*/
#include "stdafx.h"

 
/* flags used in synchronize client and server */
BOOL haveWork, workDone;

/* shared state betwwen client and server */
LONG n, res;

/* used to terminate server */
BOOL end;

VOID Client(UT_ARGUMENT arg) {
	CLIENT_ARGS *carg = (CLIENT_ARGS *)arg;
	LONG *numbers = carg->numbers;

	for (int i = 0; i < carg->totalNumbers; ++i) {
		n = numbers[i];
		haveWork = true;			  // notifica servidor
		while (!workDone) UtYield();    // espera (activamente) resposta
		workDone = false;			  // reset uma vez a notificação recebida
		numbers[i] = res;
	};

	
	// terminate
	end = true;
	haveWork = true;
}

VOID Server(UT_ARGUMENT arg) {
	while (!end) {
		while (!haveWork) UtYield(); // espera (activamente) trabalho
		haveWork = false;			 // reset uma vez a notificação recebida
		res = n * 2;
		workDone = true;			 // notifica cliente
	}
}


VOID ClientServer0(LONG vals[], INT size) {
	CLIENT_ARGS cargs;

	cargs.numbers = vals;
	cargs.totalNumbers = size;

	UtCreate(Client, (UT_ARGUMENT) &cargs);
	UtCreate(Server, NULL);

	UtRun();
	
}