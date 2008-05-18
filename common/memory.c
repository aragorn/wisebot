/* $Id$ */
/* moved to common_core.h using precompiled header 
#define _GNU_SOURCE
#include <stdio.h>    // 77: dprintf()
#include <stdlib.h>   // 66: calloc
#include <string.h>   // strerror()
#include <errno.h>    // errno
#include <unistd.h>    // 78: getpid()
#include <time.h>      // 78: time()
#include <sys/mman.h>  // 198: mmap()
#include <fcntl.h>     // 76: O_CREAT
#define CORE_PRIVATE 1
*/
#include "common_core.h"
/*
#include "log_error.h"
#include "ipc.h"
#include "memory.h"
*/

#define DEBUG_MEMORY
//#undef DEBUG_MEMORY
#warning When DEBUG_MEMORY is defined, server exits with acquire_lock() error.
#ifdef AIX5
#  undef DEBUG_MEMORY  /* there's no dprintf() on AIX.  */
#endif                 /* cannot print debug message to a file descriptor. */

#ifdef DEBUG_MEMORY
static char *local_memory_log_file = "logs/local_mem_log";
static char *shared_memory_log_file = "logs/shared_mem_log";

static int localmemlog_des = -1;
static int sharedmemlog_des = -1;

static int memlog_lock = -1;
#endif //DEBUG_MEMORY

#define INIT_LOCAL_MEM_LOG()	\
if ( localmemlog_des == -1 ) {	\
	localmemlog_des = sb_open(local_memory_log_file, O_CREAT | O_WRONLY | O_TRUNC, 00644);	\
	if ( localmemlog_des == -1 ) { \
		crit("cannot open local memory log file[%s]: %s", \
				local_memory_log_file, strerror(errno));	\
		SB_ABORT();	\
	}	\
	if ( memlog_lock == -1 ) { \
		ipc_t lock; \
		lock.type = IPC_TYPE_SEM; \
		lock.pid  = 'l'; \
		lock.pathname = local_memory_log_file; \
\
		if ( get_sem(&lock) != SUCCESS ) { \
			crit("cannot get semaphore for memory log file[%s]: %s", \
					local_memory_log_file, strerror(errno)); \
			SB_ABORT(); \
		} \
		memlog_lock = lock.id; \
	} \
}
	
#define INIT_SHARED_MEM_LOG()	\
if ( sharedmemlog_des == -1 ) {	\
	sharedmemlog_des = sb_open(shared_memory_log_file, \
								O_CREAT | O_WRONLY | O_TRUNC, 00644);	\
	if ( sharedmemlog_des == -1 ) { \
		crit("cannot open shared memory log file[%s]: %s", \
				shared_memory_log_file, strerror(errno));	\
		SB_ABORT();	\
	}	\
}
		
void *_sb_malloc(size_t size, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	void *ptr = calloc(size,1);
	if ( !ptr ) return NULL;
	
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, size, file, function, line, time(NULL));
	release_lock(memlog_lock);
	return ptr;
#else
	return calloc(size,1);
#endif //DEBUG_MEMORY
}

void *_sb_calloc(size_t nmemb, size_t size, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	void *ptr = calloc(nmemb, size);
	if ( !ptr ) return NULL;
	
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, nmemb*size, file, function, line, time(NULL));
	release_lock(memlog_lock);
	return ptr;
#else
	return calloc(nmemb, size);
#endif //DEBUG_MEMORY
}

void *_sb_strdup(char *inptr, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	int size =  strlen(inptr)+1;
	void *ptr = malloc(size);
	if ( !ptr ) return NULL;
	memcpy(ptr, inptr, size);
	
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, size, file, function, line, time(NULL));
	release_lock(memlog_lock);
	return ptr;
#else
	return strdup(inptr);
#endif //DEBUG_MEMORY
}

void *_sb_realloc(void *ptr, size_t size, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	void *new_ptr = realloc(ptr, size);
	if ( !new_ptr ) return NULL;
	
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, 0, file, function, line, time(NULL));
	dprintf(localmemlog_des, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), new_ptr, size, file, function, line, time(NULL));
	release_lock(memlog_lock);
	return new_ptr;
#else
	return realloc(ptr, size);
#endif //DEBUG_MEMORY
}

void _sb_free(void *ptr, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, 0, file, function, line, time(NULL));
	release_lock(memlog_lock);
#endif //DEBUG_MEMORY
	free(ptr);
}


pid_t _sb_fork(void){
#ifdef DEBUG_MEMORY
	pid_t tmp = fork();
	if ( tmp <= 0 ) {	//failure or child process
		return tmp;
	}
	INIT_LOCAL_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\tfork\t%d\t%ld\n",
			getpid(), tmp, time(NULL));
	release_lock(memlog_lock);
	return tmp;
#else
	return fork();
#endif //DEBUG_MEMORY
}



void _sb_alloc_shm(int id, size_t size, const char *file, const char *function){
#ifdef DEBUG_MEMORY
	INIT_SHARED_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(sharedmemlog_des, "%d\talloc\t#%d\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), id, size, file, function, 0, time(NULL));
	release_lock(memlog_lock);
#endif //DEBUG_MEMORY
}

void _sb_free_shm(int id, const char *file, const char *function){
#ifdef DEBUG_MEMORY
	INIT_SHARED_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(sharedmemlog_des, "%d\tfree\t#%d\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), id, 0, file, function, 0, time(NULL));
	release_lock(memlog_lock);
#endif //DEBUG_MEMORY
}

void *_sb_mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset,
				const char *file, const char *function, int line){
#ifdef DEBUG_MEMORY
	struct stat buf;
	void *ptr = mmap(start, length, prot, flags, fd, offset);
	if ( !ptr ) return NULL;

	if ( fstat(fd, &buf) < 0 ) return ptr;
	
	if ( flags && MAP_SHARED ) {
		INIT_SHARED_MEM_LOG();
		acquire_lock(memlog_lock);
		dprintf(sharedmemlog_des, "%d\talloc\t%ld\t%d\t%s\t%s\t%d\t%ld\t-mmap[%p]\n",
				getpid(), buf.st_ino, length, file, function, line, time(NULL), ptr);
		release_lock(memlog_lock);
	}else if ( flags && MAP_PRIVATE ) {
		INIT_LOCAL_MEM_LOG();
		acquire_lock(memlog_lock);
		dprintf(localmemlog_des, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld-mmap\n",
				getpid(), ptr, length, file, function, line, time(NULL));
		release_lock(memlog_lock);
	}
	return ptr;
#else
	return mmap(start, length, prot, flags, fd, offset);
#endif //DEBUG_MEMORY
}

int _sb_munmap(void *start, size_t length, const char *file, const char *function, int line){
#ifdef DEBUG_MEMORY
	INIT_LOCAL_MEM_LOG();
	INIT_SHARED_MEM_LOG();

	acquire_lock(memlog_lock);
	dprintf(localmemlog_des, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\t-munmap\n",
			getpid(), start, 0, file, function, line, time(NULL));
	dprintf(sharedmemlog_des, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\t-munmap[%p]\n",
			getpid(), (void*)0, 0, file, function, line, time(NULL), start);
	release_lock(memlog_lock);

	return munmap(start, length);
#else
	return munmap(start, length);
#endif //DEBUG_MEMORY
}

