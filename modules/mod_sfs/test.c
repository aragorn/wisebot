/* $Id$ */
#include "common_core.h"
#include "setproctitle.h"
#include "mod_sfs.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h> /* O_RDWR,O_CREAT,... */
#include <unistd.h> /* close(2) */
#include <time.h>
#include <stdlib.h>

static int sfs_size = 128*1024*1024;
static int block_size = 128;
#ifdef WIN32
static char path[]="sfs0";
#else
static char path[]="dat/test/sfs0";

/******************************************************************************/
#endif

#define DATA_SIZE       (32*1024*1024)
#define MAX_FILE_ID     (1*1024*1024)
#define MAX_FILE_SIZE   (32*1024*1024)
#define NO_SHUFFLE      (0)
#define SHUFFLE         (1)
#define ASCII_DATA      (0)
#define BINARY_DATA     (1)

typedef struct {
  int id;
  int size;
  int offset; /* 저장할 test data 영역에서의 시작 offset */
  int written_bytes;
} test_file_t;

/* read/write 할 byte 크기를 결정 */
typedef struct {
  int min;
  int max; /* min, max 값이 다른 경우, 이 사이의 random값을 선택 */
} test_size_t;

typedef struct {
  int  fd;
  int  load_option;
  sfs_t *sfs;
  char *data;
  int  data_type;
  test_file_t *test_files;

  test_size_t append_size;
  test_size_t read_size;

  int *test_order;

  int max_append_count;
  int append_count;
  int append_shuffle;
  int read_shuffle;

  char *buffer;
} test_input_t;
/*****************************************************************************/
static int open_test_file();
static void allocate_test_input(test_input_t *t);
static void show_test_input(test_input_t *t);
static void make_test_data(char* data, int type);
static void make_files(test_file_t* f);
static void shuffle_order(int *list, int opt, int count);

static int append_file(test_input_t *t, test_file_t *test_file);
static int read_file(test_input_t *t, test_file_t *test_file);

static int TEST(const char *name, int(*function)(test_input_t *), test_input_t *t);
static int FORMAT (test_input_t *t);
static int LOAD (test_input_t *t);
static int UNLOAD (test_input_t *t);
static int LOAD_AND_UNLOAD(test_input_t *t);

static int APPEND(test_input_t *t);
static int REPEAT_APPEND(test_input_t *t);
static int READ(test_input_t *t);


static int TEST(const char *name, int(*function)(test_input_t *), test_input_t *t)
{
	struct timeval tv1, tv2;
	double diff;
	int r;

	warn("** start to test %s *******************************", name);
	gettimeofday(&tv1, NULL);
	r = function(t);
	gettimeofday(&tv2, NULL);

	if (r == FAIL)
		error("** test failed ************************************");
	diff = timediff(&tv2, &tv1);
	warn("** end of test %s : %2.2f seconds ******************", name, diff);

	return r;
}


#ifdef WIN32
static int open_test_file() 
{
	HANDLE hFile = NULL;

	hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hFile == INVALID_HANDLE_VALUE ) {
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

		error("TEST>can't open file[%s][%s]", path, lpMsgBuf);
		LocalFree( lpMsgBuf );
		return -1;
	}

	return (int)hFile;
}
#else
static int open_test_file()
{
	int fd;

	fd = sb_open(path, O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
	if (fd == -1) { 
		error("cannot open file[%s]: %s", path, strerror(errno));
		return -1;
	}

	return fd;
}
#endif

static int FORMAT (test_input_t *t)
{
	t->sfs = sfs_create(0, t->fd, 0);
	if(t->sfs == NULL) return FAIL;

	info("format!!");
	if(sfs_format(t->sfs, O_FAT|O_HASH_ROOT_DIR, sfs_size, block_size) == FAIL) return FAIL;

	sfs_destroy(t->sfs);

	return SUCCESS;
}

static int LOAD (test_input_t *t)
{
	t->sfs = sfs_create(0, t->fd, 0);
	if(t->sfs == NULL) {
		return FAIL;
	}

	if(sfs_open(t->sfs, t->load_option) == FAIL) {
		error("TEST>can't load sfs");
		return FAIL;
	}

	return SUCCESS;
}

static int UNLOAD (test_input_t *t)
{
	sfs_close(t->sfs);
	sfs_destroy(t->sfs);

	return SUCCESS;
}

static int LOAD_AND_UNLOAD(test_input_t *t)
{
	if (LOAD(t)   == FAIL) return FAIL;
	if (UNLOAD(t) == FAIL) return FAIL;

	return SUCCESS;
}


static int append_file(test_input_t *t, test_file_t *test_file)
{
	int size, offset, bytes;

	if (t->append_size.max - t->append_size.min > 0) {
		size = t->append_size.min + (rand() % (t->append_size.max - t->append_size.min));
	} else {
		size = t->append_size.min;
	}
	offset = (test_file->offset + test_file->written_bytes) % DATA_SIZE;

	memcpy(t->buffer, t->data + offset, size);

	bytes = sfs_append(t->sfs, test_file->id, size, t->buffer);
	if (bytes < 0 )
	{
		error("sfs_append returned %d. file id[%d], append size[%d]", bytes, test_file->id, size);
		return FAIL;
	}
	if (bytes != size)
	{
		warn("segment is full. file id[%d]", test_file->id);
		return SEGMENT_FULL;
	}

	test_file->written_bytes += bytes;

	return SUCCESS;	
}

static int read_file(test_input_t *t, test_file_t *test_file)
{

	int size, bytes;
	int data_offset; /* offset in char *data */
	int file_offset; /* offset in a file */
	int offset        = test_file->offset;
	int written_bytes = test_file->written_bytes;

	if (t->read_size.max - t->read_size.min > 0) {
		size = t->read_size.min + (rand() % (t->read_size.max - t->read_size.min));
	} else {
		size = t->read_size.min;
	}
	file_offset = (written_bytes - size > 0) ? written_bytes - size : 0;
	data_offset = (offset + file_offset) % DATA_SIZE;

	bytes = sfs_read(t->sfs, test_file->id, file_offset, size, t->buffer);
	if (bytes != size && bytes != (written_bytes - file_offset))
	{
		error("sfs_read returned %d. file id[%d], read_size[%d]", bytes, test_file->id, size);
		error("written_bytes[%d] - file_offset[%d] = [%d]", written_bytes, file_offset, written_bytes-file_offset);
		
		/*
		error("sfs->super_block->file_count[%d],file_total_byte[%d]",
				t->sfs->super_block->file_count,
				t->sfs->super_block->file_total_byte);
		*/
		
		return FAIL;
	}

	if (memcmp(t->buffer, t->data + data_offset, bytes) != 0) {
		error("memcmp returned non-zero value. ");
		error("sfs_read returned %d. file id[%d], read_size[%d]", bytes, test_file->id, size);
		error("written_bytes[%d] - file_offset[%d] = [%d]", written_bytes, file_offset, written_bytes-file_offset);

		t->buffer[bytes] = '\0';
		error("read    data[%s]", t->buffer);
		memcpy(t->buffer, t->data+data_offset, bytes); t->buffer[bytes] = '\0';
		error("written data[%s]", t->buffer);
		/*
		error("sfs->super_block->file_count[%d],file_total_byte[%d]",
				sfs->super_block->file_count,
				sfs->super_block->file_total_byte);
		*/
        memset(t->buffer, 0x00, DATA_SIZE);
        bytes = sfs_read(t->sfs, test_file->id, 0, MAX_FILE_SIZE, t->buffer);
        t->buffer[bytes] = '\0';
        error("full file data[%s]", t->buffer);

		return FAIL;
	}

	return SUCCESS;	
}

static int INIT_TEST_DATA(test_input_t *t)
{
	make_test_data(t->data, t->data_type);
	make_files(t->test_files);
	shuffle_order(t->test_order, t->append_shuffle, MAX_FILE_ID);

	return SUCCESS;
}

static int APPEND(test_input_t *t)
{
	int i;
	int total_written_bytes = 0;

	//show_test_input(t);

	t->append_count = 0;
	for (i = 0; i < t->max_append_count; i++) {
		int id = t->test_order[i];
		int ret = append_file(t, &(t->test_files[id-1]));
		total_written_bytes += t->test_files[id-1].written_bytes;
		if (ret == FAIL) {
			error("%d'th append_file(id[%d]) failed.", i, id);
			error("total written bytes[%d]", total_written_bytes);
			return FAIL;
		}

		t->append_count++;

		if ( ret == SEGMENT_FULL ) break;
	}
	info("total written bytes[%d]", total_written_bytes);

	return SUCCESS;
}


static int READ(test_input_t *t)
{
	int i;

	//show_test_input(t);

	shuffle_order(t->test_order, t->append_shuffle, t->append_count);

	for (i = 0; i < t->append_count; i++) {
		int id = t->test_order[i];
		int ret = read_file(t, &(t->test_files[id-1]));
		if (ret == FAIL) {
			crit("read_file(id[%d]) failed.", id);
			break;
		}
	}

	return SUCCESS;
}

static int REPEAT_APPEND(test_input_t *t)
{
	int i, r = FAIL;
	char mesg[64];

	for(i = 0; i<10; i++) {
		sprintf(mesg, "%dth append test", i);
		if (i % 10 == 0) r = TEST(mesg, APPEND, t);
		else             r = APPEND(t);

		if (r == FAIL) {
			error("%d'th append_file failed.", i);
			return FAIL;
		}

		if ( r == SEGMENT_FULL ) break;
	}

	return r;
}


int main(int argc, char* argv[], char *envp[])
{
	int try;
	test_input_t t;

#ifndef WIN32
	init_setproctitle(argc, argv, envp);
	log_setlevelstr("debug");
#endif

	memset(&t, 0x00, sizeof(test_input_t));
	allocate_test_input(&t);

	t.fd = open_test_file();
	if (t.fd < 0) exit(EXIT_FAILURE);

	TEST("sfs format", FORMAT, &t);
	TEST("sfs load",   LOAD_AND_UNLOAD, &t);
	
	for (try = 0; try < 10; try++) {
		t.load_option = O_MMAP;
		t.max_append_count = MAX_FILE_ID;

		switch(try) {
		case 0:
			t.data_type = ASCII_DATA;
			t.append_size.min = 10;
			t.append_size.max = 10;
			t.read_size.min = 100;
			t.read_size.max = 100;
			t.append_shuffle = NO_SHUFFLE;
			t.read_shuffle   = NO_SHUFFLE;
			break;
	
		case 1:
			t.data_type = ASCII_DATA;
			t.append_size.min = 256;
			t.append_size.max = 256;
			t.read_size.min = 256;
			t.read_size.max = 256;
			t.append_shuffle = NO_SHUFFLE;
			t.read_shuffle   = NO_SHUFFLE;
			break;
		case 2:
			t.data_type = BINARY_DATA;
			t.append_size.min = 0;
			t.append_size.max = 4096;
			t.read_size.min = 1024;
			t.read_size.max = 1024;
			t.max_append_count = 1024; /* this enables repeated append */
			t.append_shuffle = SHUFFLE;
			t.read_shuffle   = SHUFFLE;
			break;

		case 3:
			t.data_type = BINARY_DATA;
			t.append_size.min = MAX_FILE_SIZE;
			t.append_size.max = MAX_FILE_SIZE;
			t.read_size.min = MAX_FILE_SIZE;
			t.read_size.max = MAX_FILE_SIZE;
			t.append_shuffle = SHUFFLE;
			t.read_shuffle   = SHUFFLE;
			break;

		case 4:
			t.data_type = ASCII_DATA;
			t.append_size.min = 0;
			t.append_size.max = 1000;
			t.read_size.min = 0;
			t.read_size.max = 1000;
			t.append_shuffle = SHUFFLE;
			t.read_shuffle   = SHUFFLE;
			break;

		default:
			continue;
		}
		FORMAT(&t); 

		LOAD(&t); INIT_TEST_DATA(&t);
		TEST("repeated append test", REPEAT_APPEND, &t);
		TEST("read(O_MMAP) test", READ, &t);
		UNLOAD(&t);

		t.load_option = O_FILE;
		LOAD(&t);
		TEST("read(O_FILE) test", READ, &t);
		UNLOAD(&t);
	}

	info("TEST>end.");
#ifdef WIN32
    sb_close_win(t.fd);
#else
    close(t.fd);
#endif

	return EXIT_SUCCESS;
}

/*****************************************************************************/

static void show_test_input(test_input_t *t)
{
	char shuffle[2][20]   = {"NO_SHUFFLE", "SHUFFLE"};
	char data_type[2][20] = {"ASCII_DATA", "BINARY_DATA"};


	info("append_size[%d][%d] read_size[%d][%d]",
		t->append_size.min, t->append_size.max,
		t->read_size.min, t->read_size.max);
	info("data_type[%s], append_shuffle[%s] read_shuffle[%s]",
		data_type[t->data_type],
		shuffle[t->append_shuffle],
		shuffle[t->read_shuffle]);
}

static void allocate_test_input(test_input_t *t)
{
	int i;

	if (t->data == NULL)
		t->data = (char *)sb_malloc(DATA_SIZE * 2);
	if (t->data == NULL) {
		error("data malloc(%d * 2) failed: %s", DATA_SIZE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (t->test_files == NULL)
		t->test_files = (test_file_t*)sb_malloc(sizeof(test_file_t)*MAX_FILE_ID);
	if (t->test_files == NULL) {
		error("test_files malloc(%d * %d) failed: %s", (int) sizeof(test_file_t), MAX_FILE_ID, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (t->test_order == NULL)
		t->test_order = (int *)sb_malloc(sizeof(int) * MAX_FILE_ID);
	if (t->test_order == NULL) {
		error("test_order malloc(%d * %d) failed: %s", (int)sizeof(int), MAX_FILE_ID, strerror(errno));
		exit(EXIT_FAILURE);
	}

    for(i = 0; i < MAX_FILE_ID; i++) { 
        t->test_order[i] = i+1;
    }

	if (t->buffer == NULL)
		t->buffer = (char *)sb_malloc(MAX_FILE_SIZE);
	if (t->buffer == NULL) {
		error("buffer malloc(%d) failed: %s", MAX_FILE_SIZE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	t->load_option = O_MMAP;
	t->data_type = ASCII_DATA;
	t->append_count = 0;

	return;
}

static void make_test_data(char* data, int type)
{
	int i;

	debug("started");
	srand( (unsigned)time( NULL ) );

	for(i = 0; i < DATA_SIZE; i++) {
		if (type == ASCII_DATA)
		  data[i] = (rand() % (0x7E - 0x21)) + 0x21;
		else
		  data[i] = (rand() % 0xFF);
	}
	memcpy(data + DATA_SIZE, data, DATA_SIZE);
}

static void make_files(test_file_t* f)
{
	int i;

	debug("started");
	for(i = 0; i < MAX_FILE_ID; i++)
	{
		f[i].id = i+1;
		f[i].size = rand() % MAX_FILE_SIZE;
		f[i].offset = rand() % DATA_SIZE;
		f[i].written_bytes = 0;
	}

}

static void shuffle_order(int *list, int opt, int count)
{
    int i, target, tmp;

	debug("started");

	if (opt == NO_SHUFFLE) return;
    
    /*
    for i = 1 to n-1
      h \in_U {i, i+1, ... , n}
      swap( a[i], a[h] )
    end for
     */
    for(i = 0; i < count-1; i++) { 
        target = i + (rand() % (count-1 - i));

		tmp = list[target];
		list[target] = list[i];
		list[i] = tmp;
    }
}
