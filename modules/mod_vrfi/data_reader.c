/* $Id$ */
#include "data_handle.h"
#include "../mod_indexer/hit.h"

#define BUF_SIZE	1024

static void show_variable_data(char *buf)
{
	static uint32_t last_docid = 1;
	doc_hit_t *dochit = (doc_hit_t*)buf;
	hit_t *hit=NULL;
	int i=0;

	if (last_docid != dochit->id) {
		printf("\n");
		last_docid = dochit->id;
	}

	printf("id: %u, nhits:%d, field:%d \n",
			dochit->id, dochit->nhits, dochit->field);

	for (i=0; i<dochit->nhits; i++) {
		hit = &(dochit->hits[i]);
		if (hit->std_hit.type == 0) {
			printf("\t[%d] field:%d, position:%u\n",
						i, hit->std_hit.field, hit->std_hit.position);
		}
		else {
			printf("\t[%d] hit type 1 is not supported yet\n", i);
		}
	}
}

int main(int argc, char** argv)
{
	int fd=0, rv=0, variable_data_size = sizeof(doc_hit_t);
	char buf[BUF_SIZE];

	if (argc <= 1) {
		printf("Usage: %s filename(header file) variable_datasize(default to %d)\n",
						argv[0], variable_data_size);
		return 0;
	}

	if (argc == 3) {
		variable_data_size = atoi(argv[2]);
	}
	printf("variable_data_size:%d\n", variable_data_size);

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		printf("open failed:%s", strerror(errno));
		return 1;
	}

	while (1) {
		rv = read(fd, buf, variable_data_size);
		if (rv == -1) {
			printf("read failed:%s", strerror(errno));
			return 1;
		}
		else if (rv == 0) {
			break;
		}
		else if (rv != variable_data_size) {
			printf("error! %d bytes read when expecting %d bytes",
							rv, variable_data_size);
			return 1;
		}

		show_variable_data(buf);
	}

	return 0;
}
