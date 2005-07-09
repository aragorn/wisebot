#include "mod_sfs.h"
#include <time.h>
#include <stdlib.h>

static int sfs_size = 256*1024*1024;
static int block_size = 256;
#ifdef WIN32
static char path[]="sfs0";
#else
static char path[]="../../dat/test/sfs0";
  
/******************************************************************************/
char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
char gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
module *static_modules;
/******************************************************************************/
#endif

#ifdef WIN32
static int open_test_file()
{
    HANDLE hFile = NULL;

    hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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

        error("TEST>can't open file[%s][%s]", path, lpMsgBuf);
        LocalFree( lpMsgBuf );
        return -1;
    }

    return (int)hFile;
}

static void close_test_file(int fd)
{
	sb_close_win(fd);
}
#else
static int open_test_file()
{
    int fd;

    fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
    if (fd == -1) {
        error("cannot open file[%s]: %s", path, strerror(errno));
        return -1;
    }

    return fd;
}

static void close_test_file(int fd)
{
	if ( close(fd) != 0 ) error("close failed: %s", strerror(errno));
}
#endif

#define BUFFER_SIZE (33237*4)
#define REPEAT      (450)
#define FRAG_COUNT  (5)

int main(int argc, char* argv[], char *envp[])
{
	int fd;
	sfs_t *sfs;

	int i, j, k, size, errcnt = 0;
	char *buffer = NULL;

#ifndef WIN32
	init_set_proc_title(argc, argv, envp);
	log_setlevelstr("debug");
#endif

	buffer = (char*) sb_malloc( BUFFER_SIZE );
	if ( buffer == NULL ) {
		error("%s", strerror(errno));
		return -1;
	}

	fd = open_test_file();
	if ( fd == -1 ) return -1;

	sfs = sfs_create(0, fd, 0);
	if ( sfs == NULL ) return -1;

	if ( sfs_format(sfs, O_FAT|O_HASH_ROOT_DIR, sfs_size, block_size) == FAIL )
		return -1;

	if ( sfs_open(sfs, O_MMAP) == FAIL ) return -1;

	if ( sfs_format(sfs, O_FAT|O_HASH_ROOT_DIR, sfs_size, block_size) == FAIL )
		return -1;

	info("test enviroment");
	info("BUFFER_SIZE: %d, REPEAT: %d, FRAG_COUNT: %d", BUFFER_SIZE, REPEAT, FRAG_COUNT);
	info("FILE_SIZE: %d, block_size: %d", sfs_size, block_size);

	/**************************************************
	 * add your code, here
	 **************************************************/

	info("appending...");
	for ( j = 0; j < FRAG_COUNT; j++ ) {
		for ( i = 0; i< REPEAT; i++ ) {
			info("%dth(%d) append", i+1, j+1);

			for ( k = 0; k < BUFFER_SIZE/4; k++ ) {
				((int*) buffer)[k] = j*BUFFER_SIZE/4+k+i;
			}

			size = sfs_append(sfs, i+1, BUFFER_SIZE, buffer);
			if ( size != BUFFER_SIZE ) {
				error("invalid size[%d], expected[%d]. but maybe segment full", size, BUFFER_SIZE);
				break;
			}
		}
		if ( size != BUFFER_SIZE ) break;
	}

	info("reading...");
	for ( k = 0; k < FRAG_COUNT; k++ ) {
		for ( i = 0; i < REPEAT; i++ ) {
			info("%dth(%d) read", i+1, k+1);
			memset( buffer, -1, BUFFER_SIZE );
			size = sfs_read(sfs, i+1, k*BUFFER_SIZE, BUFFER_SIZE, buffer);
			if ( size != BUFFER_SIZE ) {
				error("invalid size[%d], expected[%d]", size, BUFFER_SIZE);
				return -1;
			}

			for ( j = 0; j < BUFFER_SIZE/4; j++ ) {
				if ( ((int*) buffer)[j] != k*BUFFER_SIZE/4+j+i ) {
					error("[%d] invalid value[%d], expected[%d]", i+1, ((int*) buffer)[j], k*BUFFER_SIZE/4+j+i);
					if ( errcnt > 10 ) return -1;
					else errcnt++;
				}
			}
		}
	}

	buffer = sb_realloc( buffer, BUFFER_SIZE*FRAG_COUNT );

    info("reading(one path)...");
    for ( i = 0; FRAG_COUNT > 1 && i < REPEAT; i++ ) {
        info("%dth read", i+1);
        memset( buffer, -1, BUFFER_SIZE * FRAG_COUNT );
        size = sfs_read(sfs, i+1, i*4, BUFFER_SIZE * FRAG_COUNT, buffer);
        if ( size+i*4 != BUFFER_SIZE * FRAG_COUNT ) {
            error("invalid size[%d], expected[%d]", size, BUFFER_SIZE * FRAG_COUNT);
            return -1;
        }

        for ( j = 0; j < BUFFER_SIZE * FRAG_COUNT/4 - i; j++ ) {
            if ( ((int*) buffer)[j] != j+i+i ) {
                error("[%d] invalid value[%d], expected[%d]", i+1, ((int*) buffer)[j], j+i+i);
                if ( errcnt > 10 ) return -1;
                else errcnt++;
            }
        }
    }

	/**************************************************/

	if ( sfs_close(sfs) == FAIL ) return -1;
	if ( sfs_destroy(sfs) == FAIL ) return -1;

	close_test_file(fd);
	sb_free(buffer);

	return 0;
}

