#ifndef MOD_DOCATTR_GENERAL2_H
#define MOD_DOCATTR_GENERAL2_H 1

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "mod_api/docattr2.h" /* docattr_t */

#if defined(AIX5)
# define SB_PRIu64 PRIu64
# define SB_PRIx64 PRIx64
#else
# define SB_PRIu64 "%"PRIu64
# define SB_PRIx64 "%"PRIx64
#endif

#define MAX_DOCATTR_FIELD 32

typedef long		docattr_integer;
typedef long		docattr_boolean;
typedef uint64_t	docattr_md5;

/* declars incomplete types */
typedef struct general_mask_t general_mask_t;
typedef enum docattr_field_type_t docattr_field_type_t;
typedef enum docattr_value_type_t docattr_value_type_t;
typedef struct docattr_value_t docattr_value_t;
typedef struct docattr_field_t docattr_field_t;
typedef struct docattr_operand_t docattr_operand_t;
typedef struct docattr_expr_t docattr_expr_t;
typedef struct docattr_list_t docattr_list_t;
typedef enum docattr_operand_type_t docattr_operand_type_t;
typedef struct general_at_t general_at_t;
typedef struct general_sort_t general_sort_t;

struct general_mask_t {
	int init; // 0�̸� id�� 0xff�� �ʱ�ȭ�ؾ� �Ѵ�.
	int id[MAX_DOCATTR_FIELD];
	docattr_t docattr;
}; // sizeof(docattr_mask_t) ���� �۾ƾ� �Ѵ�.

enum docattr_field_type_t {
	FIELD_INTEGER = 1,
	FIELD_BIT,
	FIELD_ENUM,
	FIELD_ENUMBIT,
	FIELD_MD5,
	FIELD_STRING
};

/*
 * VALUE_LIST
 *   docattr_operand_t �� value_type �� ���� �� �ִ� ���̴�.
 *   list �ϱ� �Ϲ����� value type �� �ƴ϶�� ���� ǥ���Ѵ�
 */

enum docattr_value_type_t {
	VALUE_LIST = -1,
	VALUE_INTEGER = 1,
	VALUE_MD5,
	VALUE_STRING,
	VALUE_BOOLEAN
};

struct docattr_value_t {
	union {
		char*               string;
		docattr_integer		integer;
		docattr_boolean		boolean;
		docattr_md5			md5;
	} v;
	char my_string[MAX_DOCATTR_ELEMENT_SIZE];
};

struct docattr_field_t {
	docattr_field_type_t field_type;
	docattr_value_type_t value_type;
	char name[SHORT_STRING_SIZE];

	int offset;
	int size;

	// bit field �� ���� �ǹ��ִ�.
	int bit_offset;
	int bit_size;

	int (*set_func)(docattr_t* docattr, docattr_field_t* field, char* value);
	void (*get_func)(docattr_t* docattr,
			docattr_field_t* field, docattr_value_t* value);
	void (*get_as_string_func)(docattr_t* docattr, docattr_field_t* field,
			docattr_value_t* value);
	int (*compare_func)(docattr_value_t* value1, docattr_value_t* value2);

	// get_func ����� �����ϴ� �ӽ� �����
	// �̷� �� �����Ƿ� ���� �ٸ� document�� ���� field �� ��������.. ���� �� �ȵǰ���...
	docattr_value_t value;
};

#define is_bit_field(field_type) ( field_type == FIELD_BIT || field_type == FIELD_ENUMBIT )
#define is_enum_field(field_type) ( field_type == FIELD_ENUM || field_type == FIELD_ENUMBIT )

extern int return_docattr_field(const char* name, docattr_field_t** field);
extern docattr_integer return_enum_value(const char* value);
extern char* return_enum_name(docattr_integer value);

/**************************************************************
 * docattr query processing & filtering�� �ʿ��� ���
 **************************************************************/

struct docattr_expr_t {
	// operand 2���� ����ؼ� result�� �����Ѵ�
	int (*exec_func)(docattr_expr_t* expr);
	char operator_string[10]; // &, | ����... ������ string �׳� ��¿�

	docattr_operand_t* operand1;
	docattr_operand_t* operand2;

	docattr_value_t result;
};

struct docattr_list_t {
	docattr_operand_t** operands; 
	int size;  // operands array �� ũ��
	int count; // list �� ������ �� operand ����
};

/*
 * OPERAND_LIST
 *   <in>, <common> operator �� �� list �� operand �� ������.
 *   list operand �� Ư���� ���� �����Ƿ�
 *   value_type = VALUE_LIST, result = NULL �� �Ǹ� ������� �ʴ´�.
 */

enum docattr_operand_type_t {
	OPERAND_VALUE = 1,
	OPERAND_FIELD,
	OPERAND_EXPR,
	OPERAND_LIST
};

struct docattr_operand_t {
	docattr_operand_type_t operand_type;
	docattr_value_type_t value_type;

	union {
		docattr_value_t value;
		docattr_field_t* field;
		docattr_expr_t expr;
		docattr_list_t* list;
	} o;

	docattr_value_t* result; // operand ��� ���(��, exec_func�� ���)
	                         // query ������ �� ��, value, field->value, expr->value�� �ϳ��� ����
	                         // list �� ���� NULL �̴�.
};

struct general_at_t {
	docattr_operand_t* root_operand;
	docattr_field_t *field_list[MAX_DOCATTR_FIELD]; // �̸� ����ؾ� �ϴ� field��
	int field_list_count;
};

extern int isNumber(const char* string, docattr_integer* number);
#endif // MOD_DOCATTR_GENERAL_H

