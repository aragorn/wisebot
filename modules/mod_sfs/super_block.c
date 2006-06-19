/* $Id$ */
#include "common_core.h"
#include "super_block.h"
#include <string.h> /* memcpy(3) */

int superblock_add_file_count(super_block_t* s, int file_id)
{
	if(s->min_file_id == 0 || s->min_file_id > file_id) {
		s->min_file_id = file_id;
	}

	if(s->max_file_id == 0 || s->max_file_id < file_id) {
		s->max_file_id = file_id;
	}

	s->file_count ++;

	return s->file_count;
}

int superblock_update_write_byte(super_block_t* s, int write_byte)
{
	s->file_total_byte += write_byte;

	return s->file_total_byte;
}

void superblock_view(super_block_t * s)
{
	char magic[5];
	char sz_option[128]={0,};

	memcpy(magic, s->magic, 4); magic[4] = '\0';
	if(s->option & O_FAT)             strcat(sz_option, "O_FAT");
	if(s->option & O_ARRAY_ROOT_DIR)  strcat(sz_option, " | O_ARRAY_ROOT_DIR");
	if(s->option & O_HASH_ROOT_DIR)   strcat(sz_option, " | O_HASH_ROOT_DIR");

	info("-- super block information ------------------------------------------------------");
	info("magic[%4s]    option[%s]", magic, sz_option);

	info("block   fat[%d]~[%d]   file[%d]~[%d] free_block_num[%d]  dir[%d]~[%d]",
				s->start_fat_block, s->end_fat_block,
				s->start_file_block, s->end_file_block,
				s->free_block_num,
				s->start_dir_block, s->end_dir_block);

	info("file    id[%d]~[%d]   count[%d]   total_byte[%d]",
				s->min_file_id, s->max_file_id, s->file_count, s->file_total_byte);

	info("size    segment size[%d]   block_count[%d]   block_size[%d]",
				s->size, s->block_count, s->block_size);

	info("dir     entry_count_in_block[%d], dir_block_count[%d]",
				s->entry_count_in_block, s->dir_block_count);
}

