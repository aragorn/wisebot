/* $Id$ */
#include "super_block.h"

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
	char sz_option[128]={0,};

	if(s->option & O_FAT) {
		strcat(sz_option, "O_FAT");
	}

	if(s->option & O_ARRAY_ROOT_DIR) {
		strcat(sz_option, " | O_ARRAY_ROOT_DIR");
	}

	if(s->option & O_HASH_ROOT_DIR) {
		strcat(sz_option, " | O_HASH_ROOT_DIR");
	}

	debug("===========super block debug===========");
	debug("magic[%c%c%c%c]", s->magic[0], s->magic[1], s->magic[2], s->magic[3]);

	debug("fat_block: [%d] ~ [%d]", s->start_fat_block, s->end_fat_block);
	debug("file_block: [%d] ~ [%d]", s->start_file_block, s->end_file_block);
	debug("dir_block: [%d] ~ [%d]", s->start_dir_block, s->end_dir_block);

	debug("min_file_id[%d]", s->min_file_id);
	debug("max_file_id[%d]", s->max_file_id);
	debug("file_count[%d]", s->file_count);
	debug("file_total_byte[%d]", s->file_total_byte);

	debug("option[%s]", sz_option);
	debug("size[%d]", s->size);
	debug("block_count[%d]", s->block_count);
	debug("block_size[%d]", s->block_size);

	debug("entry_count_in_block[%d]", s->entry_count_in_block);
	debug("dir_block_count[%d]", s->dir_block_count);
	debug("free_block_num[%d]", s->free_block_num);
}

