/* $Id$ */

#include "wisebot.h"

#ifdef WIN32
int gettimeofday( struct timeval* tv, void* timezone ) 
{
    FILETIME time;
    double   timed;

    GetSystemTimeAsFileTime( &time );

    // Apparently Win32 has units of 1e-7 sec (tenths of microsecs)
    // 4294967296 is 2^32, to shift high word over
    // 11644473600 is the number of seconds between
    // the Win32 epoch 1601-Jan-01 and the Unix epoch 1970-Jan-01
    // Tests found floating point to be 10x faster than 64bit int math.

    timed = ((time.dwHighDateTime * 4294967296e-7) - 11644473600.0) +
            (time.dwLowDateTime  * 1e-7);

    tv->tv_sec  = (long) timed;
    tv->tv_usec = (long) ((timed - tv->tv_sec) * 1e6);

    return 0;
}

double timediff(struct timeval *later, struct timeval *first)
{
    return (double)(later->tv_sec - first->tv_sec +
                    (double)(later->tv_usec - first->tv_usec)/1000000);
}

void error (char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void crit(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void info(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void debug(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void notice(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void warn(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

int sb_open_win(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
				  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
				  DWORD dwCreationDisposition,
				  DWORD dwFlagsAndAttributes,
				  HANDLE hTemplateFile)
{
	HANDLE hFile = CreateFile(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if( hFile == INVALID_HANDLE_VALUE ) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("can't open file[%s][%s]", lpFileName, lpMsgBuf);
		LocalFree( lpMsgBuf );

		return -1;
	}

	return (int)hFile;
}

int sb_read_win(int fd, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
				LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	HANDLE hFile = (HANDLE)fd;
	DWORD read_byte;

	if(!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, &read_byte, lpOverlapped)) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("can't read file[%d][%s]", fd, lpMsgBuf);
		LocalFree( lpMsgBuf );
		
		return FAIL;
	}

	return SUCCESS;
}

int sb_seek_win(int fd, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	DWORD dwPtrLow, dwError;
	HANDLE hFile = (HANDLE)fd;

	dwPtrLow = SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);

	if (dwPtrLow == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR )
	{
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("can't seek file[%d][%s]", fd, lpMsgBuf);
		LocalFree( lpMsgBuf );

		return FAIL;
	}
	
	return SUCCESS;
}

int sb_write_win(int fd, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
				 LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	DWORD write_byte;
	HANDLE hFile = (HANDLE)fd;

	if(!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, &write_byte, lpOverlapped)) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("can't write file[%d][%s]", fd, lpMsgBuf);
		LocalFree( lpMsgBuf );
		
		return FAIL;
	}

	return SUCCESS;
}

void sb_close_win(int fd)
{
	HANDLE hFile = (HANDLE)fd;
	if(!CloseHandle(hFile)) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("can't write file[%d][%s]", fd, lpMsgBuf);
		LocalFree( lpMsgBuf );
	}
}

void get_sem(ipc_t* ipc)
{
	unsigned int i = 0;
	HANDLE hSemaphore;
	LONG cMax = 1;
	char path[MAX_PATH_LEN];

	sprintf(path, "%s%c", ipc->pathname, ipc->pid);
	for(i = 0; i < strlen(path); i++) {
		if(path[i] == '\\') path[i] = '_';
	}

	// Create a semaphore with initial and max. counts of 10.
	hSemaphore = CreateSemaphore( 
		NULL,   // no security attributes
		cMax,   // initial count
		cMax,   // maximum count
		path);  // unnamed semaphore

	if (hSemaphore == NULL) {
		error("can not create semaphore");
	} else {
		ipc->id = (int)hSemaphore;
	}
}

void acquire_lock(int handle)
{
	HANDLE hSemaphore = (HANDLE)handle;
	DWORD ret = 0;

	ret = WaitForSingleObject(hSemaphore, 0L);

	switch (ret) { 
    case WAIT_OBJECT_0: 
		//info("The semaphore object was signaled");
        break; 
    case WAIT_TIMEOUT: 
		info("Semaphore was nonsignaled, so a time-out occurred");
        break; 
	}
}

void release_lock(int handle)
{
	HANDLE hSemaphore = (HANDLE)handle;

	if (!ReleaseSemaphore( 
        hSemaphore,  // handle to semaphore
        1,           // increase count by one
        NULL) )      // not interested in previous count
	{
		error("can not release semaphore");
	}
}
#endif
