#include "mod_sfs.h"
#include "fat.h"
#include "directory.h"
#include "super_block.h"
#include "shared_memory.h"
#include "memory.h" /* see include/memory.h */

#ifndef WIN32
#include <string.h>
#endif

static int __mmap_sfs(sfs_t* sfs);
static int __read_superblock(sfs_t* sfs);
static int __write_superblock(sfs_t* sfs);
//static int __write_empty_fat(sfs_t* sfs);
static int __write_empty_root_dir(sfs_t* sfs);
static int __append_file(sfs_t* sfs, inode_t* inode, int size, void* buf);
static int __read_file(sfs_t* sfs, inode_t* inode, int size, void* buf);
static int __open_file(sfs_t* sfs, int file_id, inode_t* inode);
static int __seek_file(sfs_t* sfs, inode_t* inode, int offset, int option);
static void __close_file(inode_t* inode);
static int __seek_file_end(sfs_t* sfs, inode_t* inode);
static int __seek_file_current(sfs_t* sfs, inode_t* inode, int offset);
static int __seek_file_begin(sfs_t* sfs, inode_t* inode, int offset);
static int __extend_block(sfs_t* sfs, inode_t* inode, int* alloc_count);

sfs_t* sfs_create(int seq, int fd, int offset)
{
    sfs_t* sfs = (sfs_t*)sb_calloc(1, sizeof(sfs_t));
	if(sfs == NULL) {
		error("not enough memory, sizeof(sfs_t)[%d]", (int)sizeof(sfs_t));
		return NULL;
	}

	sfs->super_block = (super_block_t*)sb_calloc(1, sizeof(super_block_t));
	if(sfs == NULL) {
		error("not enough memory, sizeof(super_block_t)[%d]", (int)sizeof(super_block_t));
		return NULL;
	}

	sfs->type = O_UNLOAD;
	sfs->seq = seq;
    sfs->fd = fd;
    sfs->base_offset = offset;

	debug("sfs_create, seq[%d], fd[%d], offset[%d]", seq, fd, offset);

    return sfs;
}

int sfs_destroy(sfs_t* sfs)
{
	if ( sfs->type != O_UNLOAD ) sfs_close( sfs );
    if ( sfs->super_block ) sb_free(sfs->super_block);

	debug("sfs_destory, seq[%d], fd[%d]", sfs->seq, sfs->fd);

	sb_free(sfs);

	return SUCCESS;
}

void __view_sfs_info(sfs_t* sfs)
{
	char sz_type[12];

	switch(sfs->type) {
		case O_UNLOAD:
			strcpy(sz_type, "O_UNLOAD");
			break;
		case O_MMAP:
			strcpy(sz_type, "O_MMAP");
			break;
		case O_FILE:
			strcpy(sz_type, "O_FILE");
			break;
		default:
			strcpy(sz_type, "UNKNOWN");
			break;
	}

	info("-- sfs information --------------------------------------------------------------");
    info("sfs     fd:%02d    seq:%02d   type:%-12s", sfs->fd, sfs->seq, sz_type);
    info("        base_offset:%d     base_ptr:%p", sfs->base_offset, sfs->base_ptr);

	superblock_view(sfs->super_block);
}

int sfs_format(sfs_t* sfs, int option, int size, int block_size)
{
	super_block_t* super_block;
	int block_count = 0;

	debug("sfs_format, seq[%d], option[%d], size[%d], block_size[%d]", sfs->seq, option, size, block_size);

	super_block = sfs->super_block;
	block_count = size / block_size;

	/* input error check */
	if ( block_size * SUPER_BLOCK_COUNT < sizeof(super_block_t) ) {
        error("must be block_size[%d] * SUPER_BLOCK_COUNT[%d] > sizeof(super_block_t)[%d]", 
			   block_size, SUPER_BLOCK_COUNT, (int)sizeof(super_block_t));
		return FAIL;
	}

	if ( block_size > MAX_BLOCK_SIZE ) {
		error("block_size[%d] is larger than MAX_BLOCK_SIZE[%d]", block_size, MAX_BLOCK_SIZE);
		return FAIL;
	}

	if ( size % block_size != 0 ) {
        error("size[%d] %% block_size[%d] != 0", size, block_size);
		return FAIL;
	}

	if ( block_count > (1 << NUM_MAX_BIT) ) {
		error("block count[%d] is greater than max block number[%u]."
				" increase block_size[%d] or decrease sfs_size[%d]",
				block_count, (1 << NUM_MAX_BIT) - 1, block_size, size);
		return FAIL;
	}

	/* super_block init */
	strncpy(super_block->magic, SFS_MAGIC, strlen(SFS_MAGIC));
	super_block->min_file_id = 0;
	super_block->max_file_id = 0;
	super_block->file_count = 0;
	super_block->file_total_byte = 0;
    super_block->option = option;
	super_block->size = size;
	super_block->block_count = block_count;
	super_block->block_size = block_size;

	/* fat 영역 계산 */
	fat_init_superblock(super_block);

	// directory 영역 계산
	dir_init_superblock(super_block);

	// file 영역 계산
	super_block->start_file_block = super_block->free_block_num;
	super_block->end_file_block = super_block->start_dir_block - 1;

	/* file init */
    if(__write_superblock(sfs) != SUCCESS) {
		error("cannot init the superblock");
		return FAIL;
	}

	if(__write_empty_root_dir(sfs) != SUCCESS) {
		error("cannot init the root_dir");
		return FAIL;
	}

	__view_sfs_info(sfs);

	return SUCCESS;
}

int sfs_open(sfs_t* sfs, int type)
{
	if ( sfs->type != O_UNLOAD ) {
		warn("sfs is already loaded. sfs->type[%d]", sfs->type);

		if ( sfs->type == type ) return SUCCESS;
		else sfs_close( sfs );
	}

	debug("sfs_open, type[%d], seq[%d]", type, sfs->seq);

	if(__read_superblock(sfs) != SUCCESS) {
		error("cannot read superblock");
		return FAIL;
	}

    if(type & O_MMAP) {
		info("sfs[%d] open with [O_MMAP]", sfs->seq);
   	    if(__mmap_sfs(sfs) != SUCCESS) {
			error("cannot mapping segment");
			return FAIL;
		}
    }

	sfs->type = type;

	__view_sfs_info(sfs);
	return SUCCESS;
}

int sfs_close(sfs_t* sfs)
{
	debug("sfs_close, seq[%d]", sfs->seq);

	if(sfs->type & O_MMAP) {
		info("sfs[%d] unload with [O_MMAP]", sfs->seq);
        unmmap_memory(sfs->base_ptr, sfs->super_block->size);
		sfs->base_ptr = NULL;
    } 
	else unmmap_memory(sfs->super_block, sizeof(super_block_t));

	sfs->type = O_UNLOAD;
	sfs->super_block = (super_block_t*)sb_calloc(1, sizeof(super_block_t));
	if(sfs == NULL) {
		error("not enough memory, sizeof(super_block_t)[%d]", (int)sizeof(super_block_t));
	}

	return SUCCESS;
}

// 있으면 SUCCESS, 없으면 FILE_NOT_EXISTS, IO 실패하면 FAIL
int sfs_exist_file(sfs_t* sfs, int file_id)
{
	if(sfs->super_block->min_file_id <= file_id &&
			sfs->super_block->max_file_id >= file_id) {
		dir_hash_entry_t entry;
		entry.id = file_id;
		return dir_get_entry(sfs, &entry);
	} else {
		return FILE_NOT_EXISTS;
	}
}

int sfs_get_info(sfs_t* sfs, sfs_info_t* sfs_info, int file_id)
{
	int ret;

	sfs_info->file_count = sfs->super_block->file_count;
	sfs_info->max_file_id = sfs->super_block->max_file_id;
	sfs_info->min_file_id = sfs->super_block->min_file_id;

	if(file_id != 0) {
		dir_hash_entry_t entry;
		entry.id = file_id;

		if ( sfs->super_block->min_file_id > file_id ||
				sfs->super_block->max_file_id < file_id) {
//			debug("file_id[%d] is not in min_id[%d]~max_id[%d]",
//					file_id, sfs->super_block->min_file_id, sfs->super_block->max_file_id);
			return FILE_NOT_EXISTS;
		}
	
		ret = dir_get_entry(sfs, &entry);
		if ( ret == FILE_NOT_EXISTS ) return FILE_NOT_EXISTS;
		else if ( ret != SUCCESS ) {
			error("can not get directory entry, file_id[%d]", file_id);
			return FAIL;
		}

		sfs_info->sfs_entry_info.file_size = entry.size;
	}

	return SUCCESS;
}

// RETURN VALUE : 기록한 바이트 수
// 실패하면 FAIL
int sfs_append(sfs_t* sfs, int file_id, int size, void* buf)
{
	int write_byte = 0;
	inode_t inode;
	int ret;

	ret = sfs_exist_file(sfs, file_id);

    if( ret == SUCCESS ) {
		// do nothing
	}
	else if ( ret != FILE_NOT_EXISTS ) {
		error("failed. ret[%d], file[%d]", ret, file_id);
		return FAIL;
    } else {
		dir_hash_entry_t entry;
		entry.id = file_id;
		entry.size = 0;

        ret = dir_add_entry(sfs, &entry);
		if ( ret == SEGMENT_FULL ) return 0;
		else if ( ret != SUCCESS ) {
			error("add directory entry is fail");
			return FAIL;
		}
		superblock_add_file_count(sfs->super_block, file_id);
    }

	if ( __open_file(sfs, file_id, &inode) != SUCCESS ) {
		error("cannot sfs file open");
		return FAIL;
	}

	if( __seek_file(sfs, &inode, 0, SFS_END) != SUCCESS ) {
        error("cannot seek file, inode.dir_entry.id[%d], offset[%d], whence[%d]",
			inode.dir_entry.id, 0, SFS_END);
		__close_file(&inode);
		return FAIL;
	}

    write_byte = __append_file(sfs, &inode, size, buf);
	superblock_update_write_byte(sfs->super_block, write_byte);

	__close_file(&inode);
	return write_byte;
}

// return value
//  read size ( >=0 ), FAIL(-1), FILE_NOT_EXISTS(-2)
int sfs_read(sfs_t* sfs, int file_id, int offset, int size, void* buf)
{
	int read_byte = 0, ret;
	inode_t inode;

    ret = sfs_exist_file(sfs, file_id);

	if ( ret == FILE_NOT_EXISTS ) return FILE_NOT_EXISTS;
	else if ( ret != SUCCESS ) {
		error("failed. file[%d]", file_id);
		return FAIL;
    }

	if ( __open_file(sfs, file_id, &inode) != SUCCESS ) {
		error("cannot sfs file open");
		return FAIL;
	}

	if ( __seek_file(sfs, &inode, offset, SFS_BEGIN) != SUCCESS ) {
        error("cannot seek file");
		__close_file(&inode);
		return FAIL;
	}

    read_byte = __read_file(sfs, &inode, size, buf);

	__close_file(&inode);
	return read_byte;
}

// directory 구조만 복사한다. file size 0 인 것들이 생성된다.
int sfs_directory_copy(sfs_t* src, sfs_t* dest)
{
	if ( dir_copy(src, dest) != SUCCESS ) {
		error("dir_copy() failed: %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

static int __open_file(sfs_t* sfs, int file_id, inode_t* inode)
{
	int block_size = sfs->super_block->block_size;

	inode->dir_entry.id = file_id;

	if(dir_get_entry(sfs, &inode->dir_entry) != SUCCESS) {
		error("cannot get directory entry, file_id[%d]", file_id);
		return FAIL;
	}

	if(inode->dir_entry.first_block_num == 0) {
		inode->sfs_block_num = 0;
		inode->curr_fat_entry.count = 0;
		inode->curr_fat_entry.num = 0;
	} else {
		inode->sfs_block_num = inode->dir_entry.first_block_num;
		if(fat_get_entry(sfs, inode, inode->sfs_block_num, &inode->curr_fat_entry) != SUCCESS) {
			error("cannot get current fat block[%d]", inode->sfs_block_num);
			return FAIL;
		}
	}

	inode->end_of_sfs_blocks = inode->curr_fat_entry.count * block_size;
	inode->offset_in_sfs_blocks = 0;
	inode->file_size_in_sfs_blocks =
		( inode->end_of_sfs_blocks > inode->dir_entry.size ) ?
			inode->dir_entry.size : inode->end_of_sfs_blocks;

	return SUCCESS;
}

static int __seek_file_begin(sfs_t* sfs, inode_t* inode, int offset)
{
	int skip_bytes = 0;
	int block_size = sfs->super_block->block_size;

	//fat block를 처음 node로 옮긴다.
	if(inode->dir_entry.first_block_num == 0) {
		inode->sfs_block_num = 0;
		inode->curr_fat_entry.count = 0;
		inode->curr_fat_entry.num = 0;
	} else {
		inode->sfs_block_num = inode->dir_entry.first_block_num;
		if(fat_get_entry(sfs, inode, inode->sfs_block_num, &inode->curr_fat_entry) != SUCCESS) {
			error("cannot get current fat block[%d]", inode->sfs_block_num);
			return FAIL;
		}
	}

	inode->end_of_sfs_blocks = inode->curr_fat_entry.count * block_size;
	inode->offset_in_sfs_blocks = 0;

	// 실패로 할까, end 위치로 옮기기로 할까...
	if((uint32_t)offset > inode->dir_entry.size) {
		error("seek failed. offset[%d], file_size[%d]", offset, inode->dir_entry.size);
		return FAIL;
	}

	while(1) {
		/* offset이 현재 blocks에 포함되는 경우 */
		if(offset <= inode->end_of_sfs_blocks + skip_bytes) {
			/* file 의 끝이 현재 blocks에 포함되는 경우 */
			if(inode->dir_entry.size <= (uint32_t)(inode->end_of_sfs_blocks + skip_bytes)) {
				inode->file_size_in_sfs_blocks = inode->dir_entry.size - skip_bytes;
			} else {
				inode->file_size_in_sfs_blocks = inode->end_of_sfs_blocks;
			}

			inode->offset_in_sfs_blocks = offset - skip_bytes;

			break;
		}

		skip_bytes += inode->end_of_sfs_blocks;

		inode->sfs_block_num = inode->curr_fat_entry.num;
		if(fat_get_entry(sfs, inode, inode->curr_fat_entry.num, &inode->curr_fat_entry) != SUCCESS) {
			error("cannot get next block, may be fat is broken. inode->sfs_block_num[%d], "
				  "inode->dir_entry.size[%d], inode->dir_entry.id[%d]", 
				  inode->sfs_block_num, inode->dir_entry.size, inode->dir_entry.id);
			return FAIL;
		}
		
		inode->end_of_sfs_blocks = inode->curr_fat_entry.count * block_size;
	}

	return SUCCESS;
}

static int __seek_file_current(sfs_t* sfs, inode_t* inode, int offset)
{
	error("Not support operation");
	return FAIL;
}

static int __seek_file_end(sfs_t* sfs, inode_t* inode)
{
	int block_size = sfs->super_block->block_size;

	if(inode->dir_entry.last_block_num == 0) {
		inode->sfs_block_num = 0;
		inode->curr_fat_entry.count = 0;
		inode->curr_fat_entry.num = 0;
		inode->offset_in_sfs_blocks = 0;
	} else {
		//현재 blocks에서 file이 끝나면.
		int last_block_fill = 0;
		int allocated_blocks = 0;

		inode->sfs_block_num = inode->dir_entry.last_block_num;
		if(fat_get_entry(sfs, inode, inode->sfs_block_num, &inode->curr_fat_entry) != SUCCESS) {
			error("cannot get current fat block[%d]", inode->sfs_block_num);
			return FAIL;
		}

		if(inode->curr_fat_entry.count == 0) {
			error("should be larger than zero, inode->curr_fat_entry.count[%d]", 
				inode->curr_fat_entry.count);
			return FAIL;
		}

		allocated_blocks = (inode->curr_fat_entry.count-1) * block_size;
		last_block_fill = inode->dir_entry.size % block_size;

		//inode->offset_in_sfs_blocks == 0 이면 모두 할당된 경우임
		last_block_fill = (last_block_fill == 0) ? block_size : last_block_fill;

		inode->offset_in_sfs_blocks =  allocated_blocks + last_block_fill;
	}

	inode->file_size_in_sfs_blocks = inode->offset_in_sfs_blocks;
	inode->end_of_sfs_blocks = inode->curr_fat_entry.count * block_size;

	return SUCCESS;
}

static int __seek_file(sfs_t* sfs, inode_t* inode, int offset, int whence)
{
	int ret = 0;

	if(inode == NULL) {
		error("inode is null, must be open");
		return FAIL;
	}

	switch(whence) {
		case SFS_BEGIN:
			ret = __seek_file_begin(sfs, inode, offset);
			break;
		case SFS_CURRENT:
			ret = __seek_file_current(sfs, inode, offset);
			break;
		case SFS_END:
			ret = __seek_file_end(sfs, inode);
			break;
		default:
			error("invalid whence[%d]", whence);
			return FAIL;
	}

	if(ret != SUCCESS) {
		error("cannot seek file, inode->dir_entry.id[%d], offset[%d], whence[%d]",
				inode->dir_entry.id, offset, whence);
		return FAIL;
	}

	return SUCCESS;
}

static void __close_file(inode_t* inode)
{
	return;
}

static int __extend_block(sfs_t* sfs, inode_t* inode, int* alloc_count)
{
	fat_entry_t next_fat_entry;
	int block_size = sfs->super_block->block_size;
	int ret;

	ret = fat_allocate(sfs, &inode->curr_fat_entry, 
		            &next_fat_entry, inode->sfs_block_num, alloc_count);

	if ( ret == SEGMENT_FULL ) return SEGMENT_FULL;
	else if ( ret != SUCCESS ) {
		info("cannot allocation fat block, alloc_count[%d]", *alloc_count);
		return FAIL;
	}

	/*
	* 최초 파일이 저장될때, 다음 fat_block의 위치가 저장된 fat_block을 복사한다.
	*/
	if(inode->dir_entry.first_block_num == 0) {
		inode->dir_entry.first_block_num = inode->curr_fat_entry.num;
	}

	// 기존의 fat이 확장되거나, 새로 할당하거나...
	// 확장인 경우 inode->curr_fat_entry.num == next_fat_entry.num
	if ( inode->curr_fat_entry.num == next_fat_entry.num ) {
		inode->offset_in_sfs_blocks = inode->curr_fat_entry.count * block_size;
	}
	else {
		// 다음으로 이동
		if ( !(sfs->super_block->option & O_FAT) // FAT이면 파일이 분할되었을 리가 없다
				&& inode->curr_fat_entry.num != inode->dir_entry.first_block_num ) {
			crit("invalid current block[%d], available is only dir_entry.first_block_num[%d]",
					inode->curr_fat_entry.num, inode->dir_entry.first_block_num);
			return FAIL;
		}
		inode->sfs_block_num = inode->dir_entry.last_block_num = inode->curr_fat_entry.num;

		// dir_update_entry 는 여기서 해봐야 소용없음.
		// 어차피 나중에 file size 다시 기록해야 함

		inode->offset_in_sfs_blocks = 0;
	}

	inode->end_of_sfs_blocks = next_fat_entry.count * block_size;
	inode->file_size_in_sfs_blocks = inode->offset_in_sfs_blocks;

	memcpy(&inode->curr_fat_entry, &next_fat_entry, sizeof(fat_entry_t));

	return SUCCESS;
}

/*
 * write 에 필요한 sfs_block의 수를 구한다.
 */
static int __get_need_block(int write_size, int block_size)
{
	int need_block_count;
	int div = 0;
	int mod = 0;

	div = write_size / block_size;
	mod = write_size % block_size;


	if(mod != 0) need_block_count = div+1;
	else need_block_count = div;

	return need_block_count;
}

// 성공이면 기록한 byte. 0일 수도 있다. segment full 때문에...
// 실패면 FAIL
static int __append_file(sfs_t* sfs, inode_t* inode, int size, void* buf)
{
	int total_write_size = 0;
	int block_size = 0;
	int alloc_count = 0;
	int writable_size = 0;
	int remain_file_size = 0;
	int ret;

	block_size = sfs->super_block->block_size;

	for(total_write_size = 0; total_write_size < size; ) {
		if(inode->offset_in_sfs_blocks == inode->end_of_sfs_blocks) {
			alloc_count = __get_need_block(size - total_write_size, block_size);

			ret = __extend_block(sfs, inode, &alloc_count);
			if ( ret == SEGMENT_FULL ) {
				info("segment full. cannot extend block, alloc_count[%d]", alloc_count);
				goto exit;
			}
			else if ( ret != SUCCESS ) {
				error("failed. file[%d]", inode->dir_entry.id);
				goto exit;
			}
		}

		writable_size = inode->end_of_sfs_blocks - inode->offset_in_sfs_blocks;
		remain_file_size = size - total_write_size;

		writable_size = (writable_size > remain_file_size) ? remain_file_size : writable_size;
		ret = sfs_block_write( sfs, inode->sfs_block_num*block_size + inode->offset_in_sfs_blocks,
					writable_size, (char*)buf + total_write_size );
		if ( ret != SUCCESS ) {
			error("append failed. file[%d]", inode->dir_entry.id);
			goto exit;
		}

		total_write_size += writable_size;
		inode->offset_in_sfs_blocks += writable_size;
		inode->file_size_in_sfs_blocks = inode->offset_in_sfs_blocks;
	}
	ret = SUCCESS;

exit:
	inode->dir_entry.size += total_write_size;
	if( dir_update_entry(sfs, &inode->dir_entry) != SUCCESS ) {
		error("entry update error, file_id[%d], size[%d], free_block[%d]",
				inode->dir_entry.id, inode->dir_entry.size, sfs->super_block->free_block_num);
	}

	if ( ret == SUCCESS || ret == SEGMENT_FULL ) return total_write_size;
	else return FAIL;
}

// size 만큼 읽거나... 모자라면 모자란 만큼만 읽자.
// 에러인 경우 -1
static int __read_file(sfs_t* sfs, inode_t* inode, int size, void* buf)
{	
	int read_size = 0;
	int block_size = 0;
	char* dest = (char*)buf;

	if ( size == 0 ) return 0;

	block_size = sfs->super_block->block_size;

	for(read_size = 0; read_size < size;) {
		int readable_size;

		if(inode->offset_in_sfs_blocks == inode->end_of_sfs_blocks) {
			if( inode->curr_fat_entry.num == 0 ) goto exit; // 이미 파일의 마지막

			inode->sfs_block_num = inode->curr_fat_entry.num;

			if(fat_get_entry(sfs, inode,
						inode->curr_fat_entry.num, &inode->curr_fat_entry) != SUCCESS) {
				error("cannot move next block");
				return FAIL;
			}

			inode->end_of_sfs_blocks = inode->curr_fat_entry.count * block_size;
			inode->offset_in_sfs_blocks = 0;

			/* file 의 끝이 현재 blocks에 포함되는 경우 */
			if(inode->curr_fat_entry.num == 0) {
				int allocated_blocks = (inode->curr_fat_entry.count-1) * block_size;
				int last_block_fill = inode->dir_entry.size % block_size;

				//last_block_fill == 0 이면 모두 할당된 경우임
				last_block_fill = (last_block_fill == 0) ? block_size : last_block_fill;
				inode->file_size_in_sfs_blocks = allocated_blocks + last_block_fill;
			}
			else {
				inode->file_size_in_sfs_blocks = inode->end_of_sfs_blocks;
			}
		}

		readable_size = inode->file_size_in_sfs_blocks - inode->offset_in_sfs_blocks;
		readable_size = ( readable_size > size - read_size ) ? (size - read_size) : readable_size;

		if ( readable_size == 0 ) goto exit;

		if(sfs_block_read(sfs, inode->sfs_block_num*block_size + inode->offset_in_sfs_blocks,
				readable_size, dest + read_size) != SUCCESS) {
			error("cannot read last block");
			return FAIL;
		}

		inode->offset_in_sfs_blocks += readable_size;
		read_size += readable_size;
/*
		debug("file_id[%d], read block : inode->sfs_block_num[%d], read_size[%d],"
			  "inode->offset_in_sfs_blocks[%d], inode->curr_fat_entry.num[%d]", 
			  inode->dir_entry.id, inode->sfs_block_num, read_size, 
			  inode->offset_in_sfs_blocks, inode->curr_fat_entry.num);
*/

	} // for (read_size<size))

exit:
	return read_size;
}

static int __mmap_sfs(sfs_t* sfs)
{
	if ( sfs->base_ptr != NULL ) {
		warn("sfs->base_ptr is already assigned");
		return SUCCESS;
	}
	else {
		// HP-UX 에서 같은 영역을 2번이상 mmap 하게 되면 에러나는 문제가 있다.
		// super_block 은 base_ptr 의 맨 앞 sizeof(super_block_t) byte 니까
		// 이렇게 하자.
		if ( unmmap_memory(sfs->super_block, sizeof(super_block_t)) != SUCCESS ) 
			warn("unmmap failed. but continue...");
		sfs->base_ptr = (char*)get_shared_memory(sfs->seq, sfs->fd, sfs->base_offset, sfs->super_block->size);
		sfs->super_block = (super_block_t*) sfs->base_ptr;

		if(sfs->base_ptr == NULL) {
			error("get shared memory fail, sfs->seq[%d], sfs->fd[%d], sfs->base_offset[%d], sfs->super_block->size[%d]",
					sfs->seq, sfs->fd, sfs->base_offset, sfs->super_block->size);
			return FAIL;
		}
	}

//	if(sfs->super_block != NULL) free_mmap(sfs->super_block);

//	sfs->super_block = (super_block_t*)sfs->base_ptr;

	if(strncmp(sfs->super_block->magic, SFS_MAGIC, strlen(SFS_MAGIC)) != 0) {
		error("wrong magic number");
		return FAIL;
	}

	return SUCCESS;
}

static int __read_superblock(sfs_t* sfs)
{
	if ( sfs->super_block ) sb_free( sfs->super_block );

	// seq+3000은 mmap의 indentity 값을 임의로 만드려는 ugly hack
	sfs->super_block = get_shared_memory( sfs->seq+3000, sfs->fd, sfs->base_offset, sizeof(super_block_t) );
	if ( sfs->super_block == NULL ) {
		error("mmap to superblock failed");
		return FAIL;
	}

	if(strncmp(sfs->super_block->magic, SFS_MAGIC, strlen(SFS_MAGIC)) != 0) {
		error("wrong magic number");
		return FAIL;
	}

	return SUCCESS;
}

static int __write_superblock(sfs_t* sfs)
{
	// mmap이므로 따로 기록할 필요 없음
	if ( sfs->type != O_UNLOAD ) return SUCCESS;
	
	if(sfs_block_write(sfs, SUPER_BLOCK_NUM * sfs->super_block->block_size,
				sizeof(super_block_t), (void*) sfs->super_block) != SUCCESS) {
		error("cannot write super block");
		return FAIL;
	}

	return SUCCESS;
}

#if 0
static int __write_empty_fat(sfs_t* sfs)
{
	int i = 0;
	int block_size = sfs->super_block->block_size;
	char* buffer = NULL;
	
	buffer = (char*)sb_calloc(1, block_size);
	if(buffer == NULL) {
		error("not enough memory, block_size[%d]", block_size);
		return FAIL;
	}

	for(i = SUPER_BLOCK_COUNT; i < sfs->super_block->free_block_num; i++) {
		if(sfs_block_write(sfs, i*block_size, block_size, (void*)buffer) != SUCCESS) {
			error("cannot write super block");
			sb_free(buffer);
			return FAIL;
		}
	}

	sb_free(buffer);

	return SUCCESS;
}
#endif

static int __write_empty_root_dir(sfs_t* sfs)
{
	int i = 0;
	int block_size = sfs->super_block->block_size;
	char* buffer = NULL;
	
	buffer = (char*)sb_calloc(1, block_size);
	if(buffer == NULL) {
		error("not enough memory, block_size[%d]", block_size);
		return FAIL;
	}


	// test
	//buffer[sfs->super_block->block_size-1] = 'e';

	for(i = sfs->super_block->start_dir_block; i < sfs->super_block->block_count; i++) {
		if(sfs_block_write(sfs, i*block_size, block_size, (void*)buffer) != SUCCESS) {
			error("cannot write last block");
			sb_free(buffer);
			return FAIL;
		}
	}

	sb_free(buffer);

	return SUCCESS;
}

#ifdef WIN32
static void __win__err() {
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
}
#endif

int sfs_get_block_reference(sfs_t* sfs, int offset, void** ptr)
{
	char* base = sfs->base_ptr;

	if(sfs->type & O_MMAP) {
		*ptr =  base + offset;
		return SUCCESS;
	} else {
		*ptr = NULL;
		return FAIL;
	}
}

int sfs_block_read(sfs_t* sfs, int offset, int size, void* buf)
{
	if(sfs->type & O_MMAP) {
		char* base = sfs->base_ptr;

		memcpy(buf, base + offset, size);
	} else {
#ifdef WIN32
		int base = sfs->base_offset;

		if(sb_seek_win(sfs->fd, base + offset, NULL, FILE_BEGIN) != SUCCESS) {
			error("cannot seek file, fd[%d]", sfs->fd);
			return FAIL;
		}

		if(sb_read_win(sfs->fd, buf, size, NULL, NULL) != SUCCESS) {
			error("cannot read file, fd[%d]", sfs->fd);
			return FAIL;
		}
#else
		int base = sfs->base_offset;
		int bytes, seek_offset;

		seek_offset = lseek(sfs->fd, base + offset, SEEK_SET);
		if( seek_offset == -1 ) {
			error("lseek(sfs->fd[%d], base[%d]+offset[%d], whence[%d]) returned %d: %s",
					sfs->fd, base, offset, SEEK_SET, seek_offset, strerror(errno));
			return FAIL;
		}

		bytes = read(sfs->fd, buf, size);
		if(bytes != size) {
			error("read(sfs->fd[%d], buf[%p], size[%d]) returned %d: %s",
					sfs->fd, buf, size, bytes, strerror(errno));
			return FAIL;
		}
#endif
	}

	return SUCCESS;
}

int sfs_block_write(sfs_t* sfs, int offset, int size, void* buf)
{
	if(sfs->type & O_MMAP) {
		char* base = sfs->base_ptr;

		memcpy(base + offset, buf, size);
	} else {
#ifdef WIN32
		int base = sfs->base_offset;

		if ( sb_seek_win(sfs->fd, base + offset, NULL, FILE_BEGIN) != SUCCESS ) {
			error("cannot seek file, fd[%d]", sfs->fd);
			return FAIL;
		}

		if ( sb_write_win(sfs->fd, buf, size, NULL, NULL) != SUCCESS ) {
			error("cannot write file, fd[%d]", sfs->fd);
			return FAIL;
		}
#else
		int base = sfs->base_offset;
		int bytes, seek_offset;

		seek_offset = lseek(sfs->fd, base + offset, SEEK_SET);
		if( seek_offset == -1 ) {
			error("lseek(sfs->fd[%d], base[%d]+offset[%d], whence[%d]) returned %d: %s",
					sfs->fd, base, offset, SEEK_SET, seek_offset, strerror(errno));
			return FAIL;
		}

		bytes = write(sfs->fd, buf, size);
		if(bytes != size) {
			error("write(sfs->fd[%d], buf[%p], size[%d]) returned %d: %s",
					sfs->fd, buf, size, bytes, strerror(errno));
			return FAIL;
		}
#endif
	}

	return SUCCESS;
}

int sfs_get_file_array(sfs_t* sfs, int* file_array)
{
	return dir_get_file_array(sfs, file_array, 1);
}

#ifndef WIN32
module sfs_module = {              
        STANDARD_MODULE_STUFF,          
        NULL,                   /* config */
        NULL,                   /* registry */
        NULL,                   /* initialize */
        NULL,                   /* child_main */
        NULL,                   /* scoreboard */
        NULL,                   /* register hook api */
};    
#endif
