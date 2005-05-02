/* $Id$ */
#include "sfs_types.h"
#include "directory.h"
#include "mod_sfs.h"

/********************************************************
 * block : sfs�� �ּ� I/O����
 * entry : directory ����. block�� �������� entry�̴�.
 ********************************************************/

#define MAX_ENTRY_COUNT (MAX_BLOCK_SIZE/sizeof(dir_hash_entry_t))

static int __get_block_idx(int file_id, int block_count);
static int __hash_func(int id);
static int __compare(const void * arg1, const void *arg2);

static int __compare(const void * arg1, const void *arg2)
{
	return (*(int*)arg1 >= *(int*)arg2) ? 1 : -1;
}

int dir_get_file_array(sfs_t* sfs, int* file_array)
{
	int i = 0;
	int block_idx = 0;
	int count = 0;
	super_block_t* super_block = sfs->super_block;
	int file_count = super_block->file_count;
	int block_size = super_block->block_size;

	dir_hash_entry_t entry_in_block_in_stack[MAX_ENTRY_COUNT];
	dir_hash_entry_t* entry_in_block = entry_in_block_in_stack;
	
	for(block_idx = 0; block_idx < super_block->dir_block_count; block_idx++) {
		int offset =  (super_block->start_dir_block + block_idx)*block_size;

		if(sfs->type & O_MMAP) {
			if(sfs_get_block_reference(sfs, offset, (void**)&entry_in_block) == FAIL) {
				error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					  super_block->start_dir_block, block_idx);
				return FAIL;
			}
		} else {
			if(sfs_block_read(sfs, offset, block_size, (void*)entry_in_block_in_stack) == FAIL) {
				error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					  super_block->start_dir_block, block_idx);
				return FAIL;
			}
		}

		for(i = 0; i < super_block->entry_count_in_block; i++) {
			if(entry_in_block[i].id == 0) break;
			file_array[count++] = entry_in_block[i].id;
		}
	}

	qsort((void*)file_array, file_count, sizeof(int), __compare);

	return SUCCESS;
}

/*
 * file_id �� �ش��ϴ� dir entry�� ã�´�.
 *
 * op�� DIR_OP_ADD�̸�...
 *   ���� directory�� ������ ������ �߰��ϰ� SUCCESS�̴�
 *   entry_xxxx ���� ������� �ʴ´�.
 *
 * op�� DIR_OP_UPDATE�̸�...
 *   entry_if_file�� �����Ϸ��� ������ �����Ƿ� (MMAP�̴� FILE �̴�...)
 *   �װ� sfs�� ����Ѵ�
 *
 * op�� DIR_OP_FIND�̸�...
 *   sfs type�� MMAP�̸� entry_if_mmap �� pointer ���� �ְ�
 *   FILE �̸� entry_if_file �� ������ �ִ´�.
 */

// hash collision�� �߻��ϸ� ����  block���� ����. ������ 0�� �� �ʿ��ϴ�
static int next_block[] = { 7, 13, 17, 23, 41, 0 };

// dir_find_entry() �� ���� argument
typedef enum {
	DIR_OP_ADD,
	DIR_OP_UPDATE,
	DIR_OP_FIND
} dir_op_t;

int dir_find_entry(sfs_t* sfs, int file_id, dir_op_t op,
		dir_hash_entry_t **entry_if_mmap, dir_hash_entry_t *entry_if_file)
{
	int i = 0;
	super_block_t* super_block = sfs->super_block;
	int block_size = super_block->block_size;
	int start_block_idx = __get_block_idx(file_id, super_block->dir_block_count);
	int block_idx = start_block_idx;
	int retry = 0;

	dir_hash_entry_t entry_in_block_in_stack[MAX_ENTRY_COUNT];
	dir_hash_entry_t* entry_in_block;

	do {
		int offset =  (super_block->start_dir_block + block_idx)*block_size;

		if(sfs->type & O_MMAP) {
			if(sfs_get_block_reference(sfs, offset, (void**)&entry_in_block) != SUCCESS) {
				error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					  super_block->start_dir_block, block_idx);
				return FAIL;
			}
		} else {
			if(sfs_block_read(sfs, offset, block_size, (void*) entry_in_block_in_stack) != SUCCESS) {
				error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					  super_block->start_dir_block, block_idx);
				return FAIL;
			}

			entry_in_block = entry_in_block_in_stack;
		}

		for(i = 0; i < super_block->entry_count_in_block; i++) {
			if(entry_in_block[i].id == 0) {
				if ( op != DIR_OP_ADD ) return FILE_NOT_EXISTS;

				entry_in_block[i].id = file_id;
				if ( sfs->type & O_MMAP ) return SUCCESS;

				// MMAP �� �ƴϸ� �ٽ� �������� �Ѵ�.
				if ( sfs_block_write(sfs, offset, block_size,
							(void*)entry_in_block) != SUCCESS ) {
					error("can't write entry_in_block, block offset[%d], file[%d]",
							offset, file_id);
					return FAIL;
				}
				return SUCCESS;
			}

			if(entry_in_block[i].id != file_id) continue;

			if ( op == DIR_OP_FIND ) {
				if ( sfs->type & O_MMAP )
					*entry_if_mmap = &entry_in_block[i];
				else
					memcpy(entry_if_file, &entry_in_block[i], sizeof(dir_hash_entry_t));

				return SUCCESS;
			}
			else if ( op == DIR_OP_ADD ) {
				error("file[%d] is already exists", file_id);
				return FAIL;
			}
			else if ( op == DIR_OP_UPDATE ) {
				if ( sfs_block_write( sfs, offset + ( sizeof(dir_hash_entry_t) * i ),
							sizeof(dir_hash_entry_t), entry_if_file ) != SUCCESS ) {
					error("dir entry update failed. fild[%d]", file_id);
					return FAIL;
				}
				return SUCCESS;
			}
			else sb_assert(0);
		}

		if ( next_block[retry] == 0 ) break;

		block_idx += next_block[retry++];
		if ( (super_block->start_dir_block + block_idx) > super_block->end_dir_block )
			block_idx -= (super_block->end_dir_block - super_block->start_dir_block + 1);

		crit("hash collision [%d] - %dth, directory size[%d], next_try[%d]",
				file_id, retry, super_block->end_dir_block - super_block->start_dir_block, block_idx);
	} while( 1 );

	if ( op == DIR_OP_ADD ) {
		warn("hash full -> block is full, super_block->start_dir_block[%d] + block_idx[%d], "
			  "file_id[%d]", super_block->start_dir_block, block_idx, file_id);
		return SEGMENT_FULL;
	}
	else return FILE_NOT_EXISTS;
}

/* 
 * entry->id �� file_id�� input���� ���;� �Ѵ�.
 */
int dir_get_entry(sfs_t* sfs, dir_hash_entry_t* entry)
{
	dir_hash_entry_t *entry_if_mmap;
	int ret;

	ret = dir_find_entry( sfs, entry->id, DIR_OP_FIND, &entry_if_mmap, entry );

	if ( ret == FILE_NOT_EXISTS ) return FILE_NOT_EXISTS;
	else if ( ret != SUCCESS ) {
		error("failed. file[%d]", entry->id);
		return FAIL;
	}

	if ( sfs->type & O_MMAP )
		memcpy( entry, entry_if_mmap, sizeof(dir_hash_entry_t) );

	return SUCCESS;
}

int dir_add_entry(sfs_t* sfs, dir_hash_entry_t* entry)
{
	dir_hash_entry_t *entry_if_mmap;
	int ret;

	ret = dir_find_entry( sfs, entry->id, DIR_OP_ADD, &entry_if_mmap, NULL );

	if ( ret == SEGMENT_FULL ) return SEGMENT_FULL;
	else if ( ret != SUCCESS ) {
		error("failed. file[%d]", entry->id);
		return FAIL;
	}
	else return SUCCESS;
}

int dir_update_entry(sfs_t* sfs, dir_hash_entry_t* entry)
{
	return dir_find_entry( sfs, entry->id, DIR_OP_UPDATE, NULL, entry);
}

static int __get_block_idx(int file_id, int block_count)
{
	return __hash_func(file_id) % block_count;
}

static int __hash_func(int id)
{
	return id;
}
