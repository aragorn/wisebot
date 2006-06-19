/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "ipc.h"
#include "memory.h"
#include "setproctitle.h"
#include "mod_ifs.h"
#include "mod_ifs_defrag.h"
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h> /* sleep(3) */
#include <stdlib.h> /* atoi(3) */

// 2개의 segment를 defrag 했더니 이만큼까지 segment가 늘어날 수 있다.
#define MAX_PSEG_LIST 256

static void* buffer = NULL; // file copy 할 때 사용하는 buffer
static int buffer_size = 0;

/** module stuff **/
int defrag_group_size = 5;
static int defrag_delay = 3600;
static int indexdb_set = -1;
defrag_mode_t defrag_mode = DEFRAG_MODE_COPY;

#define OPTION (O_HASH_ROOT_DIR)

#define ACQUIRE_LOCK() \
	if ( acquire_lock(ifs->local.lock) != SUCCESS ) { \
		error("acquire_lock failed"); \
		return FAIL; \
	}

#define RELEASE_LOCK() \
    if ( release_lock(ifs->local.lock) != SUCCESS ) \
		error("release_lock failed. but go on");

// transaction 같은 건 아직 처리 안하고 있다.
static int __update_logical_segment(
		ifs_t* ifs, int lseg_start, int lseg_count, int* new_pseg_list, int new_pseg_count)
{
	int i;
	table_t *table = &ifs->shared->mapping_table;

	ACQUIRE_LOCK();

	// 중간에 에러나면 망했다고 보면 된다.
	if ( new_pseg_count != lseg_count ) {
		crit("new_pseg_count[%d] != lseg_count[%d]",new_pseg_count, lseg_count);

		if ( table_move_logical_segment( table, lseg_start+lseg_count, lseg_start+new_pseg_count ) != SUCCESS )
			crit("move segment failed. start[%d], dest[%d]. ifs crashed",
					lseg_start+lseg_count, lseg_start+new_pseg_count);
	}

	for ( i = 0; i < new_pseg_count; i++ ) {
		info("update table lseg[%d] pseg[%d] -> pseg[%d]",
				lseg_start+i, table->logical_index[lseg_start+i], new_pseg_list[i]);

		if ( table_update_logical_segment( table, lseg_start+i, new_pseg_list[i] ) != SUCCESS )
			crit("update lseg[%d] pseg[%d] -> pseg[%d] failed. ifs crashed",
					lseg_start+i, table->logical_index[lseg_start+i], new_pseg_list[i]);
	}

	table_print( table );

	RELEASE_LOCK();

	return SUCCESS;
}

// lseg_start 부터 lseg_count 만큼을 new_pseg_list로 덮어 쓴다.
static int __realign_logical_segment(
		ifs_t* ifs, int lseg_start, int lseg_count, int* new_pseg_list, int new_pseg_count)
{
	int i;
	table_t *table = &ifs->shared->mapping_table;

	ACQUIRE_LOCK();

	// 중간에 에러나면 망했다고 보면 된다.
	if ( new_pseg_count != lseg_count ) {
		crit("new_pseg_count[%d] != lseg_count[%d]", new_pseg_count, lseg_count);

		if ( table_move_logical_segment( table, lseg_start+lseg_count, lseg_start+new_pseg_count ) != SUCCESS )
			crit("move segment failed. start[%d], dest[%d]. ifs crashed",
					lseg_start+lseg_count, lseg_start+new_pseg_count);
	}

	for ( i = 0; i < lseg_count; i++ ) {
		info("overwrite table lseg[%d], pseg[%d] -> pseg[%d]",
				lseg_start+i, table->logical_index[lseg_start+i], new_pseg_list[i]);

		table_overwrite_logical_segment( table, lseg_start+i, new_pseg_list[i] );
	}

	RELEASE_LOCK();

	return SUCCESS;
}

static int __clear_defragment_sfs(table_t* table)
{
	int sec = 0;
	int seg = 0;

	for(sec = 0; sec < MAX_FILE_COUNT; sec++) {
		for(seg = 0; seg < table->segment_count_in_sector; seg++) {
			if(table->physical_sector[sec].segment[seg] == DEFRAGMENT) 
				table->physical_sector[sec].segment[seg] = EMPTY;
		}
	}

	return SUCCESS;
}

static int __allocate_sfs(ifs_t* ifs, int* free_segment, int state, int type)
{
	ACQUIRE_LOCK();
    if(table_allocate(&ifs->shared->mapping_table, free_segment, state) == FAIL) {
        error("can not allocation fail, state[%d]", state);
        release_lock(ifs->local.lock);
        return FAIL;
    }
	RELEASE_LOCK();

    if(__sfs_activate(ifs, *free_segment, type, 1, OPTION) == FAIL) {
        error("can not sfs activate, physical_segment[%d], OPTION[%d], type[%d], is_format[%d]",
            *free_segment, OPTION, type, 0);
        return FAIL;
    }
	debug("allocated physical segment: %d", *free_segment);

	return SUCCESS;
}	

/**********************************************************
 * sfs_id의 file_id를 새로운 segment에 기록한다.
 * 필요할 때마다 새로운 segment를 가져와서 new_pseg_list 에 저장한다.
 * 보통은 new_pseg_count가 2가 되기를 기대하지만
 * 실제로는 3이 되는 경우도 많은 것 같다.
 **********************************************************/
static int __sfs_read_append(
		ifs_t* ifs, int file_id, int sfs_id, int *new_pseg_list, int *new_pseg_count)
{
	int read_byte;
	int write_byte, write_byte2;
	int ret = 0;
	int offset = 0;
	int append_segment;

	if ( *new_pseg_count <= 0 ) {
		ret = __allocate_sfs(ifs, &new_pseg_list[0], DEFRAGMENT, O_FILE );
		if ( ret == FAIL )
		{
			error("cannot allocate new segment");
			return FAIL;
		}

		append_segment = new_pseg_list[0];
		*new_pseg_count = 1;
	}

	while(1)
	{
		read_byte = sfs_read(ifs->local.sfs[sfs_id], file_id, offset, buffer_size, buffer);
		if ( read_byte == FAIL )
		{
			error("cannot read file, fild_id = %d", file_id);
			return FAIL;
		}
		else if ( read_byte == FILE_NOT_EXISTS )
		{
			error("file not exists, file_id = %d", file_id);
			return FAIL;
		}
		else if ( read_byte == 0 ) break;

		write_byte = sfs_append(ifs->local.sfs[append_segment], file_id, read_byte, buffer);
		if ( write_byte < 0 )
		{
			error("cannot write file, fild_id = %d", file_id);
			return FAIL;
		}

		// 현재 segment 가 다 찼으면 다음 segment로 넘어간다.
		if ( read_byte > write_byte )
		{
			ret = __sfs_deactivate(ifs, append_segment);
			if ( ret == FAIL )
			{
				error("cannot destroy append_segment[%d]", append_segment);
				return FAIL;
			}

			if ( *new_pseg_count >= MAX_PSEG_LIST )
			{
				error("too many segment ( >= MAX_PSEG_LIST[%d] )", MAX_PSEG_LIST);
				return FAIL;
			}

			ret = __allocate_sfs(ifs, &append_segment, DEFRAGMENT, O_FILE); 
			if ( ret != SUCCESS )
			{
				error("cannot allocate new segment");
				return FAIL;
			}
			new_pseg_list[*new_pseg_count] = append_segment;
			*new_pseg_count = *new_pseg_count + 1;
			
			write_byte2 = sfs_append(
					ifs->local.sfs[append_segment], file_id, read_byte-write_byte, buffer+write_byte);
			if ( write_byte2 < 0 )
			{
				error("cannot write file, fild_id = %d", file_id);
				return FAIL;
			}
			else write_byte += write_byte2;
		}
		
		sb_assert( read_byte == write_byte );
		offset += read_byte;

		if ( read_byte != buffer_size ) break;
	}

	return SUCCESS;
}

// 리턴값은 REALIGN_TRUE, REALIGN_FALSE. 실패는 FAIL
#define REALIGN_TRUE  (1)
#define REALIGN_FALSE (0)
static int __try_realign(ifs_t* ifs, int *pseg_array, int lseg_start, int count)
{
	int pseg_array_copy[MAX_PSEG_LIST];
	sfs_info_t pinfo_array[MAX_PSEG_LIST];
	int copy_count = 0, copy_last = count - 1;
	int i, j;
	int tmp; sfs_info_t tmp_info;

	// pseg_array에서 NOT_USE 는 뒤로 빼고 진짜만 복사해 놓는다.
	for ( i = 0; i < count; i++ ) {
		debug("input pseg[%d]", pseg_array[i]);

		if ( pseg_array[i] >= 0 ) {
			pseg_array_copy[copy_count++] = pseg_array[i];
			sfs_get_info( ifs->local.sfs[pseg_array[i]], &pinfo_array[i], 0 );
			// 에러체크 하지 말자
		}
		else
			pseg_array_copy[copy_last--] = NOT_USE;
	}

	if ( copy_count <= 1 ) // 1이면 벌써 끝났다.
		goto realign_segment;

	// info의 min_file_id로 sort
	// 인접한 segment 끼리만 swap 하도록 주의한다.(wiki 참고..?)
	for( j = 0; j < copy_count - 1; j++ ) {
		for ( i = copy_count - 1; i > j; i -- ) {
			if ( pinfo_array[i-1].min_file_id <= pinfo_array[i].max_file_id )
				continue;

			tmp = pseg_array_copy[i-1];
			pseg_array_copy[i-1] = pseg_array_copy[i];
			pseg_array_copy[i] = tmp;

			tmp_info = pinfo_array[i-1];
			pinfo_array[i-1] = pinfo_array[i];
			pinfo_array[i] = tmp_info;
		} // for (i)
	} // for (j)

	// file id 순서대로 잘 정렬되어 있으면 OK
	for ( i = 0; i < copy_count - 1; i++ ) {
		debug("pseg[%d]: min[%d], max[%d] - pseg[%d]: min[%d], max[%d]",
			   pseg_array_copy[i], pinfo_array[i].min_file_id, pinfo_array[i].max_file_id,
		       pseg_array_copy[i+1], pinfo_array[i+1].min_file_id, pinfo_array[i+1].max_file_id);

		if ( pinfo_array[i].max_file_id <= pinfo_array[i+1].min_file_id )
			continue;

		return REALIGN_FALSE;
	}

realign_segment:
	// pseg_array, pseg_array_copy가 같은 내용이면 ....
	for ( i = 0; i < count; i++ ) {
		if ( pseg_array[i] == pseg_array_copy[i] ) continue;

		if ( __realign_logical_segment(
					ifs, lseg_start, count, pseg_array_copy, copy_count ) != SUCCESS )
			return FAIL;

		return REALIGN_TRUE;
	}

	debug("nothing to do. lseg_start[%d], count[%d]", lseg_start, count);
	return REALIGN_TRUE;
}

/*******************************************************************
 * pseg_file_array와 pseg_file_array_index 를 참고해서
 * 지금 어떤 파일을 기록할 차례인지 찾아낸다.
 *
 * pseg [OUT] : 파일을 읽어야 할 physical segment
 * pseg_file [OUT] : pseg에서 읽어올 file
 *
 * pseg_array [IN] : 파일을 읽을 수 있는 physical segment들
 * count [IN] : physical segment 수
 * pseg_info_array : 각 pseg_array[] 들의 info
 * pseg_file_array : 각 pseg_array[]가 가지고 있는 file id array
 *                   [pseg index][file id]
 * pseg_file_array_index : pseg_file_array의 현재위치.
 *                         작업대상으로 걸리면 +1 된다.
 *******************************************************************/
static void __get_next_job( int* pseg, int* pseg_file, int* pseg_array, int count,
			sfs_info_t* pseg_info_array, int** pseg_file_array, int* pseg_file_array_index )
{
	int i;
	int current_file, current_file_index, pseg_index;

	*pseg = -1;
	*pseg_file = -1;
	pseg_index = -1;

	for ( i = 0; i < count; i++ ) {
		// NOT_USE 등...
		if ( pseg_array[i] < 0 ) continue;

		current_file_index = pseg_file_array_index[i];

		if ( current_file_index >= pseg_info_array[i].file_count )
			continue;

		current_file = pseg_file_array[i][current_file_index];

		if ( pseg_index == -1 || current_file < *pseg_file ) {
			*pseg_file = current_file;
			*pseg = pseg_array[i];
			pseg_index = i;
		}
	}

	if ( pseg_index >= 0 )
		pseg_file_array_index[pseg_index]++;
}

/*********************************************************************
 * pseg_array : pseg_array[0] -> lseg_start ... 이렇게 대응된다
 * lseg_start : 작업대상 logical segment. 여기부터 count 갯수까지...
 * count      : 작업대상 수
 *
 * return value
 *  defrag 결과로 생긴 segment 수. 보통은 count와 같기를 기대하지만
 *  아닐 수도 있다.
 *********************************************************************/
static int __defragment(ifs_t* ifs, int* pseg_array, int lseg_start, int count)
{
	int pseg_file, pseg;
	int i;

	// defrag 이전의 source segment들
	sfs_info_t pseg_info_array[MAX_PSEG_LIST];       // segment 전체
	int*       pseg_file_array[MAX_PSEG_LIST];       // segment 전체
	int        pseg_file_array_index[MAX_PSEG_LIST]; // 현재 처리중인 file

	// defrag 결과로 생긴 새 segment들
	int new_pseg_list[MAX_PSEG_LIST]; // defrag 결과로 생긴 새 segment들
	int new_pseg_count = 0;

	if ( __sfs_all_activate(ifs, pseg_array, count, O_FILE ) != SUCCESS )
		return FAIL;

	debug("defragment lseg[%d~%d]", lseg_start, lseg_start+count-1);

	// 운 좋으면 segment 재배치 만으로 일이 끝날 수도 있다.
	i = __try_realign( ifs, pseg_array, lseg_start, count );
	if ( i == REALIGN_TRUE ) return count;
	else if ( i != REALIGN_FALSE ) return FAIL;

	info("defragment lseg[%d~%d]", lseg_start, lseg_start+count-1);

	// 각 segment의 file array 가져오기
	memset( pseg_file_array, 0, sizeof(int*) * MAX_PSEG_LIST );
	for ( i = 0; i < count; i++ ) {
		pseg = pseg_array[i];
		if ( pseg < 0 ) continue;

		if ( sfs_get_info( ifs->local.sfs[pseg], &pseg_info_array[i], 0 ) != SUCCESS ) {
			error("sfs_get_info failed. sfs[pseg:%d]", pseg);
			goto return_fail;
		}

		pseg_file_array[i] = (int*) sb_malloc( sizeof(int) * pseg_info_array[i].file_count );
		if ( pseg_file_array[i] == NULL ) {
			error("sb_malloc failed: %s", strerror(errno));
			goto return_fail;
		}

		if ( sfs_get_file_array( ifs->local.sfs[pseg], pseg_file_array[i] ) != SUCCESS ) {
			error("sfs_get_file_array[pseg:%d] failed", pseg);
			goto return_fail;
		}
	}

	memset( pseg_file_array_index, 0, sizeof(int) * MAX_PSEG_LIST );

	while(1)
	{
		__get_next_job( &pseg, &pseg_file, pseg_array, count,
					pseg_info_array, pseg_file_array, pseg_file_array_index );

		if ( pseg_file <= 0 ) break;

		if (  __sfs_read_append(ifs, pseg_file, pseg, new_pseg_list, &new_pseg_count) == FAIL )
		{		
			error("cannot append file, fild_id = %d", pseg_file);
			goto return_fail;
		}
	} // while(1)

	if ( __update_logical_segment(ifs, lseg_start, count, new_pseg_list, new_pseg_count) == FAIL ) {
		error("failed to modify table lseg[%d~%d]", lseg_start, lseg_start+count-1);
		goto return_fail;
	}

	for ( i = 0; i < count; i++ ) {
		if ( pseg_file_array[i] != NULL ) sb_free( pseg_file_array[i] );
	}
	return new_pseg_count;

return_fail:

	for ( i = 0; i < count; i++ ) {
		if ( pseg_file_array[i] != NULL ) sb_free( pseg_file_array[i] );
	}
	return FAIL;

}

/////////////////////////////// bubble defrag /////////////////////////////
// 앞으로 남은 defrag가 몇 번 인지 계산한다
// t, start_segment, end_segment 는 현재 진행중인 segment 이다.
// 앞의 세 값을 보고 현재 여기까지 진행했다는 것을 알고 나머지를 구한다
int calculate_remains(int t, int group_size,
		int start_segment, int end_segment, int segment_count, int lseg1)
{
	int remain_step = 0;

	// defrag를 몇번 해야되는지 계산
	// ifs_defrag() 에서 for-loop 변수들을 몽땅 들고 왔다
	// 현재 값들에서부터 끝까지 시뮬레이션 해 보면 남은 횟수를 알 수 있다.
	for( ; t < segment_count; ) {
		for ( ; start_segment < end_segment; start_segment++ ) {
			remain_step += ( lseg1 - start_segment + 1 );
			lseg1 = end_segment - 1;
		}

		t = end_segment + 1;

		start_segment = t;
		end_segment = t + (group_size-1);

		if ( end_segment >= segment_count )
			end_segment = segment_count - 1;

		lseg1 = end_segment - 1;
	}

	return remain_step;
}

/***************************************************************
 * 2개씩 짝을 지어 실행하는 defrag.
 * space가 적게 들어가지만 시간이 많이 걸린다
 *
 * RETURN : defrag 완료된 마지막 logical segment
 ***************************************************************/
static int bubble_defrag(ifs_t* ifs, int t, int start_segment, int end_segment,
		int* pseg_array, int segment_count, scoreboard_t* scoreboard)
{
	int lseg_start;
	int pseg1, pseg2;
	int ret;

	static int total_step;
	static int current_step; // 진행상황 표시에 필요하다

	// 지금이 처음이면 초기화
	if ( start_segment == 0 ) {
		total_step = 0;
		current_step = 0;
	}

	for ( ; start_segment < end_segment; start_segment++ )
	{
		for ( lseg_start = end_segment - 1; lseg_start >= start_segment; lseg_start-- )
		{
			total_step = current_step
					+ calculate_remains(t, defrag_group_size, start_segment, end_segment,
							segment_count, lseg_start);
			current_step++;

			setproctitle("softbotd: ifs bubble defrag[%s] %d/%d",
					ifs->shared->root_path, current_step, total_step);
			debug("softbotd: ifs bubble defrag[%s] %d/%d",
					ifs->shared->root_path, current_step, total_step);

			pseg1 = pseg_array[lseg_start];
			pseg2 = pseg_array[lseg_start+1];

			ret = __defragment(ifs, &pseg_array[lseg_start], lseg_start, 2);

			if (ret < 0)
			{
				error("Defragment between segment[l:%d, p:%d] and segment[l:%d, p:%d] is fail", 
						lseg_start, pseg1, lseg_start+1, pseg2);
				return FAIL;
			}
			if ( ret != 2 ) {
				crit("segment count changed 2->%d, original is [%d, phy:%d], [%d, phy:%d]",
						ret, lseg_start, pseg1, lseg_start+1, pseg2);
			}

			// defrag 결과에 따라 segment가 늘어나면 group도 바뀐다
			end_segment = end_segment + ( ret-2 );

			if ( scoreboard
					&& ( scoreboard->shutdown || scoreboard->graceful_shutdown ) ) break;
		} // for lseg_start

		if ( scoreboard
				&& ( scoreboard->shutdown || scoreboard->graceful_shutdown ) ) break;
	} // for start_segment

	return end_segment;
}

/***************************************************************
 * source 와 target의 크기를 같이 잡아서 한꺼번에 복사
 * 시간은 짧게 걸리지만 사용 용량이 많다.
 *
 * RETURN : defrag 완료된 마지막 logical segment
 ***************************************************************/
static int copy_defrag(ifs_t* ifs, int t, int start_segment, int end_segment,
		int* pseg_array, int segment_count, scoreboard_t* scoreboard)
{
	int defrag_segment_count;
	int ret;

	static int total_step;
	static int current_step; // 진행상황 표시에 필요하다

	// 지금이 처음이면 초기화
	if ( start_segment == 0 ) {
		total_step = 0;
		current_step = 0;
	}

	setproctitle("softbotd: ifs copy defrag[%s] %d/%d", ifs->shared->root_path, t, segment_count);
	debug("softbotd: ifs copy defrag[%s] %d/%d", ifs->shared->root_path, t, segment_count);

	defrag_segment_count = end_segment - start_segment + 1;

	ret = __defragment(ifs, &pseg_array[start_segment], start_segment, defrag_segment_count);

	if (ret < 0)
	{
		error("Defragment lseg[%d~%d] failed", start_segment, end_segment);
		return FAIL;
	}
	if ( ret != defrag_segment_count ) {
		crit("defrag [%d~%d] segment count changed %d->%d",
				start_segment, end_segment, defrag_segment_count, ret);
	}

	// defrag 결과에 따라 segment가 늘어나면 group도 바뀐다
	end_segment = end_segment + ( ret-defrag_segment_count );

	return end_segment;
}

/********************************************************************************** 
 * scoreboard : NULL 이면 사용하지 않는다.
 **********************************************************************************/
int ifs_defrag(ifs_t* ifs, scoreboard_t* scoreboard)
{
	int pseg_array[MAX_LOGICAL_COUNT]; // array[logical] = physical
	int segment_count = 0; // 전체 logical segment수. 위 array count이기도 하다.

	int t, start_segment, end_segment;

	int ret = 0;

	int lockfd;
	char lockpath[MAX_PATH_LEN];

	snprintf(lockpath, sizeof(lockpath), "%s/defrag.lock", ifs->shared->root_path);
	lockfd = sb_lockfile( lockpath );
	if ( lockfd < 0 ) {
		error("another process is defragging");
		return FAIL;
	}

    ret = __clear_defragment_sfs(&ifs->shared->mapping_table);
	if(ret == FAIL) {
		error("clear_defragment_sfs fail");
		sb_unlockfile( lockfd );
		return FAIL;
	}	

	// segment_count 가 필요하다
	segment_count = table_get_read_segments(&ifs->shared->mapping_table, pseg_array);
	
	if (pseg_array[segment_count - 1] == ifs->shared->append_segment)
		segment_count--;

	table_print(&ifs->shared->mapping_table);

	if ( buffer == NULL ) {
		buffer_size = 1*1024*1024;
		buffer = (char *)sb_realloc(buffer, buffer_size);
	}

	if ( buffer == NULL ) {
		error("memory allocation fail, size[%d]", buffer_size);
		sb_unlockfile( lockfd );
		return FAIL;
	}	

	for( t = 0; t < segment_count ; )
	{
		start_segment = t;
		end_segment = t + (defrag_group_size-1);

		if ( end_segment >= segment_count )
			end_segment = segment_count - 1;

		if ( defrag_mode == DEFRAG_MODE_BUBBLE ) {
			end_segment = bubble_defrag( ifs, t, start_segment, end_segment,
					pseg_array, segment_count, scoreboard );
		}
		else if ( defrag_mode == DEFRAG_MODE_COPY ) {
			end_segment = copy_defrag( ifs, t, start_segment, end_segment,
					pseg_array, segment_count, scoreboard );
		}
		else {
			warn("unknown defrag_mode:%d", (int) defrag_mode);
			sb_unlockfile( lockfd );
			return FAIL;
		}

		if ( end_segment < 0 ) {
			error("defrag group (start from %d) failed", start_segment);
			__clear_defragment_sfs( &ifs->shared->mapping_table );
			sb_unlockfile( lockfd );
			return FAIL;
		}

		t = end_segment + 1;

		if ( scoreboard
				&& ( scoreboard->shutdown || scoreboard->graceful_shutdown ) ) break;

		segment_count = table_get_read_segments(&ifs->shared->mapping_table, pseg_array);
			
		if (pseg_array[segment_count - 1] == ifs->shared->append_segment)
			segment_count--;

	} // for t

	sb_free( buffer );
	buffer = NULL;

	sb_unlockfile( lockfd );
	return SUCCESS;
}

/***********************************************************************
                               module stuff
 ***********************************************************************/

#define MONITORING_PERIOD (2)

// defrag는 항상 하나만?
static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(2) };

static void _do_nothing(int sig)
{
    return;
}

static void _shutdown(int sig)
{
    struct sigaction act;

    memset(&act, 0x00, sizeof(act));

//    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    act.sa_handler = _do_nothing;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    scoreboard->shutdown++;
}

static void _graceful_shutdown(int sig)
{
    struct sigaction act;

    memset(&act, 0x00, sizeof(act));

//    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    act.sa_handler = _do_nothing;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    scoreboard->graceful_shutdown++;
}

static int child_main(slot_t *slot)
{
	index_db_t* ifs;
	int delay;
	time_t before, after;
	char* defrag_mode_string;

	if ( defrag_mode == DEFRAG_MODE_BUBBLE ) defrag_mode_string = "bubble";
	else if ( defrag_mode == DEFRAG_MODE_COPY ) defrag_mode_string = "copy";
	else defrag_mode_string = "unknown";

	if ( ifs_open( &ifs, indexdb_set ) != SUCCESS ) {
		error("ifs_open failed.");
		return -1;
	}

	while (1) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

		notice("defrag start...");
		setproctitle("softbotd: ifs %s defrag start", defrag_mode_string);

		ifs_defrag((ifs_t*)ifs->db, scoreboard);
		notice("defrag end.");
		
		delay = defrag_delay;
		while( delay > 0 ) {
			setproctitle("softbotd: ifs %s defrag end. sleep %d sec",
					defrag_mode_string, delay);
			time( &before );
			sleep( 1 );
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
			time( &after );

			delay -= ( after - before );
		}
	}

	ifs_close( ifs );

	return EXIT_SUCCESS;
}

static int module_main(slot_t *slot)
{
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	setproctitle("softbotd: mod_ifs_defrag.c");
	scoreboard->size = 1;

	sb_run_init_scoreboard(scoreboard);
	sb_run_spawn_processes(scoreboard, "ifs defrag process", child_main);

	scoreboard->period = MONITORING_PERIOD;
	sb_run_monitor_processes(scoreboard);

	return 0;
}

/************** config functions ************/
static void set_indexdb_set(configValue v)
{
	indexdb_set = atoi( v.argument[0] );
}

static void set_defrag_group_size(configValue v)
{
	defrag_group_size = atoi(v.argument[0]);
	if ( defrag_group_size > MAX_PSEG_LIST ) {
		warn( "DefragGroupSize[%d] is larger than MAX_PSEG_LIST[%d]",
				defrag_group_size, MAX_PSEG_LIST );
		defrag_group_size = MAX_PSEG_LIST;
	}
}

static void set_defrag_delay(configValue v)
{
	defrag_delay = atoi(v.argument[0]);
}

static void set_defrag_mode(configValue v)
{
	if ( strncasecmp( v.argument[0], "bubble", 7 ) == 0 )
		defrag_mode = DEFRAG_MODE_BUBBLE;
	else if ( strncasecmp( v.argument[0], "copy", 5 ) == 0 )
		defrag_mode = DEFRAG_MODE_COPY;
	else warn("unknown DefragMode [%s]", v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("IndexDbSet", set_indexdb_set, 1, "e.g> IndexDbSet 1"),
	CONFIG_GET("DefragGroupSize", set_defrag_group_size, 1, "defrag segment group size"),
	CONFIG_GET("DefragDelay", set_defrag_delay, 1, "sleep time between defrags (seconds)"),
	CONFIG_GET("DefragMode", set_defrag_mode, 1, "defrag mode [bubble|copy]"),
	{NULL}
};

module ifs_defrag_module = {
	STANDARD_MODULE_STUFF,
	config,                  /* config */
	NULL,                    /* registry */
	NULL,                    /* initialize */
	module_main,             /* child main */
	scoreboard,              /* scoreboard */
	NULL,                    /* register hook api */
};

