#include "proc_list.h"
#include "softbot.h"

#define RESTART_DELAY 10

int add_to_last(proc_node** list, int slot_id)
{
	proc_node* current = *list;
	proc_node* node = sb_malloc( sizeof(proc_node) );

	node->slot_id = slot_id;
	node->time    = time(NULL) + RESTART_DELAY;
	node->next    = NULL;

	warn( "slot[%d] is going to be restarted after %d seconds", slot_id, RESTART_DELAY );

	if ( *list == NULL ) {
		*list = node;
	}
	else {
		while ( current->next ) current = current->next;
		current->next = node;
	}

	return SUCCESS;
}

/* node->time 을 보고 아직 꺼낼때가 아니면 NULL을 리턴한다. */
proc_node* delete_from_first(proc_node** list)
{
	proc_node* node = *list;

	if ( node == NULL ) return NULL;
	if ( node->time > time(NULL) ) return NULL;

	*list = (*list)->next;

	return node;
}

