/* $Id$ */
#ifdef WIN32
#else
#  include <sys/mman.h>
#endif

#include "shared_memory.h"
#include "mod_sfs.h"

int unmmap_memory(void* base_address, int size)
{
#ifdef WIN32
	if( UnmapViewOfFile(base_address) == FAIL) {
		error("fail unmap");
		return FAIL;
	}
#else
	if(munmap(base_address, size) == -1) {
		error("fail unmap");
		return FAIL;
	}
#endif

	return SUCCESS;
}

void* get_shared_memory(int seq, int fd, int offset, int size)
{
	void* p;
	int off = 0;
	int create = 0;
#ifdef WIN32
	char identity[128];
	HANDLE  hMap;
#endif

#ifdef WIN32
	sprintf(identity, "%d", seq);
    
	off = GetFileSize((HANDLE)fd, NULL);

	if(off < offset + size) {
		if(sb_seek_win(fd, offset + size - 1, NULL, SFS_BEGIN) == FAIL) {
			error("fail lseek offset[%d], fd[%d] : %s", 
				  offset+size, fd, strerror(errno));
			return NULL;
		}

		if(sb_write_win(fd, "\0", 1, NULL, NULL) == FAIL) {
			error("fail write offset[%d] : %s", offset+size, strerror(errno));
			return NULL;
		}

		create = 1;
	}

	info("OpenFileMapping, fd[%d], identity[%s]", fd, identity);
	hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, identity);
	if(!hMap) {
		hMap = CreateFileMapping((HANDLE)fd, NULL, PAGE_READWRITE, 0, 0, identity);
		info("CreateFileMapping, fd[%d], identity[%s]", fd, identity);
		if ( !hMap ) {
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

			error("error[%s]", lpMsgBuf);
			LocalFree( lpMsgBuf );

			return NULL;
		}
	}

	info("MapViewOfFile, offset[%d], size[%d], hMap[%d]", offset, size, hMap);
	p = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, offset, size);
	if ( p == 0x00 ) {
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

		error("error[%s]", lpMsgBuf);
		LocalFree( lpMsgBuf );

		error("can't not mapping");
		return NULL;
	}

#else
	off = lseek(fd, 0, SEEK_END);
    if (off == (off_t)-1) {
        error("fail lseek end, fd[%d] : %s", fd, strerror(errno));
        return NULL;
    }

	if(off < offset + size) {
		if(lseek(fd, offset + size - 1, SEEK_SET) != offset+size-1) {
			error("fail lseek offset[%d], fd[%d] : %s", 
				  offset+size, fd, strerror(errno));
			return NULL;
		}

		if(write(fd, "\0", 1) != 1) {
			error("fail write offset[%d] : %s", offset+size, strerror(errno));
			return NULL;
		}

		create = 1;
	}

	p = mmap(0x00, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
	if ( p == MAP_FAILED ) {
            error("can't mmap fd[%d], offset[%d], size[%d]: %s", fd, offset, size, strerror(errno));
	    return NULL;
	}
#endif

	if(create) {
		memset(p, 0x00, size);
	}
	return p;
}
