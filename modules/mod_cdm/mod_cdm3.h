#ifndef _MOD_CDM3_H_
#define _MOD_CDM3_H_

#include <inttypes.h> // for uint32_t
//#include "mod_api/did.h"
#include "mod_api/indexdb.h"

#define MAX_CDM_FILE_COUNT 256

typedef struct {
	uint32_t last_docid;
} cdm_db_shared_t;

typedef struct {
	cdm_db_shared_t* shared;

    int lock_id;
	index_db_t* ifs;
} cdm_db_custom_t;

typedef struct {
	// indexHandle.c
	unsigned long dwDBNo;
	unsigned long offset;
	unsigned long length;

	int shortfield_count;
	char* shortfield_names[MAX_FIELD_NUM];
	char* shortfield_values[MAX_FIELD_NUM];
	// string length가 아니고, field value가 차지할 수 있는 실제공간. \0은 제외
	int shortfield_size[MAX_FIELD_NUM];

	int longfield_count;
	char* longfield_names[MAX_FIELD_NUM];
	uint32_t longfield_textpositions[MAX_FIELD_NUM]; // see wiki [mod_cdm2]
	uint32_t longfield_nextpositions[MAX_FIELD_NUM]; // see wiki [mod_cdm2]

	// longfield_bodies가 시작하는 위치. see wiki [mod_cdm2]
	size_t longfield_bodies_start_offset;

	char* data; // shortstring_fields, longfield_headers
} cdm_doc_custom_t;

typedef struct {
	int set;

	// always set with default
	char cdm_path[MAX_PATH_LEN];
	char shared_file[MAX_PATH_LEN];
    int lock_id;
} cdm_db_set_t;

/////////////////////////////////////////////////////////////
// for config Field, DocAttrField, FieldRootName
struct cdmfield_t {
	char name[SHORT_STRING_SIZE];
	enum field_type_t { SHORT, LONG } type;
	int size;   // for SHORT type
	int is_index;     // if non-zero, not updatable
	int is_docattr;   // if non-zero, true
};

/*extern struct cdmfield_t fields[MAX_FIELD_NUM];
extern int field_count;

extern char docattr_fields[MAX_FIELD_NUM][SHORT_STRING_SIZE];
extern int docattr_field_count;

extern char field_root_name[SHORT_STRING_SIZE];*/
/////////////////////////////////////////////////////////////

#endif // _MOD_CDM2_H_

