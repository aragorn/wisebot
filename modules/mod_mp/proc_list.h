/* $Id$ */
#ifndef PROC_LIST_H
#define PROC_LIST_H

typedef struct proc_node proc_node;

struct proc_node {
	int slot_id;
	int time; // 이 시간 이후에 꺼낼 수 있다.
	proc_node* next;
};

int add_to_last(proc_node** list, int slot_id);
proc_node* delete_from_first(proc_node** list);

#endif // PROC_LIST_H
