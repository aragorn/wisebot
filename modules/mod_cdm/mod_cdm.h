/* $Id$ */
#ifndef _MOD_CDM_H_
#define _MOD_CDM_H_ 1

// private header

#include "mod_api/vbm.h"

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

int CDM_put(uint32_t docId, VariableBuffer *pCannedDoc);
int CDM_get(uint32_t docId, VariableBuffer *pCannedDoc);
int CDM_get_as_pointer(uint32_t docId, void *pCannedDoc, int size);
int
CDM_getAbstract(int            numRetrievedDoc, 
                RetrievedDoc   aRetrievedDoc[],
				VariableBuffer aCannedDoc[]);
#endif
