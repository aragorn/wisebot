/* $Id$ */
#ifndef __MOD_SFS_H__
#define __MOD_SFS_H__

#ifdef WIN32
#  include "wisebot.h"
#else
#  include "softbot.h"
#endif

#include "sfs_types.h"

/*******************************************
 * error codes
 *******************************************/
#define FILE_NOT_EXISTS (-2)
#define SEGMENT_FULL    (-3)

sfs_t* sfs_create(int seq, int fd, int offset);
int sfs_destroy(sfs_t* sfs);

int sfs_format(sfs_t* sfs, int option, int size, int block_size);

int sfs_open(sfs_t* sfs, int type);
int sfs_close(sfs_t* sfs);

int sfs_append(sfs_t* sfs, int file_id, int size, void* buf);
int sfs_read(sfs_t* sfs, int file_id, int offset, int size, void* buf);

int sfs_exist_file(sfs_t* sfs, int file_id);

int sfs_block_read(sfs_t* sfs, int offset, int size, void* buf);
int sfs_block_write(sfs_t* sfs, int offset, int size, void* buf);
int sfs_get_block_reference(sfs_t* sfs, int offset, void** ptr);

int sfs_get_file_array(sfs_t* sfs, int* file_array);
int sfs_get_info(sfs_t* sfs, sfs_info_t* sfs_info, int file_id);

void __view_sfs_info(sfs_t* sfs);
#endif
