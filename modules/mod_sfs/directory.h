/* $Id$ */

#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include "sfs_types.h"

int dir_get_entry(sfs_t* sfs, dir_hash_entry_t* entry);
int dir_add_entry(sfs_t* sfs, dir_hash_entry_t* entry);
int dir_update_entry(sfs_t* sfs, dir_hash_entry_t* entry);

int dir_get_file_array(sfs_t* sfs, int* file_array);

#endif

