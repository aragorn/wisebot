#ifndef __TEST_H__
#define __TEST_H__

#include "mod_ifs.h"
#include <time.h>
#include <stdlib.h>

static int segment_size = 128*1024*1024;
static int block_size = 128;

#define DATA_SIZE       (32*1024*1024)
#define MAX_FILE_ID     (1024)
#define MAX_TEST_FILE_SIZE   (32*1024*1024)
#define NO_SHUFFLE      (0)
#define SHUFFLE         (1)
#define ASCII_DATA      (0)
#define BINARY_DATA     (1)
#define RESOURCE_MAX	(10)
#define MAGIC "TEST"
#define DATA_TYPE ASCII_DATA //ASCII_DATA, BINARY_DATA

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
  test_size_t append_size;
  test_size_t read_size;
  int append_shuffle;
  int read_shuffle;
} ifs_test;

typedef struct {
  int lock;
  int  fd;
  ifs_t *ifs;
  char *buffer;
  int *test_order;
  ifs_test test;
  int max_append_count;
  char *data;
  test_file_t *test_files;
} ifs_local_t;

typedef struct {
  char magic[4];
  int  data_type;
  int append_count;
} ifs_shared_t;

typedef struct {
  ifs_local_t local;
  ifs_shared_t* shared;
  char* p;
} ifs_test_input_t;
/*****************************************************************************/

ifs_test_input_t* test_load(ifs_test* test_load);
void test_unload();
void test_get_file_info(ifs_test_input_t* t, int i, test_file_t* f);
void test_set_file_info(ifs_test_input_t* t, int i, test_file_t* f);
void test_update_append_count(ifs_test_input_t* t, int count);
void test_shuffle_order(int *list, int opt, int count);

#endif

