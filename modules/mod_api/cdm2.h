/* $Id$ */
#ifndef CDM2_H
#define CDM2_H 1

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#include "did.h"

/* canned document manager status code */
#define CDM2_NOT_ENOUGH_BUFFER       (-11)
#define CDM2_UPDATE_NOT_AVAILABLE    (-12)
#define CDM2_PUT_NOT_WELL_FORMED_DOC (-13)
#define CDM2_PUT_OID_DUPLICATED      (-14)
#define CDM2_GET_INVALID_DOCID       (-15)
#define CDM2_FIELD_NOT_EXISTS        (-16)

#define MAX_FIELD_NAME_LEN          STRING_SIZE

typedef struct _cdm_db_t
{
	int set;

	void* db;
} cdm_db_t;

typedef struct _cdm_doc_t
{
	cdm_db_t* cdm_db;
	uint32_t docid;
	uint32_t deleted;

	void* data;
} cdm_doc_t;

#define MAX_CDM_SET 10

SB_DECLARE_HOOK(int, cdm_open, (cdm_db_t** cdm_db, int opt))
SB_DECLARE_HOOK(int, cdm_close, (cdm_db_t* cdm_db))
SB_DECLARE_HOOK(int, cdm_get_doc, (cdm_db_t* cdm_db, uint32_t docid, cdm_doc_t** doc))
SB_DECLARE_HOOK(int, cdmdoc_get_field_count, (cdm_doc_t* doc))
SB_DECLARE_HOOK(int, cdmdoc_get_field_names, (cdm_doc_t* doc, char** fieldnames,
		int max_field_count, char* buf, size_t size))
SB_DECLARE_HOOK(int, cdmdoc_get_field, (cdm_doc_t* doc, const char* fieldname,
		char* buf, size_t size))
SB_DECLARE_HOOK(int, cdmdoc_get_field_by_bytepos, (cdm_doc_t* doc, const char* fieldname,
		int bytepos, char* buf, size_t size))
SB_DECLARE_HOOK(int, cdmdoc_get_field_by_wordpos, (cdm_doc_t* doc, const char* fieldname,
		int wordpos, char* buf, size_t size))
SB_DECLARE_HOOK(int, cdmdoc_update_field, (cdm_doc_t* doc, char* fieldname,
		char* buf, size_t size))
SB_DECLARE_HOOK(int, cdmdoc_destroy, (cdm_doc_t* doc))
SB_DECLARE_HOOK(int, cdm_delete_doc, (cdm_db_t* cdm_db, uint32_t docid))
SB_DECLARE_HOOK(int, cdm_put_xmldoc, (cdm_db_t* cdm_db, did_db_t* did_db, char* oid,
		const char* xmldoc, size_t size, uint32_t* newdocid, uint32_t* olddocid))
SB_DECLARE_HOOK(int, cdm_get_xmldoc, (cdm_db_t* cdm_db, uint32_t docid, char* buf, size_t size))
SB_DECLARE_HOOK(uint32_t, cdm_last_docid, (cdm_db_t* cdm_db))

#endif
