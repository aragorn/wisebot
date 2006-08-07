#ifndef _GRAMMAR_H
#define _GRAMMAR_H 1

#include "moran.h"

int HANL_left_connectable(int current,
						  unsigned int left[MAX_STACK_SIZE][2],
						  int level,
						  STACKS_INFO * stacks);

int HANL_right_connectable(int current,
						   unsigned int right[MAX_STACK_SIZE][2],
						   int level,
						   STACKS_INFO *stacks);

int HANL_grammar_check(char *zookey,
					   int from,
					   int to,
					   int level,
					   FINAL_INFO *final_info,
					   int DICT_TABLE[][MAX_DICT_REF],
					   STACKS_INFO *stacks);

/**
 *  �������κ��� �м��� �Ͽ� ������ ������ ��쿡�� �������� �ٽ� ����
 *  �˻縦 �ؼ� �߰��� ��������ϴ� ǰ�縦 �����Ͽ��� �Ѵ�.
 */
void HANL_right2left(int size,int level,STACKS_INFO * stacks);

#endif
