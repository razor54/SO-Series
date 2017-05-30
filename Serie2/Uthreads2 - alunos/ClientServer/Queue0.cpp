#include "stdafx.h"
#include "Queue0.h"

//#define SYNC_QUEUE

VOID QueueInit0(PQUEUE0 queue) {
	InitializeListHead(&queue->list);
	EventInit(&queue->itemsAvaiable, FALSE);
#ifdef SYNC_QUEUE
	MutexInit(&queue->mutex, FALSE);
#endif
	queue->nItems=0;
}


PREQUEST0 QueueGet0(PQUEUE0 queue) {
#ifdef SYNC_QUEUE
	MutexAcquire(&queue->mutex);
#endif
	while (QueueIsEmpty0(queue)) {
#ifdef SYNC_QUEUE
		MutexRelease(&queue->mutex);
#endif
		//event just memorize one notification.
		//this could mean unecessary server blocking, if two or more
		//servers are at this point when the notifications are done.
		//The sleep next line is done to exacerbate the problem 
		// of course it only ocurrs in a preemptive environment, not
		// in normal uthreads.
		//Sleep(10);
		EventWait(&queue->itemsAvaiable);
#ifdef SYNC_QUEUE	 
		MutexAcquire(&queue->mutex);
#endif
	}
	
	queue->nItems--;
	PREQUEST0 r = (PREQUEST0)
		CONTAINING_RECORD(RemoveHeadList(&queue->list), REQUEST0, link);
#ifdef SYNC_QUEUE	
	MutexRelease(&queue->mutex);
#endif
	return r;
}
	 

VOID QueuePut0(PQUEUE0 queue, PREQUEST0 entry) {
#ifdef SYNC_QUEUE
	MutexAcquire(&queue->mutex);
#endif
	InsertTailList(&queue->list, &entry->link);
	queue->nItems++;
#ifdef SYNC_QUEUE
	MutexRelease(&queue->mutex);
#endif
	EventSet(&queue->itemsAvaiable);
}