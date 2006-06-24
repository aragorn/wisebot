/* $Id$ */
#define CORE_PRIVATE 1
#include "common_core.h"
#include "log_error.h"
#include "memory.h"
#include "ipc.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_SYS5_IPC (200)

typedef struct {
	int type;
	int id;
	char path[SHORT_STRING_SIZE];
	int pid;
	key_t key;
} ipc_list_t;

static ipc_list_t allocated_ipcs[MAX_SYS5_IPC];
static int last_ipc = 0;

int free_ipcs()
{
	int i, fail = 0, ignored=0;

	for (i = 0; i < last_ipc; i++) {
		switch ( allocated_ipcs[i].type ) {
		case IPC_TYPE_SHM:
			if (shmctl(allocated_ipcs[i].id, IPC_RMID, NULL) != 0) {
				warn("error while marking shared memory to be destroyed: %s",
															strerror(errno));
				fail++;
			}
#ifdef DEBUG_SOFTBOT
			_sb_free_shm(allocated_ipcs[i].id, __FILE__, __FUNCTION__);
#endif //DEBUG_SOFTBOT
			break;
		case IPC_TYPE_SEM:
			if (semctl(allocated_ipcs[i].id, ignored, IPC_RMID) != 0) {
				warn("error while removing semaphore: %s", strerror(errno));
				fail++;
			}
			break;
		default:
			warn("invalid ipc type: %d", allocated_ipcs[i].type);
		}
	}

	if ( fail ) return FAIL;
	return SUCCESS;
}

/* due to lack of uniqueness guarantee by ftok(3) */
/*
static int is_really_allocated_ipc(ipc_t *ipc, const char *file, char *caller)
{
	int i=0,pid=0;
	char *path=NULL;
	key_t key=0;
	for (i=0; i < last_ipc; i++) {
		path=allocated_ipcs[i].path;
		pid=allocated_ipcs[i].pid;
		key=allocated_ipcs[i].key;
		if (strncmp(ipc->pathname, path, SHORT_STRING_SIZE) == 0 &&
				ipc->pid == pid) {
			return TRUE;
		}
		else if (ipc->key == key) {
			crit("Uniqueness of key failed for %s:%s() (%s,%d)",
						file, caller, ipc->pathname, ipc->pid);
			crit("Duplicate key is made by (%s,%d)", path, pid);
			return FALSE;
		}
	}

	error("Should not reach here");
	return FALSE;
}
*/

/*** SYS5 Semaphore ***********************************************************/

int _acquire_lock(int semid,int idx,const char *file,const char* caller)
{
	struct sembuf semopt;
	int n;

	semopt.sem_num = idx;
	semopt.sem_op = -1;

	/* process exit시에 kernel이 sem value를 adjusting할 수 있게 */
	semopt.sem_flg = SEM_UNDO;
	/* interrupt 는 무시하고 계속 기다린다 */
	while ( (n=semop(semid,&semopt,1)) == -1 && errno == EINTR );

	if (n == -1) {
		error("semop failed while acquiring lock(semid=%d,file=%s,caller=%s): %s",
				semid, file, caller, strerror(errno));
		return FAIL;
	}
	return SUCCESS;
}

int _acquire_lock_nowait(int semid,int idx,const char* file, const char* caller)
{
	struct sembuf semopt;

	semopt.sem_num = idx;
	semopt.sem_op = -1;

	/* process exit시에 kernel이 sem value를 adjusting할 수 있게 */
	semopt.sem_flg = SEM_UNDO | IPC_NOWAIT;
	if (semop(semid,&semopt,1) == -1) {
		debug("semop failed while acquiring lock(semid=%d,file=%s,caller=%s): %s",
				semid, file, caller, strerror(errno));
		return FAIL;
	}
	return SUCCESS;
}

int _release_lock(int semid,int idx,const char* file, const char* caller)
{
	struct sembuf semopt;

	semopt.sem_num = idx;
	semopt.sem_op = 1;
	semopt.sem_flg = SEM_UNDO;

	if (semop(semid,&semopt,1) == -1) {
		error("semop failed while releasing lock(semid=%d,file=%s,caller=%s): %s",
				semid, file, caller, strerror(errno));
		return FAIL;
	}
	return SUCCESS;
}

int _get_nsem(ipc_t *ipc,int num,const char* file, const char* caller)
{
	int created=1;
	union semun semarg;
	char path[STRING_SIZE];


	debug("file[%s], caller[%s]", file, caller);

	if (ipc->pathname == NULL) {
		ipc->key = IPC_PRIVATE;
		debug("ipc->pathname is null. use IPC_PRIVATE.");
	}
	else {
		if (ipc->pathname[0] != '/') {
			snprintf(path,STRING_SIZE,"%s/%s",gSoftBotRoot,ipc->pathname);
			path[STRING_SIZE-1] = '\0';
		}
		else {
			strncpy(path,ipc->pathname,STRING_SIZE);
			path[STRING_SIZE-1] = '\0';
		}
		info("get semaphore with path: %s", path);

        ipc->key = ftok(path, ipc->pid);
		if (ipc->key == -1) {
			if (errno == ENOENT) {
				int fd = open(path, O_RDONLY|O_CREAT, 0600);
				info("tried to make empty file for sem - fd = %d, errno = %d - now retrying ftok.", fd, errno);
				if (fd != -1) {
					close(fd);
    				ipc->key = ftok(path, ipc->pid);
    				if (ipc->key == -1) {
        		        crit("error getting key[%s:%s()]: %s[%d]",
        								file,caller,strerror(errno), errno);
                        return FAIL;
                   }
				} else {
        			crit("error while getting key[%s:%s()]: %s[%d]",
        										file,caller,strerror(errno), errno);
        			crit(" path: %s", path);
        			crit(" pid:  %d", ipc->pid);
        			return FAIL;
                }
			}
		} /* if (ipc->key == -1) */
	} /* if (ipc->pathname == NULL) */
	debug("key:"KEY_T_FORMAT,ipc->key);

	/* semget(ipc->key,num, IPC_CREAT|IPC_EXCL|0600);
	 *                                         ~~~~
	 * semget()'s third argument, semflag defines the access permission.
	 * 0600 means that only the owner can access the semaphore.
	 */
	ipc->id = semget(ipc->key,num, IPC_CREAT|IPC_EXCL|0600);
	if (ipc->id == -1) {
		/* semaphore already exist */
		notice("[%s:%s()] use existing semaphore.", file, caller);

		ipc->id = semget(ipc->key,num,0);
        if(ipc->id == -1) {
		    crit("[%s:%s()] error while semget(2): %s",file, caller, strerror(errno));
		    return FAIL;
        }
		created=0;
	} else { /* semaphore (newly) created, we do initialization here */
		/* TODO: we get semarg as argument to get_sem and sizeof semarg
		 *       and pass it to semctl SETVAL
		 *         2002/07/11 --jiwon
		 */
		int i;
		for (i=0; i<num; i++) {
			semarg.val = 1;
			if (semctl(ipc->id,i,SETVAL,semarg) == -1) {
				error("[%s:%s()] semctl(2) SETVAL error: %s", file, caller, strerror(errno));
				return FAIL;
			}
		}
		created=1;
	}
	/* semaphore will be removed automatically when shutdown */
	if (created) {
		allocated_ipcs[last_ipc].type = IPC_TYPE_SEM;
		allocated_ipcs[last_ipc].id = ipc->id;
		allocated_ipcs[last_ipc].key = ipc->key;
		if (ipc->pathname != NULL) {
			strncpy(allocated_ipcs[last_ipc].path,ipc->pathname,SHORT_STRING_SIZE);
		} else {
			allocated_ipcs[last_ipc].path[0]='\0';
		}
		allocated_ipcs[last_ipc].path[SHORT_STRING_SIZE-1] = '\0';
		allocated_ipcs[last_ipc].pid=ipc->pid;
		last_ipc++;
		if ( last_ipc == MAX_SYS5_IPC ) {
			warn("cannot save ipc id for later disposal.");
			warn("array is full with size[%d].", MAX_SYS5_IPC);
			last_ipc--;
		}
	}

	info("[%s:%s()] got semaphore[%d]: %d", file, caller, ipc->id, num);
	
	return SUCCESS;
}

int add_semid_to_allocated_ipcs(int semid)
{
	allocated_ipcs[last_ipc].type = IPC_TYPE_SEM;
	allocated_ipcs[last_ipc].id = semid;
	allocated_ipcs[last_ipc].path[0]='\0';
	last_ipc++;
	if ( last_ipc == MAX_SYS5_IPC ) {
		warn("cannot save ipc id for later disposal.");
		warn("array is full with size[%d].", MAX_SYS5_IPC);
		last_ipc--;
	}

	return SUCCESS;
}

/*** Reader/Writer Lock *******************************************************/
struct rwlock_t {
  pthread_rwlock_t *pthread_rwlock;
};

int rwlock_init(rwlock_t **rwlp)
{
	void *ptr = calloc(1, sizeof(rwlock_t) + sizeof(pthread_rwlock_t));
	if (ptr == NULL) {
		error("cannot calloc(3) for rwlock: %s", strerror(errno));
		return ENOMEM;
	}
	*rwlp = ptr;
	(*rwlp)->pthread_rwlock = ptr + sizeof(rwlock_t);
	SB_DEBUG_ASSERT((void*) *rwlp + sizeof(rwlock_t) == (void*) (*rwlp)->pthread_rwlock);
	/*
	CRIT("ptr=%p,rwlp=%p,*rwlp=%p,sizeof(rwlock_t)=%d,pthread_rwlock=%p",
		ptr, rwlp, *rwlp, sizeof(rwlock_t), (*rwlp)->pthread_rwlock);
	*/

	return pthread_rwlock_init((*rwlp)->pthread_rwlock, NULL);
}

int rwlock_destroy(rwlock_t *rwlp)
{
	int r = pthread_rwlock_destroy(rwlp->pthread_rwlock);
	if (r != 0) error("pthread_rwlock_destroy returned %d", r);
	free(rwlp);
	return r;
}

int rwlock_rdlock(rwlock_t *rwlp)
{
	return pthread_rwlock_rdlock(rwlp->pthread_rwlock);
}

int rwlock_wrlock(rwlock_t *rwlp)
{
	return pthread_rwlock_wrlock(rwlp->pthread_rwlock);
}

int rwlock_unlock(rwlock_t *rwlp)
{
	return pthread_rwlock_unlock(rwlp->pthread_rwlock);
}

int rwlock_tryrdlock(rwlock_t *rwlp)
{
	return pthread_rwlock_tryrdlock(rwlp->pthread_rwlock);
}

int rwlock_trywrlock(rwlock_t *rwlp)
{
	return pthread_rwlock_trywrlock(rwlp->pthread_rwlock);
}

/*** SYS5 Shared Memory *******************************************************/
int _alloc_shm(ipc_t *ipc,const char* file,const char* caller)
{
	static int counter = 0, size=0;
	int created=1, retry_limit=10;
	char path[STRING_SIZE];

	if ( ipc->size == 0 ) {
		error("invalid size: %d (from file:%s, caller:%s)", ipc->size, file, caller);
		return FAIL;
	}

	counter++;
	size += ipc->size;

    if(ipc->pathname != NULL) {
	   info("counter:%d, size:%d, pathname:%s", counter, size, ipc->pathname);
    } else {
	   info("counter:%d, size:%d, pathname:%s", counter, size, "");
    }

	if ( ipc->pathname == NULL ) {
		ipc->key = IPC_PRIVATE;
	} 
	else {
		if (ipc->pathname[0] != '/') {
			snprintf(path,STRING_SIZE,"%s/%s",gSoftBotRoot,ipc->pathname);
			path[STRING_SIZE-1] = '\0';
		}
		else {
			strncpy(path,ipc->pathname,STRING_SIZE);
			path[STRING_SIZE-1] = '\0';
		}

		// brian added this -- create an empty file for the segment if it wasn't there already.
        ipc->key = ftok(path, ipc->pid);

		if (ipc->key == -1) {
			if (errno == ENOENT) {
				int fd = open(path, O_RDONLY|O_CREAT, 0600);
				crit("tried to make empty file for shm - fd = %d, errno = %d - now retrying ftok.", fd, errno);

				if (fd != -1) {
                    ipc->key = ftok(path, ipc->pid);
        		    if (ipc->key == -1) {
        				crit("tried to make empty file for shm - fd = %d, errno = %d - now retrying ftok.", fd, errno);
        				return FAIL;
                    }
					close(fd);
				} else {
        			crit("error while getting key[%s:%s()]: %s[%d]", 
        							file, caller, strerror(errno), errno);
        			crit(" path: %s", path);
        			crit(" pid : %d", ipc->pid);
        		   return FAIL;
				}
			}
		}
	}
	info("key:"KEY_T_FORMAT,ipc->key);

RETRY:
	ipc->id = shmget(ipc->key, ipc->size, SHM_R|SHM_W|IPC_CREAT|IPC_EXCL);

	if (ipc->id == -1 && errno == EEXIST) {
//		struct shmid_ds ds;

		notice("using existing shared memory[%s:%s()]",file,caller);
		ipc->id = shmget(ipc->key, 0, SHM_R|SHM_W);
		if (ipc->id == -1) {
			crit("error while trying to use existing shared memory[%s:%s()]: %s",
									file, caller, strerror(errno));
			return FAIL;
		}

#if 0
		/* attach count가 0이면 created로 간주한다.
		 * ipc->attr = SHM_CREATED 가 되면, caller가 해당 shared memory를 적절히
		 * 초기화할 것이다.
		 * 이렇게 되면 엔진 종료 후 비정상적으로 남은 shared memory를 재활용할 때
		 * 오류를 방지할 수 있다.
		 * added by chaeyk
		 */
		if ( shmctl( ipc->id, IPC_STAT, &ds ) < 0 ) {
			crit("error with shmctl: %s:%s():%s", file, caller, strerror(errno));
			return FAIL;
		}

		if ( ds.shm_nattch == 0 ) {
			warn("shm_nattch == 0. so, we regard it as created shared memory.");
			ipc->attr = SHM_CREATED;
			created = 1;
		} else {
			ipc->attr = SHM_ATTACHED;
			created = 0;
		}
#endif
		ipc->attr = SHM_ATTACHED;
		created = 0;
	}
	else if (ipc->id == -1) {
		crit("error while getting shared memory[%s:%s()]: %s [%d]", 
				file, caller, strerror(errno), errno);
		crit("HINT: check the maximum size for shared memory segment, "
			 "SHMMAX value of the kernel.");
		info("    caller: %s, file: %s", caller, file);
		info("    path: %s", ipc->pathname);
		info("    key: "KEY_T_FORMAT, ipc->key);
		info("    size: %d", ipc->size);
		
		ipc->key++;

		retry_limit--;
		if ( retry_limit ) goto RETRY;

		return FAIL;
	}

	ipc->addr = shmat(ipc->id,NULL,SHM_R|SHM_W);
	info("shmat(ipc->id:%d) returned ipc->addr:%p, errno:%d, [%s]", ipc->id, ipc->addr, errno, strerror(errno));
	if ((intptr_t)(ipc->addr) == -1) {
		crit("error while attaching shared memory[%s:%s()]: %s", 
			  file, caller, strerror(errno));
		if (errno == EMFILE)
			crit("HINT: check the maximum number of shm segments \
attached to the calling process, SHMSEG value of the kernel.");
		else
			crit("EACCES[%d] EINVAL[%d] ENOMEM[%d] this errno:%d",
			  EACCES, EINVAL, ENOMEM, errno);
		return FAIL;
	}

	/* shared memory will be removed automatically when shutdown */
	if (created) {
#ifdef DEBUG_SOFTBOT
		_sb_alloc_shm(ipc->id, ipc->size, file, caller);
#endif //DEBUG_SOFTBOT
		/* initialize the newly allocated memory */
		memset(ipc->addr, 0x00, ipc->size);
		
		ipc->attr = SHM_CREATED;
		
		allocated_ipcs[last_ipc].type = IPC_TYPE_SHM;
		allocated_ipcs[last_ipc].id = ipc->id;
// #ifdef DEBUG_SOFTBOT XXX: conditional compilation needed?
		allocated_ipcs[last_ipc].key = ipc->key;
		if (ipc->pathname != NULL) {
			strncpy(allocated_ipcs[last_ipc].path,ipc->pathname,SHORT_STRING_SIZE);
		} else {
			allocated_ipcs[last_ipc].path[0]='\0';
		}
		allocated_ipcs[last_ipc].path[SHORT_STRING_SIZE-1] = '\0';
		allocated_ipcs[last_ipc].pid=ipc->pid;
// #endif 			
		last_ipc++;
		if ( last_ipc == MAX_SYS5_IPC ) {
			warn("cannot save ipc id to remove later. array is full with size[%d].",
																	MAX_SYS5_IPC);
			last_ipc--;
		}
	}

	info("[%s:%s()] got shared memory[%d], size[%d], addr[%p]",
			file, caller, ipc->id, ipc->size, ipc->addr);
	return SUCCESS;
}

/**** mmap *****************************************************************/

/*********************************************************
 * _alloc_mmap()
 *
 *  INPUT VALUE
 *   ipc->type     : IPC_TYPE_MMAP (don't need)
 *   ipc->pathname : mmap file
 *   ipc->size     : file size
 *   offset        : mmap offset
 *
 *  RETURN VALUE
 *   return value  : SUCCESS or FAIL
 *   ipc->addr     : assigned address
 *   ipc->attr     : MMAP_CREATED - 파일 생성 or 확장
 *                   MMAP_ATTACHED - 원래 있던 파일...
 *********************************************************/
int _alloc_mmap(ipc_t *ipc, off_t offset, const char* file, const char* caller)
{
	char path[STRING_SIZE];
	int fd, ret;
	off_t off;
	int offset_correction;
	char zero = '\0'; // end of file 에 기록할 byte

	if ( ipc->pathname[0] != '/' ) {
		snprintf(path,STRING_SIZE,"%s/%s",gSoftBotRoot,ipc->pathname);
		path[STRING_SIZE-1] = '\0';
	}
	else {
		strncpy(path,ipc->pathname,STRING_SIZE);
		path[STRING_SIZE-1] = '\0';
	}

	fd = open( path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );
	if ( fd < 0 ) {
		error( "open() failed: [%s:%s()], %s", file, caller, strerror(errno) );
		return FAIL;
	}

	off = lseek( fd, 0, SEEK_END );
	if ( off == (off_t)-1 ) {
		error( "lseek() failed: [%s:%s()], %s", file, caller, strerror(errno) );
		close( fd );
		return FAIL;
	}

	if ( off < offset+ipc->size ) {
		// expand file
		off = lseek( fd, offset+ipc->size-1, SEEK_SET );
		if ( off == (off_t)-1 ) {
			error( "error seeking file[%s] to size[%d]: [%s:%s()], %s",
					ipc->pathname, ipc->size, file, caller, strerror(errno) );
			close( fd );
			return FAIL; 
		}

		ret = write( fd, &zero, 1 );
		if ( ret != 1 ) {
			error( "error expanding file[%s] to size[%d]: [%s:%s()], %s",
					ipc->pathname, ipc->size, file, caller, strerror(errno) );
			close( fd );
			return FAIL; 
		}

		ipc->attr = MMAP_CREATED;
		info( "file created: %s", ipc->pathname );
	}
	else {
		ipc->attr = MMAP_ATTACHED;
		info( "use existing file: %s", ipc->pathname );
	}

	offset_correction = (int) (offset % getpagesize());
	if ( offset_correction != 0 ) {
		info( "offset["OFF_T_FORMAT"] is not multiple of PAGE_SIZE[%d], but OK - correction[%d]",
				offset, getpagesize(), offset_correction);
	}

	ipc->addr = mmap( NULL, ipc->size + offset_correction,
					PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset - offset_correction );
	if ( ipc->addr == MAP_FAILED ) {
		error( "mmap() failed: [%s:%s()], size:%d, offset:"OFF_T_FORMAT", %s",
				file, caller, ipc->size, offset, strerror(errno) );
		close( fd );
		return FAIL;
	}
	else ipc->addr += offset_correction;

	info( "[%s:%s()] mmap successed at[0x%p]: %s("OFF_T_FORMAT"~"OFF_T_FORMAT")",
			file, caller, ipc->addr, ipc->pathname,
			offset, offset+ipc->size );
	close( fd );
	return SUCCESS;
}

int _sync_mmap(void* start, int size, const char* file, const char* caller)
{
	int ret;
	int offset_correction;
   
	offset_correction = (int)((uintptr_t)start % getpagesize());
	if ( offset_correction != 0 ) {
		crit( "offset[0x%p] is not multiple of PAGE_SIZE[%d] - correction[%d]",
				start, getpagesize(), offset_correction);
	}

	ret = msync( start-offset_correction, size+offset_correction, MS_ASYNC );
	if ( ret == -1 ) {
		error( "msync failed [%s:%s()]: %s", file, caller, strerror(errno) );
		return FAIL;
	}
	else return SUCCESS;
}

int _free_mmap(void* start, int size, const char* file, const char* caller)
{
	int ret;
	int offset_correction;
   
	offset_correction = (int)((uintptr_t)start % getpagesize());
	if ( offset_correction != 0 ) {
		crit( "offset[0x%p] is not multiple of PAGE_SIZE[%d] - correction[%d]",
				start, getpagesize(), offset_correction);
	}

	ret = munmap( start-offset_correction, size+offset_correction );
	if ( ret == 0 ) return SUCCESS;
	else {
		error( "munmap() failed: [%s:%s()], %s", file, caller, strerror(errno) );
		return FAIL;
	}
	info( "munmap() called[0x%p, %d]", start, size);
}
