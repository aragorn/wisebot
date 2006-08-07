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
 *  좌측으로부터 분석을 하여 성공한 성공한 경우에는 우측부터 다시 문법
 *  검사를 해서 중간에 사라져야하는 품사를 제거하여야 한다.
 */
void HANL_right2left(int size,int level,STACKS_INFO * stacks);

#endif
