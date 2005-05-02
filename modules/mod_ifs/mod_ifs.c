#include "mod_api/indexdb.h"
#include "mod_ifs.h"
#include "../mod_sfs/shared_memory.h"

ifs_set_t* ifs_set = NULL;
static int current_ifs_set = -1;

static int __move_file_segment(ifs_t* ifs, int from_seg, int to_seg);
int __get_start_segment(ifs_t* ifs, int* pseg, int count, int file_id, int offset, 
							   int* start_segment, int* start_offset);
int __get_file_size(ifs_t* ifs, int* pseg, int count, int file_id);
int __sfs_activate(ifs_t* ifs, int p, int type, int do_format, int format_option);
int __sfs_all_activate(ifs_t* ifs, int* physical_segment_array, int count, int type);
int __sfs_deactivate(ifs_t* ifs, int p);
int __file_open(ifs_t* ifs, int sec);

static int file_size = DEFAULT_FILE_SIZE;

#define ACQUIRE_LOCK() \
	if ( acquire_lock( ifs->local.lock ) != SUCCESS ) { \
		error("acquire_lock failed"); \
		return FAIL; \
	}

#define RELEASE_LOCK() \
	if ( release_lock( ifs->local.lock ) != SUCCESS ) { \
		error("release_lock failed. but go on..."); \
	}

int ifs_init()
{
	ipc_t lock;
	int i;

	if ( ifs_set == NULL ) return SUCCESS;

    lock.type = IPC_TYPE_SEM;
    lock.pid = SYS5_IFS;

	for ( i = 0; i < MAX_INDEXDB_SET; i++ ) {
		if ( !ifs_set[i].set ) continue;

		if ( !ifs_set[i].set_ifs_path ) {
			warn("IfsPath [IndexDbSet:%d] is not set", i);
			continue;
		}

    	lock.pathname = ifs_set[i].ifs_path;

		if ( get_sem(&lock) != SUCCESS )
			return FAIL;

		ifs_set[i].lock_id = lock.id;
		ifs_set[i].set_lock_id = 1;
	}

	return SUCCESS;
}

int __file_open(ifs_t* ifs, int sec)
{
	if(ifs->local.sfs_fd[sec] <= 0) {
		sprintf(ifs->local.full_path[sec], "%s%c%s_%d", ifs->shared->root_path, PATH_SEP, SFS_FILE_NAME, sec);
		info("open %s", ifs->local.full_path[sec]);
		ifs->local.sfs_fd[sec] = sb_open(ifs->local.full_path[sec], O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
		if(ifs->local.sfs_fd[sec] == -1) {
			error("can't open file[%s] : %s", ifs->local.full_path[sec], strerror(errno));
			return FAIL;
		}
	}

	return SUCCESS;
}

// format_option은 do_format이 아니면 필요없다.
int __sfs_activate(ifs_t* ifs, int p, int type, int do_format, int format_option)
{
	int offset = 0;
	segment_info_t info;

	if ( p < 0 ) {
		error("invalid p[%d]", p);
		return FAIL;
	}

	// type이 다르면 다시 열어야 한다.
	if ( ifs->local.sfs[p] != NULL && ifs->local.sfs[p]->type != type ) {
		warn("reopen physical segment[%d]", p);
		__sfs_deactivate( ifs, p );
	}

	if ( ifs->local.sfs[p] != NULL && do_format == DO_NOT_FORMAT ) return SUCCESS;

	if( ifs->local.sfs[p] == NULL ) {
		table_get_segment_info(p, ifs->shared->mapping_table.segment_count_in_sector, &info);
		if( __file_open(ifs, info.sector ) != SUCCESS ) {
			error("cannot activate sfs[%d], sector[%d]", p, info.sector);
			return FAIL;
		}
		
		offset = info.segment * ifs->shared->segment_size;
		ifs->local.sfs[p] = sfs_create(p, ifs->local.sfs_fd[info.sector], offset);
		if( ifs->local.sfs[p] == NULL ) {
			error("cannot activate(sfs_create) sfs[%d], fd[%d], offset[%d]",
				  p, ifs->local.sfs_fd[p], offset);
			return FAIL;
		}
	}

	if( do_format ) {
		if( sfs_format(ifs->local.sfs[p], format_option, 
				ifs->shared->segment_size, ifs->shared->block_size) != SUCCESS ) {
			error("cannot activate(sfs_format) sfs[%d], option[%d], "
				  "segment_size[%d], block_size[%d]",
				  p, format_option, ifs->shared->segment_size, ifs->shared->block_size);
			return FAIL;
		}
	}

	if ( sfs_open(ifs->local.sfs[p], type) != SUCCESS ) {
		error("cannot activate(sfs_open) sfs[%d], type[%d], fd[%d], offset[%d]",
			  p, type, ifs->local.sfs_fd[p], offset);
		return FAIL;
	}

	return SUCCESS;
}

int __sfs_all_activate(ifs_t* ifs, int* physical_segment_array, int count, int type)
{
	int i, p;

	for ( i = 0; i < count; i++ ) {
		p = physical_segment_array[i];
		if ( p == NOT_USE || p == ifs->shared->append_segment ) continue;

		if ( __sfs_activate( ifs, p, type, DO_NOT_FORMAT, O_NONE ) != SUCCESS ) {
			error("return FAIL");
			return FAIL;
		}
	}

	return SUCCESS;
}

int __sfs_deactivate(ifs_t* ifs, int p)
{
	if ( p == NOT_USE ) warn("segment is NOT_USE");
	else if ( p == EMPTY ) warn("segment is EMPTY");

	if ( p < 0 || ifs->local.sfs[p] == NULL ) return SUCCESS;

    sfs_close(ifs->local.sfs[p]);
	sfs_destroy(ifs->local.sfs[p]);

	ifs->local.sfs[p] = NULL;

	return SUCCESS;
}

// segment_size <= 0 이거나 block_size <=0 인 경우는 
// create는 허용하지 않고 load만 된다.
int _ifs_open(ifs_t* ifs, char* root_path, int segment_size, int block_size)
{
	int mod = 0;
	char path[MAX_PATH_LEN];
	int append_segment;

	if ( segment_size > 0 ) {
		mod = file_size % segment_size;
		if(mod != 0) {
			error("cannot create ifs, should be "
				  "'file_size[%d] mod segment_size[%d]' is zero",
				  file_size, segment_size);
			return FAIL;
		}
	}

	sprintf(path, "%s%c%s", root_path, PATH_SEP, IFS_FILE_NAME);

    ifs->local.ifs_fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
    if(ifs->local.ifs_fd == -1) {
        error("can't open file[%s] : %s", path, strerror(errno));
        return FAIL;
    }
	info("open ifs: %s", path);

	ifs->shared = get_shared_memory(2000, ifs->local.ifs_fd, 0, sizeof(shared_t));
	if(ifs->shared == NULL) {
		error("cannot ifs get mmap block, root_path[%s]", root_path);
		return FAIL;
	}

	ACQUIRE_LOCK();

	/* 최초 생성시 */
	if(strncmp(ifs->shared->magic, IFS_MAGIC, strlen(IFS_MAGIC)) != 0) {
		info("ifs created first");
		if ( segment_size <= 0 || block_size <= 0 ) {
			error("invalid segment_size[%d] or block_size[%d]", segment_size, block_size);
			goto fail;
		}

		ifs->shared->segment_size = segment_size;
		ifs->shared->block_size = block_size;
		strncpy(ifs->shared->magic, IFS_MAGIC, strlen(IFS_MAGIC));
		strncpy(ifs->shared->root_path, path, strlen(root_path));
		table_init(&ifs->shared->mapping_table, file_size / segment_size);

		if(table_allocate(&ifs->shared->mapping_table, &append_segment, INDEX) != SUCCESS) {
			error("table_allocated failed. state[%d]", INDEX);
			goto fail;
		}

		if(table_update_logical_segment(&ifs->shared->mapping_table, 0, append_segment) != SUCCESS) {
			error("cannot append segment[%d] to logical table", append_segment);
			goto fail;
		}

		if(__sfs_activate(ifs, append_segment, O_MMAP, DO_FORMAT, O_FAT|O_HASH_ROOT_DIR) != SUCCESS) {
			error("cannot sfs activate, segment[%d]", append_segment);
			goto fail;
		}

		ifs->shared->append_segment = append_segment;
	}
	else {
		append_segment = ifs->shared->append_segment;

		if(__sfs_activate(ifs, append_segment, O_MMAP, DO_NOT_FORMAT, O_NONE) != SUCCESS) {
			error("cannot sfs activate, segment[%d]", append_segment);
			goto fail;
		}
	}

	release_lock(ifs->local.lock);
	return SUCCESS;

fail:
	release_lock(ifs->local.lock);
	return FAIL;
}

int ifs_open(index_db_t** indexdb, int opt)
{
	ifs_t* ifs = NULL;
	index_db_t* index_db = NULL;

	if ( ifs_set == NULL ) {
		error("ifs_set is NULL. you must set IndexDbSet in config file");
		return DECLINE;
	}

	if ( opt >= MAX_INDEXDB_SET || opt < 0 ) {
		error("opt[%d] is invalid. MAX_INDEXDB_SET[%d]", opt, MAX_INDEXDB_SET);
		return FAIL;
	}

	if ( !ifs_set[opt].set ) {
		warn("IndexDbSet[opt:%d] is not defined", opt);
		return DECLINE;
	}

	if ( !ifs_set[opt].set_ifs_path ) {
		error("IfsPath is not set [IndexDbSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !ifs_set[opt].set_segment_size ) {
		error("SegmentSize is not set [IndexDbSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !ifs_set[opt].set_block_size ) {
		error("BlockSize is not set [IndexDbSet:%d]. see config", opt);
		return FAIL;
	}

	if ( !ifs_set[opt].set_lock_id ) {
		error("invalid lock_id [IndexDbSet:%d]. maybe failed from init()", opt);
		return FAIL;
	}

	ifs = (ifs_t*) sb_malloc(sizeof(ifs_t));
	if ( ifs == NULL ) goto error;

	index_db = (index_db_t*) sb_malloc(sizeof(index_db_t));
	if ( index_db == NULL ) goto error;

	memset(&ifs->local, 0x00, sizeof(local_t));
	ifs->local.lock = ifs_set[opt].lock_id;

	if ( _ifs_open(ifs, ifs_set[opt].ifs_path,
				ifs_set[opt].segment_size, ifs_set[opt].block_size) != SUCCESS )
		goto error;

	index_db->set = opt;
	index_db->db = (void*) ifs;

	*indexdb = index_db;
	return SUCCESS;

error:
	if ( ifs ) sb_free( ifs );
	if ( index_db ) sb_free( index_db );

	return FAIL;
}

int ifs_close(index_db_t* indexdb)
{
	ifs_t* ifs;
	int i = 0;

	if ( ifs_set == NULL || !ifs_set[indexdb->set].set )
		return DECLINE;
	ifs = (ifs_t*) indexdb->db;

    for(i = 0; i < MAX_SEGMENT_COUNT*MAX_SECTOR_COUNT; i++) {
		if(ifs->local.sfs[i] != NULL) {
			sfs_close(ifs->local.sfs[i]);
			sfs_destroy(ifs->local.sfs[i]);
			ifs->local.sfs[i] = NULL;
		}
	}

	for(i = 0; i < MAX_FILE_COUNT; i++) {
		if(ifs->local.sfs_fd[i] != 0) {
			close(ifs->local.sfs_fd[i]);
			ifs->local.sfs_fd[i] = 0;
		}
    }

    unmmap_memory(ifs->shared, sizeof(shared_t));
	ifs->shared = NULL;

	sb_free( indexdb->db );
	sb_free( indexdb );

	return SUCCESS;
}

static int __lock_append_segment(ifs_t* ifs, int p)
{
	if(p == ifs->shared->append_segment) {
		ACQUIRE_LOCK();
	}
	return SUCCESS;
}

static void __release_append_segment(ifs_t* ifs, int p)
{
	if(p == ifs->shared->append_segment) {
		RELEASE_LOCK();
	}
}

/*******************************************************************
 * lock을 여러번 잡고 풀고 하는데 
 * lock잡는데 실패했다고 중단하고 그냥 포기할 수가 없다....
 * 관리 잘해야지...
 *******************************************************************/
int ifs_append(index_db_t* indexdb, int file_id, int size, void* buf)
{
	ifs_t* ifs;
    int append_byte = 0;
    int try_append_byte = 0;
	int total_byte = 0;
	int append_segment;
	int physical_segment = 0;
	int free_segment = 0;
	sfs_t* s;

	if ( ifs_set == NULL || !ifs_set[indexdb->set].set )
		return MINUS_DECLINE;
	ifs = (ifs_t*) indexdb->db;

	ACQUIRE_LOCK();

    while(1) {
		append_segment = ifs->shared->append_segment;
		s = ifs->local.sfs[append_segment];
    
		try_append_byte = size - total_byte;
        append_byte = sfs_append(s, file_id, try_append_byte, (char*)buf + total_byte);
		if(append_byte == -1) {
			error("cannot sfs append, file_id[%d], total_byte[%d],"
				  "append_byte[%d]", file_id, total_byte, append_byte);
			goto fail;
		}

		total_byte += append_byte;

		/* full segment */
        if(try_append_byte > append_byte) {
	        info("sfs is full, append_segment[%d], size[%d], append_byte[%d]", append_segment, size, append_byte);

			if(table_allocate(&ifs->shared->mapping_table, &free_segment, INDEX) != SUCCESS) {
				error("cannot allocation fail, state[%d]", INDEX);
				goto fail;
			}

			/* read process를 위해 임계구역을 최소화 한다. */
			RELEASE_LOCK();
            if(__move_file_segment(ifs, append_segment, free_segment) != SUCCESS) {
				error("cannot move file segment[%d]", append_segment);
				return FAIL;
			}
			// ACQUIRE_LOCK()을 쓸 수 없다. 죽이되더라도 진행해야 하기 때문에..
			acquire_lock(ifs->local.lock);

			if(table_update_last_logical_segment(&ifs->shared->mapping_table, free_segment) != SUCCESS) {
				error("cannot add logical segment, maybe logical segment is full, segment[%d]",
					physical_segment);
				goto fail;
			}

			notice("formatting append_segment...");
			if(__sfs_activate(ifs, append_segment, O_MMAP, DO_FORMAT, O_FAT|O_HASH_ROOT_DIR) != SUCCESS) {
				error("format failed. segment[%d], size[%d], block_size[%d]",
						append_segment, ifs->shared->segment_size, ifs->shared->block_size);
				return FAIL;
			}
			notice("formatting done.");

			if(table_append_logical_segment(&ifs->shared->mapping_table, append_segment) != SUCCESS) {
				error("table append failed. segment[%d]", append_segment);
				goto fail;
			}
        } else {
//			debug("append completed, file_id[%d], size[%d]", file_id, size);
			break;
		}
    }

	RELEASE_LOCK();
	return total_byte;

fail:
	RELEASE_LOCK();
	return FAIL;
}

// size보다 적게 읽을 수도 있다.
// 리턴값은 실제 읽은 크기
// INDEXDB_FILE_NOT_EXISTS : 파일 없음
// FAIL : 일반 에러
int ifs_read(index_db_t* indexdb, int file_id, int offset, int size, void* buf)
{
	ifs_t* ifs;
	int read_byte, total_byte;
    int physical_segment_array[MAX_LOGICAL_COUNT];
	int count, table_version;
	int start_segment, start_offset;
	sfs_t* sfs;
	int i, file_exists;

	const int type = O_FILE;

	if ( ifs_set == NULL || !ifs_set[indexdb->set].set )
		return MINUS_DECLINE;
	ifs = (ifs_t*) indexdb->db;

retry:
	ACQUIRE_LOCK();
	count = table_get_read_segments(&ifs->shared->mapping_table, physical_segment_array);
	table_version = ifs->shared->mapping_table.version;
	RELEASE_LOCK();

	if ( __sfs_all_activate(ifs, physical_segment_array, count, type) != SUCCESS ) {
		return FAIL;
	}

	// 파일이 없으면??
    if(__get_start_segment(ifs, physical_segment_array, count, file_id, offset, 
			&start_segment, &start_offset) != SUCCESS) {
		return INDEXDB_FILE_NOT_EXISTS;
	}
    
	total_byte = 0;
	file_exists = 0;
    for(i = start_segment; i < count; i++) {
		if ( physical_segment_array[i] == NOT_USE ) continue;

		if ( __lock_append_segment(ifs, physical_segment_array[i]) != SUCCESS ) {
			error("lock failed");
			return FAIL;
		}

		sfs = ifs->local.sfs[physical_segment_array[i]];
        read_byte = sfs_read(sfs, file_id, start_offset, size - total_byte, buf + total_byte);

		__release_append_segment(ifs, physical_segment_array[i]);

		if ( read_byte == INDEXDB_FILE_NOT_EXISTS ) continue; // 파일 없음
		else if ( read_byte == -1 ) {
			error("cannot sfs read, file_id[%d], total_byte[%d],"
				  "read_byte[%d]", file_id, total_byte, read_byte);
			return FAIL;
		}
        
		file_exists = 1;
		total_byte += read_byte;
        if(total_byte == size) break;
		start_offset = 0;
    }

	if ( table_version != ifs->shared->mapping_table.version ) {
		warn("table_version[%d] is changed to [%d]. retry",
				table_version, ifs->shared->mapping_table.version);
		goto retry;
	}

	if ( file_exists ) return total_byte;
	else {
		error("file[%d] not found", file_id);
		return INDEXDB_FILE_NOT_EXISTS;
	}
}

// 리턴값 : 파일크기
// INDEXDB_FILE_NOT_EXISTS : 파일없음
// FAIL : 일반오류
int ifs_getsize(index_db_t* indexdb, int file_id)
{
	ifs_t* ifs;
    int physical_segment_array[MAX_LOGICAL_COUNT];
	int count, file_size;
	int table_version;

	const int type = O_FILE;

	if ( ifs_set == NULL || !ifs_set[indexdb->set].set )
		return MINUS_DECLINE;
	ifs = (ifs_t*) indexdb->db;

retry:
	ACQUIRE_LOCK();
	count = table_get_read_segments(&ifs->shared->mapping_table, physical_segment_array);
	table_version = ifs->shared->mapping_table.version;
	RELEASE_LOCK();

	if ( __sfs_all_activate(ifs, physical_segment_array, count, type) != SUCCESS ) {
		return FAIL;
	}

	file_size = __get_file_size(ifs, physical_segment_array, count, file_id);

	if ( table_version != ifs->shared->mapping_table.version ) {
		warn("table_version[%d] is changed to [%d]. retry",
				table_version, ifs->shared->mapping_table.version);
		goto retry;
	}

	return file_size;
}

/*
 *     seg = segment array
 *     count = segment(seg) 개수
 *     offset = 23 의 시작 위치
 *     sfs 시작위치 start = 2
 *     sfs 내에서의 오프셋 in_offset = 3
 *
 *           0              1             2            3         segment number
 *     +-------------+-------------+-------------+-------------+
 *     |     10      |     10      |     20      |     6       | file_size
 *     +-------------+-------------+-------------+-------------+
 *   
 */
int __get_start_segment(ifs_t* ifs, int* pseg, int count, int file_id, int offset, 
							   int* start_segment, int* start_offset)
{
	int i = 0, ret;
	int total_size = 0;
	sfs_info_t sfs_info;

    *start_offset = offset;
    
    for(i = 0; i < count; i++) {
		if ( pseg[i] == NOT_USE ) continue;

		if ( __lock_append_segment(ifs, pseg[i]) != SUCCESS ) {
			error("lock failed");
			return FAIL;
		}

		ret = sfs_get_info(ifs->local.sfs[pseg[i]], &sfs_info, file_id);
		__release_append_segment(ifs, pseg[i]);

		if ( ret == INDEXDB_FILE_NOT_EXISTS ) {
			continue;  /* not exist directory entry */
		}
		else if ( ret != SUCCESS ) {
			error("failed. file[%d]", file_id);
			return FAIL;
		}

        if(sfs_info.sfs_entry_info.file_size == 0) continue;
        
		total_size += sfs_info.sfs_entry_info.file_size;
        if(total_size > offset) {
            *start_segment = i;
            return SUCCESS;
        }

		*start_offset -= sfs_info.sfs_entry_info.file_size;
    }
    
    return FAIL;
}

int __get_file_size(ifs_t* ifs, int* pseg, int count, int file_id)
{
	int i, total_size = 0;
	int file_exists = 0;
	sfs_info_t sfs_info;
	int ret;

    for(i = 0; i < count; i++) {
		if ( pseg[i] == NOT_USE ) continue;

		if ( __lock_append_segment(ifs, pseg[i]) != SUCCESS ) {
			error("lock failed");
			return FAIL;
		}

        ret = sfs_get_info(ifs->local.sfs[pseg[i]], &sfs_info, file_id);
		__release_append_segment(ifs, pseg[i]);

		if ( ret == INDEXDB_FILE_NOT_EXISTS ) {
			continue;  /* not exist directory entry */
		}
		else if ( ret != SUCCESS ) {
			error("failed. file[%d]", file_id);
			return FAIL;
		}

		file_exists++;
		total_size += sfs_info.sfs_entry_info.file_size;
    }
    
	if ( file_exists ) return total_size;
	else return INDEXDB_FILE_NOT_EXISTS;
}

#define MAX_BUF_SIZE (1*1024*1024) /* 1M */
static int __move_file_segment(ifs_t* ifs, int from_seg, int to_seg)
{
	int i = 0;
	uint32_t size = 0;
	int read_size = 0;
	int append_size = 0;
	int total_read_size = 0;
	int total_append_size = 0;
	int option = O_HASH_ROOT_DIR;
	int offset = 0;
	int* file_array = NULL;
	sfs_info_t sfs_info;
	char buf[MAX_BUF_SIZE];

	size = MAX_BUF_SIZE;

	/* file_id == 0 이면 entry정보는 가져오지 않는다. */
	if(sfs_get_info(ifs->local.sfs[from_seg], &sfs_info, 0) != SUCCESS) {
		error("cannot get sfs[%d] info", from_seg);
		return FAIL;
	}

	info("from segment[%d], to segment[%d]", from_seg, to_seg);

	file_array = (int*)sb_malloc(sizeof(int)*sfs_info.file_count);
	if(sfs_get_file_array(ifs->local.sfs[from_seg], file_array) != SUCCESS) {
		error("cannot get file array, sfs[%d]", from_seg);
		goto fail;
	}

	/* 새로 만드는 segment에는 FAT가 필요없다. */
	if(__sfs_activate(ifs, to_seg, O_FILE, DO_FORMAT, O_HASH_ROOT_DIR) != SUCCESS) {
		error("cannot activate sfs, physical_segment[%d], option[%d], type[O_FILE], do_format[%d]",
			to_seg, option, DO_FORMAT);
		goto fail;
	}

	for(i = 0; i < sfs_info.file_count; i++) {
		debug("moving[%d]...", file_array[i]);
		do {
			read_size = sfs_read(ifs->local.sfs[from_seg], file_array[i], 
								 offset, size, buf);
			if(read_size < 0) {
				error("cannot read file, sfs[%d], file_id[%d], offset[%d], size[%d]",
					  from_seg, file_array[i], offset, size);
				goto fail;
			}
			else if (read_size == 0) break;

			offset += read_size;
			total_read_size += read_size;

			append_size = sfs_append(ifs->local.sfs[to_seg], file_array[i], read_size, buf);
			if(append_size < 0) {
				error("cannot append file, sfs[%d], file_id[%d], read_size[%d]",
					  to_seg, file_array[i], read_size);
				goto fail;
			} else if (append_size != read_size) {
				crit("sfs_append(sfs[%d],file[%d],size[%d],data) should have appended size[%d] bytes, but appended %d bytes.",
						to_seg, file_array[i], read_size, read_size, append_size);
				goto fail;
			}

			total_append_size += append_size;

			if(size > (uint32_t)read_size) break;
		} while(read_size != 0);
		offset = 0;
	}

	if(total_read_size != total_append_size) {
		error("invalid size, total_read_size[%d], total_append_size[%d]",
			  total_read_size, total_append_size);
		goto fail;
	}

    if(__sfs_deactivate(ifs, to_seg) != SUCCESS) {
		error("cannot deactivate sfs, num[%d]", to_seg);
	}

	free(file_array);

	return SUCCESS;

fail:
    if(__sfs_deactivate(ifs, to_seg) == FAIL) {
		error("cannot deactivate sfs, num[%d]", to_seg);
		free(file_array);
		return -1;
	}

	free(file_array);

	return FAIL;
}

/******************************************************
 *                   module stuff
 ******************************************************/

static void set_temp_alive_time(configValue v)
{
	// <= 0 이면 바로 삭제된다
	temp_alive_time = atoi(v.argument[0]);
}

static void set_file_size(configValue v)
{
	file_size = atoi( v.argument[0] ) * 1024 * 1024;
	if ( file_size <= 0 ) {
		warn("invalid file_size[%s]. set to default[%d]",
				v.argument[0], DEFAULT_FILE_SIZE);
		file_size = DEFAULT_FILE_SIZE;
	}
}

static void set_ifs_set(configValue v)
{
	static ifs_set_t local_ifs_set[MAX_INDEXDB_SET];
	int value = atoi( v.argument[0] );

	if ( value < 0 || value >= MAX_INDEXDB_SET ) {
		error("Invalid IndexDbSet value[%s], MAX_INDEXDB_SET[%d]",
				v.argument[0], MAX_INDEXDB_SET);
		return;
	}

	if ( ifs_set == NULL ) {
		memset( local_ifs_set, 0, sizeof(local_ifs_set) );
		ifs_set = local_ifs_set;
	}

	current_ifs_set = value;
	ifs_set[value].set = 1;
}

static void set_ifs_path(configValue v)
{
	if ( ifs_set == NULL || current_ifs_set < 0 ) {
		error("first, set IndexDbSet");
		return;
	}

	strncpy( ifs_set[current_ifs_set].ifs_path, v.argument[0], MAX_PATH_LEN-1 );
	ifs_set[current_ifs_set].set_ifs_path = 1;
}

static void set_segment_size(configValue v)
{
	int segment_size;

	if ( ifs_set == NULL || current_ifs_set < 0 ) {
		error("first, set IndexDbSet");
		return;
	}

	segment_size = atoi( v.argument[0] ) * 1024 * 1024;
	if ( segment_size <= 0 ) {
		warn("invalid segment_size[%s]. set to default[%d]",
				v.argument[0], DEFAULT_SEGMENT_SIZE);
		segment_size = DEFAULT_SEGMENT_SIZE;
	}

	ifs_set[current_ifs_set].segment_size = segment_size;
	ifs_set[current_ifs_set].set_segment_size = 1;
}

static void set_block_size(configValue v)
{
	int block_size;

	if ( ifs_set == NULL || current_ifs_set < 0 ) {
		error("first, set IndexDbSet");
		return;
	}

	block_size = atoi( v.argument[0] );
	if ( block_size <= 0 ) {
		warn("invalid block_size[%s]. set to default[%d]",
				v.argument[0], DEFAULT_BLOCK_SIZE);
		block_size = DEFAULT_BLOCK_SIZE;
	}

	ifs_set[current_ifs_set].block_size = block_size;
	ifs_set[current_ifs_set].set_block_size = 1;
}

static config_t config[] = {
	// not in IndexDbSet
	CONFIG_GET("FileSize", set_file_size, 1,
					"size of one file (segment * sector. e.g: FileSize 1024(default))"),
	CONFIG_GET("TempAliveTime", set_temp_alive_time, 1, "time for temp segment is kept alive."),

	// in IndexDbSet
	CONFIG_GET("IndexDbSet", set_ifs_set, 1, "ifs set (e.g: IndexDbSet 1)"),
	CONFIG_GET("IfsPath", set_ifs_path, 1, "ifs path(not file). but, remove last '/'"),
	CONFIG_GET("SegmentSize", set_segment_size, 1, "sfs segment size(MB) (e.g: SegmentSize 256"),
	CONFIG_GET("BlockSize", set_block_size, 1, "sfs block size (e.g: BlockSize 256)"),
	{NULL}
};

static void register_hooks(void)
{
    sb_hook_indexdb_open(ifs_open,NULL,NULL,HOOK_MIDDLE);
    sb_hook_indexdb_close(ifs_close,NULL,NULL,HOOK_MIDDLE);
    sb_hook_indexdb_append(ifs_append,NULL,NULL,HOOK_MIDDLE);
    sb_hook_indexdb_read(ifs_read,NULL,NULL,HOOK_MIDDLE);
	sb_hook_indexdb_getsize(ifs_getsize,NULL,NULL,HOOK_MIDDLE);
}

module ifs_module = {              
    STANDARD_MODULE_STUFF,          
    config,                 /* config */
    NULL,                   /* registry */
    ifs_init,               /* initialize */
    NULL,                   /* child_main */
    NULL,                   /* scoreboard */
    register_hooks,         /* register hook api */
};    

