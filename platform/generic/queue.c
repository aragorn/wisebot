/* $Id$ */
#include "queue.h"

/**
 * increment
 * x의 다음 큐번호를 리턴한다.
 */
int increment(Queue *qd, int x) {
	if (++x == qd->queueSize)
		x = 0;
	
	return x;
}

/**
 * initQueue
 * 큐를 초기화한다.

 * @qsize : 큐의 길이
 * @nsize : 큐의 단위 사이즈
 * @start : 큐가 시작할 (미리 할당된)장소
 * @qd : queue descriptor
 */
SB_DECLARE(int) initQueue(Queue *qd, int qsize, int nsize, void *start) {
/*	qd->queue = start;*/
	qd->currentSize = 0;
	qd->back = -1;
	qd->front = 0;
	qd->queueSize = qsize;
	qd->unitSize = nsize;

	return 1;
}

int loadQueue(Queue *qd, void *start) {
/*	qd->queue = start;*/
	return 1;
}

/**
 * enqueue
 * 큐에 데이타를 집어넣는다.
 
 * @qd : queue descriptor
 * @data : 집어넣을 데이타.
 */
int enqueue(Queue *qd, void *data) {
	void *tmp;

	info("enqueueing data queue:%p, data:%p", qd, data);
	
	if (isFull(qd))
		return QUEUE_FULL;
	
	if (data == NULL)
		return -1;

	qd->back = increment(qd, qd->back);
/*	tmp = (void *)(qd->queue + qd->back * qd->unitSize);*/
	tmp = (void *)((void *)qd + sizeof(Queue) + qd->back * qd->unitSize);
	
	memcpy(tmp, data, qd->unitSize);
	
	(qd->currentSize)++;

	return 1;
}

/**
 * dequeue
 * 큐의 데이타를 하나 지우면서 받아온다.

 * @qd : queue descriptor
 * @data : 받아올 장소
 */
int dequeue(Queue *qd, void *data) {
	void *tmp;

	if (isEmpty(qd))
		return QUEUE_EMPTY;

	if (data == NULL)
		return -1;

/*	tmp = (void *)(qd->queue + qd->front * qd->unitSize);*/
	tmp = (void *)((void *)qd + sizeof(Queue) + qd->front * qd->unitSize);
	memcpy(data, tmp, qd->unitSize);

	(qd->currentSize)--;
	qd->front = increment(qd, qd->front);

	return 1;
}

/**
 * getFront
 * 큐의 데이타를 하나 얻어온다.
 
 * @qd : queue descriptor
 * @data : 데이타를 받아올 장소
 */
int getFront(Queue *qd, void *data) {
	void *tmp;

	if (isEmpty(qd))
		return QUEUE_EMPTY;

	if (data == NULL)
		return -1;

/*	tmp = (void *)(qd->queue + qd->front * qd->unitSize);*/
	tmp = (void *)((void *)qd + sizeof(Queue) + qd->front * qd->unitSize);
	memcpy(data, tmp, qd->unitSize);

	return 1;
}

/**
 * isEmpty
 * 큐가 비어있는지 확인한다.

 * qd : queue descriptor
 */
int isEmpty(Queue *qd) {
	return qd->currentSize == 0;
}

/**
 * isFull
 * 큐가 꽉차있는지 확인한다.

 * @qd : queue descriptor
 */
int isFull(Queue *qd) {
	info("current size:%d, queue size:%d", qd->currentSize, qd->queueSize);
	return qd->currentSize == qd->queueSize;
}


