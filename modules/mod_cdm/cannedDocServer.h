/* $Id$ */
#ifndef _CANNED_DOC_PRIVATE_
#define _CANNED_DOC_PRIVATE_ 1

#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>

//#include "mod_api/cdm.h"

#define MAX_PARA_NUM		16

/* file name handling */
#define GET_DIT_PATH(a,b,c)					sprintf(a, "%scdm%06u.db", b, c)
#define GET_IDX_PATH(a,b)					sprintf(a, "%s%s", b, INDEX_FILE_NAME)
#define GET_LOG_PATH(a)						sprintf(a, "%s%s", 

/**
 * type definition of structure 'IndexFileElement' 
 * --- keys.idx ---
 */
typedef struct _INDEXFILEELEMENT {
	uint32_t docId;
	unsigned long dwDBNo;
	unsigned long offset;
	unsigned long length;
//#ifdef PARAGRAPH_POSITION
//	uint16_t para[MAX_PARA_NUM];
//#endif
} IndexFileElement;

/**
 * private methods(functions) declairation 
 */
//int ParseCannedDoc(const char *aCannedDoc, Element *pElements);
//int CheckCannedDoc(const char *aCannedDoc);

int IsExistDoc(int fdIndexFile, uint32_t docId);
int InsertIndexElement(int fdIndexFile, IndexFileElement *pIndexFileElement);
int SelectIndexElement(int fdIndexFile, uint32_t docId, IndexFileElement *pIndexElement);
int DeleteIndexElement(int fdIndexFile, uint32_t docId);

#endif
