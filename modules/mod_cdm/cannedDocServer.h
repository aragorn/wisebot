/* $Id$ */
#ifndef _CANNED_DOC_PRIVATE_
#define _CANNED_DOC_PRIVATE_ 1

#include "softbot.h"

#include "mod_cdm/mod_cdm.h"
#include "mod_vbm/mod_vbm.h"
#include "mod_api/cdm.h"
#include "mod_api/vbm.h"

//#include "syncManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

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
	DocId docId;
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

int IsExistDoc(int fdIndexFile, DocId docId);
int InsertIndexElement(int fdIndexFile, IndexFileElement *pIndexFileElement);
int SelectIndexElement(int fdIndexFile, DocId docId, IndexFileElement *pIndexElement);
int DeleteIndexElement(int fdIndexFile, DocId docId);

#endif
