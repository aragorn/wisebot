/* $Id$ */
#ifndef _MOD_CDM_H_
#define _MOD_CDM_H_ 1

// private header

#include "softbot.h"
#include "mod_api/vbm.h"
#include "mod_api/cdm.h"

/* followings are definitions that's 
 * shared by mod_cdm, mod_cdmclt
 */
#define MAX_TAG_NAME                        32
#define MAX_PATH                            256
#define MAX_TAG_NUM                         32
#define ENOUGH_BUFFER                       64

#define INDEX_FILE_NAME                     "keys.idx"

#define SLAVE_PROCESS_NUM                   10
#define SLAVE_CHECK_ALARM_TIME              60

#ifndef MAX_FIELD_NUM
#define MAX_FIELD_NUM				32
#endif

int CDM_put(DocId docId, VariableBuffer *pCannedDoc);
int CDM_get(DocId docId, VariableBuffer *pCannedDoc);
int CDM_get_as_pointer(DocId docId, void *pCannedDoc, int size);
int
CDM_getAbstract(int            numRetrievedDoc, 
                RetrievedDoc   aRetrievedDoc[],
				VariableBuffer aCannedDoc[]);
#endif
