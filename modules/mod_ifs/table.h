/* $Id$ */
#ifndef __TABLE_H__
#define __TABLE_H__

#ifndef WIN32
#include "softbot.h"
#else
#include "../mod_sfs/wisebot.h"
#endif

/*************************************************************
 * logical table : EMPTY, NOT_USE, or physical segment number
 * physical table : EMPTY, INDEX, ALLOCATED, DEFRAGMENT, TEMP
 *************************************************************/

#define EMPTY          (-1)    /* 비어있는 segment */
#define NOT_USE        (-2)    /* defragment로 인해 사용하지 않게 된 logical segment */
#define INDEX          (0)     /* index process에 의하여 임시 할당되는 segment */
#define ALLOCATED      (1)     /* index process에 의하여 할당되는 segment */
#define DEFRAGMENT     (2)     /* defragment process에 의해서 할당되는 segment - defragment process 만이 변경가능함 */
#define TEMP           (3)     /* ALLOCATED -> EMPTY로 전이할때의 중간단계 append process에 의해 변경 */

#define MAX_SECTOR_COUNT  100    /* max sector */
#define MAX_SEGMENT_COUNT (16)   /* MAX_FILE_SIZE / segement_size(256M) */
#define MAX_LOGICAL_COUNT (MAX_SECTOR_COUNT*MAX_SEGMENT_COUNT*2) /* NOT_USE를 커버하기 위해.. */

#define SECTOR_MAX_BIT 16	   //65536
#define SEGMENT_MAX_BIT 16     //65536

typedef struct {
    uint32_t sector    : SECTOR_MAX_BIT;
    uint32_t segment   : SEGMENT_MAX_BIT;
#ifdef WIN32
} segment_info_t;
#else
} __attribute__((packed)) segment_info_t;
#endif

typedef struct {
	int segment[MAX_SEGMENT_COUNT];
	struct timeval modify[MAX_SEGMENT_COUNT];
} sector_t;

typedef struct {
    int      logical_index[MAX_LOGICAL_COUNT];
    sector_t physical_sector[MAX_SECTOR_COUNT];
	int      segment_count_in_sector;
	int      allocated_physical_sector;
	int      allocated_physical_segment; // total

	uint32_t version; // 수정할 때마다 1증가
} table_t;

extern int temp_alive_time;
extern int *table_version;

int table_init(table_t* table, int count);
//int table_get_index(segment_info_t* seg, int count);
void table_get_segment_info(int index, int count, segment_info_t* seg);
int table_allocate(table_t* table, int* p, int state);
int table_update_last_logical_segment(table_t* table, int p);
int table_append_logical_segment(table_t* table, int p);
int table_update_logical_segment(table_t* table, int l, int p);
void table_overwrite_logical_segment(table_t* table, int l, int p);
int table_move_logical_segment(table_t* table, int lstart, int ldest);
int table_swap_logical_segment(table_t* table, int l1, int l2);
int table_get_read_segments(table_t* table, int* arr_read_seg);
int table_get_append_segment(table_t* table, int* p);
int table_clear_tmp_segment(table_t* table, int state);
void table_print(table_t* table);

#endif

