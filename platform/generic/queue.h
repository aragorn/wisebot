/* $Id$ */
#ifndef _SB_QUEUE_H
#define _SB_QUEUE_H 1

#include "softbot.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define QUEUE_EMPTY		-501
#define QUEUE_FULL		-502

typedef struct _VARS {
	void *queue;
	int queueSize;
	int unitSize;
	int front;
	int back;
	int currentSize;
} Queue;

SB_DECLARE(int) initQueue(Queue *qd, int qsize, int nsize, void *start);
SB_DECLARE(int) loadQueue(Queue *qd, void *start);
SB_DECLARE(int) enqueue(Queue *qd, void *data);
SB_DECLARE(int) dequeue(Queue *qd, void *data);
SB_DECLARE(int) getFront(Queue *qd, void *data);
SB_DECLARE(int) isEmpty(Queue *qd);
SB_DECLARE(int) isFull(Queue *qd);
SB_DECLARE(int) increment(Queue *qd, int x);


#endif
