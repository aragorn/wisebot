/* $Id$ */
#include "common_core.h"
#include "super_block.h" /* refered for superblock_view() */
#include "mod_sfs.h"
#include "directory.h"
#include <stdlib.h> /* qsort(3) */
#include <string.h> /* memcpy(3) */

/********************************************************
 * block : sfs�� �ּ� I/O����
 * entry : directory ����. block�� �������� entry�̴�.
 ********************************************************/

#define MAX_ENTRY_COUNT (MAX_BLOCK_SIZE/sizeof(dir_hash_entry_t))

static int __get_block_idx(int file_id, int block_count);
static int __hash_func(int key);
static int __compare(const void * arg1, const void *arg2);

static int __compare(const void * arg1, const void *arg2)
{
	return (*(int*)arg1 >= *(int*)arg2) ? 1 : -1;
}

void dir_init_superblock(super_block_t *sb)
{
	int block_size      = sb->block_size;
	int block_count     = sb->block_count;
	int dir_size        = 0;
	int dir_block_count = 0;

	/* dir_size : dir_hash_entry_t�� ����� �ִ� ������ ũ��
       = sizeof(dir_hash_entry_t)
		x (
		 	block_count   // segment�� ��ü block ��. size / block_size
            -
			super_block->free_block_num  // free_block_num �������� file�� �������� ���Ѵ�
		  )
     */
	dir_size = sizeof(dir_hash_entry_t) * (block_count - sb->free_block_num);
	dir_block_count = 
		((dir_size % block_size) == 0) ? (dir_size / block_size) : (dir_size / block_size) + 1;

	sb->start_dir_block = block_count - dir_block_count;
	sb->end_dir_block = block_count - 1;

	sb->entry_count_in_block = block_size / sizeof(dir_hash_entry_t);
	sb->dir_block_count = dir_block_count;
}

// b_sort �� 0�̸� ����� sorting���� �ʴ´�.
int dir_get_file_array(sfs_t* sfs, int* file_array, int b_sort)
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
#warning "(void**)�� typecasting���� �ʰ� �۵��� �� �ְ� ������ �� ���̴�. --������"
			if(sfs_get_block_reference(sfs, offset, (void**)&entry_in_block) == FAIL) {
				error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					  super_block->start_dir_block, block_idx);
				return FAIL;
			}
		} else {
#warning "(void*)�� typecasting���� �ʰ� �۵��� �� �ְ� ������ �� ���̴�. --������"
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

	if ( b_sort )
		qsort((void*)file_array, file_count, sizeof(int), __compare);

	return SUCCESS;
}

#define check_value(a,b,c) \
	if ( (a) != (b) ) { \
		error("different %s: src[%d], dest[%d]", c, a, b); \
		return FAIL; \
	}

int dir_copy(sfs_t* src, sfs_t* dest)
{
	int i;
	int block_idx = 0;
	int block_size = src->super_block->block_size;
	int entry_count_in_block = src->super_block->entry_count_in_block;

	dir_hash_entry_t entry_in_block[MAX_ENTRY_COUNT];

	// directory �� start block, block count ���� �ٸ���
	// hash function ����� �޶����Ƿ� �����ؾ� �Ѵ�.
	check_value( src->super_block->block_size,
			dest->super_block->block_size, "block size" );
	check_value( src->super_block->start_dir_block,
			dest->super_block->start_dir_block, "directory start block" );
	check_value( src->super_block->dir_block_count,
			dest->super_block->dir_block_count, "directory block count" );
	check_value( src->super_block->entry_count_in_block,
			dest->super_block->entry_count_in_block, "entry_count_in_block" );

	for(block_idx = 0; block_idx < src->super_block->dir_block_count; block_idx++) {
		int offset =  (src->super_block->start_dir_block + block_idx)*block_size;

		if(sfs_block_read(src, offset, block_size, entry_in_block) == FAIL) {
			error("can't read entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
				  src->super_block->start_dir_block, block_idx);
			return FAIL;
		}

		for ( i = 0; i < entry_count_in_block; i++ ) {
			if ( entry_in_block[i].id == 0 ) break;

			entry_in_block[i].size = 0;
			entry_in_block[i].first_block_num = 0;
			entry_in_block[i].last_block_num = 0;
		}

		if ( i == 0 ) continue;

		if(sfs_block_write(dest, offset, block_size, entry_in_block) == FAIL) {
			error("cant't write entry_in_block, super_block->start_dir_block[%d] + block_idx[%d]",
					dest->super_block->start_dir_block, block_idx);
			return FAIL;
		}
	}

	dest->super_block->file_count = src->super_block->file_count;
	dest->super_block->min_file_id = src->super_block->min_file_id;
	dest->super_block->max_file_id = src->super_block->max_file_id;

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

/* hash collision�� �߻��ϸ� ����  block���� ����. ������ 0�� �� �ʿ��ϴ�. */
static int next_block[] = { 7, 13, 17, 23, 41, 0 };
//static int next_block[] = { 7*7, 13*13, 17*17, 23*23, 31*31, 43*43, 53*53, 0 };
                           /* make it a series of square of selected prime numbers */
/* prime numbers: http://www.utm.edu/research/primes/lists/small/1000.txt */

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
#warning "(void**)�� typecasting���� �ʰ� �۵��� �� �ְ� ������ �� ���̴�. --������"
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

		/* retry Ƚ���� ���� log level�� �ٲ۴�. */
		if ( op == DIR_OP_ADD )
		switch ( retry ) {
		case 0:
		case 1:
		case 2:
			debug("%dth hash collision file_id[%d], num of dir blocks[%d]",
				retry, file_id, super_block->end_dir_block - super_block->start_dir_block);
			break;
		case 3:
		case 4:
			info("%dth hash collision file_id[%d], num of dir blocks[%d]",
				retry, file_id, super_block->end_dir_block - super_block->start_dir_block);
			break;
		default:
			crit("%dth hash collision file_id[%d], num of dir blocks[%d]",
				retry, file_id, super_block->end_dir_block - super_block->start_dir_block);
		}
	} while( 1 );

	if ( op == DIR_OP_ADD ) {
		warn("hash full -> block is full, super_block->start_dir_block[%d] + block_idx[%d], "
			  "file_id[%d]", super_block->start_dir_block, block_idx, file_id);
		superblock_view(super_block);
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


/* wiki:SegmentFileSystem/HashCollision ���� */
#if 0
/* see http://www.concentric.net/~Ttwang/tech/inthash.htm for more information
 * about this hash function. */
static int __hash_func(int key)
{
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);

  return key;
}
#else
static int __hash_func(int key)
{
  return key;
}
#endif
