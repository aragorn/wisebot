/* $Id$ */
#ifndef CDM_H
#define CDM_H 1


#include "softbot.h"
#include "mod_api/vbm.h"

/* canned document manager status code */
#define CDM_UNKNOWN_DATABASE		(-1)
#define CDM_STORAGE_FULL			(-2)
#define CDM_NOT_WELL_FORMED_DOC		(-3)
#define CDM_ALREADY_EXIST			(-4)
#define CDM_NOT_EXIST				(-5)
#define CDM_TRY_AGAIN				(-6)
#define CDM_DELETED					(-7)

#define MAX_NUM_RETRIEVED_DOC		COMMENT_LIST_SIZE
//#define MAX_FIELD_NUM				32
#define MAX_FIELD_NAME_LEN			STRING_SIZE

#ifndef PARAGRAPH_POSITION
#define PARAGRAPH_POSITION 1
#endif
//#ifdef PARAGRAPH_POSITION
//#undef PARAGRAPH_POSITION
//#endif

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

#define QP_MAX_NUM_ABSTRACT_INFO    32
typedef struct {
    unsigned long docId;
    int rank;
    float rsv;
    int numAbstractInfo;
    CdAbstractInfo cdAbstractInfo[QP_MAX_NUM_ABSTRACT_INFO];
}  RetrievedDoc;

#if defined (__cplusplus)
extern "C" {
#endif

SB_DECLARE_HOOK(int,server_canneddoc_init,())
SB_DECLARE_HOOK(int,server_canneddoc_close,())
SB_DECLARE_HOOK(int,server_canneddoc_put, \
	(DocId docid, VariableBuffer* pDocument))
SB_DECLARE_HOOK(int,server_canneddoc_put_with_oid, \
	(char *oid, DocId *registeredDocid, VariableBuffer* pDocument))
SB_DECLARE_HOOK(int,server_canneddoc_get, \
	(DocId docid, VariableBuffer* pDocument))	
SB_DECLARE_HOOK(int,server_canneddoc_get_as_pointer, \
	(DocId docid, void* pDocument, int size))	
SB_DECLARE_HOOK(int,server_canneddoc_get_size, (DocId docid))	
SB_DECLARE_HOOK(int,server_canneddoc_get_abstract,\
	(int numRetrievedDoc,RetrievedDoc aDoc[],VariableBuffer* pDocument))
SB_DECLARE_HOOK(DocId,server_canneddoc_last_registered_id,())
SB_DECLARE_HOOK(int,server_canneddoc_delete,(DocId docid))
SB_DECLARE_HOOK(int,server_canneddoc_update, \
	(DocId docid, VariableBuffer* pDocument))	
SB_DECLARE_HOOK(int,server_canneddoc_drop,())


#if defined (__cplusplus)
}
#endif

#endif
