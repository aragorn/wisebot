#ifndef _PROC_LIST_H_
#define _PROC_LIST_H_

typedef struct _proc_node {
	int slot_id;
	int time; // �� �ð� ���Ŀ� ���� �� �ִ�.
	struct _proc_node* next;
} proc_node;

int add_to_last(proc_node** list, int slot_id);
proc_node* delete_from_first(proc_node** list);

#endif // _PROC_LIST_H_
