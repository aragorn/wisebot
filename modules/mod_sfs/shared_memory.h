/* $Id$ */
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

int unmmap_memory(void* base_address, int size);
void*  get_shared_memory(int seq, int fd, int offset, int size);

#endif
