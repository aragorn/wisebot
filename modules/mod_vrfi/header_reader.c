/* $Id$ */
#include <stdio.h> /* printf(3) */
#include <string.h> /* strerror(3) */
#include <unistd.h> /* read(2) */
#include <fcntl.h> /* O_RDWR */
#include <errno.h>
#include "header_handle.h"

#define BUF_SIZE	sizeof(header_data_t)

int main(int argc, char** argv)
{
	int fd=0, rv=0;
	char buf[BUF_SIZE];
	header_data_t *ptr=NULL;

	if (argc <= 1) {
		printf("Usage: %s filename(header file)\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		printf("open failed:%s", strerror(errno));
		return 1;
	}

	while (1) {
		rv = read(fd, buf, BUF_SIZE);
		if (rv == -1) {
			printf("read failed:%s", strerror(errno));
			return 1;
		}
		else if (rv == 0) {
			break;
		}
		else if (rv != BUF_SIZE) {
			printf("error! %d bytes read when expecting %d bytes",
							rv, (int)BUF_SIZE);
			return 1;
		}

		ptr = (header_data_t*)buf;
		printf("key:%d, [start:%u, nelm:%u, fileno:%d], [prev fileno:%d, pos:%d]\n",
				ptr->metadata.key,
				ptr->header.start, ptr->header.nelm, ptr->header.fileno,
				ptr->metadata.prev.fileno, ptr->metadata.prev.position);
	}

	return 0;
}
