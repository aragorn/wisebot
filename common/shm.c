/* $Id$ */

#include "common_core.h"
#include "shm.h"
#include "mm.h"

void message_receptor(int ignore);

// wrapper for shared memory management
//
void shared_init(size_t size)
{
	MM_create(size, NULL);
}


void shared_attach(void *addr, size_t size)
{
	MM_attach(addr, size, NULL);
}


void shared_extent(void **addr, size_t *size)
{
	MM_extent(addr, size);
}


void *shared_malloc(size_t size)
{
	return MM_malloc(size);
}


void shared_free(void *ptr)
{
	MM_free(ptr);
}


void *shared_realloc(void *ptr, size_t size)
{
	return MM_realloc(ptr, size);
}


char *shared_strdup(char *str)
{
	return MM_strdup(str);
}

