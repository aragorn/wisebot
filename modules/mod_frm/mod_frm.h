#include "frm.h"

#define FRM_MAGIC "FRMF"
#define MAX_BLOCK_NUM (128)
#define MAX_BLOCK_SIZE (1024*1024*1)    //1M

typedef struct {
    char magic[4];
    int allocated_blocks;
	int block_size; /* should be multiple of pagesize and less than MAX_BLOCK_SIZE */
} header_t;

struct mmap_file_t {
	header_t *header;
	int allocated_blocks;
	int fd;
    void *block[MAX_BLOCK_NUM];
};

