/* $Id$ */
#ifndef __WISEBOT_H__
#define __WISEBOT_H__

#ifdef WIN32
	#pragma pack(1)
	#include <stdio.h>
	#include <windows.h>
	#include <io.h>
	#define sb_malloc(a) calloc(a, 1)
	#define sb_free(a) free(a);
	#define FAIL 0
	#define SUCCESS 1
	#define MAX_PATH_LEN (256)

	typedef unsigned int uint32_t;
	typedef int key_t;

	void error(char *fmt, ...);
    void crit(char *fmt, ...);
	void info(char *fmt, ...);
	void debug(char *fmt, ...);
	void notice(char *fmt, ...);
	void warn(char *fmt, ...);

	int gettimeofday( struct timeval* tv, void* timezone );
	double timediff(struct timeval *later, struct timeval *first);

	int sb_open_win(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
					  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
					  DWORD dwCreationDisposition,
					  DWORD dwFlagsAndAttributes,
					  HANDLE hTemplateFile);
	void sb_close_win(int fd);
	int sb_read_win(int fd, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
					LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	int sb_seek_win(int fd, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
	int sb_write_win(int fd, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
					 LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

	/* 동기화 관련 */
	#define IPC_TYPE_SHM    (100)
	#define IPC_TYPE_SEM    (200)
	#define IPC_TYPE_MSG    (300)
	#define IPC_TYPE_MMAP   (400)

	#define SHM_CREATED     (400)
	#define SHM_ATTACHED    (500)

	#define MMAP_CREATED    (400)
	#define MMAP_ATTACHED   (500)

	#define SYS5_ACCEPT             'A'   /* semaphore for accept(2) lock */
	#define SYS5_REGISTRY			'R'	
	#define SYS5_SCOREBOARD			'S'     
	#define SYS5_DATA               'D'
	#define SYS5_VRF                'V'
	#define SYS5_INDEXER			'I'
	#define SYS5_LEXICON			'L'   
	#define SYS5_RMAC2				'Q'   /* rmac2, spool은 동시사용안함 */
	#define SYS5_SPOOL              'Q'
	#define SYS5_INDEXER_SPOOL      's'
	#define SYS5_DOCID              'O'
	#define SYS5_VRM                'v'
	#define SYS5_FRM                'f'
	#define SYS5_IFS                'i'

	typedef struct _ipc_t ipc_t;
	struct _ipc_t {
			int type;   /* ipc type which is defined above */
			int id;         /* shmid, semid or msgid */
			char *pathname;
			int pid;        /* project id of ftok(3) */
			key_t key;
			int size;
			void *addr; /* in case of shared memory */
			int attr;
	};

	void get_sem(ipc_t* ipc);
	void acquire_lock(int handle);
	void release_lock(int handle);
#endif

#endif
