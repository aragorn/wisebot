/* $Id$ */
#ifndef MEMORY_H
#define MEMORY_H 1

#include <sys/types.h>
#include <unistd.h>

#undef sb_malloc
#undef sb_calloc
#undef sb_realloc
#undef sb_free
#undef sb_strdup

#undef sb_mmap
#undef sb_munmap

#undef sb_alloc_shm
#undef sb_free_shm

#undef sb_fork

#ifdef DEBUG_SOFTBOTD
#	define sb_malloc(s)			_sb_malloc(s, __FILE__, __FUNCTION__, __LINE__)
#	define sb_calloc(n,s)		_sb_calloc(n, s, __FILE__, __FUNCTION__, __LINE__)
#	define sb_realloc(p,s)		_sb_realloc(p, s, __FILE__, __FUNCTION__, __LINE__)
#	define sb_free(p)			_sb_free(p, __FILE__, __FUNCTION__, __LINE__)
#	define sb_strdup(s)			_sb_strdup(s, __FILE__, __FUNCTION__, __LINE__)

#	define sb_mmap(start, length, prot, flags, fd, offset)	\
		_sb_mmap(start, length, prot, flags, fd, offset, __FILE__, __FUNCTION__, __LINE__)
#	define sb_munmap(start, length)	\
		_sb_munmap(start, length, __FILE__, __FUNCTION__, __LINE__)

#	define sb_alloc_shm(id, s)	_sb_alloc_shm(id, s, __FILE__, __FUNCTION__)
#	define sb_free_shm(id)		_sb_free_shm(id, __FILE__, __FUNCTION__)

#	define sb_fork()			_sb_fork()
#else 
//#	define sb_malloc(s)			malloc(s)
#	define sb_malloc(s)			calloc(s, 1)
#	define sb_calloc(n,s)		calloc(n, s)
#	define sb_realloc(p,s)		realloc(p, s)
#	define sb_free(p)			free(p)
#	define sb_strdup(s)			strdup(s)

#	define sb_mmap(start, length, prot, flags, fd, offset)	\
		mmap(start, length, prot, flags, fd, offset)
#	define sb_munmap(start, length)	munmap(start, length)
	
#	define sb_alloc_shm(id, s)	{}
#	define sb_free_shm(id)		{}

#	define sb_fork()			fork()
#endif

SB_DECLARE(void *) _sb_malloc(size_t size, char *file, char *function, int line);
SB_DECLARE(void *) _sb_calloc(size_t nmemb, size_t size, char *file, char *function, int line);
SB_DECLARE(void *) _sb_realloc(void *ptr, size_t size, char *file, char *function, int line);
SB_DECLARE(void *) _sb_strdup(char *ptr, char *file, char *function, int line);
SB_DECLARE(void  ) _sb_free(void *ptr, char *file, char *function, int line);

SB_DECLARE(void *) _sb_mmap(void *start, size_t length, int prot, int flags, 
					int fd, off_t offset, char *file, char *function, int line);
SB_DECLARE(int   ) _sb_munmap(void *start, size_t length, char *file, char *function, int line);
			  
SB_DECLARE(void  ) _sb_alloc_shm(int id, size_t size, char *file, char *function);
SB_DECLARE(void  ) _sb_free_shm(int id, char *file, char *function);

SB_DECLARE(pid_t ) _sb_fork(void);

typedef struct sb_mem_block_t sb_mem_block_t;

struct sb_mem_block_t {
	char *data;
	size_t size;
};

#define sb_mem_block_free(p) free((p)->data); free((p))

#include "standalone_debug.h"

#endif //MEMORY_H
