/*-------------------------------------------------------------------------
  definitions used in the queue syncronizer

  SO, 2011
--------------------------------------------------------------------------*/

#ifndef _QUEUE0_H
#define _QUEUE0_H


#include "../Include/List.h"

typedef struct REQUEST0 {
	PEVENT answerEvent;
	long n;
	long res;
	BOOL end;
	
	LIST_ENTRY link;
} REQUEST0, *PREQUEST0;

FORCEINLINE
VOID InitializeQueueEntry0(PREQUEST0 entry, PEVENT answerEvent, long n) {
	entry->n=n;
	entry->end = FALSE;
	entry->answerEvent = answerEvent;
}

typedef struct QUEUE0 {
	LIST_ENTRY list;
	EVENT itemsAvaiable;
	// Mutex is used to protect concurrent queyue access in a preemptive environment
	MUTEX mutex;
	int nItems;
} QUEUE0, *PQUEUE0;


VOID QueueInit0(PQUEUE0 queue);

FORCEINLINE 
BOOL QueueIsEmpty0(PQUEUE0 queue) {
	return queue->nItems==0;
}

FORCEINLINE 
INT QueueCount0(PQUEUE0 queue) {
	return queue->nItems;
}

/* bloqueia a thread invovante enquanto a fila está vazia */
PREQUEST0 QueueGet0(PQUEUE0 queue);

VOID QueuePut0(PQUEUE0 queue, PREQUEST0 entry);

#endif