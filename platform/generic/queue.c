/* $Id$ */
#include "queue.h"

/**
 * increment
 * x�� ���� ť��ȣ�� �����Ѵ�.
 */
int increment(Queue *qd, int x) {
	if (++x == qd->queueSize)
		x = 0;
	
	return x;
}

/**
 * initQueue
 * ť�� �ʱ�ȭ�Ѵ�.

 * @qsize : ť�� ����
 * @nsize : ť�� ���� ������
 * @start : ť�� ������ (�̸� �Ҵ��)���
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
 * ť�� ����Ÿ�� ����ִ´�.
 
 * @qd : queue descriptor
 * @data : ������� ����Ÿ.
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
 * ť�� ����Ÿ�� �ϳ� ����鼭 �޾ƿ´�.

 * @qd : queue descriptor
 * @data : �޾ƿ� ���
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
 * ť�� ����Ÿ�� �ϳ� ���´�.
 
 * @qd : queue descriptor
 * @data : ����Ÿ�� �޾ƿ� ���
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
 * ť�� ����ִ��� Ȯ���Ѵ�.

 * qd : queue descriptor
 */
int isEmpty(Queue *qd) {
	return qd->currentSize == 0;
}

/**
 * isFull
 * ť�� �����ִ��� Ȯ���Ѵ�.

 * @qd : queue descriptor
 */
int isFull(Queue *qd) {
	info("current size:%d, queue size:%d", qd->currentSize, qd->queueSize);
	return qd->currentSize == qd->queueSize;
}


