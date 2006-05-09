/* $Id$ */
#include "common_core.h"
#include "cdm2.h"

HOOK_STRUCT(
	HOOK_LINK(cdm_open)
	HOOK_LINK(cdm_close)
	HOOK_LINK(cdm_get_doc)
	HOOK_LINK(cdmdoc_get_field_count)
	HOOK_LINK(cdmdoc_get_field_names)
	HOOK_LINK(cdmdoc_get_field)
	HOOK_LINK(cdmdoc_get_field_by_bytepos)
	HOOK_LINK(cdmdoc_get_field_by_wordpos)
	HOOK_LINK(cdmdoc_update_field)
	HOOK_LINK(cdmdoc_destroy)
	HOOK_LINK(cdm_delete_doc)
	HOOK_LINK(cdm_put_xmldoc)
	HOOK_LINK(cdm_get_xmldoc)
	HOOK_LINK(cdm_last_docid)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdm_open,
		(cdm_db_t** cdm_db, int opt), (cdm_db, opt), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdm_close,
		(cdm_db_t* cdm_db), (cdm_db), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdm_get_doc,
		(cdm_db_t* cdm_db, uint32_t docid, cdm_doc_t** doc), (cdm_db, docid, doc), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_get_field_count,
		(cdm_doc_t* cdm_doc), (cdm_doc), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_get_field_names,
		(cdm_doc_t* cdm_doc, char** fieldnames, int max_field_count, char* buf, size_t size),
		(cdm_doc, fieldnames, max_field_count, buf, size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_get_field,
		(cdm_doc_t* cdm_doc, const char* fieldname, char* buf, size_t size),
		(cdm_doc, fieldname, buf, size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_get_field_by_bytepos,
		(cdm_doc_t* cdm_doc, const char* fieldname, int bytepos, char* buf, size_t size),
		(cdm_doc, fieldname, bytepos, buf, size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_get_field_by_wordpos,
		(cdm_doc_t* cdm_doc, const char* fieldname, int wordpos, char* buf, size_t size),
		(cdm_doc, fieldname, wordpos, buf, size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_update_field,
		(cdm_doc_t* cdm_doc, char* fieldname, char* buf, size_t size),
		(cdm_doc, fieldname, buf, size), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdmdoc_destroy,
		(cdm_doc_t* cdm_doc), (cdm_doc), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdm_delete_doc,
		(cdm_db_t* cdm_db, uint32_t docid), (cdm_db, docid), DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, cdm_put_xmldoc,
		(cdm_db_t* cdm_db, did_db_t* did_db, char* oid, const char* xmldoc, size_t size,
		 uint32_t* newdocid, uint32_t* olddocid),
		(cdm_db, did_db, oid, xmldoc, size, newdocid, olddocid), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, cdm_get_xmldoc,
		(cdm_db_t* cdm_db, uint32_t docid, char* buf, size_t size),
		(cdm_db, docid, buf, size), MINUS_DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(uint32_t, cdm_last_docid, (cdm_db_t* cdm_db), (cdm_db), ((uint32_t)-1))

