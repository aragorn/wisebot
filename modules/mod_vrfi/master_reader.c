/* $Id$ */
#include "mod_vrfi.h"
#include "mod_indexer/hit.h"

#define BUFSIZE(fixed_size) (fixed_size + \
			sizeof(variable_data_info_t) + sizeof(header_pos_t) )
#define MAX_BUF 1024
int fixed_size = 8;

/*
void show_master_data(uint32_t key, char *buf)
{
	header_pos_t *headerpos=NULL;
	variable_data_info_t *info=NULL;
	inv_idx_header_t *inv_idx_header=NULL;

	headerpos = (header_pos_t*)(buf+fixed_size+sizeof(variable_data_info_t));
	info = (variable_data_info_t*)(buf+fixed_size);
	
	printf("key:%u fixeddata:%s, ndata:%u, last_header(fileno:%d, position:%u)\n",
			key, buf, 
			info->ndata,
			headerpos->fileno, headerpos->position);
}
*/
void show_master_data(uint32_t key, char *buf)
{
	header_pos_t *headerpos=NULL;
	variable_data_info_t *info=NULL;
	inv_idx_header_t *inv_idx_header=NULL;

	headerpos = (header_pos_t*)(buf+fixed_size+sizeof(variable_data_info_t));
	info = (variable_data_info_t*)(buf+fixed_size);
	
	inv_idx_header = (inv_idx_header_t*)buf;

	printf("key:%u ndocs:%u, ntotal_hits:%u, ndata:%u, last_header(fileno:%d, position:%u)\n",
			key, inv_idx_header->ndocs, inv_idx_header->ntotal_hits,
			info->ndata,
			headerpos->fileno, headerpos->position);
}

int main(int argc, char** argv)
{
	int fd=0, rv=0;
	char buf[MAX_BUF];
	uint32_t key=0;

	if (argc <= 1) {
		printf("Usage: %s filename(header file) fixed_size(default to %d)\n",
						argv[0], fixed_size);
		return 1;
	}

	if (argc == 3) {
		fixed_size = atoi(argv[2]);
	}
	printf("fixed_size:%d\n", fixed_size);

	if (MAX_BUF < BUFSIZE(fixed_size)) {
		printf("MAX_BUF(%d) < BUFSIZE(fixed_size)(%d)\n",
				MAX_BUF, (int)BUFSIZE(fixed_size));
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		printf("open failed:%s\n", strerror(errno));
		return 1;
	}

	rv = lseek(fd, INFO_SIZE, SEEK_SET);
	if (rv == -1) {
		printf("lseek failed:%s\n", strerror(errno));
		return 1;
	}

	key=0;
	while (1) {
		rv = read(fd, buf, BUFSIZE(fixed_size));
		if (rv == -1) {
			printf("read failed:%s\n", strerror(errno));
			return 1;
		}
		else if (rv == 0) {
			break;
		}
		else if (rv != BUFSIZE(fixed_size)) {
			printf("error! %d bytes read when expecting %d bytes\n",
							rv, (int)BUFSIZE(fixed_size));
			return 1;
		}

		show_master_data(key, buf);

		key++;
	}

	return 0;
}
