/* $Id$ */
#include "mod_sfs.h"
#include "fat.h"
#include "memory.h" /* see include/memory.h */

static int __move_free_block(sfs_t* sfs, int n);
static int __get_fat_entry_offset_in_sfs_block(int num, int block_size);
static int __read_fat_entry(sfs_t* sfs, inode_t* inode, int num, fat_entry_t* entry);
static int __write_fat_entry(sfs_t* sfs, int num, fat_entry_t* entry);
static int __get_sfs_block(int fat_entry_num, int block_size);


void fat_init_superblock(super_block_t *sb)
{
	int fat_size = sb->block_count * sizeof(fat_entry_t);

	if ( sb->option & O_FAT ) {
		sb->start_fat_block = SUPER_BLOCK_COUNT;
		sb->end_fat_block   = SUPER_BLOCK_COUNT + fat_size / sb->block_size;
		if ( fat_size % sb->block_size == 0 ) sb->end_fat_block -= 1;

		sb->free_block_num = sb->end_fat_block + 1;
	}
	else {
		sb->start_fat_block = -1;
		sb->end_fat_block   = -1;
		sb->free_block_num  = SUPER_BLOCK_COUNT;
	}
}


/*
 * block_count ��ŭ�� ���ӵ� block�� �Ҵ��Ѵ�.
 * ���� �ִ� free block�� ���ٸ� �� ��ŭ�� �Ҵ��Ѵ�.
 * before_block_num�� ������ ����Ǵ� block (���� ���� �ִ� ���Ͽ� append �� ���...)
 * entry : ���� ������ entry, next_entry : ���� ����� entry
 *
 * ������ ó������ ����� �� (before_block_num==0)
 * ���� block number�� entry->num�� �־ return
 *
 *
 * ���� fat�� �Ҵ��ؼ� ���̴� ���
 * entry                   next_entry
 * -------+---------------+-------+---
 *  num:a | ....          | a     |
 * -------+---------------+-------+---
 *   |                    ^ allocated
 *   +--------------------+
 *
 *
 * ���� ������ segment�� �������̶� �׳� �����ϴ� ���...
 *    max count
 * -----------------+
 * entry            |  (next_entry�� ����)
 * -------+---------+------------------
 *   eof  |allocated| free_block_num
 * -------+---------+------------------
 *   next entry
 */
int fat_allocate(sfs_t* sfs, fat_entry_t* entry, fat_entry_t* next_entry, int before_block_num, int* alloc_count)
{
	int block_size = 0;
	int free_block_num = 0;
	int free_block_count = 0;
	int max_alloc_count = 0;   /* ���ӵǾ� �Ҵ��Ҽ� �ִ� block�� �� - COUNT_MAX_BIT�� ���� ������ */
	int just_extended = 0; // boolean. next_entry�� ���ΰ���, �׳� entry�� ������ ����...
	super_block_t* super_block = sfs->super_block;

	block_size = super_block->block_size;
	max_alloc_count = (1 << COUNT_MAX_BIT) - 1;
	free_block_num = super_block->free_block_num;
	free_block_count = super_block->end_file_block - free_block_num + 1;

	if(free_block_count <= 0) {
		info("segment file system is full, alloc_count[%d], super_block->end_file_block[%d], " 
		     "super_block->free_block_num[%d] + 1",
		      *alloc_count, super_block->end_file_block, free_block_num);
		return SEGMENT_FULL;
	}

	if ( *alloc_count > free_block_count ) {
	    *alloc_count = free_block_count;
	}

	if ( *alloc_count > max_alloc_count ) {
	    *alloc_count = max_alloc_count;
	}

	// ���� entry�� �ְ�, ���� ������ �� ������ �����̸�, ���� fat count�� ������ �ʾ��� ��
	if ( before_block_num != 0 && before_block_num+entry->count == free_block_num
			&& entry->count < max_alloc_count ) {

		just_extended = (1>0);
		if ( entry->count + *alloc_count > max_alloc_count )
			*alloc_count = max_alloc_count - entry->count;
	}
	
	// FAT�� ���� ���� �ʱ� append�� Ȯ�常 �����ϴ�.
	if ( before_block_num != 0
			&& !(sfs->super_block->option & O_FAT) && !just_extended ) {
		error("invalid append");
		return FAIL;
	}

	__move_free_block(sfs, *alloc_count);
	//debug("alloc_count[%d], free_block_num[%d]", *alloc_count, super_block->free_block_num);

	if(before_block_num == 0) {
		/*
		 * ���� �Ҵ�, �� ������ directory�� �����ϸ�
		 * sfs::__extend_blocks ���� dir_update_entry�� �����Ѵ�.
		 */
		entry->num = free_block_num;
	}
	else if ( !just_extended ) {
		// �� entry�� ���� �Ҵ��� block num���� �����ؼ� �����Ѵ�
		entry->num = free_block_num;

		if(__write_fat_entry(sfs, before_block_num, entry) == FAIL) {
			error("can not write before_block_num[%d]", before_block_num);
			return FAIL;
		}
	}

	if ( just_extended ) {
		next_entry->count = entry->count + *alloc_count;
		next_entry->num = 0;

		// ���� entry�� �����.
		if(__write_fat_entry(sfs, before_block_num, next_entry) == FAIL) {
			error("can not write before_block_num[%d]", before_block_num);
			return FAIL;
		}
	}
	else {
		next_entry->count = *alloc_count;
		next_entry->num = 0;

		// ���ο� entry��ġ�� ����
		if(__write_fat_entry(sfs, free_block_num, next_entry) == FAIL) {
			error("can not write before_block_num[%d]", before_block_num);
			return FAIL;
		}
	}

	return SUCCESS;
}

// block[block_num] �� fat_block entry ��������
int fat_get_entry(sfs_t* sfs, inode_t *inode, int block_num, fat_entry_t* entry)
{
	super_block_t* super_block = sfs->super_block;

	if(block_num >= SUPER_BLOCK_NUM && 
		block_num < SUPER_BLOCK_COUNT) {
		error("can not access super_block([%d] ~ [%d]), block_num[%d]",
			  SUPER_BLOCK_NUM, SUPER_BLOCK_NUM+SUPER_BLOCK_COUNT-1, block_num);
		return FAIL;
	}

	if(block_num >= super_block->free_block_num) {
		error("Invalid super_block block number, block_num[%d], super_block->free_block_num[%d]", 
			   block_num, super_block->free_block_num);
		return FAIL;
	}

	if(__read_fat_entry(sfs, inode, block_num, entry) == FAIL) {
		error("can't read block, block_num[%d]", block_num);
		return FAIL;
	}

	return SUCCESS;
}

static int __get_sfs_block(int fat_entry_num, int block_size)
{
    int fat_data_offset = 0;

    fat_data_offset = fat_entry_num * sizeof(fat_entry_t);

    return fat_data_offset / block_size + SUPER_BLOCK_COUNT;
}

/*
 *                                
 *   FAT_ARRAY(n ���� sfs blocks)
 *   |    i      |    i+1    |   i+2     |    i+3    |     i+4   | ..... |
 *                           |||||||||||||
 *                                |
 *                                v
 *                            fat block number
 *                           <--->
 *                             offset
 *
 */
static int __get_fat_entry_offset_in_sfs_block(int fat_entry_num, int block_size)
{
	int offset = 0;

	offset = fat_entry_num * sizeof(fat_entry_t);

	return offset % block_size;
}

/*
 * super_block�� �׻� mmap�̹Ƿ� �ٷ� �����ȴ�.
 */
static int __move_free_block(sfs_t* sfs, int n)
{
	sfs->super_block->free_block_num += n;
	return sfs->super_block->free_block_num;
}

/*
 * super_block�� �׻� mmap�̴�.
 * �׷��Ƿ� block���� io�� ���� �ʰ� �ش� offset�� fat_block�� memcpy�ϴ°�����
 * �����Ѵ�.
 * - �Ϲ����� block io(sfs_block_read/write)�� �̿��Ұ�� 
 *   sfs block size * 2 ��ŭ�� memcpy ����� �ҿ�ȴ�.
 */
static int __write_fat_entry(sfs_t* sfs, int fat_entry_num, fat_entry_t* entry)
{
	int block_size = 0;
	int offset = 0;
	int sfs_block_num = 0;
	super_block_t* super_block = sfs->super_block;

	// fat ������...
	if ( !( sfs->super_block->option & O_FAT ) ) {

		return SUCCESS;
	}

	if(fat_entry_num >= SUPER_BLOCK_NUM && 
		fat_entry_num < SUPER_BLOCK_COUNT) {
		error("can not access super_block([%d] ~ [%d]), fat_entry_num[%d]",
			  SUPER_BLOCK_NUM, SUPER_BLOCK_NUM+SUPER_BLOCK_COUNT-1, fat_entry_num);
		return FAIL;
	}

	block_size = super_block->block_size;
	sfs_block_num = __get_sfs_block(fat_entry_num, block_size);
	offset = __get_fat_entry_offset_in_sfs_block(fat_entry_num, block_size);

	if ( sfs_block_write( sfs,
				sfs_block_num*block_size+offset, sizeof(fat_entry_t), entry ) != SUCCESS ) {
		error("can not write fat entry: %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

/*
 * sfs�� O_MMAP�� ��� 
 * 1. sfs_block�� reference�� ��´�.
 * sfs�� O_FILE�� ���
 * 1. sfs_block block�� ���纻�� ��´�.
 *
 * sfs_block���� offset�� ��ġ�� fat_block�� "���纻"�� ��´�.
 */
static int __read_fat_entry(sfs_t* sfs, inode_t* inode, int fat_entry_num, fat_entry_t* entry)
{
	int sfs_block_num = 0;
	int offset = 0;
	int block_size = 0;
	super_block_t* super_block = sfs->super_block;

	if ( sfs->super_block->option & O_FAT ) {
		block_size = super_block->block_size;
		sfs_block_num = __get_sfs_block(fat_entry_num, block_size);
		offset = __get_fat_entry_offset_in_sfs_block(fat_entry_num, block_size);

		if ( sfs_block_read( sfs, sfs_block_num * block_size + offset,
					sizeof(fat_entry_t), entry ) != SUCCESS ) {
			error("can't read fat. block[%d], offset[%d]", sfs_block_num, offset);
			return FAIL;
		}
	}
	else {
		// ����ȭ ���� �����Ƿ� num�� �׻� 0�̴�.
		entry->num = 0;
		entry->count = inode->dir_entry.size / sfs->super_block->block_size + 1;

		if ( inode->dir_entry.size % sfs->super_block->block_size == 0 )
			entry->count--;

		if ( inode->dir_entry.first_block_num != fat_entry_num ) {
			error("invalid request of entry num[%d]. available is [%d]",
					fat_entry_num, inode->dir_entry.first_block_num);
			return FAIL;
		}
	}

	return SUCCESS;
}
