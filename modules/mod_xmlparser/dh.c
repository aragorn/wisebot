/* $Id$ */
#include "softbot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct _dh_t dh_t;
typedef struct _saving_part_dh_t saving_part_dh_t;
typedef struct _dir_t dir_t;
typedef struct _bucket_header_t bucket_header_t;
typedef struct _data_header_t data_header_t;

struct _dh_t {
	/* saving data: must be same with struct _saving_part_dh_t */
	int datasize;						/* data size */
	int usedsize;						/* used bucket number */

	/* non-saving data */
	int perm;							/* permition to insert or delete data
										   into this dynamic hash object */
#define PERM_RD			1
#define PERM_WR			2
#define	PERM_RD_ONLY	1
#define PERM_RD_WR		3
	int allocatedsize;					/* allocated bucket number */
	dir_t *dirs;
	void *buckets;

	unsigned short dirsize;				/* current directory array size */
	unsigned char dataperbucket;		/* data number per bucket */
};

struct _saving_part_dh_t {
	int datasize;
	int usedsize;
	unsigned short dirsize;
	unsigned char dataperbucket;
        char padding;
};

struct _dir_t {
	int offset;							/* directory has offset of bucket;
										   it could be order number of bucket, 
										   but each data number of each bucket 
										   can be different. so, we use offset 
										   of bucket that start point is 
										   'buckets' member of
										   dynamic hash object */
};

#define MAX_DATA_PER_BUCKET				255
struct _bucket_header_t {
	unsigned short level;
	unsigned short key;
	unsigned char datanum;
        char padding[3];
};

#define MAX_KEY_LEN						128
struct _data_header_t {
	char key[MAX_KEY_LEN];
};

#define DEFAULT_DIR_SIZE				8
#define DEFAULT_BUCKET_NUM				8
#define EXPANSION_BUCKET_UNIT			8
#define DEFAULT_DATA_PER_BUCKET			8
/* return size of memory to append to 'buckets' that is member of
 * dynamic hash object
 * a: dynamic hash object */
#define EXPANSION_UNIT_SIZE(a)	(sizeof(bucket_header_t)+((a)->datasize+ \
					 			sizeof(data_header_t))*(a)->dataperbucket)* \
								EXPANSION_BUCKET_UNIT
/* return bucket pointer (void *)
 * a: dynamic hash object pointer
 * b: offset of bucket */
#define BUCKET(a,b)						((a)->buckets + (b))
/* return bucket header structure pointer (bucket_header_t *)
 * a: dynamic hash object
 * b: offset of bucket */
#define BUCKET_HEADER(a,b)				((bucket_header_t *)((a)->buckets+(b)))
/* return default bucket size
 * a: size of input data */
#define DEFAULT_BUCKET_SIZE(a)			(sizeof(bucket_header_t)+ \
										DEFAULT_DATA_PER_BUCKET* \
										(sizeof(data_header_t)+(a)))
/* return bucket size variable for input data size and data num per bucket
 * a: size of input data
 * b: data number per bucket */
#define BUCKET_SIZE(a,b)				(sizeof(bucket_header_t)+(b)* \
										(sizeof(data_header_t)+(a)))
/* return data header structure pointer (data_header_t *)
 * a: bucket start pointer (void *)
 * b: size of each data
 * i: order number of data */
#define DATA_HEADER(a,b,i)				((data_header_t *)((a)+ \
										sizeof(bucket_header_t)+ \
										((b)+sizeof(data_header_t))*(i)))
/* return data chunk start pointer
 * a: data header start point */
#define DATA_CHUNK(a)					((void *)(a)+sizeof(data_header_t))
#define IS_OVERFLOW(a,b)				((a)->usedsize+(b)>(a)->allocatedsize)
#define NEED_TO_DOUBLING(a,b)			((a)->dirsize <= \
										((bucket_header_t *)(b))->level)

static data_header_t *searchempty(const void *bucket, const int datasize);
static void *searchbucket(const void *bucket, const int datasize, 
		const char *key);
static unsigned short hashfnct(const char *key, const short dirsize);
static int doubling(dh_t *dh);
static int split(dh_t *dh, const int bucket);
static int expandbucket(dh_t *dh);

/* create dynamic hash object and return
 * size: size of each value data to insert this hash */
dh_t *dh_create(int size)
{
	int i, j, bucketsize;
	dh_t *dh;

	dh = (dh_t *)sb_malloc(sizeof(dh_t));
	if (dh == NULL) {
		// error("cannot allocate hash object: %s", strerror(errno));
		return NULL;
	}

	/* allocate directory array */
	dh->dirsize = DEFAULT_DIR_SIZE;
	dh->dirs = (dir_t *)sb_malloc(DEFAULT_DIR_SIZE * sizeof(dir_t));
	if (dh->dirs == NULL) {
		// error report
		sb_free(dh);
		return NULL;
	}

	/* allocate buckets */
	dh->datasize = size;
	dh->dataperbucket = DEFAULT_DATA_PER_BUCKET;

	bucketsize = DEFAULT_BUCKET_SIZE(size);
	dh->buckets = (void *)sb_malloc(DEFAULT_BUCKET_NUM * bucketsize);
	if (dh->buckets == NULL) {
		// error report
		sb_free(dh->dirs);
		sb_free(dh);
		return NULL;
	}
	dh->allocatedsize = bucketsize * DEFAULT_BUCKET_NUM;

	/* initialize directory array & buckets */
	for (i=0; i<DEFAULT_DIR_SIZE; i++) {
		dh->dirs[i].offset = i * bucketsize;
		BUCKET_HEADER(dh, dh->dirs[i].offset)->level = DEFAULT_DIR_SIZE;
		BUCKET_HEADER(dh, dh->dirs[i].offset)->key = i;
		BUCKET_HEADER(dh, dh->dirs[i].offset)->datanum = 
			DEFAULT_DATA_PER_BUCKET;
		for (j=0; j<DEFAULT_DATA_PER_BUCKET; j++)
			DATA_HEADER(BUCKET(dh, dh->dirs[i].offset), size, j)->key[0] = '\0';
	}

	dh->usedsize = DEFAULT_DIR_SIZE * bucketsize;

	dh->perm = PERM_RD_WR;

	return dh;
}

void dh_destroy(dh_t *dh)
{
	if (dh->perm & PERM_WR) {
		sb_free(dh->dirs);
		dh->dirs = NULL;
		sb_free(dh->buckets);
		dh->buckets = NULL;
	}
	sb_free(dh);
}

/* read dynamic hash data in sequential memory
 * and create dh_t object
 * and return address of dh_t object.
 * whole data is newly duplicate to another memory, so
 * caller can free input data after calling this function.
 *
 * datalen: input size of data, output read size
 */
dh_t *dh_load(void *data, int *datalen)
{
	int offset;
	dh_t *dh;

	if (sizeof(saving_part_dh_t) > *datalen) {
		// error("head chunk to load is not proper format");
		return NULL;
	}

	dh = (dh_t *)sb_malloc(sizeof(dh_t));
	if (dh == NULL) {
		// error("cannot allocate hash object: %s", strerror(errno));
		return NULL;
	}
	memcpy(dh, data, sizeof(dh_t));
	offset = sizeof(saving_part_dh_t);

	if (sizeof(saving_part_dh_t) + sizeof(dir_t) * dh->dirsize + dh->usedsize 
			> *datalen) {
		// error("data chunk to load is not proper format");
		sb_free(dh);
		return NULL;
	}

	dh->dirs = (dir_t *)sb_malloc(sizeof(dir_t) * dh->dirsize);
	if (dh->dirs == NULL) {
		// error report
		sb_free(dh);
		return NULL;
	}
	memcpy(dh->dirs, data + offset, sizeof(dir_t) * dh->dirsize);
	offset += sizeof(dir_t) * dh->dirsize;

	dh->allocatedsize = dh->usedsize;
	dh->buckets = (void *)sb_malloc(dh->allocatedsize);
	if (dh->buckets == NULL) {
		// error report
		sb_free(dh->dirs);
		sb_free(dh);
		return NULL;
	}
	memcpy(dh->buckets, data + offset, dh->usedsize);
	offset += dh->usedsize;

	dh->perm = PERM_RD_WR;

	*datalen = offset;
	return dh;
}

/* read dynamic hash data in sequential memory
 * and create dh_t object
 * and return address of dh_t object.
 * but, it doesn't duplicate data, so don't free

 * data that is inputed as parameter after calling
 * this function.
 *
 * datalen: input size of data, output read size
 */
dh_t *dh_load2(void *data, int *datalen)
{
	int offset;
	dh_t *dh;

	offset = 0;

	if (sizeof(saving_part_dh_t) > *datalen) {
		// error("head chunk to load is not proper format");
		return NULL;
	}

	dh = (dh_t *)sb_malloc(sizeof(dh_t));
	if (dh == NULL) {
		// error("cannot allocate hash object: %s", strerror(errno));
		return NULL;
	}
	memcpy(dh, data, sizeof(saving_part_dh_t));
	offset += sizeof(saving_part_dh_t);

	if (sizeof(saving_part_dh_t) + sizeof(dir_t) * dh->dirsize + dh->usedsize
			> *datalen) {
		// error("data chunk to load is not proper format");
		sb_free(dh);
		return NULL;
	}

	dh->dirs = (dir_t *)(data + offset);
	offset += sizeof(dir_t) * dh->dirsize;

	dh->buckets = data + offset;
	offset += dh->usedsize;

	dh->perm = PERM_RD_ONLY;

	*datalen = offset;
	return dh;
}

/* save whole data in sequential memory.
 * buffer to save data is must be allocated by caller.
 * return total size of data
 */
int dh_save(dh_t *dh, void *buf, int buflen)
{
	int offset;

	if (buflen < sizeof(saving_part_dh_t) + sizeof(dir_t) * dh->dirsize + dh->usedsize) {
		// error("buffer length is too short");
		return -1;
	}

	offset = 0;

	memcpy(buf, dh, sizeof(saving_part_dh_t));
	offset += sizeof(saving_part_dh_t);

	memcpy(buf + offset, dh->dirs, sizeof(dir_t) * dh->dirsize);
	offset += sizeof(dir_t) * dh->dirsize;

	memcpy(buf + offset, dh->buckets, dh->usedsize);
	offset += dh->usedsize;

	return offset;
}

/* return three vectors to save.
 * saving data is up to caller. caller need not to know
 * what these data mean. just save in sequential memory.
 * return value is summation of each data size.
 */
int dh_save2(dh_t *dh,
		  void **chunk1, int *len1,
		  void **chunk2, int *len2,
		  void **chunk3, int *len3)
{
	*chunk1 = dh; *len1 = sizeof(saving_part_dh_t);
	*chunk2 = dh->dirs; *len2 = sizeof(dir_t) * dh->dirsize;
	*chunk3 = dh->buckets; *len3 = dh->usedsize;
	return *len1 + *len2 + *len3;
}

int dh_get_savingsize(dh_t *dh)
{
	return sizeof(saving_part_dh_t) + 
		sizeof(dir_t) * dh->dirsize +
		dh->usedsize;
}

/* search value that matches to key
 * return -1 if fault
 * return value as ptr */
int dh_search(dh_t *dh, const char *key, void **ptr)
{
	dir_t *dir;
	void *ret;
	dir = &(dh->dirs[hashfnct(key, dh->dirsize)]);
	if ((ret = searchbucket(BUCKET(dh, dir->offset), dh->datasize, key))
			== NULL) {
		// error("no data");
		*ptr = NULL;
		return -1;
	}
	*ptr = ret;
	return 1;
}

/* insert data of which size is that you input when create
 * dynamic hash object(calling 'dh_create') 
 * return -1 if fault */
int dh_insert(dh_t *dh, const char *key, void *value)
{
	dir_t *dir;
	data_header_t *data = NULL;
	int offset;

	if (!(dh->perm & PERM_WR)) {
		// error("no permition to insert data into this hash");
		return -1;
	}

	dir = &(dh->dirs[hashfnct(key, dh->dirsize)]);
	offset = dir->offset;

	while ((data = searchempty(BUCKET(dh, offset), dh->datasize)) == NULL) {
		if (NEED_TO_DOUBLING(dh, BUCKET(dh, offset))) {
			if (doubling(dh) == -1) {
				// error("cannot doubling");
				return -1;
			}
		}

		if (split(dh, offset) == -1) {
			// error("cannot split bucket");
			return -1;
		}

		dir = &(dh->dirs[hashfnct(key, dh->dirsize)]);
		offset = dir->offset;
	}

	if (data == NULL) {
		// error("'data' must not be null: something goes wrong");
		return -1;
	}

	/* copy data */
	strncpy(data->key, key, MAX_KEY_LEN);
	data->key[MAX_KEY_LEN-1] = '\0';
	memcpy(DATA_CHUNK(data), value, dh->datasize);

	return 1;
}

/* not yet implemented */
int dh_delete(dh_t *dh, const char *key)
{
	if (!(dh->perm & PERM_WR)) {
		// error("no permition to delete data into this hash");
		return -1;
	}
	return 1;
}

/* search empty data slot in bucket
 * return start point of empyt data slot
 * return type is pointer of data header structure */
static data_header_t *searchempty(const void *bucket, const int datasize)
{
	int i;
	bucket_header_t *b;
	data_header_t *d = NULL;

	b = (bucket_header_t *)bucket;
	/* linear search */
	for (i=0; i<b->datanum; i++) {
		d = DATA_HEADER(bucket, datasize, i);
		if (d->key[0] == '\0') {
			break;
		}
	}
 
/*	fprintf(stderr, "i(%d) == b->datanum(%d)\n", i, b->datanum);*/
	return (i == b->datanum) ? NULL : d;
}

/* search data slot matches to key
 * return type is pointer of data header structure */
static void *searchbucket(const void *bucket, const int datasize, const char *key)
{
	int i;
	bucket_header_t *b;
	data_header_t *d = NULL;

	b = (bucket_header_t *)bucket;
	/* linear search */
	for (i=0; i<b->datanum; i++) {
		d = DATA_HEADER(bucket, datasize, i);
		if (strncasecmp(d->key, key, MAX_KEY_LEN) == 0) {
			break;
		}
	}

	return (i == b->datanum || d == NULL) ? NULL : 
		(void *)d + sizeof(data_header_t);
}

static unsigned short hashfnct(const char *key, const short dirsize)
{
	int sum, i;
	for (sum=0, i=0; key[i]!='\0' && i<MAX_KEY_LEN; i++) {
		sum += (int)(tolower(key[i]) & 0x7f);
	}
/*	printf("sum: %d, dirsize: %d, mod: %d\n", sum, dirsize, (unsigned short)(sum%dirsize)); */
	return sum % dirsize;
}

static int doubling(dh_t *dh)
{
	dir_t *tmp;

	tmp = (dir_t *)sb_realloc(dh->dirs, sizeof(dir_t) * dh->dirsize * 2);
	if (tmp == NULL) {
		// error("cannot doubling directory: %s", strerror(errorno));
		return -1;
	}

	memcpy(&(tmp[dh->dirsize]), tmp, sizeof(dir_t) * dh->dirsize);

/*	printf("doubling is occured: %d -> %d\n", dh->dirsize, dh->dirsize*2);*/
	dh->dirs = tmp;
	dh->dirsize *= 2;

	return 1;
}

static int split(dh_t *dh, const int offset)
{
	int i, dataperbucket, next;
	dir_t *dir;
	bucket_header_t *b;
	data_header_t *d0, *d1;

	b = BUCKET_HEADER(dh, offset);

	dataperbucket = dh->dataperbucket;
	if (b->level*2 > dh->dirsize) {
		/* FIXME: what to be done here? */
		fprintf(stderr, "databucket is wrong: bucket level(%d) > "
						"dynamic hash dirsize(%d)\n",
				b->level, dh->dirsize);
		return -1;
	}

	/* make new bucket */
	while (IS_OVERFLOW(dh, BUCKET_SIZE(dh->datasize, dataperbucket))) {
		if (expandbucket(dh) == -1) {
			fprintf(stderr, "cannot expand bucket");
			return -1;
		}

		b = BUCKET_HEADER(dh, offset);
	}

	/* set directory & initialize bucket */
/*	fprintf(stderr, "new buckets offset: %d\n", dh->usedsize);*/
	BUCKET_HEADER(dh, dh->usedsize)->level = b->level * 2;
	BUCKET_HEADER(dh, dh->usedsize)->key = b->key + b->level;
/*	fprintf(stderr, "new buckets datanum: %d\n", dataperbucket);*/
	BUCKET_HEADER(dh, dh->usedsize)->datanum = dataperbucket;
	for (i=0; i<dataperbucket; i++) {
		DATA_HEADER(BUCKET(dh, dh->usedsize), dh->datasize, i)->key[0] = '\0';
	}

	for (i=1; i*(b->level)<dh->dirsize; i+=2) {
		dir = &(dh->dirs[b->key + i * b->level]);
		dir->offset = dh->usedsize;
	}

	dh->usedsize += BUCKET_SIZE(dh->datasize, dataperbucket);
	b->level *= 2;

/*	fprintf(stderr, "b->datanum: %d\n", b->datanum);*/
	for (i=0; i<b->datanum; i++) {
		d0 = DATA_HEADER(BUCKET(dh, offset), dh->datasize, i);

		dir = &(dh->dirs[hashfnct(d0->key, dh->dirsize)]);
		next = dir->offset;
		if (offset != next) {
/*			fprintf(stderr, "next buckets offset: %d\n", next);*/
			d1 = searchempty(BUCKET(dh, next), dh->datasize);
			if (d1 == NULL) {
				fprintf(stderr, 
						"'next[%d]' bucket must have empty data slot\n",
						next);
				return -1;
			}

			strncpy(d1->key, d0->key, MAX_KEY_LEN);
			d1->key[MAX_KEY_LEN-1] = '\0';
			memcpy(DATA_CHUNK(d1), DATA_CHUNK(d0), dh->datasize);

			d0->key[0] = '\0';
		}
	}

	return 1;
}

static int expandbucket(dh_t *dh)
{
	void *tmp;
	tmp = sb_realloc(dh->buckets, dh->allocatedsize + EXPANSION_UNIT_SIZE(dh));
	if (tmp == NULL) {
		fprintf(stderr, "fail realloc: %s", strerror(errno));
		return -1;
	}

	dh->buckets = tmp;
	dh->allocatedsize += EXPANSION_UNIT_SIZE(dh);
	return 1;
}
