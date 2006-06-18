/* $Id$ */
#ifndef PROC_LIST_H
#define PROC_LIST_H

typedef struct proc_node proc_node;

struct proc_node {
	int slot_id;
	int time; // �� �ð� ���Ŀ� ���� �� �ִ�.
	proc_node* next;
};

int add_to_last(proc_node** list, int slot_id);
proc_node* delete_from_first(proc_node** list);

#endif // PROC_LIST_H
