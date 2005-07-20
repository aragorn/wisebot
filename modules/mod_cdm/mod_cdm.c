/* $Id$
 * created by YoungHoon  
 */

#include <strings.h>
#include "cannedDocServer.h"
#include "mod_api/morpheme.h"
#include "mod_api/xmlparser.h"
#include "mod_api/docattr.h"
#include "mod_api/did.h"

#ifdef CYGWIN
#include "expat.h"
#else
#include "expat/expat.h"
#endif

#define MAX_FILE_SIZE		250000000

#define TOTAL_LOCK_NUM		(dwMaxDBFileNum + 1)
#define TOTAL_COND_NUM		0

#define INDEX_LOCK			0
#define DIT_LOCK(a)			(int)(a + 1)

#define BUFFERING_SIZE		2048000


#define PROCESS_HANDLE
//#undef PROCESS_HANDLE

/* other system parameters */
char aDBPath[MAX_PATH] = "dat/cdm/";
unsigned long dwMaxDBFileSize = MAX_FILE_SIZE;
unsigned long dwMaxDBFileNum = 100;
unsigned long dwMaxDocNum = (int)(MAX_FILE_SIZE/ sizeof(IndexFileElement));

static char fieldRootName[MAX_FIELD_NAME_LEN] = "Document";
static char *docattrFields[MAX_FIELD_NUM] = { NULL };

/* file descriptors */
static int fdIndexFile = -1;
static int *fdDBFile = NULL;

static struct cdm_shared_t {
	uint16_t currentDBNo;
	uint32_t    lastDocId;
	int      cdm_stat[6];
} *cdm_shared = NULL;
static char cdm_shared_file[MAX_FILE_LEN] = "dat/cdm/cdm.shared";

/* 
 * stat[0] : 0 - 16k
 * stat[1] : 17k - 32k
 * stat[2] : 33k - 64k
 * stat[3] : 65k - 128k
 * stat[4] : 129k - 512k
 * stat[5] : 513k -
 */

//static SyncInfo syncInfo;
static int CDM_getWithIndexElement(uint32_t docId, VariableBuffer *pCannedDoc,
		IndexFileElement *indexElement);

/**
 * InitIndexFile
 * 인덱스파일을 초기화한다.
 
 * return		: 성공하면 SUCCESS(1), 실패하면 FAIL(-1)
 */
static int InitIndexFile() {
	char pTmp[MAX_PATH];
	off_t offset;
	
	GET_IDX_PATH(pTmp, aDBPath);
	fdIndexFile = sb_open(pTmp, O_CREAT | O_RDWR | O_SYNC, 0666);
	if (fdIndexFile == -1) {
		error("cannot open file: %s", pTmp);
		return FAIL;
	}

	offset = lseek(fdIndexFile, dwMaxDocNum * sizeof(IndexFileElement), SEEK_SET);
	if (offset == (off_t)-1) {
		error("disk size is too small to allocate index file (key.idx) whose size is %ld.",
				dwMaxDocNum * sizeof(IndexFileElement));
		return FAIL;
	}

	pTmp[0] = '\0';
	if (write(fdIndexFile, pTmp, 1) == -1) {
		error("fail write: %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;

}

static int InitDBFiles() {
	int iCount;
	char pTmp[MAX_PATH];
	
	fdDBFile = (int *)sb_malloc(dwMaxDBFileNum * sizeof(int));
	if (fdDBFile == NULL) {
		error("cannot allocate memory for file descriptor of cdm db file");
		return FAIL;
	}
	bzero(fdDBFile, dwMaxDBFileNum * sizeof(int));
	
	for (iCount=0; iCount<dwMaxDBFileNum; iCount++) {
		GET_DIT_PATH(pTmp, aDBPath, iCount);
		fdDBFile[iCount] = sb_open(pTmp, O_CREAT | O_RDWR | O_SYNC, 0666);
		if (fdDBFile[iCount] == -1) {
			error("cannot open cdm db file[%d]", iCount);
			return FAIL;
		}
	}
	
	return SUCCESS;
}

/**
 * CDM_init
 * 문서 CannedDoc.doc 참조
 */
static int CDM_init() {
	int iResult; //, iCount;
	ipc_t ipc;

	ipc.type = IPC_TYPE_MMAP;
	ipc.pathname = cdm_shared_file;
	ipc.size = sizeof(struct cdm_shared_t);

	iResult = alloc_mmap(&ipc, 0);
	if (iResult < 0) {
		error("alloc mmap to cdm_shared failed");
		return FAIL;
	}
	cdm_shared = (struct cdm_shared_t*) ipc.addr;

	if ( ipc.attr == MMAP_CREATED )
		memset( cdm_shared, 0, ipc.size );

	// initialize index file
	iResult = InitIndexFile();
	if (iResult < 0) {
		error("error in InitIndexFile()");
		return FAIL;
	}

	// initialize dit files
	iResult = InitDBFiles();
	if (iResult < 0) {
		error("error in InitDBFiles()");
		return FAIL;
	}

	return SUCCESS;
}

static int CDM_close() {
#if 0
	int iCount, iResult;

	/////////////////////////////////////////////////////
	// close index file

	// lock index file
	if (SYNC_lock(&syncInfo, INDEX_LOCK) < 0) {
		error("error in SYNC_lock()");
		return FAIL;
	}

	close (fdIndexFile);

	// unlock index file
	if (SYNC_unlock(&syncInfo, INDEX_LOCK) < 0) {
		error("error in SYNC_lock()");
		return FAIL;
	}

	//////////////////////////////////////////////////////
	// close db(dit) files

	// lock dit file
	for (iCount=0; iCount<dwMaxDBFileNum; iCount++) {
		iResult = SYNC_lock(&syncInfo, DIT_LOCK(iCount));
		if (iResult < 0) {
			error("error in SYNC_lock()");
			return FAIL;
		}

		close(fdDBFile[iCount]);
	}

	// unlock dit file
	for (iCount=0; iCount<dwMaxDBFileNum; iCount++) {
		iResult = SYNC_unlock(&syncInfo, DIT_LOCK(iCount));
		if (iResult < 0) {
			error("error in SYNC_unlock()");
			return FAIL;
		}
	}

	return SUCCESS;
#endif
	return FAIL;
}

/**
 * CDM_put
 * 문서 CannedDoc.doc 참조
 */
char *_trim(char *str, int *len)
{
	char *tmp, *start;

	if (*len == 0) return str;

	start = str;
	for (tmp=str; tmp<str+*len; tmp++) {
		if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
			start = tmp;
			break;
		}
	}
	if (tmp == str + *len) {
		*len = 0;
		return str;
	}
	for (tmp=str+*len-1; tmp>=start; tmp--) {
		if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
			break;
		}
	}
	*len = tmp - start + 1;
	return start;
}

int CDM_put(uint32_t docId, VariableBuffer *pCannedDoc) {
	int iResult, ditNo;
	long iSize, dwCurrentDBOffset;
	IndexFileElement indexElement;
	char path[STRING_SIZE];
	parser_t *p;
	field_t *f;
	//Paragraph para;
	//char *paragraph;
	//int idx_paragraph, paralen;
    int i;

#ifdef PROCESS_HANDLE
	static char *aCannedDoc = NULL;
	if (aCannedDoc == NULL) {
		aCannedDoc = (char *)sb_malloc(DOCUMENT_SIZE);
		if (aCannedDoc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
#else
	char aCannedDoc[DOCUMENT_SIZE];
#endif

	// seek dit file
	ditNo = cdm_shared->currentDBNo;

	// get canned doc from variable buffer
	iSize = sb_run_buffer_getsize(pCannedDoc);
	if (iSize < 0) {
		error("error in buffer_getsize()");
		return FAIL;
	}

	if (iSize >= DOCUMENT_SIZE) {
		warn("size of document[%u] exceed maximum limit; fail to register doc",
				docId);
		return FAIL;
	}

	if (iSize < 16 * 1024) {
		cdm_shared->cdm_stat[0]++;
	}
	else if (iSize < 32 * 1024) {
		cdm_shared->cdm_stat[1]++;
	}
	else if (iSize < 64 * 1024) {
		cdm_shared->cdm_stat[2]++;
	}
	else if (iSize < 128 * 1024) {
		cdm_shared->cdm_stat[3]++;
	}
	else if (iSize < 512 * 1024) {
		cdm_shared->cdm_stat[4]++;
	}
	else {
		cdm_shared->cdm_stat[5]++;
	}

	iResult = sb_run_buffer_get(pCannedDoc, 0, iSize, aCannedDoc);
	if (iResult < 0) {
		error("error in buffer_get()");
		return FAIL;
	}
	aCannedDoc[iSize] = '\0';

	/* parse cdm */
	p = sb_run_xmlparser_parselen("CP949", aCannedDoc, iSize);
	if (p == NULL) {
		error("1. cannot parse document[%u]", docId);
		return FAIL;
	}

	{ /* insert some field into docattr db */
		docattr_t docattr;
		int len;
		char *val, value[STRING_SIZE];

		DOCATTR_SET_ZERO(&docattr);
		for (i=0; i<MAX_FIELD_NUM && docattrFields[i]; i++) {
			strcpy(path, "/");
			strcat(path, fieldRootName);
			strcat(path, "/");
			strcat(path, docattrFields[i]);

			f = sb_run_xmlparser_retrieve_field(p, path);
			if (f == NULL) {
				warn("cannot get field[/%s/%s] of ducument[%u] (path:%s)", 
						fieldRootName, 
						docattrFields[i], docId, path);
				continue;
			}

			len = f->size;
			val = _trim(f->value, &len);
			len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
			strncpy(value, val, len);
			value[len] = '\0';

			if (len == 0) {
				continue;
			}

			if(sb_run_docattr_set_docattr_function(&docattr, docattrFields[i],
						value) == -1) {
				warn("wrong type of value of field[/%s/%s] of ducument[%u]", 
						fieldRootName, 
						docattrFields[i], docId);
			}
		}

		if (sb_run_docattr_set(docId, &docattr) == -1) {
			warn("cannot insert field[/%s/%s/] into docattr db", fieldRootName,
					docattrFields[i]);
		}
	}

	sb_run_xmlparser_free_parser(p);

	if (wr_lock(fdDBFile[ditNo], SEEK_SET, 0, 0) == -1) {
		error("cannot flock[%d]: %s", fdDBFile[ditNo], strerror(errno));
		return FAIL;
	}

	dwCurrentDBOffset = (long)lseek(fdDBFile[ditNo], 0, SEEK_END);
	if (dwCurrentDBOffset == -1) {
		error("error in lseek()");
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	while (dwCurrentDBOffset + iSize > dwMaxDBFileSize) {
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);

		cdm_shared->currentDBNo++;
		ditNo = cdm_shared->currentDBNo;

		if (ditNo == dwMaxDBFileNum) {
			error("database is full! (dit file number needs to modified)");
			return FAIL;
		}

		if (wr_lock(fdDBFile[ditNo], SEEK_SET, 0, 0) == -1) {
			error("cannot flock[%d]: %s", fdDBFile[ditNo], strerror(errno));
			return FAIL;
		}

		dwCurrentDBOffset = lseek(fdDBFile[ditNo], 0, SEEK_END);
		if (dwCurrentDBOffset == (off_t)-1) {
			error("error in lseek()");
			un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
			return FAIL;
		}
	}
	/* insert infomation about this document to index file */
	indexElement.docId = docId;
	indexElement.dwDBNo = ditNo;
	indexElement.offset = dwCurrentDBOffset;
	indexElement.length = iSize;

	if (wr_lock(fdIndexFile, SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdIndexFile, strerror(errno));
		return FAIL;
	}

	iResult = InsertIndexElement(fdIndexFile, &indexElement);
	if (iResult < 0) {
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
		un_lock(fdIndexFile, SEEK_SET, 0, 0);
		return iResult;
	}

	un_lock(fdIndexFile, SEEK_SET, 0, 0);

	iResult = write(fdDBFile[ditNo], aCannedDoc, iSize);
	if (iResult == -1) {
		error("fail write: %s", strerror(errno));
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);

	// FIXME -- lock lastDocId also!!!
	// set last registered docid
	if (cdm_shared->lastDocId < docId) {
		cdm_shared->lastDocId = docId;
	}

	return iResult;
}

int CDM_putWithOid(void* did_db, char *oid, uint32_t *registeredDocId, VariableBuffer *pCannedDoc) {
	int iResult, ditNo;
	long iSize, dwCurrentDBOffset;
	IndexFileElement indexElement;
	char path[STRING_SIZE];
	parser_t *p;
	field_t *f;
	//char *paragraph;
	//int idx_paragraph, paralen;
	int i;

	int len;
	char *val, value[STRING_SIZE];
	docattr_mask_t docmask;

#ifdef PROCESS_HANDLE
	static char *aCannedDoc = NULL;
	if (aCannedDoc == NULL) {
		aCannedDoc = (char *)sb_malloc(DOCUMENT_SIZE);
		if (aCannedDoc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
		info("malloc %d bytes for aCannedDoc at %p", DOCUMENT_SIZE, aCannedDoc);
	}
#else
	char aCannedDoc[DOCUMENT_SIZE];
	info("local CannedDoc");
#endif

	// seek dit file
	ditNo = cdm_shared->currentDBNo;

	// get canned doc from variable buffer
	iSize = sb_run_buffer_getsize(pCannedDoc);
	if (iSize < 0) {
		error("error in buffer_getsize()");
		goto return_fail;
	}

	if (iSize >= DOCUMENT_SIZE) {
		warn("size of document[%s] exceed maximum limit; fail to register doc",
				oid);
		goto return_fail;
	}

	if (iSize < 16 * 1024) {
		cdm_shared->cdm_stat[0]++;
	}
	else if (iSize < 32 * 1024) {
		cdm_shared->cdm_stat[1]++;
	}
	else if (iSize < 64 * 1024) {
		cdm_shared->cdm_stat[2]++;
	}
	else if (iSize < 128 * 1024) {
		cdm_shared->cdm_stat[3]++;
	}
	else if (iSize < 512 * 1024) {
		cdm_shared->cdm_stat[4]++;
	}
	else {
		cdm_shared->cdm_stat[5]++;
	}

	iResult = sb_run_buffer_get(pCannedDoc, 0, iSize, aCannedDoc);
	if (iResult < 0) {
		error("error in buffer_get()");
		goto return_fail;
	}
	aCannedDoc[iSize] = '\0';

	/*******************************
	 * write document in data file
	 */
	if (wr_lock(fdDBFile[ditNo], SEEK_SET, 0, 0) == -1) {
		error("cannot flock[%d]: %s", fdDBFile[ditNo], strerror(errno));
		goto return_fail;
	}

	dwCurrentDBOffset = (long)lseek(fdDBFile[ditNo], 0, SEEK_END);
	if (dwCurrentDBOffset == -1) {
		error("error in lseek()");
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
		goto return_fail;
	}
	while (dwCurrentDBOffset + iSize > dwMaxDBFileSize) {
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);

		cdm_shared->currentDBNo++;
		ditNo = cdm_shared->currentDBNo;

		if (ditNo == dwMaxDBFileNum) {
			error("database is full! (dit file number needs to modified)");
		    goto return_fail;
		}

		if (wr_lock(fdDBFile[ditNo], SEEK_SET, 0, 0) == -1) {
			error("cannot flock[%d]: %s", fdDBFile[ditNo], strerror(errno));
		    goto return_fail;
		}

		dwCurrentDBOffset = lseek(fdDBFile[ditNo], 0, SEEK_END);
		if (dwCurrentDBOffset == (off_t)-1) {
			error("error in lseek()");
			un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
		    goto return_fail;
		}
	}
	/* insert infomation about this document to index file */
	indexElement.dwDBNo = ditNo;
	indexElement.offset = dwCurrentDBOffset;
	indexElement.length = iSize;

	iResult = write(fdDBFile[ditNo], aCannedDoc, iSize);
	if (iResult == -1) {
		error("fail write: %s", strerror(errno));
		un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);
	    goto return_fail;
	}

	un_lock(fdDBFile[ditNo], SEEK_SET, 0, 0);

	/*********************************
	 * prepare docattr db
	 */

	/* parse cdm */
	p = sb_run_xmlparser_parselen("CP949", aCannedDoc, iSize);
	if (p == NULL) {
		error("2. cannot parse document[%s]", oid);
	    goto return_fail;
	}
	else if (p == (parser_t*)1) {
		warn("sb_run_xmlparser_parselen() returned DECLINE(1)");
		goto return_fail;
	}


	DOCMASK_SET_ZERO(&docmask);
	for (i=0; i<MAX_FIELD_NUM && docattrFields[i]; i++) {
		strcpy(path, "/");
		strcat(path, fieldRootName);
		strcat(path, "/");
		strcat(path, docattrFields[i]);

		f = sb_run_xmlparser_retrieve_field(p, path);
		if (f == NULL) {
			warn("cannot get field[/%s/%s] of ducument[%s] (path:%s)", 
					fieldRootName, 
					docattrFields[i], oid, path);
			continue;
		}
		else if ( f == (field_t*)1 ) {
			warn("sb_run_xmlparser_retrieve_field() returned DECLINE(1)");
			continue;
		}

		len = f->size;
		val = _trim(f->value, &len);
		len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
		strncpy(value, val, len);
		value[len] = '\0';

		if (len == 0) {
			continue;
		}

		if ( sb_run_docattr_set_docmask_function(
					&docmask, docattrFields[i], value) != SUCCESS ) {
			warn("wrong type of value of field[/%s/%s] of ducument[%s]", 
					fieldRootName, docattrFields[i], oid);
		}
	}
	sb_run_xmlparser_free_parser(p);

	/**************************
	 * write index file
	 */
	if (wr_lock(fdIndexFile, SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdIndexFile, strerror(errno));
	    goto return_fail;
	}

	/* create docid */
	{
		uint32_t docid=0, olddocid=0;
		docattr_mask_t docmask;

		iResult = sb_run_get_new_docid(did_db, oid, &docid, &olddocid); 
		if ( iResult < 0 ) {
			error ("cannot get new document id of oid[%s]:error(%d)", 
					oid, iResult);
			un_lock(fdIndexFile, SEEK_SET, 0, 0);
	        goto return_fail;
		}

		if (iResult == DOCID_OLD_REGISTERED) {
			info("old docid[%u] of OID[%s] is deleted. new docid is %u", 
					olddocid, oid, docid);
			DOCMASK_SET_ZERO(&docmask);
			sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
			sb_run_docattr_set_array(&olddocid, 1, SC_MASK, &docmask);
		}
		else {
			info("OID[%s] is registered by docid[%u]", oid, docid);
		}

		*registeredDocId = docid;
	}

	/* write index file */
	indexElement.docId = *registeredDocId;

	iResult = InsertIndexElement(fdIndexFile, &indexElement);
	if (iResult < 0) {
		un_lock(fdIndexFile, SEEK_SET, 0, 0);
	    goto return_fail;
		//return iResult;
	}
	/* set last registered docid */
	if (cdm_shared->lastDocId + 1 != *registeredDocId) {
		crit("last docid of did daemon is not equal to lastdocid of registry");
	}

	if (cdm_shared->lastDocId < *registeredDocId) {
		cdm_shared->lastDocId = *registeredDocId;
	}

	un_lock(fdIndexFile, SEEK_SET, 0, 0);

	if ( sb_run_docattr_set_array(
				registeredDocId, 1, SC_MASK, &docmask) != SUCCESS ) {
		warn("cannot insert field[/%s/%s/] into docattr db", fieldRootName,
				docattrFields[i]);
	}

#ifdef PROCESS_HANDLE
	/* XXX aCannedDoc is a static value. no need to free this.
	if (aCannedDoc != NULL) {
		sb_free(aCannedDoc);
		aCannedDoc = 0x00;
	}
	*/
#endif
	return iResult;

return_fail:
#ifdef PROCESS_HANDLE
	/* if (aCannedDoc != NULL) {
		sb_free(aCannedDoc);
		aCannedDoc = 0x00;
	} */
#endif
	return FAIL;
}

int CDM_getSize(uint32_t docId)
{
	int iResult;
	docattr_t attr;
	char buf[SHORT_STRING_SIZE];
	IndexFileElement indexElement;

	/* chech docattr */
	if (sb_run_docattr_get(docId, &attr) == -1) {
		error("canot get docattr");
		return FAIL;
	}

	if (sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE)
			== -1) {
		error("cannot get value of Delete field");
		return FAIL;
	}

	if (buf[0] == '1') {
		info("document[%u] is deleted", docId);
		return FAIL;
	}

	iResult = SelectIndexElement(fdIndexFile, docId, &indexElement);
	if (iResult == FAIL) {
		warn("no document retrieved by docid[%u]", docId);
		return FAIL;
	}

	return indexElement.length;
}

/**
 * CDM_get
 * 문서 CannedDoc.doc 참조
 */
int CDM_get(uint32_t docId, VariableBuffer *pCannedDoc)
{
	IndexFileElement ele;
	return CDM_getWithIndexElement(docId, pCannedDoc, &ele);
}

static int CDM_getWithIndexElement(uint32_t docId, VariableBuffer *pCannedDoc, IndexFileElement *indexElement) {
	int iResult;
/*	IndexFileElement indexElement;*/
	docattr_t attr;
#ifdef _KHYANG_
	char buf[SHORT_STRING_SIZE];
#endif

#ifdef PROCESS_HANDLE
	static char *aCannedDoc = NULL;
	if (aCannedDoc == NULL) {
		aCannedDoc = (char *)sb_malloc(DOCUMENT_SIZE);
		if (aCannedDoc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
#else
	char aCannedDoc[DOCUMENT_SIZE];
#endif

	/* chech docattr */
	if (sb_run_docattr_get(docId, &attr) == -1) {
		error("canot get docattr");
		return FAIL;
		//goto INGNORE_DOCATTR;
	}

#ifdef _KHYANG_
	if (sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE)
			== -1) {
		error("cannot get value of Delete field");
		return FAIL;
		//goto INGNORE_DOCATTR;
	}

	if (buf[0] == '1') {
		info("document[%u] is deleted", docId);
		return CDM_DELETED;
	}
#endif

//INGNORE_DOCATTR:
	if (rd_lock(fdIndexFile, SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdIndexFile, strerror(errno));
		return FAIL;
	}

	/* get index information */
	iResult = SelectIndexElement(fdIndexFile, docId, indexElement);
//	debug("indexElement {docId=%u, offset=%u, length=%u, dwDBNo=%u}", 
//	       indexElement.docId, indexElement.offset,
//		   indexElement.length, indexElement.dwDBNo);

	un_lock(fdIndexFile, SEEK_SET, 0, 0);

	if (iResult == FAIL) {
		warn("no document retrieved by docid[%u]", docId);
		return CDM_NOT_EXIST;
	}

	if (indexElement->length > DOCUMENT_SIZE) {
		warn("document is bigger than DOCUMENT_SIZE, [%ld]",
						indexElement->length);
		return FAIL;
	}

	if (rd_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdDBFile[indexElement->dwDBNo], strerror(errno));
		return FAIL;
	}

	// read data from dit file
	iResult = lseek(fdDBFile[indexElement->dwDBNo], indexElement->offset, SEEK_SET);
	if (iResult == -1) {
		error("error in lseek()");
		un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	iResult = read(fdDBFile[indexElement->dwDBNo], aCannedDoc, indexElement->length);
	if (iResult == -1) {
		error("error in read(): %s", strerror(errno));
		un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);

	iResult = sb_run_buffer_append(pCannedDoc, 
			indexElement->length, aCannedDoc);
	if (iResult < 0) {
		error("error in buffer_append()");
		return FAIL;
	}

	return indexElement->length;
}

/**
 * CDM_get_as_pointer
 * VariableBuffer를 사용하지않고 pointer로 바로 받는다.
 */
int CDM_get_as_pointer(uint32_t docId, void *pCannedDoc, int size)
{
	IndexFileElement ele;
	IndexFileElement *indexElement = &ele;
	int iResult;
	docattr_t attr;
#ifdef _KHYANG_
	char buf[SHORT_STRING_SIZE];
#endif

	if (pCannedDoc == NULL) {
		crit("invalid pCannedDoc address [NULL]");
		return FAIL;
	}

	/* chech docattr */
	if (sb_run_docattr_get(docId, &attr) == -1) {
		error("canot get docattr");
		return FAIL;
		//goto INGNORE_DOCATTR;
	}

#ifdef _KHYANG_
	if (sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE)
			== -1) {
		error("cannot get value of Delete field");
		return FAIL;
		//goto INGNORE_DOCATTR;
	}

	if (buf[0] == '1') {
		info("document[%u] is deleted", docId);
		return CDM_DELETED;
	}
#endif

//INGNORE_DOCATTR:
	if (rd_lock(fdIndexFile, SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdIndexFile, strerror(errno));
		return FAIL;
	}

	/* get index information */
	iResult = SelectIndexElement(fdIndexFile, docId, indexElement);
//	debug("indexElement {docId=%u, offset=%u, length=%u, dwDBNo=%u}", 
//	       indexElement.docId, indexElement.offset,
//		   indexElement.length, indexElement.dwDBNo);

	un_lock(fdIndexFile, SEEK_SET, 0, 0);

	if (iResult == FAIL) {
		warn("no document retrieved by docid[%u]", docId);
		return CDM_NOT_EXIST;
	}

	if (indexElement->length > size) {
		warn("document is bigger than DOCUMENT_SIZE, [%ld]",
						indexElement->length);
		return FAIL;
	}

	if (rd_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdDBFile[indexElement->dwDBNo], strerror(errno));
		return FAIL;
	}

	// read data from dit file
	iResult = lseek(fdDBFile[indexElement->dwDBNo], indexElement->offset, SEEK_SET);
	if (iResult == -1) {
		error("error in lseek()");
		un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	iResult = read(fdDBFile[indexElement->dwDBNo], pCannedDoc, indexElement->length);
	if (iResult == -1) {
		error("error in read(): %s", strerror(errno));
		un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);
		return FAIL;
	}

	un_lock(fdDBFile[indexElement->dwDBNo], SEEK_SET, 0, 0);

	return indexElement->length;
}

/**
 * CDM_getAbstract
 * 문서 CannedDoc.doc 참조
 */
struct comments {
	char field[MAX_FIELD_NAME_LEN];
	char buf[LONG_LONG_STRING_SIZE];
	int iscdata;
};

static void init_comment(struct comments cmts[])
{
	int i;
	for (i=0; i<QP_MAX_NUM_ABSTRACT_INFO; i++) {
		cmts[i].field[0] = '\0';
		cmts[i].buf[0] = '\0';
		cmts[i].iscdata = 0;
	}
}

static void append_comment(struct comments cmts[], char *name,
		char *str, int len, int iscdata)
{
	int i;
	for (i=0; i<QP_MAX_NUM_ABSTRACT_INFO && cmts[i].field[0]!='\0'; i++) {
		if (strcmp(cmts[i].field, name) == 0) {
			strcat(cmts[i].buf, "...");
			strncat(cmts[i].buf, str, 
					LONG_LONG_STRING_SIZE-strlen(cmts[i].buf)>len?
					len:LONG_LONG_STRING_SIZE-strlen(cmts[i].buf));
			cmts[i].iscdata = iscdata;
			break;
		}
	}
	if (i<QP_MAX_NUM_ABSTRACT_INFO && cmts[i].field[0] == '\0') {
		strcpy(cmts[i].field, name);
		strncat(cmts[i].buf, str, LONG_LONG_STRING_SIZE>len?len:LONG_LONG_STRING_SIZE);
		cmts[i].iscdata = iscdata;
	}
}

static void make_abstracted_cdm(char *buf, struct comments cmts[])
{
	int i;
	strcpy(buf, "<");
	strcat(buf, fieldRootName);
	strcat(buf, ">");
	for (i=0; i<QP_MAX_NUM_ABSTRACT_INFO && cmts[i].field[0]!='\0'; i++) {
		strcat(buf, "<");
		strcat(buf, cmts[i].field);
		strcat(buf, ">");
		if (cmts[i].iscdata)
			strcat(buf, "<![CDATA[");
		strncat(buf, cmts[i].buf, DOCUMENT_SIZE-strlen(buf));
		if (cmts[i].iscdata)
			strcat(buf, "]]>");
		strcat(buf, "</");
		strcat(buf, cmts[i].field);
		strcat(buf, ">");
	}
	strcat(buf, "</");
	strcat(buf, fieldRootName);
	strcat(buf, ">");
}

int
CDM_getAbstract(int            numRetrievedDoc, 
                RetrievedDoc   aRetrievedDoc[],
				VariableBuffer aCannedDoc[]) {
	int iResult, size , iDoc, j;
	//int16_t wordpos, bytepos, nextpos;
	char *tmpstr;
/*	char tmpstr[DOCUMENT_SIZE];*/
	struct comments *comments;
/*	struct comments comments[QP_MAX_NUM_ABSTRACT_INFO];*/
	VariableBuffer buf;
	parser_t *p;
	field_t *f;
	char *paragraph, path[STRING_SIZE];
	IndexFileElement ele;
	//Morpheme morp;

	size = 0;

	tmpstr = (char*)sb_calloc(DOCUMENT_SIZE, sizeof(char));
	comments = (struct comments *)sb_calloc(QP_MAX_NUM_ABSTRACT_INFO,sizeof(struct comments));

	for (iDoc = 0; iDoc<numRetrievedDoc; iDoc++) {
		if (aRetrievedDoc[iDoc].docId == 0) {
			warn("invaild document id(0)");
			continue;
		}


		/* get whole document */
		sb_run_buffer_initbuf(&buf);
		iResult = CDM_getWithIndexElement(aRetrievedDoc[iDoc].docId, 
										&buf, &ele);
		if (iResult == CDM_NOT_EXIST) {
			continue;
		} else if (iResult < 0) {
			warn("cannot get document[%ld] to abstract", 
					aRetrievedDoc[iDoc].docId);
			continue;
		}

		size = sb_run_buffer_getsize(&buf);
		sb_run_buffer_get(&buf, 0, size, tmpstr);
		tmpstr[size] = '\0';
		sb_run_buffer_freebuf(&buf);

		/* parse canned document */
		p = sb_run_xmlparser_parselen("CP949", tmpstr, size);
		if (p == NULL) {
			error("cannot parse successfully");
			continue;
		}
/*		info("document[%d]: %s", aRetrievedDoc[iDoc].docId, tmpstr);*/

		/* lexing and abstract */
		init_comment(comments);
		for (j=0; j<aRetrievedDoc[iDoc].numAbstractInfo; j++) {
			/* retrieve field */
			sprintf(path, "/%s/%s", fieldRootName, 
					aRetrievedDoc[iDoc].cdAbstractInfo[j].field);
			f = sb_run_xmlparser_retrieve_field(p, path);
			if (f == NULL) {
				warn("cannot retrieve field[%s]", path);
				continue;
			}

			paragraph = f->value;

#define TEST(a)     ((a)?1:0)
			append_comment(comments,
					aRetrievedDoc[iDoc].cdAbstractInfo[j].field,
					f->value,
					f->size,
					TEST(f->flag & CDATA_SECTION));
		}
		sb_run_xmlparser_free_parser(p);

		tmpstr[0] = '\0';
		make_abstracted_cdm(tmpstr, comments);

		size = strlen(tmpstr);
		//INFO("Abstracted Doc[%d]size%d: %s", aRetrievedDoc[iDoc].docId, size, tmpstr);
		iResult = sb_run_buffer_append(&(aCannedDoc[iDoc]), size, tmpstr);
		if (iResult < 0) {
			error("error in buffer_append()");
			sb_free(tmpstr);
			sb_free(comments);
			return FAIL;
		}
	}

	sb_free(comments);
	sb_free(tmpstr);

	return SUCCESS;
}

/**
 * CDM_delete
 * 문서 CannedDoc.doc 참조
 */
static int CDM_delete(uint32_t docId) {
	int iResult;
	IndexFileElement indexElement;

	if (wr_lock(fdIndexFile, SEEK_SET, 0, 0) == -1) {
		error("error flock[%d]: %s", fdIndexFile, strerror(errno));
		return FAIL;
	}

	/* get index information */
	iResult = SelectIndexElement(fdIndexFile, docId, &indexElement);
	if (iResult == FAIL) {
		un_lock(fdIndexFile, SEEK_SET, 0, 0);
		return CDM_NOT_EXIST;
	}

	iResult = DeleteIndexElement(fdIndexFile, docId);
	if (iResult == FAIL) {
		un_lock(fdIndexFile, SEEK_SET, 0, 0);
		return FAIL;
	}

	un_lock(fdIndexFile, SEEK_SET, 0, 0);

	return SUCCESS;
}

/*
 * CDM_update
 * 문서 CannedDoc.doc 참조
 */
static int CDM_update(uint32_t docId, VariableBuffer *pDoc) {
	int iResult;
	iResult = CDM_delete(docId);
	if (iResult != CDM_NOT_EXIST && iResult < 0) {
		error("cannot update document[%u] because of cdm error code[%d]", docId, iResult);
		return iResult;
	}

	return CDM_put(docId, pDoc);
}

/**
 * CDM_print
 * 문서 CannedDoc.doc 참조
 */
#if 0
static int CDM_print(uint32_t docId, FILE *pFp) {
	int iResult;
	VariableBuffer buf;
	
	iResult = sb_run_buffer_initbuf(&buf);
	if (iResult < 0) {
		return FAIL;
	}
	
	iResult = CDM_get(docId, &buf);
	if (iResult < 0) {
		return iResult;
	}
	
	iResult = sb_run_buffer_print(pFp, &buf);
	if (iResult < 0) {
		return FAIL;
	}
	
	sb_run_buffer_freebuf(&buf);

	return SUCCESS;	
}
#endif 

/**
 * CDM_printDocList
 * 문서 CannedDoc.doc 참조
 */
#if 0
static int CDM_printDocList(FILE *pFp) {
	return FAIL;
}
#endif

static uint32_t CDM_getLastId (void) {
	return cdm_shared->lastDocId;
}

static int CDM_drop(void) {
#if 0
	char pTmp[MAX_PATH];
	int iCount, iResult;
	
	// initilize lastDocId
	*lastDocId = 0;

	/////////////////////////////////////////////////////
	// drop index file

	// lock index file
	if (SYNC_lock(&syncInfo, INDEX_LOCK) < 0) {
		error("error in SYNC_lock()");
		return FAIL;
	}

	close (fdIndexFile);

	GET_IDX_PATH(pTmp, aDBPath);
	debug("delete %s", pTmp);
	iResult = sb_unlink(pTmp);
	if (iResult == -1) {
		error("cannot index file because of %s", strerror(errno));
	}

	// re-initialize index file
	InitIndexFile();

	// unlock index file
	if (SYNC_unlock(&syncInfo, INDEX_LOCK) < 0) {
		error("error in SYNC_lock()");
		return FAIL;
	}

	//////////////////////////////////////////////////////
	// drop db(dit) file

	// lock dit file
	for (iCount=0; iCount<dwMaxDBFileNum; iCount++) {
		iResult = SYNC_lock(&syncInfo, DIT_LOCK(iCount));
		if (iResult < 0) {
			error("error in SYNC_lock()");
			return FAIL;
		}

		GET_DIT_PATH(pTmp, aDBPath, iCount);
		debug("delete %s", pTmp);
		iResult = sb_unlink(pTmp);
		if (iResult == -1) {
			error("cannot delete did file because of %s", strerror(errno));
		}
		close(fdDBFile[iCount]);
	}
	sb_free(fdDBFile);

	*currentDBNo = 0;
/*	currentOffset = 0;*/

	// re-initialize dit files
	InitDBFiles();

	// unlock dit file
	for (iCount=0; iCount<dwMaxDBFileNum; iCount++) {
		iResult = SYNC_unlock(&syncInfo, DIT_LOCK(iCount));
		if (iResult < 0) {
			error("error in SYNC_unlock()");
			return FAIL;
		}
	}

	return SUCCESS;
#endif
	return FAIL;
}

static int init (void) {
	return SUCCESS;
}

/*****************************************************************************/

static void get_db_path(configValue v) {
	strncpy(aDBPath, v.argument[0], MAX_PATH);
}

static void get_max_db_file_size(configValue v) {
	dwMaxDBFileSize = atoi(v.argument[0]);
}

static void get_max_db_file_num(configValue v) {
	dwMaxDBFileNum = atoi(v.argument[0]);
}

static void get_max_doc_num(configValue v) {
	dwMaxDocNum = atoi(v.argument[0]);
}

static void get_field(configValue v) {
	int iCount;
	static char docattr_fields[MAX_FIELD_NUM][MAX_FIELD_NAME_LEN];

	for (iCount=0; iCount<MAX_FIELD_NUM && docattrFields[iCount]; iCount++);

	if (iCount == MAX_FIELD_NUM) {
		error("too many fields are setted");
		return;
	}
	strncpy(docattr_fields[iCount], v.argument[0], MAX_FIELD_NAME_LEN);
	docattr_fields[iCount][MAX_FIELD_NAME_LEN-1] = '\0';
	docattrFields[iCount] = docattr_fields[iCount];
	info("read DocAttrField[%s]", docattrFields[iCount]);
}

static void get_field_root_name(configValue v) {
	strncpy(fieldRootName, v.argument[0], MAX_FIELD_NAME_LEN);
	fieldRootName[MAX_FIELD_NAME_LEN-1] = '\0';
}

static void get_shared_file(configValue v) {
	strncpy(cdm_shared_file, v.argument[0], MAX_FILE_LEN);
	cdm_shared_file[MAX_FILE_LEN-1] = '\0';
}

static void register_hooks(void)
{
	sb_hook_server_canneddoc_init(CDM_init,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_close(CDM_close,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_put(CDM_put,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_put_with_oid(CDM_putWithOid,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_get(CDM_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_get_as_pointer(CDM_get_as_pointer, NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_get_size(CDM_getSize,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_get_abstract(CDM_getAbstract,NULL,NULL,HOOK_MIDDLE);
	// XXX: 아래 hooking 함수는 return값이 error가 아니라 원하는 data.
	sb_hook_server_canneddoc_last_registered_id(CDM_getLastId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_delete(CDM_delete,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_update(CDM_update,NULL,NULL,HOOK_MIDDLE);
	sb_hook_server_canneddoc_drop(CDM_drop,NULL,NULL,HOOK_MIDDLE);
}

static config_t config[] = {
	CONFIG_GET("db_path", get_db_path, 1, "db repository pass"),
	CONFIG_GET("max_db_file_num", get_max_db_file_num, 1, "maximum db file number"),
	CONFIG_GET("max_db_file_size", get_max_db_file_size, 1, "maximum size of dit file"),
	CONFIG_GET("max_doc_num", get_max_doc_num, 1, \
			"maximun number of document that server can register"),

	CONFIG_GET("FieldRootName", get_field_root_name, 1, \
			"canned document root element name"),
	CONFIG_GET("DocAttrField", get_field, VAR_ARG, "fields inserted into docattr db"),
	CONFIG_GET("SharedFile", get_shared_file, 1, \
			"cdm shared memory file. (ex. SharedFile dat/cdm/cdm.shared)"),
	{NULL}
};

module cdm_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,    			/* registry */
	init,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};
