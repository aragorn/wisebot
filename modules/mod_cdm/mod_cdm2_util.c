#include "common_core.h"
#include "memory.h"

#include "mod_api/cdm2.h" // for CDM2_NOT_ENOUGH_BUFFER

#include "mod_cdm2_util.h"

/*
 * cdm_doc_custom_t ó�� ���� allocation�� �߻��ϰ�, size�� ũ��
 * �Ҵ������ 1024�� ���ؼ�, �� ������ alloc�ϵ��� �Ѵ�.
 * memory fragmentation ������ ���̷��� �Ѵ�.
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
 * term�� ���� ������ buf�� ���� *pos�� ������Ų��.
 * buf���� term�� ã����, �� ���ڸ����� ���߰� ����ġ pointer�� �����Ѵ�.
 *
 * *pos == max_pos �� �Ǹ� buf�� ���� �ʰ� �ٷ� return
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
 * pass_til���� �ݴ��, buf���� term�� ������ ���� ���...
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
 * strncat �� ��������� �ִ� size-1 ���� ����ϰ� size���� '\0'�� �Ǵ°��� �����Ѵ�.
 * return ���� ������ ũ��('\0' �� ����)��.
 * ���� ���۰� ���ڶ����� return ���� CDM2_NOT_ENOUGH_BUFFER
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

