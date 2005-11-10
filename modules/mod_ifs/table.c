/* $Id$ */
#include "table.h"

int temp_alive_time = 60*1; // 1분, <= 0 이면 바로 삭제된다

/***********************************************
 * 단어 간단 설명
 * p: physical sector number
 * l: logical index number
 ***********************************************/

static int table_get_index(segment_info_t* seg, int count)
{
	return (seg->sector*count) + seg->segment;
}

void table_get_segment_info(int index, int count, segment_info_t* seg)
{
	seg->sector = index / count;
	seg->segment = index % count;

	return;
}

static int table_update_physical_segment(table_t* table, int p, int state)
{
	segment_info_t p_info;
	table_get_segment_info(p, table->segment_count_in_sector, &p_info);

	if ( p < 0 ) {
		error("invalid physical segment[%d]", p);
		return FAIL;
	}

	if ( p_info.segment >= MAX_SEGMENT_COUNT || p_info.sector >= MAX_SECTOR_COUNT ) {
		error("invalid segment_info[segment:%d, sector%d]", p_info.segment, p_info.sector);
		return FAIL;
	}

	table->physical_sector[p_info.sector].segment[p_info.segment] = state;
	gettimeofday(&table->physical_sector[p_info.sector].modify[p_info.segment], NULL);

	return SUCCESS;
}

static int table_get_physical_segment_state(table_t* table, int p)
{
	segment_info_t p_info;
	table_get_segment_info(p, table->segment_count_in_sector, &p_info);

	return table->physical_sector[p_info.sector].segment[p_info.segment];
}

int table_init(table_t* table, int count)
{
	int sec = 0;

	table->segment_count_in_sector = count;
	table->allocated_physical_sector = 0;
	table->allocated_physical_segment = 0;
	table->version = 0;

	memset(table->logical_index, EMPTY, sizeof(int)*MAX_LOGICAL_COUNT);
	for(sec = 0; sec < MAX_SECTOR_COUNT; sec++) {
		memset(table->physical_sector[sec].segment, EMPTY, sizeof(int)*MAX_SEGMENT_COUNT);
	}

	debug("table->segment_count_in_sector[%d]", table->segment_count_in_sector);

	return SUCCESS;
}

// p: 할당된 physical segment
// state: 새로 할당한 segment에 주는 state
int table_allocate(table_t* table, int* p, int state)
{
	int sec = 0;
	int seg = 0;
	segment_info_t info;

	table_clear_tmp_segment( table, TEMP );

	for(sec = 0; sec < MAX_SECTOR_COUNT; sec++) {
		for(seg = 0; seg < table->segment_count_in_sector; seg++) {
			if(table->physical_sector[sec].segment[seg] != EMPTY) continue;

			info.sector = sec;
			info.segment = seg;
			*p = table_get_index(&info, table->segment_count_in_sector);
			if(table->allocated_physical_sector < sec) {
				table->allocated_physical_sector = sec;
			}

			if(table->allocated_physical_segment < *p) {
				table->allocated_physical_segment = *p;
			}

			/* 상태 변경 */
			table_update_physical_segment(table, *p, state);

			debug("find empty segment, sector[%d], segment[%d], p[%d], state[%d]", sec, seg, *p, state);

			return SUCCESS;
		}
	}

	error("cannot find empty segment. table_allocate(,,%d) failed", state);
	return FAIL;
}

/***************************************************************************
 * p : logical table 마지막에 붙이려고 하는 physical segment
 *
 * logical table의 마지막에 붙어있는 append segment를 p로 교체한다.
 * append segment의 내용이 p에 복사되어 있는 상태이며
 * 앞으로는 p를 사용하도록 하려고 한다.
 * append segment는 곧 format 된다.
 *
 * ** 현재 logical table에는 1개 이상의 segment가 이미 있어야 한다.
 ***************************************************************************/
int table_update_last_logical_segment(table_t* table, int p)
{
	int i;

	for( i = 0; ; i++ ) {
		if ( table->logical_index[i] != EMPTY && i < MAX_LOGICAL_COUNT) continue;
		sb_assert( i > 0 );

		table->logical_index[i-1] = p;
		table_update_physical_segment(table, p, ALLOCATED);
		table->version++;

		table_print(table);
		return SUCCESS;
	}

	error("you can't reach here");
	return FAIL;
}

/************************************************************
 * p : append segment. O_MMAP으로 열린 상태일 것이다.
 * logical table의 마지막에 physical segment p를 추가한다.
 ************************************************************/
int table_append_logical_segment(table_t* table, int p)
{
	int i;

	for ( i = 0; i < MAX_LOGICAL_COUNT; i++ ) {
		if ( table->logical_index[i] != EMPTY ) continue;

		table->logical_index[i] = p;
		table_update_physical_segment(table, p, ALLOCATED);

		table_print(table);
		return SUCCESS;
	}

	error("there is no EMPTY logical segment. MAX_LOGICAL_COUNT[%d]",
			MAX_LOGICAL_COUNT);
	return FAIL;
}

int table_update_logical_segment(table_t* table, int l, int p)
{
	int old_p = table->logical_index[l];

	// old_p는 EMPTY, NOT_USE인경우가 있다
	// segment는 당장 삭제하는 것이 아니고 일단 TEMP 상태가 된다.
	if ( old_p >= 0 
			&& table_update_physical_segment( table, old_p, TEMP ) == FAIL ) {
		error("table_update failed. p[%d] -> TEMP", old_p);
		return FAIL;
	}

	table_overwrite_logical_segment( table, l, p );

	// p가 NOT_USE인 경우도 있다
	if ( p >= 0
			&& table_update_physical_segment( table, p, ALLOCATED ) == FAIL ) {
		error("table_update failed. l[%d], p[%d] -> ALLOCATED", l, p);
		return FAIL;
	}

	//TempAliveTime을 믿는다면 version을 변경할 필요가 없다.
	//table->version++;

	//table_print(table);

	return SUCCESS;
}

// table_update_logical_segment와 비슷한 일을 하지만 
// pseg나 원래 그 자리에 있던 physical segment의 상태를 바꾸지 않는다.
// 보통 logical segment 들의 순서를 바꾸거나 할 때
// physical segment의 상태를 바꿀 필요가 없을 때 쓴다.
void table_overwrite_logical_segment(table_t* table, int l, int p)
{
	table->logical_index[l] = p;
}

/*****************************************************************
 * lstart 부터 마지막까지 logical table 에 있는 내용을
 * ldest 로 옮긴다.
 *
 * 옮기고 나서 빈자리는 뒷부분이면 EMPTY로 채우고
 * 앞이면 NOT_USE로 채운다
 *****************************************************************/
int table_move_logical_segment(table_t* table, int lstart, int ldest)
{
	int lcount, i;

	if ( lstart == ldest ) return SUCCESS;

	for ( lcount = lstart+1; lcount < MAX_LOGICAL_COUNT; lcount++ )
		if ( table->logical_index[lcount] == EMPTY ) break;

	// logical segment list 가 앞으로 당겨질 경우
	// overwrite 되는 영역의 physical segment 들의 state 를 TEMP 로 수정해야 한다
	// Bug20051110-1 을 확인한다.
	if ( lstart > ldest ) {
		for ( i = ldest; i < lstart; i++ ) {
			int p = table->logical_index[i];
			if ( p < 0 ) continue;

			int state = table_get_physical_segment_state(table, p);
			if ( state != ALLOCATED ) {
				warn("physical segment[%d] is in wrong state[%d], expected[%d]",
						p, state, ALLOCATED);
			}

			if ( table_update_physical_segment(table, p, TEMP) != SUCCESS ) {
				error("updating physical segment[%d] state failed. (to TEMP)", p);
				return FAIL;
			}
		}
	}
	
	memmove( &table->logical_index[ldest],
				&table->logical_index[lstart], lcount*sizeof(int) );
	if ( lstart < ldest ) {
		for ( i = lstart; i < ldest; i++ )
			table->logical_index[i] = NOT_USE;
	}
	else { // lstart > ldest
		for ( i = ldest+lcount; i < lstart+lcount; i++ )
			table->logical_index[i] = EMPTY;
	}

	return SUCCESS;
}

// arr_read_seg의 크기는 무조건 MAX_LOGICAL_COUNT이다.
// return 값에는 NOT_USE segment도 포함되어 있다.
// logical index를 제대로 맞추려면 어쩔 수 없다.
int table_get_read_segments(table_t* table, int* arr_read_seg)
{
	int i = 0;

	memcpy( arr_read_seg, table->logical_index, sizeof(int)*MAX_LOGICAL_COUNT );

	for ( i = 0; i < MAX_LOGICAL_COUNT; i++ ) {
		if ( table->logical_index[i] == EMPTY ) break;
	}

	return i;
}

int table_clear_tmp_segment(table_t* table, int state)
{
	segment_info_t p_info;
	int p;
	struct timeval now;
	double diff = 0;

	if( state == ALLOCATED ) {
		error("can not clear allocated_segment");
		return FAIL;
	}

	gettimeofday(&now, NULL);

	for( p_info.sector = 0; p_info.sector < MAX_SECTOR_COUNT; p_info.sector++ ) {
		for( p_info.segment = 0; p_info.segment < table->segment_count_in_sector; p_info.segment++ ) {
			p = table_get_index( &p_info, table->segment_count_in_sector );
			if ( table_get_physical_segment_state( table, p ) != state ) continue;

			if( state == TEMP ) {
				diff = timediff( &now, &table->physical_sector[p_info.sector].modify[p_info.segment] );
				if( diff < temp_alive_time ) continue;
			}

			table_update_physical_segment( table, p, EMPTY );
		}
	}

	// version은 변경할 필요 없음

	table_print(table);

	return SUCCESS;
}

// physical segment 의 sector, segment 로 logical segment 번호를 찾는다.
// 없으면 -1
int table_get_logical_segment(table_t* table, int p1)
{
	int l, p2;

	for ( l = 0; l < MAX_LOGICAL_COUNT; l++ ) {
		p2 = table->logical_index[l];

		if ( p2 == p1 ) return l;
		else if ( p2 == EMPTY ) break;
	}

	return -1;
}

/******************************************************************************
 * Bug20051110-1 의 내용을 수정하기 위해 만든다.
 * physical segment 의 상태가 ALLOCATED 이지만 logical segment list 에 없으면
 * 해당 physical segment 를 TEMP 상태로 수정한다.
 *
 * mod_ifs.c 의 _ifs_fix_physical_segment_state() 에서 호출되고
 * ifs에 대한 lock을 걸고 들어온다.
 *
 * 성공하면 수정된 segment 수를 리턴한다 (>=0)
 * 실패는 FAIL (-1)
 ******************************************************************************/
int table_fix_physical_segment_state(table_t* table)
{
	int l, p, fix_count = 0;
	segment_info_t p_info;

	// logical segment list 에 연결되지 않은 alocated physical segment 를 찾는다.
	// 중대한 오류를 찾아내려고 하는 거다.
	for ( p_info.sector = 0; p_info.sector < MAX_SECTOR_COUNT; p_info.sector++ ) {
		for ( p_info.segment = 0; p_info.segment < table->segment_count_in_sector; p_info.segment++ ) {
			int state = table->physical_sector[p_info.sector].segment[p_info.segment];

			p = table_get_index(&p_info, table->segment_count_in_sector);
			l = table_get_logical_segment(table, p);

			// ALLOCATED 상태이면서 logical segment 에 없다는 것은 이상한 것이다.
			if ( state == ALLOCATED && l < 0 ) {
				warn("physical segment[%d] is not exists in logical segment list. fix it.", p);
				// 실패할 리가 없지만 실패하면 아주 치명적이다.
				if ( table_update_physical_segment(table, p, TEMP) != SUCCESS ) {
					error("update physical segment[%d] to TEMP failed", p);
					return FAIL;
				}
				fix_count++;
			}
			// ALLOCATED 가 아닌데 logical segment 에 있다는 것도 이상하다.
			if ( state != ALLOCATED && l >= 0 ) {
				warn("physical segment[%d] should be in ALLOCATED state", p);
				// 실패할 리가 없지만 실패하면 아주 치명적이다.
				if ( table_update_physical_segment(table, p, ALLOCATED) != SUCCESS ) {
					error("update physical segment[%d] to ALLOCATED failed", p);
					return FAIL;
				}
				fix_count++;
			}
		} // for segment
	} // for sector

	return fix_count;
}

void table_print(table_t* table)
{
	int p, l;
	int index_count = 0, defragment_count = 0, temp_count = 0;
	segment_info_t p_info;
	char str_state[5][16] = {"EMPTY", "INDEX", "ALLOCATED", "DEFRAGMENT", "TEMP"};

	debug("=============== MAPPING TABLE =======================");
	debug("\tsector\tsegment\tlogical\tphy idx\tphysical"); 
	for ( l = 0; l < MAX_LOGICAL_COUNT; l++ ) {
		p = table->logical_index[l];
		if ( p == EMPTY ) break;

		if ( p >= 0 ) {
			table_get_segment_info( p, table->segment_count_in_sector, &p_info );

			debug("\t%u\t%u\t%d\t%d\t%s",
					p_info.sector, p_info.segment, l, table->logical_index[l],
					str_state[table->physical_sector[p_info.sector].segment[p_info.segment]+1]);
		}
		else {
			debug("\t-1\t-1\t%d\t%d\tNOT_USE", l, table->logical_index[l]);
		}

	}
	debug("=============== MAPPING TABLE END ===================");

	/**********************************************************************
	 * table_print() 는 밖에서 lock 을 걸고 들어오나?
	 * 아니라면 여기서 보여주는 에러는 일시적인 걸 수도 있다
	 **********************************************************************/

	// logical segment list 에 연결되지 않은 alocated physical segment 를 찾는다.
	// 중대한 오류를 찾아내려고 하는 거다.
	for ( p_info.sector = 0; p_info.sector < MAX_SECTOR_COUNT; p_info.sector++ ) {
		for ( p_info.segment = 0; p_info.segment < table->segment_count_in_sector; p_info.segment++ ) {
			int state = table->physical_sector[p_info.sector].segment[p_info.segment];

			switch (state) {
				case INDEX: index_count++; break;
				case DEFRAGMENT: defragment_count++; break;
				case TEMP: temp_count++; break;
			}

			p = table_get_index(&p_info, table->segment_count_in_sector);
			l = table_get_logical_segment(table, p);

			// ALLOCATED 상태이면서 logical segment 에 없다는 것은 이상한 것이다.
			if ( state == ALLOCATED && l < 0 ) {
				warn("physical segment[%d, %s] is not exists in logical segment list", p, str_state[state+1]);
			}
			// ALLOCATED 가 아닌데 logical segment 에 있다는 것도 이상하다.
			if ( state != ALLOCATED && l >= 0 ) {
				warn("physical segment[%d, %s] should be in ALLOCATED state", p, str_state[state+1]);
			}
		} // for segment
	} // for sector

	if ( index_count > 0 )
		warn("physical segment in state[INDEX] : %d", index_count);
	if ( defragment_count > 0 )
		warn("physical segment in state[DEFRAGMENT] : %d", defragment_count);
	if ( temp_count > 0 )
		warn("physical segment in state[TEMP] : %d", temp_count);

	return;
}
