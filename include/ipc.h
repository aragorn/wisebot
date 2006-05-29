/* $Id$ */
#ifndef IPC_H
#define IPC_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#include <sys/types.h> /* key_t */

#if !defined(SHM_R)
#define SHM_R	0400
#endif
#if !defined(SHM_W)
#define SHM_W	0600
#endif

/* FIXME well, let me see... i'm not sure below ipc_type is needed.
 * i'll make a decision soon. --aragorn*/
#define IPC_TYPE_SHM    (100)
#define IPC_TYPE_SEM    (200)
#define IPC_TYPE_MSG    (300)
#define IPC_TYPE_MMAP   (400)

#define SHM_CREATED		(400)
#define SHM_ATTACHED	(500)

#define MMAP_CREATED    (400)
#define MMAP_ATTACHED   (500)

typedef struct ipc_t ipc_t;
struct ipc_t {
	int type;   /* ipc type which is defined above */
	int id;		/* shmid, semid or msgid */
	char *pathname;
	int pid;	/* project id of ftok(3) */
	key_t key;
	int size;
	void *addr; /* in case of shared memory */
	int attr;
};

/* ftok(3) project id */
#define SYS5_ACCEPT		'A'   /* semaphore for accept(2) lock */
#define SYS5_REGISTRY	'R'
#define SYS5_SCOREBOARD	'S'
#define SYS5_DATA		'D'
#define SYS5_VRF		'V'
#define SYS5_INDEXER	'I'
#define SYS5_LEXICON	'L'
#define SYS5_RMAC2      'Q'   /* rmac2, spool은 동시사용안함 */
#define SYS5_SPOOL		'Q'
#define SYS5_INDEXER_SPOOL	's'
#define SYS5_DOCID		'O'
#define SYS5_VRM		'v'
#define SYS5_FRM		'f'
#define SYS5_IFS		'i'
#define SYS5_CDM		'd'

// refer to man page of semctl(2) for followings
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                  /* value for SETVAL */
	struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;    /* array for GETALL, SETALL */
						   /* Linux specific part: */
	struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif

#define acquire_lock(semid)	_acquire_lock(semid,0,__FILE__,__FUNCTION__)
#define acquire_lock_nowait(semid)	_acquire_lock_nowait(semid,0,__FILE__,__FUNCTION__)
#define release_lock(semid)	_release_lock(semid,0,__FILE__,__FUNCTION__)
#define get_sem(ipc) _get_nsem(ipc,1,__FILE__,__FUNCTION__)

#define acquire_lockn(semid,idx)	_acquire_lock(semid,idx,__FILE__,__FUNCTION__)
#define acquire_lockn_nowait(semid,idx)	_acquire_lock_nowait(semid,idx,__FILE__,__FUNCTION__)
#define release_lockn(semid,idx)	_release_lock(semid,idx,__FILE__,__FUNCTION__)
#define get_nsem(ipc,nsem) _get_nsem(ipc,nsem,__FILE__,__FUNCTION__)

SB_DECLARE(int) _acquire_lock(int semid,int idx,const char *file,const char* caller);
SB_DECLARE(int) _acquire_lock_nowait(int semid,int idx,const char *file,const char* caller);
SB_DECLARE(int) _release_lock(int semid,int idx,const char *filename,const char* caller);
SB_DECLARE(int) _get_nsem(ipc_t *ipc,int num,const char* file,const char* caller);
SB_DECLARE(int) add_semid_to_allocated_ipcs(int semid);

#define alloc_shm(ipc) _alloc_shm(ipc,__FILE__,__FUNCTION__)

SB_DECLARE(int) _alloc_shm(ipc_t *ipc,const char* file,const char* caller);

#define alloc_mmap(ipc,offset) _alloc_mmap(ipc,offset,__FILE__,__FUNCTION__)
#define sync_mmap(start,size) _sync_mmap(start,size,__FILE__,__FUNCTION__)
#define free_mmap(start,size) _free_mmap(start,size,__FILE__,__FUNCTION__)

SB_DECLARE(int) _alloc_mmap(ipc_t *ipc,off_t offset,const char* file,const char* caller);
SB_DECLARE(int) _sync_mmap(void* start,int size,const char* file,const char* caller);
SB_DECLARE(int) _free_mmap(void* start,int size,const char* file,const char* caller);

SB_DECLARE(int) free_ipcs(void);

#ifdef CORE_PRIVATE
# ifdef CYGWIN
#  define OFF_T_FORMAT "0x%llX"
#  define KEY_T_FORMAT "%lld"
# else
#  define OFF_T_FORMAT "0x%lX"
#  define KEY_T_FORMAT "%d"
# endif
#endif /* CORE_PRIVATE */

#endif
