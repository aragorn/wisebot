/* $Id$ */
#ifndef CDM_H
#define CDM_H 1

#include "vbm.h"
#include "did.h"
#include "indexer.h" /* MAX_EXT_FIELD */

/* canned document manager status code */
#define CDM_UNKNOWN_DATABASE		(-1)
#define CDM_STORAGE_FULL			(-2) // put
#define CDM_NOT_WELL_FORMED_DOC		(-3) // put
#define CDM_ALREADY_EXIST			(-4)
#define CDM_NOT_EXIST				(-5)
#define CDM_TRY_AGAIN				(-6)
#define CDM_DELETED					(-7)
#define CDM_DELETE_OLD				(2) // put

#define MAX_NUM_RETRIEVED_DOC		COMMENT_LIST_SIZE
#define MAX_FIELD_NAME_LEN			STRING_SIZE

#ifndef PARAGRAPH_POSITION
#define PARAGRAPH_POSITION 1
#endif

/* 
 * if size is -1, return whole data of field 
 */

typedef struct {
    char field[MAX_FIELD_NAME_LEN];
#ifdef PARAGRAPH_POSITION
	short paragraph_position;
#endif
    long position;
    long size;
}  CdAbstractInfo;

typedef struct {
    unsigned long docId;
    int rank;
    float rsv;
    int numAbstractInfo;
    CdAbstractInfo cdAbstractInfo[MAX_EXT_FIELD];
}  RetrievedDoc;

#if defined (__cplusplus)
extern "C" {
#endif

SB_DECLARE_HOOK(int,server_canneddoc_init,())
SB_DECLARE_HOOK(int,server_canneddoc_close,())
SB_DECLARE_HOOK(int,server_canneddoc_put, \
	(uint32_t docid, VariableBuffer* pDocument))
SB_DECLARE_HOOK(int,server_canneddoc_put_with_oid, \
	(void* did_db, char *oid, uint32_t *registeredDocid,
	 uint32_t *deletedDocid, VariableBuffer* pDocument))
SB_DECLARE_HOOK(int,server_canneddoc_get, \
	(uint32_t docid, VariableBuffer* pDocument))	
SB_DECLARE_HOOK(int,server_canneddoc_get_as_pointer, \
	(uint32_t docid, void* pDocument, int size))	
SB_DECLARE_HOOK(int,server_canneddoc_get_size, (uint32_t docid))	
SB_DECLARE_HOOK(int,server_canneddoc_get_abstract,\
	(int numRetrievedDoc,RetrievedDoc aDoc[],VariableBuffer* pDocument))
SB_DECLARE_HOOK(uint32_t,server_canneddoc_last_registered_id,())
SB_DECLARE_HOOK(int,server_canneddoc_delete,(uint32_t docid))
SB_DECLARE_HOOK(int,server_canneddoc_update, \
	(uint32_t docid, VariableBuffer* pDocument))	
SB_DECLARE_HOOK(int,server_canneddoc_drop,())


#if defined (__cplusplus)
}
#endif

#endif
