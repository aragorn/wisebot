/* $Id$ */
//FIXME : name is inappropriate.. isn't it? -- eerun

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "softbot.h"
#include "memory.h"

#define DEBUG_MEMORY
#undef  DEBUG_MEMORY
#ifdef AIX5
#  undef DEBUG_MEMORY  /* there's no dprintf() on AIX.  */
#endif                 /* cannot print debug message to a file descriptor. */

/*#include "search.h"*/

/*static sb_hash_t malloc_hash;*/
/*static int is_hash_exists = 0;*/
/*#define MALLOC_HASH_KEY_LEN 8*/

/*inline static void _sb_malloc_hash_init() {*/
/*	if (!is_hash_exists) {*/
/*		if (sb_hash_init(1, MALLOC_HASH_KEY_LEN, sizeof(int), NULL, &malloc_hash)*/
/*				!= SB_HASH_SUCCESS) {*/
/*			error("sb_hash_failed");*/
/*		} else {*/
/*			is_hash_exists = 1;*/
/*		}*/
/*	}*/

/*}*/

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
	dprintf(sharedmemlog_des, "%d\talloc\t%d\t%d\t%s\t%s\t%d\t%ld\n",
			getpid(), id, size, file, function, 0, time(NULL));
	release_lock(memlog_lock);
#endif //DEBUG_MEMORY
}

void _sb_free_shm(int id, const char *file, const char *function){
#ifdef DEBUG_MEMORY
	INIT_SHARED_MEM_LOG();
	acquire_lock(memlog_lock);
	dprintf(sharedmemlog_des, "%d\tfree\t%d\t%d\t%s\t%s\t%d\t%ld\n",
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

#if 0
/* TODO: sb_malloc등을 nodebug mode로 compile할 때에,
 *       바로 malloc을 부르게 하지 말고, strerror(errno)를
 *       찍어주는 wrapper를 부르도록 한다.
 */

#define MAX_SECTION_NUM 	4
#define SMALL				(64 * 1024)
#define MEDIUM				(1024 * 1024)
#define LARGE				(10 * 1024 * 1024)

typedef struct _memory_stat_key_t memory_stat_key_t;

typedef struct _memory_stat_t {
	uint32_t total;
	uint32_t cumulative_total;
	uint32_t nalloc;
	uint32_t nrealloc;
	uint32_t nfree;
	uint32_t section[MAX_SECTION_NUM];
	memory_stat_key_t *key;
} memory_stat_t;

struct _memory_stat_key_t {
	char key[STRING_SIZE];
	memory_stat_t *stat;
};

#define MAX_HASHKEY_LEN			12
#define MAX_MSTAT_KEY			101 /* prime number */
#define MAX_MSTAT_EXT_SLOT		10
#define MAX_MEMORY_STAT (MAX_MSTAT_KEY * MAX_MSTAT_EXT_SLOT)

REGISTRY int *nmemorystat = NULL;
REGISTRY memory_stat_t *memorystat = NULL;
REGISTRY memory_stat_key_t *memorystatkey[MAX_MSTAT_KEY] = {NULL};

static memory_stat_t *get_memory_stat(const char *file, const char *function);
static memory_stat_t *search_memory_stat(char *key);
static memory_stat_t *create_memory_stat(char *key);
static int get_hash_key(char *key);


void *_sb_malloc(size_t size, const char *file, const char *function, int line)
{
	memory_stat_t *mstat = NULL;
	void *ptr=NULL;
	FILE *fp;

	
	ptr = malloc(size);
		
	if (ptr == NULL) {
		error("malloc failed:%s (requested size:%d)", strerror(errno), (int)size);
		return NULL;
	}
	
	fp = sb_fopen ("memory.log", "a");
	if (fp == NULL)
	{
		error("memory.log open failed : %s", strerror(errno));
	}
	else
	{
		fprintf(fp, "%p|%d|%d|%s|%s|%d\n", ptr, getpid(), size, file, function, line);
		fclose(fp);
	}
	
	mstat = get_memory_stat(file,function);
	if (mstat != NULL) {
/*		mstat->total += size;*/
		mstat->cumulative_total += size;
		mstat->nalloc++;

		if (size < SMALL) {
			mstat->section[0] += size;
		}
		else if (size < MEDIUM) {
			mstat->section[1] += size;
		}
		else if (size < LARGE) {
			mstat->section[2] += size;
		}
		else {
			mstat->section[3] += size;
		}
	}
	return ptr;
}

void *_sb_calloc(size_t nmemb, size_t size, const char *file, const char *function, int line)
{
	memory_stat_t *mstat = NULL;
	void *ptr=NULL;
	FILE *fp;

/*	fp = fopen ("/local2/nextsoftbot/size.log", "a");*/
/*	fprintf(fp, "%d|%d|%s|%s|%d\n", getpid(), size, file, function, line);*/
/*	fclose(fp);*/
	
	ptr = calloc(nmemb, size);
	if (ptr == NULL) {
		error("calloc failed:%s (requested size:%d*%d)", strerror(errno), (int)nmemb, (int)size);
		return NULL;
	}
	
	fp = sb_fopen ("memory.log", "a");
	if (fp == NULL)
	{
		error("memory.log open failed : %s", strerror(errno));
	}
	else
	{
		fprintf(fp, "%p|%d|%d|%s|%s|%d\n", ptr, getpid(), size*nmemb, file, function, line);
		fclose(fp);
	}

	mstat = get_memory_stat(file,function);
	if (mstat != NULL) {
/*		mstat->total += size;*/
		mstat->cumulative_total += size * nmemb;
		mstat->nalloc++;

		if (size < SMALL) {
			mstat->section[0] += size * nmemb;
		}
		else if (size < MEDIUM) {
			mstat->section[1] += size * nmemb;
		}
		else if (size < LARGE) {
			mstat->section[2] += size * nmemb;
		}
		else {
			mstat->section[3] += size * nmemb;
		}
	}
	return ptr;
}

void *_sb_free(void *ptr, const char *file, const char *function)
{
	memory_stat_t *mstat = NULL;
	FILE *fp;

	sb_free(ptr);

	fp = sb_fopen ("memory_free.log", "a");
	if (fp == NULL)
	{
		error("memory_free.log open failed : %s", strerror(errno));
	}
	else
	{
		fprintf(fp, "%p|%d|%s|%s\n", ptr, getpid(), file, function);
		fclose(fp);
	}

	mstat = get_memory_stat(file,function);
	if (mstat != NULL) {
/*		mstat->total -= size;*/
		mstat->nfree++;
	}
	return ptr;
}

void *_sb_realloc(void *ptr, size_t size, const char *file, const char *function)
{
	memory_stat_t *mstat = NULL;

	ptr = realloc(ptr, size);
	if (ptr == NULL) {
		error("realloc failed:%s (requested size:%d)", strerror(errno), (int)size);
		return NULL;
	}
	
	mstat = get_memory_stat(file,function);
	if (mstat != NULL) {
/*		mstat->total += size;*/
		mstat->cumulative_total += size;
		mstat->nrealloc++;

		if (size < SMALL) {
			mstat->section[0] += size;
		}
		else if (size < MEDIUM) {
			mstat->section[1] += size;
		}
		else if (size < LARGE) {
			mstat->section[2] += size;
		}
		else {
			mstat->section[3] += size;
		}
	}
	return ptr;
}

static memory_stat_t *get_memory_stat(const char *file,const char *function)
{
	memory_stat_t *mstat=NULL;
	char key[STRING_SIZE];

	snprintf(key, STRING_SIZE, "%s/%s()", file, function);

	mstat = search_memory_stat(key);
	if (mstat == NULL) {
		return create_memory_stat(key);
	}

	return mstat;
}

static memory_stat_t *search_memory_stat(char *key)
{
	int i, hashkey = get_hash_key(key);

	i = 0;
	while (i<MAX_MSTAT_EXT_SLOT && 
			strncmp(key, memorystatkey[hashkey][i].key, STRING_SIZE) != 0)
		i++;

	if (i == MAX_MSTAT_EXT_SLOT) return NULL;

	return memorystatkey[hashkey][i].stat;
}

static memory_stat_t *create_memory_stat(char *key)
{
	int i, hashkey = get_hash_key(key);
	memory_stat_t *mstat=NULL;

	if (*nmemorystat == MAX_MEMORY_STAT) {
		error("MAX_MEMORY_STAT(%d) is full. increase and recompile",
				MAX_MEMORY_STAT);
		return NULL;
	}

	i=0;
	while (i<MAX_MSTAT_EXT_SLOT && memorystatkey[hashkey][i].key[0]!='\0'){
/*        INFO("memorystatkey[%d][%d].key:|%s|",
 *        			hashkey,i,memorystatkey[hashkey][i].key);*/
		i++;
	}

	if (i == MAX_MSTAT_EXT_SLOT) {
		crit("MAX_MSTAT_EXT_SLOT(%d) is full(hashkey:%d). increase and recompile",
				MAX_MSTAT_EXT_SLOT, hashkey);
		return NULL;
	}

	strcpy(memorystatkey[hashkey][i].key,key);
	mstat = memorystatkey[hashkey][i].stat = &(memorystat[(*nmemorystat)++]);

	mstat->total = 0;
	mstat->cumulative_total = 0;
	mstat->nalloc = 0;
	mstat->nrealloc = 0;
	mstat->nfree = 0;

	mstat->key = &(memorystatkey[hashkey][i]);

	for (i=0; i<MAX_SECTION_NUM; i++) mstat->section[i] = 0;

	return mstat;
}

static int get_hash_key(char *key)
{
	int sum, i;
	for (sum=0, i=0; i<MAX_HASHKEY_LEN && key[i]; i++) {
		sum += key[i];
	}
	return sum % MAX_MSTAT_KEY;
}

/***** registry *****/
REGISTRY void init_memory_stat_key(void *data)
{
	int i=0;

	memset(data, 0x00, 
		sizeof(memory_stat_key_t) * MAX_MSTAT_KEY *
		MAX_MSTAT_EXT_SLOT);

	for (i=0; i<MAX_MSTAT_KEY; i++) {
		memorystatkey[i] = 
			(memory_stat_key_t *)data + i * MAX_MSTAT_EXT_SLOT;
/*        INFO("memorystatkey[%d].key[%s]",i,memorystatkey[i][0].key);*/
	}
}

REGISTRY void init_memory_stat(void *data)
{
	memorystat = (memory_stat_t *)data;
}

#define MSTAT_STRTING_LEN LONG_LONG_STRING_SIZE * 2
REGISTRY char *get_memory_stat_string()
{
	char buf[STRING_SIZE];
	static char result[MSTAT_STRTING_LEN];
	int i, left;
	left = MSTAT_STRTING_LEN;

	result[0]='\0';
	CRIT("nmemorystat:%d",*nmemorystat);
	result[0]='\0';
	for (i=0; i<*nmemorystat; i++) {
		sprintf(buf, "\n%s:cumulative_total[%.2fM] nalloc[%d] nrealloc[%d] nfree[%d]\n"
				"\t0-64k[%.3fk] 64k-1M[%.2fM] 1M-10M[%.1fM] 10M-[%.0fM]\n", 
				memorystat[i].key->key,
				(float)memorystat[i].cumulative_total/1024/1024, 
				memorystat[i].nalloc,
				memorystat[i].nrealloc,
				memorystat[i].nfree,
				(float)memorystat[i].section[0]/1024, 
				(float)memorystat[i].section[1]/1024/1024,
				(float)memorystat[i].section[2]/1024/1024, 
				(float)memorystat[i].section[3]/1024/1024);
		strncat(result, buf, left>0?left:0);
		left -= strlen(buf);
	}
	result[MSTAT_STRTING_LEN-1] = '\0';
	return result;
}

REGISTRY void init_nmemorystat(void *data)
{
	nmemorystat = (int *)data;
	*nmemorystat = 0;
}

static registry_t registry[] = {
	RUNTIME_REGISTRY("MemoryStatKey", "Key(filename) for memory statistic", \
		sizeof(memory_stat_key_t) * MAX_MSTAT_KEY * MAX_MSTAT_EXT_SLOT, \
		init_memory_stat_key, NULL, NULL),
	RUNTIME_REGISTRY("MemoryStat", "Memory use statistic", \
		sizeof(memory_stat_t) * MAX_MEMORY_STAT, \
		init_memory_stat, get_memory_stat_string, NULL),
	RUNTIME_REGISTRY("MemoryStatNum", "number of key(filename) for memory" \
		"statistic", sizeof(int), init_nmemorystat, NULL, NULL),
	NULL_REGISTRY
};
module memory_module = {
	CORE_MODULE_STUFF,
	NULL,				/* config */
	registry,			/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	NULL				/* register hook api */
};

#endif

module memory_module = {
	CORE_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	NULL				/* register hook api */
};

