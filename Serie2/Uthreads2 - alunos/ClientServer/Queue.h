/*-------------------------------------------------------------------------
  definitions used in the queue syncronizer

  SO,2011
--------------------------------------------------------------------------*/

#ifndef _QUEUE_H
#define _QUEUE_H

#include "../Include/List.h"

typedef struct REQUEST {
	LIST_ENTRY link;
	PEVENT answerEvent;
	long n;
	long res;
	BOOL end;
} REQUEST, *PREQUEST;

FORCEINLINE
VOID InitializeQueueEntry(PREQUEST entry, PEVENT answerEvent, long n) {
	entry->n=n;
	entry->answerEvent = answerEvent;
	entry->end = FALSE;
}

typedef struct QUEUE {
	LIST_ENTRY list;
	// Mutex is used to protect concurrent queyue access in a preemptive environment
	MUTEX lock;
	SEMAPHORE itemsAvaiable;
	int nItems;
} QUEUE, *PQUEUE;


VOID QueueInit(PQUEUE queue);

FORCEINLINE 
BOOL QueueIsEmpty(PQUEUE queue) {
	return queue->nItems==0;
}

FORCEINLINE 
INT QueueCount(PQUEUE queue) {
	return queue->nItems;
}

/* bloqueia a thread invovante enquanto a fila está vazia */
PREQUEST QueueGet(PQUEUE queue);

VOID QueuePut(PQUEUE queue, PREQUEST entry);

#endif