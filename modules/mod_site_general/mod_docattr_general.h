#ifndef MOD_DOCATTR_GENERAL_H
#define MOD_DOCATTR_GENERAL_H 1

#include "mod_api/docattr.h"
#include <inttypes.h>

#define MAX_DOCATTR_FIELD 32

typedef long		docattr_integer;
typedef long		docattr_boolean;
typedef uint64_t	docattr_md5;

typedef struct {
	int init; // 0�̸� id�� 0xff�� �ʱ�ȭ�ؾ� �Ѵ�.
	int id[MAX_DOCATTR_FIELD];
	docattr_t docattr;
} general_mask_t; // sizeof(docattr_mask_t) ���� �۾ƾ� �Ѵ�.

typedef enum {
	FIELD_INTEGER = 1,
	FIELD_BIT,
	FIELD_ENUM,
	FIELD_ENUM8,
	FIELD_ENUMBIT,
	FIELD_MD5,
	FIELD_STRING
} docattr_field_type_t;

typedef enum {
	VALUE_INTEGER = 1,
	VALUE_MD5,
	VALUE_STRING,
	VALUE_BOOLEAN
} docattr_value_type_t;

typedef struct _docattr_value_t {
	union {
		char*               string;
		docattr_integer		integer;
		docattr_boolean		boolean;
		docattr_md5			md5;
	} v;
	char my_string[MAX_DOCATTR_ELEMENT_SIZE];
} docattr_value_t;

typedef struct _docattr_field_t {
	docattr_field_type_t field_type;
	docattr_value_type_t value_type;
	char name[SHORT_STRING_SIZE];

	int offset;
	int size;

	// bit field �� ���� �ǹ��ִ�.
	int bit_offset;
	int bit_size;

	int (*set_func)(docattr_t* docattr, struct _docattr_field_t* field, char* value);
	void (*get_func)(docattr_t* docattr,
			struct _docattr_field_t* field, docattr_value_t* value);
	void (*get_as_string_func)(docattr_t* docattr, struct _docattr_field_t* field,
			docattr_value_t* value);
	int (*compare_func)(docattr_value_t* value1, docattr_value_t* value2);

	// get_func ����� �����ϴ� �ӽ� �����
	// �̷� �� �����Ƿ� ���� �ٸ� document�� ���� field �� ��������.. ���� �� �ȵǰ���...
	docattr_value_t value;
} docattr_field_t;

extern docattr_field_t docattr_field[MAX_DOCATTR_FIELD];
extern int docattr_field_count;
extern int return_docattr_field(const char* name, docattr_field_t** field);

/**************************************************************
 * docattr query processing & filtering�� �ʿ��� ���
 **************************************************************/

typedef struct _docattr_operand_t docattr_operand_t;
typedef struct _docattr_expr_t docattr_expr_t;

struct _docattr_expr_t {
	// operand 2���� ����ؼ� result�� �����Ѵ�
	int (*exec_func)(docattr_expr_t* expr);
	char operator_string[10]; // &, | ����... ������ string �׳� ��¿�

	struct _docattr_operand_t* operand1;
	struct _docattr_operand_t* operand2;

	docattr_value_t result;
};

typedef enum {
	OPERAND_VALUE = 1,
	OPERAND_FIELD,
	OPERAND_EXPR
} docattr_operand_type_t;

struct _docattr_operand_t {
	docattr_operand_type_t operand_type;
	docattr_value_type_t value_type;

	union {
		docattr_value_t value;
		docattr_field_t* field;
		docattr_expr_t expr;
	} o;

	docattr_value_t* result; // operand ��� ���(��, exec_func�� ���)
	                         // query ������ �� ��, value, field->value, expr->value�� �ϳ��� ����
};

typedef struct {
	docattr_operand_t* root_operand;
	docattr_field_t *field_list[MAX_DOCATTR_FIELD]; // �̸� ����ؾ� �ϴ� field��
	int field_list_count;
} general_cond_t;

extern int isNumber(const char* string, docattr_integer* number);

/**************************************************************
 * docattr sorting�� �ʿ��� ���
 **************************************************************/

typedef struct {
	int set; // 0 �̸� �ʱ�ȭ ���� ���� ��.
	char string[STRING_SIZE];

	// �� �κ��� init() ���� �ʱ�ȭ.. field�� ���� �� ���� �����ϱ�..
	struct {
		docattr_field_t* field;
		int order;
	} condition[MAX_SORTING_CRITERION];
	int condition_count;
} general_sort_t;

#endif // MOD_DOCATTR_GENERAL_H

