#include "mod_docattr_general.h"
#include "mod_api/morpheme.h"
#include "mod_api/qp.h"
#include "mod_qp/mod_qp.h"
#include <limits.h>

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE
#define BIT_PER_BYTE		8
#define MAX_BIT_SIZE		((int)(sizeof(docattr_integer)*BIT_PER_BYTE-2))

docattr_field_t docattr_field[MAX_DOCATTR_FIELD];
int docattr_field_count = 0;

docattr_field_t* rid_field = NULL;
char rid_field_name[SHORT_STRING_SIZE] = "";

// zero 를 대표하는 값. 하나 만들어 놓고 두고두고 쓴다...
docattr_value_t value_zero;

// 발견하면 field 수정하고, 못찾으면 field = NULL
// 리턴값은 항상 현재 index. 그래서 새로운 field 할당에도 쓸 수 있다.
int return_docattr_field(const char* name, docattr_field_t** field)
{
	int i;

	for ( i = 0; i < docattr_field_count; i++ ) {
		if ( strncasecmp( name, docattr_field[i].name,
					sizeof(docattr_field[i].name) ) == 0 ) {
			*field = &docattr_field[i];
			return i;
		}
	}

	*field = NULL;
	return i;
}

static char *enum_names[MAX_ENUM_NUM] = { NULL };
static int64_t enum_values[MAX_ENUM_NUM];

// Enum값 찾지 못하면 0이다.
int64_t return_enum_value(const char* value)
{
	int i;

	for ( i = 0; i < MAX_ENUM_NUM && enum_names[i]; i++ ) {
		if ( strncasecmp( value, enum_names[i], MAX_ENUM_LEN ) == 0 ) {
			break;
		}
	}
	if ( i == MAX_ENUM_NUM || enum_names[i] == NULL ) {
		return 0;
	}
	return enum_values[i];
}

char* return_enum_name(int64_t value)
{
	int i;

	for ( i = 0; i < MAX_ENUM_NUM && enum_names[i]; i++ ) {
		if ( enum_values[i] == value ) break;
	}

	if ( i == MAX_ENUM_NUM || enum_names[i] == NULL ) {
		return NULL;
	}

	return enum_names[i];
}

// 제대로된 숫자인지 판별해서 저장
// 앞뒤 공백문자 같은 건 없다고 간주한다. SUCCESS/FAIL
int isNumber(const char* string, docattr_integer* number)
{
	char* end_of_string;
	long result;

	errno = 0; // strtol 함수가 errno 값을 제대로 setting하지 않는다.
	result = strtol( string, &end_of_string, 0 );

	if ( *end_of_string != '\0' ) {
		result = (long) return_enum_value( string );
		if ( result == 0 ) return FAIL;
	}
	else if ( errno != 0 ) return FAIL;

	*number = (docattr_integer) result;
	return SUCCESS;
}

// 미리 컴파일된 sorting 조건들
#define MAX_GENERAL_SORT 32
static general_sort_t* general_sort = NULL;

/********************************************************
 *                field 연산 함수들
 ********************************************************/

#define DATA_POSITION (docattr->rsv + field->offset)

static int string_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	// 한자 -> 한글 처리 해야 한다
	int left;
	char title[SHORT_STRING_SIZE], _value[SHORT_STRING_SIZE];
	WordList wordlist;
	Morpheme morp;

	title[0] = '\0';
	strncpy(_value, value, SHORT_STRING_SIZE-1);
	_value[SHORT_STRING_SIZE-1] = '\0';
	sb_run_morp_set_text(&morp, _value, 4);
	left = SHORT_STRING_SIZE;
	while( sb_run_morp_get_wordlist( &morp, &wordlist, 4 ) != FAIL &&
			left > 0 ) {
		strncat( title, wordlist.words[1].word, left );
		left -= strlen( wordlist.words[1].word );
	}

	strncpy( DATA_POSITION, value, field->size );
	((char*) DATA_POSITION)[field->size-1] = '\0';
	return SUCCESS;
}

static void string_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->string = DATA_POSITION;
}

static int string_compare_func(docattr_value_t* value1, docattr_value_t* value2)
{
	return hangul_strncmp( value1->string, value2->string, (int) sizeof(value1->my_string) );
}

static int integer_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	docattr_integer number;

	if ( isNumber( value, &number ) != SUCCESS ) {
		error("invalid number(%s) when setting docattr field[%s]", value, field->name);
		return FAIL;
	}

	*((docattr_integer*) DATA_POSITION) = number;
	return SUCCESS;
}

static void integer_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->integer = *((docattr_integer*) DATA_POSITION);
}

static void integer_get_as_string_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	field->get_func( docattr, field, value );
	snprintf( value->my_string, sizeof(value->my_string),
			"%ld", value->integer);
	value->string = value->my_string;
}

static int integer_compare_func(docattr_value_t* value1, docattr_value_t* value2)
{
	return value1->integer - value2->integer;
}

static int integer4_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	docattr_integer number;

	if ( isNumber( value, &number ) != SUCCESS ) {
		error("invalid number(%s) when setting docattr field[%s]", value, field->name);
		return FAIL;
	}

	*((int32_t*) DATA_POSITION) = (int32_t) number;
	return SUCCESS;
}

static void integer4_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->integer = *((int32_t*) DATA_POSITION);
}

static int integer2_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	docattr_integer number;

	if ( isNumber( value, &number ) != SUCCESS ) {
		error("invalid number(%s) when setting docattr field[%s]", value, field->name);
		return FAIL;
	}

	if ( number != (int16_t) number ) {
		error("overflowed number(%s). field[%s - Integer(2)]", value, field->name);
		return FAIL;
	}

	*((int16_t*) DATA_POSITION) = (int16_t) number;
	return SUCCESS;
}

static void integer2_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->integer = *((int16_t*) DATA_POSITION);
}

static int integer1_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	docattr_integer number;

	if ( isNumber( value, &number ) != SUCCESS ) {
		error("invalid number(%s) when setting docattr field[%s]", value, field->name);
		return FAIL;
	}

	if ( number != (int8_t) number ) {
		error("overflowed number(%s). field[%s - Integer(1)]", value, field->name);
		return FAIL;
	}

	*((int8_t*) DATA_POSITION) = (int8_t) number;
	return SUCCESS;
}

static void integer1_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->integer = *((int8_t*) DATA_POSITION);
}

static int onebit_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	if ( value[1] != '\0' || ( value[0] != '0' && value[0] != '1' ) ) {
		error("1 bit value is only 0 or 1. current[%s]. not 01, 0x1 ...", value);
		return FAIL;
	}

	if ( value[0] == '0' )
		*((docattr_integer*) DATA_POSITION) &= ( ~(1<<field->bit_offset) );
	else
		*((docattr_integer*) DATA_POSITION) |= (1<<field->bit_offset);

	return SUCCESS;
}

static void onebit_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	if ( *((docattr_integer*) DATA_POSITION) & (1<<field->bit_offset) )
		value->integer = 1;
	else value->integer = 0;
}

static void onebit_get_as_string_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	onebit_get_func( docattr, field, value );
	if ( value->integer == 1 ) value->my_string[0] = '1';
	else value->my_string[0] = '0';
	value->my_string[1] = '\0';
	value->string = value->my_string;
}

static int bit_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	docattr_integer number, mask;
	docattr_integer* attr_position = (docattr_integer*) DATA_POSITION;
	
	if ( isNumber( value, &number ) != SUCCESS ) {
		error("invalid number(%s) when setting docattr field[%s]", value, field->name);
		return FAIL;
	}

	if ( field->bit_size < sizeof(docattr_integer)*BIT_PER_BYTE
			&& ((1<<field->bit_size) <= number) ) {
		error("overflow number(%s): field[%s - bit(%d)]",
				value, field->name, field->bit_size);
		return FAIL;
	}

	number = (number << (field->bit_offset));
	mask = (1 << (field->bit_size)) - 1;
	mask = ~(mask << (field->bit_offset));

	*attr_position = (*attr_position & mask) | number;
	return SUCCESS;
}

static void bit_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	docattr_integer number, mask;

	mask = (1 << field->bit_size) - 1;

	number = *((docattr_integer*) DATA_POSITION);
	number = (number >> field->bit_offset) & mask;

	value->integer = number;
}

// bit_compare_func -> integer_compare_func

static void bit_get_as_string_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	bit_get_func( docattr, field, value );
	snprintf( value->my_string, sizeof(value->my_string),
			"%ld", value->integer );
	value->string = value->my_string;
}

static void enum_get_as_string_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	docattr_value_t integer_value;

	field->get_func( docattr, field, &integer_value );
	value->string = return_enum_name( integer_value.integer );
	if ( value->string == NULL ) {
		warn("enum name of [%ld] is not defined. field:%s",
				integer_value.integer, field->name);
		snprintf( value->my_string, sizeof(value->my_string),
				"%ld", integer_value.integer );
		value->string = value->my_string;
	}
}

static int md5_set_func(docattr_t* docattr, docattr_field_t* field, char *value)
{
	MD5_CTX context;
	unsigned char digest[16];
	unsigned int len = strlen(value);

	MD5Init(&context);
	MD5Update(&context, value, len);
	MD5Final(digest, &context);

	memcpy( DATA_POSITION, digest, field->size );
	return SUCCESS;
}

static void md5_get_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	value->md5 = *((docattr_md5*) DATA_POSITION);
}

static int md5_compare_func(docattr_value_t* value1, docattr_value_t* value2)
{
	return value1->md5 - value2->md5;
}

static void md5_get_as_string_func(docattr_t* docattr,
		docattr_field_t* field, docattr_value_t* value)
{
	md5_get_func( docattr, field, value );
	snprintf( value->my_string, sizeof(value->my_string),
			"0x%"PRIx64"", value->md5 );
	value->string = value->my_string;
}

/**********************************************************************************
 *                                api 함수들...
 **********************************************************************************/

static int set_docmask_function(void* dest, char* key, char* value)
{
	int field_id, i;
	docattr_field_t* field;
	general_mask_t* mask = (general_mask_t*) dest;

	field_id = return_docattr_field( key, &field );
	if ( field == NULL ) {
		error("there is no docattr field: [%s]", key);
		return FAIL;
	}

	if ( !mask->init ) {
		memset( mask->id, 0xff, sizeof(mask->id) );
		mask->init = 1;
	}

	if ( field->set_func( &mask->docattr, field, value ) != SUCCESS )
		return FAIL;

	for ( i = 0; i < MAX_DOCATTR_FIELD; i++ ) {
		if ( mask->id[i] < 0 ) {
			mask->id[i] = field_id;
			break;
		}
		if ( mask->id[i] == field_id ) break;
	}

	return SUCCESS;
}

// 거의 사용하지 않는다.. 대충 만들도록 한다.
static int mask_function(void* dest, void* source_mask)
{
	docattr_t* docattr = (docattr_t*) dest;
	general_mask_t* mask = (general_mask_t*) source_mask;
	docattr_field_t* field;
	int i;

	if ( !mask->init ) {
		warn("mask->init is 0. nothing to set");
		return SUCCESS;
	}

	for ( i = 0; mask->id[i] >= 0; i++ ) {
		field = &docattr_field[mask->id[i]];

		if ( field->field_type == FIELD_BIT ) {
			docattr_integer bit_mask = ( ((1<<field->bit_size)-1)<<field->bit_offset );
			docattr_integer *src, *dest;

			src = (docattr_integer*) (mask->docattr.rsv + field->offset);
			dest = (docattr_integer*) (docattr->rsv + field->offset);

			*dest = (*dest & ~bit_mask) | (*src & bit_mask);
		}
		else {
			memcpy( docattr->rsv + field->offset, mask->docattr.rsv + field->offset,
					field->size );
		}
	}

	return SUCCESS;
}

static int set_docattr_function(void* dest, char *key, char *value)
{
	docattr_field_t* field;

	return_docattr_field( key, &field );
	if ( field == NULL ) {
		error("there is no docattr field: [%s]", key);
		return FAIL;
	}

	return field->set_func( (docattr_t*) dest, field, value );
}

static int get_docattr_function(void* dest, char* key, char* buf, int buflen)
{
	docattr_t* docattr = (docattr_t*) dest;
	docattr_field_t* field;

	return_docattr_field( key, &field );
	if ( field == NULL ) {
		warn("unknown field name: %s", key);
		return FAIL;
	}

	// docattr 의 string은 '\0'으로 끝나지 않을 수도 있다.
	field->get_as_string_func( docattr, field, &field->value );
	if ( field->value_type == VALUE_STRING && buflen > field->size ) {
		strncpy( buf, field->value.string, field->size );
		buf[field->size] = '\0';
	}
	else {
		strncpy( buf, field->value.string, buflen );
		buf[buflen-1] = '\0';
	}
	return SUCCESS;
}

static int compare_rid_md5(const void* dest, const void* sour)
{
	docattr_t *attr1, *attr2;
	docattr_value_t value1, value2;

	if ( sb_run_docattr_ptr_get( ((doc_hit_t*) dest)->id, &attr1 ) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}

	if ( sb_run_docattr_ptr_get( ((doc_hit_t*) sour)->id, &attr2 ) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}

	rid_field->get_func( attr1, rid_field, &value1 );
	rid_field->get_func( attr2, rid_field, &value2 );

	if ( value1.md5 > value2.md5 ) return 1;
	else if ( value1.md5 == value2.md5 ) return 0;
	else return -1;
}

static int docattr_distinct_rid_md5(int id, index_list_t* list)
{
	int i, abst;
	docattr_t *attr1, *attr2;
	docattr_value_t value1, value2;

	qsort( list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid_md5);

	abst = 0;

	if ( sb_run_docattr_ptr_get( list->doc_hits[0].id, &attr1 ) != SUCCESS ) {
		error("cannot get docattr element");
		return FAIL;
	}
	rid_field->get_func( attr1, rid_field, &value2 );
	info("md5: %" PRIu64, value2.md5);

	for ( i = 0; i < list->ndochits; ) {
		value1.md5 = value2.md5;

		if ( abst != i ) {
			memcpy( &(list->doc_hits[abst]), &(list->doc_hits[i]),
					sizeof(doc_hit_t) );
		}

		for ( i++; i < list->ndochits; i++ ) {
			if ( sb_run_docattr_ptr_get( list->doc_hits[i].id, &attr2 ) != SUCCESS ) {
				error("cannot get docattr element");
				return FAIL;
			}
			
			rid_field->get_func( attr2, rid_field, &value2 );
			info("md5: %" PRIu64, value2.md5);

			if ( value1.md5 == 0 || value2.md5 == 0 ) break;
			if ( value1.md5 != value2.md5 ) break;
		}

		abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}

static int compare_rid_general(const void* dest, const void* sour)
{
	docattr_t *attr1, *attr2;
	docattr_value_t value1, value2;

	if ( sb_run_docattr_ptr_get( ((doc_hit_t*) dest)->id, &attr1 ) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}

	if ( sb_run_docattr_ptr_get( ((doc_hit_t*) sour)->id, &attr2 ) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}

	rid_field->get_func( attr1, rid_field, &value1 );
	rid_field->get_func( attr2, rid_field, &value2 );

	return rid_field->compare_func( &value2, &value1 );
}

// value.string 은 attr 내부를 가리키고 있으므로 조심
static int docattr_distinct_rid_general(int id, index_list_t* list)
{
	int i, abst;
	docattr_t *attr1, *attr2;
	docattr_value_t value1, value2;

	qsort( list->doc_hits, list->ndochits, sizeof(doc_hit_t), compare_rid_general);

	abst = 0;

	if ( sb_run_docattr_ptr_get( list->doc_hits[0].id, &attr1 ) != SUCCESS ) {
		error("cannot get docattr element");
		return FAIL;
	}
	rid_field->get_func( attr1, rid_field, &value2 );

	for ( i = 0; i < list->ndochits; ) {
		value1 = value2;

		if ( abst != i ) {
			memcpy( &(list->doc_hits[abst]), &(list->doc_hits[i]),
					sizeof(doc_hit_t) );
		}

		for ( i++; i < list->ndochits; i++ ) {
			if ( sb_run_docattr_ptr_get( list->doc_hits[i].id, &attr2 ) != SUCCESS ) {
				error("cannot get docattr element");
				return FAIL;
			}
			
			rid_field->get_func( attr2, rid_field, &value2 );

			if ( rid_field->compare_func( &value1, &value_zero ) == 0
					|| rid_field->compare_func( &value2, &value_zero ) == 0 ) break;
			if ( rid_field->compare_func( &value1, &value2 ) != 0 ) break;

			info("same value: %s, %s", value1.string, value2.string);
		}

		info("abst inc");
		abst++;
	}

	list->ndochits = abst;

	return SUCCESS;
}

static int compare_function_for_qsort(const void* dest, const void* sour, void* userdata)
{
	int i, diff = 0;
	docattr_t *attr1, *attr2;
	docattr_value_t value1, value2;
	general_sort_t *sort;
	docattr_field_t* field;

	if ( general_sort == NULL ) return 1;

	sort = &general_sort[((docattr_sort_t*) userdata)->index];
	if ( !sort->set ) return 0;

	if ( sb_run_docattr_ptr_get(((doc_hit_t*) dest)->id, &attr1) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}
	if ( sb_run_docattr_ptr_get(((doc_hit_t*) sour)->id, &attr2) != SUCCESS ) {
		error("cannot get docattr element");
		return 0;
	}

	for ( i = 0; i < sort->condition_count; i++ ) {
		field = sort->condition[i].field;

		field->get_func( attr1, field, &value1 );
		field->get_func( attr2, field, &value2 );

		diff = field->compare_func( &value1, &value2 );

		if ( diff != 0 ) {
			diff = diff > 0 ? 1 : -1;
			return diff * sort->condition[i].order;
		}
	} // for (i)

	return 0;
}

/****************************************************
 * INPUT
 *  field_type : Integer(4), Integer, Integer()
 *
 * OUTPUT
 *  type : Integer, 버퍼크기는 충분하다고 본다.
 *  size : 4, size가 없는 경우는 0
 *
 * RETURN
 *  SUCCESS/FAIL
 ****************************************************/
static int parse_field_type(char* field_type, char* type, docattr_integer* size)
{
	char *lparen, *rparen;

	strcpy( type, field_type );
	lparen = strchr( type, '(' );
	rparen = strchr( type, ')' );

	// 괄호가 완전히 있거나 없거나... 해야 정상인데..
	if ( (lparen && !rparen) || (!lparen && rparen) ) {
		error("parenthesis mismatch in [%s]", field_type);
		return FAIL;
	}

	if ( lparen && lparen+1 != rparen ) {
		*lparen = '\0';
		*rparen = '\0';

		if ( isNumber( lparen+1, size ) != SUCCESS ) {
			error("invalid size: [%s]", field_type);
			return FAIL;
		}
	}
	else *size = 0;

	return SUCCESS;
}

static int check_number_in_range(int number, int min, int max)
{
	if ( number < min ) {
		error("[%d] is too small. minimum value is [%d]", number, min);
		return FAIL;
	}

	if ( number > max ) {
		error("[%d] is too big. maximum value is [%d]", number, max);
		return FAIL;
	}

	return SUCCESS;
}

static int add_docattr_field(char* field_name, char* field_type)
{
	docattr_field_t* field;
	int field_id;
	// field_type parsing 결과
	char type[SHORT_STRING_SIZE]; docattr_integer size;

	field_id = return_docattr_field( field_name, &field );
	if ( field != NULL ) {
		warn("field [%s] is already defined. now overwrite it", field_name);
	}
	if ( field_id >= MAX_DOCATTR_FIELD ) {
		error("too many fields. MAX_DOCATTR_FIELD[%d], field [%s - %s] is ignored",
				MAX_DOCATTR_FIELD, field_name, field_type);
		return FAIL;
	}

	field = &docattr_field[field_id];

	if ( strlen( field_name ) >= sizeof(field->name) ) {
		error("too long field name: %s, sizeof(field->name): %d",
				field_name, (int) sizeof(field->name));
		return FAIL;
	}

	if ( parse_field_type( field_type, type, &size ) != SUCCESS ) {
		error("Invalid field definition format: %s %s", field_name, field_type);
		return FAIL;
	}

	if ( strcasecmp( type, "String" ) == 0 ) {
		if ( check_number_in_range( size, 1, MAX_DOCATTR_ELEMENT_SIZE ) != SUCCESS ) {
			error("string size out of range[1~%d]: %s %s",
					MAX_DOCATTR_ELEMENT_SIZE, field_name, field_type);
			return FAIL;
		}

		field->field_type = FIELD_STRING;
		field->value_type = VALUE_STRING;
		field->size = size;
		field->set_func = string_set_func;
		field->get_func = string_get_func;
		field->get_as_string_func = string_get_func;
		field->compare_func = string_compare_func;
	}
	else if ( strcasecmp( type, "Integer" ) == 0 ) {
		if ( size == 0 ) size = (int) sizeof(docattr_integer);
		else if ( size > sizeof(docattr_integer) ) {
			error("max integer size is %d", (int) sizeof(docattr_integer));
			return FAIL;
		}

		if ( size == (int) sizeof(docattr_integer) ) {
			field->field_type = FIELD_INTEGER;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer_set_func;
			field->get_func = integer_get_func;
			field->get_as_string_func = integer_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 4 ) {
			field->field_type = FIELD_INTEGER;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer4_set_func;
			field->get_func = integer4_get_func;
			field->get_as_string_func = integer_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 2 ) {
			field->field_type = FIELD_INTEGER;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer2_set_func;
			field->get_func = integer2_get_func;
			field->get_as_string_func = integer_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 1 ) {
			field->field_type = FIELD_INTEGER;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer1_set_func;
			field->get_func = integer1_get_func;
			field->get_as_string_func = integer_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else {
			error("unsupported integer size: %s %s", field_name, field_type);
			return FAIL;
		}
	}
	else if ( strcasecmp( type, "Bit" ) == 0 ) {
		if ( check_number_in_range( size, 1, MAX_BIT_SIZE ) != SUCCESS ) {
			error("string size out of range[1~%d]: %s %s",
					MAX_DOCATTR_ELEMENT_SIZE, field_name, field_type);
			return FAIL;
		}

		if ( size == 1 ) {
			field->set_func = onebit_set_func;
			field->get_func = onebit_get_func;
			field->get_as_string_func = onebit_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 8 || size == 16 || size == 32 || size == 64 ) {
			char integer_type[SHORT_STRING_SIZE];

			snprintf( integer_type, sizeof(integer_type), "Integer(%ld)", (size%BIT_PER_BYTE) );
			warn("Bit(%ld) equals to %s - %s %s", size, integer_type, field_name, field_type);
			return add_docattr_field( field_name, integer_type );
		}
		else {
			field->set_func = bit_set_func;
			field->get_func = bit_get_func;
			field->get_as_string_func = bit_get_as_string_func;
			field->compare_func = integer_compare_func;
		}

		field->field_type = FIELD_BIT;
		field->value_type = VALUE_INTEGER;
		field->size = sizeof(docattr_integer);
		field->bit_size = size;
	}
	else if ( strcasecmp( type, "Enum" ) == 0 ) {
		if ( size == 0 ) size = sizeof(docattr_integer);
		else if ( size > sizeof(docattr_integer) ) {
			error("max enum size is %d", (int) sizeof(docattr_integer));
			return FAIL;
		}

		if ( size == sizeof(docattr_integer) ) {
			field->field_type = FIELD_ENUM;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer_set_func;
			field->get_func = integer_get_func;
			field->get_as_string_func = enum_get_as_string_func;
			field->compare_func = integer_compare_func;
		}

		if ( size == 4 ) {
			field->field_type = FIELD_ENUM;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer4_set_func;
			field->get_func = integer4_get_func;
			field->get_as_string_func = enum_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 2 ) {
			field->field_type = FIELD_ENUM;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer2_set_func;
			field->get_func = integer2_get_func;
			field->get_as_string_func = enum_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else if ( size == 1 ) {
			field->field_type = FIELD_ENUM;
			field->value_type = VALUE_INTEGER;
			field->size = size;
			field->set_func = integer1_set_func;
			field->get_func = integer1_get_func;
			field->get_as_string_func = enum_get_as_string_func;
			field->compare_func = integer_compare_func;
		}
		else {
			error("unsupported integer size: %s %s", field_name, field_type);
			return FAIL;
		}
	}
	else if ( strcasecmp( type, "MD5" ) == 0 ) {
		field->field_type = FIELD_MD5;
		field->value_type = VALUE_MD5;
		field->size = sizeof(docattr_md5);
		field->set_func = md5_set_func;
		field->get_func = md5_get_func;
		field->get_as_string_func = md5_get_as_string_func;
		field->compare_func = md5_compare_func;
	}
	else {
		error("unknown type of field [%s - %s]", field_name, field_type);
		return FAIL;
	}

	strncpy( field->name, field_name, sizeof(field->name) );
	field->name[sizeof(field->name)-1] = '\0';

	docattr_field_count++;
	return SUCCESS;
}

static int build_field_offset()
{
	int i;
	int offset = 0, size, bit_offset;
	int assigned_field[MAX_DOCATTR_FIELD] = { 0 };
	int assigned_count = 0;
	docattr_field_t* field;

	// need field offset initialize. 8, 4, 2, 1, bit, other 순서로..

	// 8, 4, 2, 1
	for ( size = sizeof(uint64_t); size > 0; size /= 2 ) {
		for ( i = 0; i < docattr_field_count; i++ ) {
			if ( size == 1 && offset%sizeof(long) == 0 ) break;

			field = &docattr_field[i];
			if ( assigned_field[i] || field->size != size
					|| field->field_type == FIELD_BIT || field->field_type == FIELD_ENUMBIT ) continue;

			field->offset = offset;
			offset += size;

			assigned_field[i] = 1;
			assigned_count++;

			info("DocAttrField [%s] - offset: %d, size: %d",
					field->name, field->offset, field->size);
		}
	}

	// bit
	// 일단 byte align 맞추고...
	while( offset%sizeof(long) ) offset++;
	bit_offset = 0;

	for ( i = 0; i < docattr_field_count; i++ ) {
		field = &docattr_field[i];
		if ( field->field_type != FIELD_BIT && field->field_type != FIELD_ENUMBIT ) continue;

		if ( bit_offset + field->bit_size > sizeof(long)*BIT_PER_BYTE ) {
			bit_offset = 0;
			offset += sizeof(long);
		}

		field->bit_offset = bit_offset;
		field->offset = offset;

		bit_offset += field->bit_size;

		assigned_field[i] = 1;
		assigned_count++;

		info("DocAttrField [%s] - offset: %d, size: %d, bit offset: %d, bit size: %d",
				field->name, field->offset, field->size, field->bit_offset, field->bit_size);
	}

	if ( bit_offset ) {
		offset += (bit_offset-1)/BIT_PER_BYTE+1;
	}

	// other. byte align 필요없음
	for ( i = 0; i < docattr_field_count; i++ ) {
		if ( assigned_field[i] ) continue;
		field = &docattr_field[i];

		field->offset = offset;
		offset += field->size;

		assigned_field[i] = 1;
		assigned_count++;

		info("DocAttrField [%s] - offset: %d, size: %d",
				field->name, field->offset, field->size);
	}

	info("DocAttr Size: %d, Field Count: %d", offset, assigned_count);

	return SUCCESS;
}

// 왠만한 에러로는 FAIL을 리턴하면 안된다... init() 의 성공여부가 달려있어서..
static int build_sort_field()
{
	char *string;
	char *field_name, *sort_order;
	docattr_field_t* field;
	int i, j;

	if ( general_sort == NULL ) {
		warn("no sort condition set");
		return SUCCESS;
	}

	string = (char*) sb_malloc( sizeof(general_sort[0].string) );

	for ( i = 0; i < MAX_GENERAL_SORT; i++ ) {
		if ( !general_sort[i].set ) continue;
		info("build sort condition: %d, %s", i, general_sort[i].string);

		strcpy( string, general_sort[i].string );
		field_name = strtok( string, ";" );
		j = 0;

		do {
			sort_order = strchr( field_name, ':' );
			if ( sort_order == NULL ) {
				warn("invalid condition: %s, index:%d", field_name, i);
				continue;
			}

			*(sort_order++) = '\0';

			return_docattr_field( field_name, &field );
			if ( field == NULL ) {
				warn("unknown field[%s], index:%d", field_name, i);
				continue;
			}

			general_sort[i].condition[j].field = field;

			if ( strcasecmp( sort_order, "ASC" ) == 0 )
				general_sort[i].condition[j].order = 1;
			else if ( strcasecmp( sort_order, "DESC" ) == 0 )
				general_sort[i].condition[j].order = -1;
			else {
				warn("unknown sort order:%s, index:%d", field_name, i);
				continue;
			}

			j++;
		} while ( (field_name = strtok( NULL, ";" )) != NULL );

		general_sort[i].condition_count = j;
	}

	sb_free(string);
	return SUCCESS;
}

static int init()
{
	docattr_field_t* delete_field;

	if ( sizeof(docattr_t) > sizeof(docattr_t) ) {
		crit("sizeof(docattr_t) (%d) > sizeof(docattr_t) (%d)",
				(int) sizeof(docattr_t), (int) sizeof(docattr_t) );
		return FAIL;
	}

	if ( sizeof(general_mask_t) > sizeof(docattr_mask_t) ) {
		crit("sizeof(general_mask_t) (%d) > sizeof(docattr_mask_t) (%d)",
				(int) sizeof(general_mask_t), (int) sizeof(docattr_mask_t) );
		return FAIL;
	}

	// value_zero 초기화
	memset( &value_zero, 0, sizeof(value_zero) );
	value_zero.string = value_zero.my_string;

	// 반드시 있어야 하는 field 강제로 추가
	return_docattr_field( "Delete", &delete_field );
	if ( delete_field == NULL || delete_field->value_type != VALUE_INTEGER ) {
		add_docattr_field( "Delete", "Bit(1)" );
	}

	if ( build_field_offset() != SUCCESS ) {
		error("build field offset failed");
		return FAIL;
	}

	// rid field 설정
	if ( rid_field_name[0] != '\0' ) {
		return_docattr_field( rid_field_name, &rid_field );
		if ( rid_field == NULL ) {
			warn("cannot find docattr field [%s] for rid field. rid is ignored",
					rid_field_name);
		}
		else {
			if ( rid_field->field_type == FIELD_MD5 ) {
				info("RidField is [%s]. type is MD5", rid_field->name);
				sb_hook_docattr_modify_index_list(
						docattr_distinct_rid_md5, NULL, NULL, HOOK_MIDDLE);
			}
			else {
				warn("Rid field is not MD5 type. maybe slow");
				sb_hook_docattr_modify_index_list(
						docattr_distinct_rid_general, NULL, NULL, HOOK_MIDDLE);
			}
		}
	}

	if ( build_sort_field() != SUCCESS ) {
		error("build sort field failed");
		return FAIL;
	}

	return SUCCESS;
}

static void get_docattr_field(configValue v)
{
	char* field_name = v.argument[0];
	char* field_type = v.argument[1];

	add_docattr_field( field_name, field_type );
}

static void get_rid_field(configValue v)
{
	strncpy( rid_field_name, v.argument[0], sizeof(rid_field_name) );
	rid_field_name[sizeof(rid_field_name)-1] = '\0';
}

static void get_enum(configValue v)
{
	int i;
	docattr_integer enum_value;
	static char enums[MAX_ENUM_NUM][MAX_ENUM_LEN];

	for ( i = 0; i < MAX_ENUM_NUM && enum_names[i]; i++ );
	if ( i == MAX_ENUM_NUM ) {
		error("too many constant is defined");
		return;
	}

	if ( isNumber( v.argument[0], &enum_value ) == SUCCESS ) {
		warn("%s is number or already registered enum value", v.argument[0]);
		return;
	}

	strncpy( enums[i], v.argument[0], MAX_ENUM_LEN );
	enums[i][MAX_ENUM_LEN-1] = '\0';
	enum_names[i] = enums[i];

	enum_values[i] = atoi( v.argument[1] );
	info("Enum[%s]: %" PRId64, enum_names[i], enum_values[i]);
}

static void get_field_sorting_order(configValue v)
{
	static general_sort_t local_general_sort[MAX_GENERAL_SORT];
	int index;

	if ( general_sort == NULL ) {
		general_sort = local_general_sort;
		memset( local_general_sort, 0, sizeof(local_general_sort) );
	}

	index = atoi( v.argument[0] );
	if ( local_general_sort[index].set )
		warn("overwrite sorting condition[%d]", index);

	if ( strlen( v.argument[1] ) >= sizeof(local_general_sort[index].string) )
		warn("sort condition is too long. FieldSortingOrder %s %s",
				v.argument[0], v.argument[1]);

	local_general_sort[index].set = 1;
	strcpy( local_general_sort[index].string, v.argument[1] );
}

static config_t config[] = {
	CONFIG_GET("DocAttrField", get_docattr_field, 2, "docattr field : name type"),
	CONFIG_GET("RidField", get_rid_field, 1, "rid field name"),
	CONFIG_GET("Enum", get_enum, 2, "constant"),
	CONFIG_GET("FieldSortingOrder", get_field_sorting_order, 2,
			"FieldSortingOrder index field:(asc|desc);..."),
	{NULL}
};

static void register_hooks()
{
    sb_hook_docattr_mask_function(mask_function, NULL, NULL, HOOK_MIDDLE);
    sb_hook_docattr_sort_function(compare_function_for_qsort, NULL, NULL, HOOK_MIDDLE);
//  sb_hook_docattr_hit_sort_function(compare_hit_for_qsort, NULL, NULL, HOOK_MIDDLE);

    sb_hook_docattr_set_docattr_function(set_docattr_function, NULL, NULL, HOOK_MIDDLE);
    sb_hook_docattr_get_docattr_function(get_docattr_function, NULL, NULL, HOOK_MIDDLE);
//    sb_hook_docattr_set_doccond_function(set_doccond_function, NULL, NULL, HOOK_MIDDLE);
    sb_hook_docattr_set_docmask_function(set_docmask_function, NULL, NULL, HOOK_MIDDLE);
}

module docattr_general_module = {
	STANDARD_MODULE_STUFF,
	config,                /* config */
	NULL,                  /* registry */
	init,                  /* initialize */
	NULL,                  /* child_main */
	NULL,                  /* scoreboard */
	register_hooks         /* register hook api */
};

