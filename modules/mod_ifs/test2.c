#include "test.h"
#include "../mod_sfs/mod_sfs.h"
#include "../mod_sfs/shared_memory.h"
#include "table.h"
#include "mod_ifs_defrag.h"
//#include "../mod_sfs/super_block.h"

//static int segment_size = 128*1024*1024;
//static int block_size = 128;

static char path[]="dat/test/ifs";

/******************************************************************************/
char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
char gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
module *static_modules;
/******************************************************************************/

//#define BUFFER_SIZE (3546*4)
//#define REPEAT      (26790)
//#define FRAG_COUNT  (5)

#define BUFFER_SIZE (846*4)
#define REPEAT      (16790)
#define FRAG_COUNT  (5)

int main(int argc, char* argv[], char* envp[])
{
	char *buffer=NULL;
	index_db_t *indexdb;
	ifs_t *ifs;
	int i, j, k, size, errcnt = 0;
	int arg = 0;
	int start_offset;
	ifs_set_t local_ifs_set[MAX_INDEXDB_SET];

    init_set_proc_title(argc, argv, envp);
    log_setlevelstr("debug");

	// make ifs_set
	memset(local_ifs_set, 0x00, sizeof(local_ifs_set));
	ifs_set = local_ifs_set;
	ifs_set[0].set = 1;
	ifs_set[0].set_ifs_path = 1;
	strncpy( ifs_set[0].ifs_path, path, MAX_PATH_LEN-1 );
	ifs_set[0].set_segment_size = 1;
	ifs_set[0].segment_size = segment_size;
	ifs_set[0].set_block_size = 1;
	ifs_set[0].block_size = block_size;
    
	if ( BUFFER_SIZE % 4 != 0 ) {
		error("invalid BUFFER_SIZE[%d]", BUFFER_SIZE);
		return -1;
	}

	if ( argc > 1 ) arg = atoi(argv[1]);
	if ( arg == 0 ) error("need arg 1~15");

	temp_alive_time = 0;

	info("test enviroment");
	info("BUFFER_SIZE: %d, REPEAT: %d, FRAG_COUNT: %d", BUFFER_SIZE, REPEAT, FRAG_COUNT);
	info("FILE_SIZE: %d", BUFFER_SIZE*FRAG_COUNT);
	info("sizeof(super_block_t): %d", sizeof(super_block_t));

	ifs_init();
	if ( ifs_open(&indexdb, 0) != SUCCESS ) {
		error("ifs open failed");
		return -1;
	}
	ifs = (ifs_t*)indexdb->db;

	table_print( &ifs->shared->mapping_table );

	buffer = (char*) sb_malloc( BUFFER_SIZE );
	i = ifs_getsize( indexdb, 1378947 );
	info("%d", i);
	i = ifs_read( indexdb, 1378947, 0, 100, buffer );
	info("%d", i);

	if ( (arg & 0x01) == 0 ) goto read;
	info("appending...");
	for ( j = 0; j < FRAG_COUNT; j++ ) {

		for ( i = 0; i < REPEAT; i++ ) {
			info("%dth(%d) append", i+1, j+1);

			for ( k = 0; k < BUFFER_SIZE/4; k++ ) {
				((int*) buffer)[k] = j*BUFFER_SIZE/4+k+i;
			}

			size = ifs_append(indexdb, i+1, BUFFER_SIZE, buffer);
			if ( size != BUFFER_SIZE ) {
				error("invalid size[%d], expected[%d]", size, BUFFER_SIZE);
				return -1;
			}
		}
	}

read:
	if ( (arg & 0x02) == 0 ) goto readfull;
	info("reading...");
	for ( k = 0; k < FRAG_COUNT; k++ ) {
		for ( i = 0; i < REPEAT; i++ ) {
			info("%dth(%d) read", i+1, k+1);
			memset( buffer, -1, BUFFER_SIZE );
			size = ifs_read(indexdb, i+1, k*BUFFER_SIZE, BUFFER_SIZE, buffer);
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

readfull: // 1번 파일은 0부터 읽고 2번 파일은 4~ 부터 읽고...
	if ( (arg & 0x04) == 0 ) goto defrag;
	buffer = sb_realloc( buffer, BUFFER_SIZE * FRAG_COUNT );

	info("reading(one path)...");
	for ( i = 0; FRAG_COUNT > 1 && i < REPEAT; i++ ) {
		info("%dth read", i+1);
		memset( buffer, -1, BUFFER_SIZE * FRAG_COUNT );

		start_offset = i*4 % (BUFFER_SIZE*FRAG_COUNT);
		size = ifs_read(indexdb, i+1, start_offset, BUFFER_SIZE * FRAG_COUNT, buffer);
		if ( size+start_offset != BUFFER_SIZE * FRAG_COUNT ) {
			error("invalid size[%d], expected[%d]", size, BUFFER_SIZE * FRAG_COUNT);
			return -1;
		}

		for ( j = 0; j < BUFFER_SIZE * FRAG_COUNT/4 - i; j++ ) {
			if ( ((int*) buffer)[j] != j+i + start_offset/4 ) {
				error("[%d] invalid value[%d], expected[%d]", i+1, ((int*) buffer)[j], j+i + start_offset/4);
				if ( errcnt > 10 ) return -1;
				else errcnt++;
			}
		}
	}

defrag:
	defrag_mode = DEFRAG_MODE_COPY;
	defrag_group_size = 5;

	if ( (arg & 0x08) == 0 ) goto end;

    i = ifs_defrag(ifs, NULL);
    if (i == FAIL)
	    error("defragment fail");

end:
	ifs_close(indexdb);

	if ( errcnt > 0 ) error("error count: %d", errcnt);

	return 0;
}

