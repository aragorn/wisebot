#include "common_core.h"
#include "memory.h"

#include "mod_api/cdm2.h" // for CDM2_NOT_ENOUGH_BUFFER

#include "mod_cdm2_util.h"

/*
 * cdm_doc_custom_t 처럼 자주 allocation이 발생하고, size도 크면
 * 할당단위를 1024로 정해서, 그 단위로 alloc하도록 한다.
 * memory fragmentation 문제를 줄이려고 한다.
 */
void* big_calloc(size_t size)
{
	size_t optimized_size = ((size-1)/1024+1)*1024;
	return sb_calloc(sizeof(long), optimized_size/sizeof(long));
}

void* big_realloc(void* buf, size_t size)
{
	size_t optimized_size = ((size-1)/1024+1)*1024;
	return sb_realloc(buf, optimized_size);
}

/*
 * term이 나올 때까지 buf를 지나 *pos를 증가시킨다.
 * buf에서 term을 찾으면, 딱 그자리에서 멈추고 그위치 pointer를 리턴한다.
 *
 * *pos == max_pos 가 되면 buf는 보지 않고 바로 return
 */
char* pass_til(char* buf, char term, int* pos, int max_pos)
{
	int l_pos = *pos;

	while ( l_pos < max_pos ) {
		if ( *buf == term ) break;

		l_pos++;
		buf++;
	}

	*pos = l_pos;
	return buf;
}

/*
 * pass_til과는 반대로, buf에서 term이 나오는 동안 계속...
 */
char* pass_not(char* buf, char term, int* pos, int max_pos)
{
	int l_pos = *pos;

	while ( l_pos < max_pos ) {
		if ( *buf != term ) break;

		l_pos++;
		buf++;
	}

	*pos = l_pos;
	return buf;
}

/*
 * strncat 과 비슷하지만 최대 size-1 까지 기록하고 size에서 '\0'이 되는것을 보장한다.
 * return 값은 복사한 크기('\0' 은 빼고)다.
 * 만약 버퍼가 모자랐으면 return 값은 CDM2_NOT_ENOUGH_BUFFER
 */
size_t copy_string(char* buf, const char* text, size_t size)
{
	size_t i;

	for ( i = 0; i < size; i++ ) {
		buf[i] = text[i];
		if ( text[i] == '\0' ) return i;
	}

	buf[size-1] = '\0';
	return CDM2_NOT_ENOUGH_BUFFER;
}

