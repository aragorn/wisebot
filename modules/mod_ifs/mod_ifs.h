/* $Id$ */
#ifndef MOD_IFS_H
#define MOD_IFS_H
#include "mod_api/indexdb.h"
#include "mod_sfs/mod_sfs.h"
#include "table.h"

#define PATH_SEP '/'

#define IFS_MAGIC "IFS0"
#define IFS_FILE_NAME "ifs"
#define SFS_FILE_NAME "sfs"

#define DEFAULT_FILE_SIZE  (1*1024*1024*1024)        /* 1*1024*1024*1024 */
#define DEFAULT_SEGMENT_SIZE (256*1024*1024)
#define DEFAULT_BLOCK_SIZE (128)
#define MAX_FILE_COUNT MAX_SECTOR_COUNT    /* index file max 200Gbyte */

/* __sfs_activate() options : do_format */
#define DO_FORMAT      (1)
#define DO_NOT_FORMAT  (0)

typedef struct {
	int ifs_fd;
    int sfs_fd[MAX_FILE_COUNT];
    sfs_t* sfs[MAX_SECTOR_COUNT*MAX_SEGMENT_COUNT];
	char full_path[MAX_FILE_COUNT][MAX_PATH_LEN];	/* index file system path, one more ifs can not exist in the path */
	int lock;
} local_t;

typedef struct {
    char magic[4];

	table_t mapping_table;		/* index logical/physical mmapping table */
	
    int segment_size;
	int block_size;
	int append_segment;         /* indexer에 의해서만 수정 가능하다 */

	char root_path[MAX_PATH_LEN];	/* index file system path, one more ifs can not exist in the path */
} shared_t;

typedef struct {
	local_t local;
	shared_t* shared;
} ifs_t;

typedef struct {
	int set;

	int set_ifs_path;
	char ifs_path[MAX_PATH_LEN];

	int set_segment_size;
	int segment_size;

	int set_block_size;
	int block_size;

	int set_lock_id;
	int lock_id;
} ifs_set_t;

extern ifs_set_t* ifs_set;

int ifs_init();
int ifs_open(index_db_t** indexdb, int opt);
int ifs_close(index_db_t* indexdb);
int ifs_append(index_db_t* indexdb, int file_id, int size, void* buf);
int ifs_read(index_db_t* indexdb, int file_id, int offset, int size, void* buf);
int ifs_getsize(index_db_t* indexdb, int file_id);
int _ifs_fix_physical_segment_state(ifs_t* ifs);

int __sfs_activate(ifs_t* ifs, int p, int type, int do_format, int format_option);
int __sfs_all_activate(ifs_t* ifs, int* physical_segment_array, int count, int type);
int __sfs_deactivate(ifs_t* ifs, int p);
int __file_open(ifs_t* ifs, int sec);

int __get_start_segment(ifs_t* ifs, int* pseg, int count, int file_id, int offset,
		                               int* start_segment, int* start_offset);

#endif

