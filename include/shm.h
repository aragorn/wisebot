/* $Id$ */
#ifndef __MY_SHM_H
#define __MY_SHM_H

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <fcntl.h>
//#include <signal.h>

void shared_init(size_t size);
void shared_attach(void *addr, size_t size);
void shared_extent(void **addr, size_t *size);
void *shared_malloc(size_t size);
void shared_free(void *ptr);
void *shared_realloc(void *ptr, size_t size);
char *shared_strdup(char *str);

#endif
