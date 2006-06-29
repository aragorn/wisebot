#include "vrm.h"
#include <inttypes.h>

#ifndef _MOD_VRM_H_
#define _MOD_VRM_H_

#define VRM_MAGIC "VRMF"
#define MAX_BLOCK_SIZE_BIT (23)
#define MAX_BLOCK_SIZE (1<<MAX_BLOCK_SIZE_BIT)
#define MAX_BLOCK_NUM  ((1<<(31-MAX_BLOCK_SIZE_BIT))-2)
/* MAX_BLOCK_SIZE_BIT     MAX_BLOCK_SIZE
 *           24 BIT  ->   16MB
 *           23 BIT  ->   8MB
 *           22 BIT  ->   4MB
 *           21 BIT  ->   2MB
 *           20 BIT  ->   1MB
 *           16 BIT  ->   64KB
 *         
 * max file size = MAX_BLOCK_SIZE + MAX_BLOCK_NUM * MAX_BLOCK_SIZE
 *                 ~~~~~~~~~~~~~~
 *                    header size
 */

typedef struct {
    uint32_t block_num : (32-MAX_BLOCK_SIZE_BIT);
	uint32_t offset : MAX_BLOCK_SIZE_BIT;
} __attribute__((packed)) block_info_t;

typedef struct {
    char magic[4];
    int next_position;
    int allocated_blocks;
	int block_size; /* should be multiple of pagesize and less than MAX_BLOCK_SIZE */
} header_t;

typedef struct {
	header_t *header;
	int allocated_blocks;
	int fd;
    void *block[MAX_BLOCK_NUM];
} mmap_file_t;

struct vrm_t{
    int lock_id;
	mmap_file_t index;
	mmap_file_t data;
};
#endif
