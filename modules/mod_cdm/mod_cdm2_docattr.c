/*
 * docattr 을 access하는 코드를 모아놨다
 */

#include <string.h> // for memset

#include "common_core.h"
#include "mod_api/docattr.h"
#include "mod_cdm2.h"
#include "mod_cdm2_docattr.h"

int is_deleted(uint32_t docid, int* deleted)
{
	docattr_t attr;
	char buf[10];

	if ( sb_run_docattr_get(docid, &attr) != SUCCESS ) {
		error("cannot get docattr");
		return FAIL;
	}

	if ( sb_run_docattr_get_docattr_function(&attr, "Delete", buf, sizeof(buf)) != SUCCESS ) {
		error("cannot get Delete field value");
		return FAIL;
	}

	*deleted = (buf[0] == '1');

	return SUCCESS;
}

int set_delete(uint32_t docid)
{
	docattr_mask_t docmask;

	DOCMASK_SET_ZERO(&docmask);
	sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
	sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);

	return SUCCESS;
}

/*
 * field_value 는 '\0' 으로 끝나지 않고, docattr api는 '\0' 으로 끝나는 것을 원하니까
 * 복사해서 처리하는 수밖에...
 */
int set_docattr_mask(docattr_mask_t* mask,
		char* field_name, char* field_value, int value_length, const char* oid)
{
	char _tmp[1024]; // docattr에 쓰는 field는 이정도면 충분?
	char* tmp;
	int copy_size, ret;

	// maybe overflow but ok?
	if ( field_value[value_length] == '\0' ) {
		tmp = field_value; // 복사안하고 행복하게...
	}
	else {
		if ( value_length >= sizeof(_tmp) ) {
			warn("too long field for docattr. but, first 1024 bytes would be OK");
			copy_size = sizeof(_tmp)-1;
		}
		else {
			copy_size = value_length;
		}

		_tmp[0] = '\0';
		strncat(_tmp, field_value, copy_size);
		tmp = _tmp;
	}

	ret = sb_run_docattr_set_docmask_function(mask, field_name, tmp);
	if ( ret != SUCCESS ) {
		warn("wrong type of value of field[%s] of document[%s]", field_name, oid);
	}

	return ret;
}

