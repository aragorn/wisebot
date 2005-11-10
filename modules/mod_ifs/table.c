/* $Id$ */
#include "table.h"

int temp_alive_time = 60*1; // 1��, <= 0 �̸� �ٷ� �����ȴ�

/***********************************************
 * �ܾ� ���� ����
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

// p: �Ҵ�� physical segment
// state: ���� �Ҵ��� segment�� �ִ� state
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

			/* ���� ���� */
			table_update_physical_segment(table, *p, state);

			debug("find empty segment, sector[%d], segment[%d], p[%d], state[%d]", sec, seg, *p, state);

			return SUCCESS;
		}
	}

	error("cannot find empty segment. table_allocate(,,%d) failed", state);
	return FAIL;
}

/***************************************************************************
 * p : logical table �������� ���̷��� �ϴ� physical segment
 *
 * logical table�� �������� �پ��ִ� append segment�� p�� ��ü�Ѵ�.
 * append segment�� ������ p�� ����Ǿ� �ִ� �����̸�
 * �����δ� p�� ����ϵ��� �Ϸ��� �Ѵ�.
 * append segment�� �� format �ȴ�.
 *
 * ** ���� logical table���� 1�� �̻��� segment�� �̹� �־�� �Ѵ�.
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
 * p : append segment. O_MMAP���� ���� ������ ���̴�.
 * logical table�� �������� physical segment p�� �߰��Ѵ�.
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

	// old_p�� EMPTY, NOT_USE�ΰ�찡 �ִ�
	// segment�� ���� �����ϴ� ���� �ƴϰ� �ϴ� TEMP ���°� �ȴ�.
	if ( old_p >= 0 
			&& table_update_physical_segment( table, old_p, TEMP ) == FAIL ) {
		error("table_update failed. p[%d] -> TEMP", old_p);
		return FAIL;
	}

	table_overwrite_logical_segment( table, l, p );

	// p�� NOT_USE�� ��쵵 �ִ�
	if ( p >= 0
			&& table_update_physical_segment( table, p, ALLOCATED ) == FAIL ) {
		error("table_update failed. l[%d], p[%d] -> ALLOCATED", l, p);
		return FAIL;
	}

	//TempAliveTime�� �ϴ´ٸ� version�� ������ �ʿ䰡 ����.
	//table->version++;

	//table_print(table);

	return SUCCESS;
}

// table_update_logical_segment�� ����� ���� ������ 
// pseg�� ���� �� �ڸ��� �ִ� physical segment�� ���¸� �ٲ��� �ʴ´�.
// ���� logical segment ���� ������ �ٲٰų� �� ��
// physical segment�� ���¸� �ٲ� �ʿ䰡 ���� �� ����.
void table_overwrite_logical_segment(table_t* table, int l, int p)
{
	table->logical_index[l] = p;
}

/*****************************************************************
 * lstart ���� ���������� logical table �� �ִ� ������
 * ldest �� �ű��.
 *
 * �ű�� ���� ���ڸ��� �޺κ��̸� EMPTY�� ä���
 * ���̸� NOT_USE�� ä���
 *****************************************************************/
int table_move_logical_segment(table_t* table, int lstart, int ldest)
{
	int lcount, i;

	if ( lstart == ldest ) return SUCCESS;

	for ( lcount = lstart+1; lcount < MAX_LOGICAL_COUNT; lcount++ )
		if ( table->logical_index[lcount] == EMPTY ) break;

	// logical segment list �� ������ ����� ���
	// overwrite �Ǵ� ������ physical segment ���� state �� TEMP �� �����ؾ� �Ѵ�
	// Bug20051110-1 �� Ȯ���Ѵ�.
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

// arr_read_seg�� ũ��� ������ MAX_LOGICAL_COUNT�̴�.
// return ������ NOT_USE segment�� ���ԵǾ� �ִ�.
// logical index�� ����� ���߷��� ��¿ �� ����.
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

	// version�� ������ �ʿ� ����

	table_print(table);

	return SUCCESS;
}

// physical segment �� sector, segment �� logical segment ��ȣ�� ã�´�.
// ������ -1
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
 * Bug20051110-1 �� ������ �����ϱ� ���� �����.
 * physical segment �� ���°� ALLOCATED ������ logical segment list �� ������
 * �ش� physical segment �� TEMP ���·� �����Ѵ�.
 *
 * mod_ifs.c �� _ifs_fix_physical_segment_state() ���� ȣ��ǰ�
 * ifs�� ���� lock�� �ɰ� ���´�.
 *
 * �����ϸ� ������ segment ���� �����Ѵ� (>=0)
 * ���д� FAIL (-1)
 ******************************************************************************/
int table_fix_physical_segment_state(table_t* table)
{
	int l, p, fix_count = 0;
	segment_info_t p_info;

	// logical segment list �� ������� ���� alocated physical segment �� ã�´�.
	// �ߴ��� ������ ã�Ƴ����� �ϴ� �Ŵ�.
	for ( p_info.sector = 0; p_info.sector < MAX_SECTOR_COUNT; p_info.sector++ ) {
		for ( p_info.segment = 0; p_info.segment < table->segment_count_in_sector; p_info.segment++ ) {
			int state = table->physical_sector[p_info.sector].segment[p_info.segment];

			p = table_get_index(&p_info, table->segment_count_in_sector);
			l = table_get_logical_segment(table, p);

			// ALLOCATED �����̸鼭 logical segment �� ���ٴ� ���� �̻��� ���̴�.
			if ( state == ALLOCATED && l < 0 ) {
				warn("physical segment[%d] is not exists in logical segment list. fix it.", p);
				// ������ ���� ������ �����ϸ� ���� ġ�����̴�.
				if ( table_update_physical_segment(table, p, TEMP) != SUCCESS ) {
					error("update physical segment[%d] to TEMP failed", p);
					return FAIL;
				}
				fix_count++;
			}
			// ALLOCATED �� �ƴѵ� logical segment �� �ִٴ� �͵� �̻��ϴ�.
			if ( state != ALLOCATED && l >= 0 ) {
				warn("physical segment[%d] should be in ALLOCATED state", p);
				// ������ ���� ������ �����ϸ� ���� ġ�����̴�.
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
	 * table_print() �� �ۿ��� lock �� �ɰ� ������?
	 * �ƴ϶�� ���⼭ �����ִ� ������ �Ͻ����� �� ���� �ִ�
	 **********************************************************************/

	// logical segment list �� ������� ���� alocated physical segment �� ã�´�.
	// �ߴ��� ������ ã�Ƴ����� �ϴ� �Ŵ�.
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

			// ALLOCATED �����̸鼭 logical segment �� ���ٴ� ���� �̻��� ���̴�.
			if ( state == ALLOCATED && l < 0 ) {
				warn("physical segment[%d, %s] is not exists in logical segment list", p, str_state[state+1]);
			}
			// ALLOCATED �� �ƴѵ� logical segment �� �ִٴ� �͵� �̻��ϴ�.
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
