#include "stdafx.h"
#include "Queue.h"


VOID QueueInit(PQUEUE queue) {
	InitializeListHead(&queue->list);
	MutexInit(&queue->lock, FALSE);
	SemaphoreInit(&queue->itemsAvaiable, 0, LONG_MAX);
	queue->nItems=0;
}


PREQUEST QueueGet(PQUEUE queue) {
	SemaphoreAcquire(&queue->itemsAvaiable,1);
	MutexAcquire(&queue->lock);
	queue->nItems--;
	PREQUEST r =  CONTAINING_RECORD(RemoveHeadList(&queue->list),REQUEST,link);
	MutexRelease(&queue->lock);
	return r;
}

VOID QueuePut(PQUEUE queue, PREQUEST entry) {
	MutexAcquire(&queue->lock);
	InsertTailList(&queue->list, &entry->link);
	queue->nItems++;
	MutexRelease(&queue->lock);
	SemaphoreRelease(&queue->itemsAvaiable,1);
}