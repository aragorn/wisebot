/* $Id$ */
/* moved to common_core.h using precompiled header 
#define _GNU_SOURCE
#include <stdio.h>    // 77: fprintf()
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

#ifdef DEBUG_MEMORY
#warning DEBUG_MEMORY IS AVAILABLE.

static int debug_memory = 0;
static char *local_memory_log_file = "logs/local_mem_log";
static char *shared_memory_log_file = "logs/shared_mem_log";

static FILE* local_log  = NULL;
static FILE* shared_log = NULL;

static int memlog_lock = -1;
static void init_memory_debug();
#endif //DEBUG_MEMORY

void sb_memory_debug_on(void)
{
	debug_memory = 1;
	init_memory_debug();
}

void sb_memory_debug_off(void)
{
	debug_memory = 0;
}

void init_memory_debug()
{
	if (local_log == NULL)
	{
		if ((local_log = sb_fopen(local_memory_log_file, "w")) == NULL)
		{
			crit("cannot open local memory log file[%s]: %s", 
				local_memory_log_file, strerror(errno));
			SB_ABORT();
		}
		setvbuf(local_log, (char*)NULL, _IOLBF, 0);
	}

	if (shared_log == NULL)
	{
		if ((shared_log = sb_fopen(shared_memory_log_file, "w")) == NULL)
		{
			crit("cannot open shared memory log file[%s]: %s", 
				shared_memory_log_file, strerror(errno));
			SB_ABORT();
		}
		setvbuf(shared_log, (char*)NULL, _IOLBF, 0);
	}

	if ( memlog_lock == -1 ) {
		ipc_t lock;
		lock.type = IPC_TYPE_SEM;
		lock.pid  = 'l';
		lock.pathname = local_memory_log_file;

		if ( get_sem(&lock) != SUCCESS ) {
			crit("cannot get semaphore for memory log file[%s]: %s",
					local_memory_log_file, strerror(errno));
			SB_ABORT();
		}
		memlog_lock = lock.id;
	}
}
		
void *_sb_malloc(size_t size, const char *file, const char *function, int line)
{
#ifdef DEBUG_MEMORY
	void *ptr = calloc(size,1);
	if (debug_memory == 0) return ptr;
	if ( !ptr ) return NULL;
	
	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
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
	if (debug_memory == 0) return ptr;
	if ( !ptr ) return NULL;
	
	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
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
	if (debug_memory == 0) return ptr;
	
	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
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
	if (debug_memory == 0) return new_ptr;
	
	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, 0, file, function, line, time(NULL));
	fprintf(local_log, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld\n",
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
	free(ptr);
	if (debug_memory == 0) return;

	if (memlog_lock > 0) acquire_lock(memlog_lock);
	fprintf(local_log, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), ptr, 0, file, function, line, time(NULL));
	if (memlog_lock > 0)release_lock(memlog_lock);

	return;
#endif //DEBUG_MEMORY
}


pid_t _sb_fork(void){
#ifdef DEBUG_MEMORY
	pid_t pid = fork();
	if ( pid <= 0 ) {	//failure or child process
		return pid;
	}
	if (debug_memory == 0) return pid;

	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\tfork\t%d\t%ld\n",
			getpid(), pid, time(NULL));
	release_lock(memlog_lock);
	return pid;
#else
	return fork();
#endif //DEBUG_MEMORY
}



void _sb_alloc_shm(int id, size_t size, const char *file, const char *function){
#ifdef DEBUG_MEMORY
	if (debug_memory == 0) return;

	acquire_lock(memlog_lock);
	fprintf(shared_log, "%d\talloc\t#%d\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), id, size, file, function, 0, time(NULL));
	release_lock(memlog_lock);
#endif //DEBUG_MEMORY
}

void _sb_free_shm(int id, const char *file, const char *function){
#ifdef DEBUG_MEMORY
	if (debug_memory == 0) return;

	acquire_lock(memlog_lock);
	fprintf(shared_log, "%d\tfree\t#%d\t%d\t%s\t%s\t%d\t%ld\n",
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
	if (debug_memory == 0) return ptr;

	if ( fstat(fd, &buf) < 0 ) return ptr;
	
	if ( flags && MAP_SHARED ) {
		acquire_lock(memlog_lock);
		fprintf(shared_log, "%d\talloc\t#%ld\t%d\t%s\t%s\t%d\t%ld\t-mmap[%p]\n",
				getpid(), buf.st_ino, length, file, function, line, time(NULL), ptr);
		release_lock(memlog_lock);
	} else if ( flags && MAP_PRIVATE ) {
		acquire_lock(memlog_lock);
		fprintf(local_log, "%d\talloc\t%p\t%d\t%s\t%s\t%d\t%ld-mmap\n",
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
	if (debug_memory == 0) return munmap(start, length);

	acquire_lock(memlog_lock);
	fprintf(local_log, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\t-munmap\n",
			getpid(), start, 0, file, function, line, time(NULL));
	fprintf(shared_log, "%d\tfree\t%p\t%d\t%s\t%s\t%d\t%ld\t-munmap[%p]\n",
			getpid(), (void*)0, 0, file, function, line, time(NULL), start);
	release_lock(memlog_lock);

	return munmap(start, length);
#else
	return munmap(start, length);
#endif //DEBUG_MEMORY
}

