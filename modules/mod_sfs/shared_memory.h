/* $Id$ */

#ifndef __SHARED_MEMORY_H__
#define __SHARED_MEMORY_H__

#ifdef WIN32
#  include "wisebot.h"
#else
#  include "softbot.h"
#endif

int unmmap_memory(void* base_address, int size);
void*  get_shared_memory(int seq, int fd, int offset, int size);

#endif
