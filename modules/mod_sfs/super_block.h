/* $Id$ */
#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H

#include "sfs_types.h"
int superblock_add_file_count(super_block_t* s, int file_id);
int superblock_update_write_byte(super_block_t* s, int write_byte);
void superblock_view(super_block_t * s);

#endif
