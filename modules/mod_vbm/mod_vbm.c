/* $Id$ */
#include "common_core.h"
#include "memory.h"
#include <string.h> /* memcpy(3),strcat(3) */
#include <stdlib.h> /* atoi(3) */
#include "mod_api/vbm.h"
#include "mod_vbm.h"

static int m_bufBlockSize=2048;
static int m_numBufBlock=20000;

// memory block들을 가리킬 linked list node
typedef struct {
	int usedBytes;
	void* currentBlock;
	void* nextInfo;
} BlockInfo;
static BlockInfo* m_blockInfoList=NULL;
static void* m_bufBlocks=NULL;  //large memory blocks 
								//from which we lend small memory blocks

static void* getPosition(int index);// trivial function. see it, know it
static void freeInfoList(BlockInfo* theRoot); // theRoot에서 시작하는
									// BlockInfo 의 list를 free시킨다

/// free block manager related functions
static BlockInfo *m_freeBlockRoot=NULL;
static void fbm_init(BlockInfo* theRoot);
static void fbm_pushOneBlock(BlockInfo* blockInfo);
static void* fbm_popOneBlock();

static int init()
{
	return SUCCESS;
}

int VBM_initModule() {
	int i = 0;

	if ( m_freeBlockRoot != NULL ) {
		warn("initmodule() was already called");
		return SUCCESS;
	}

	if (m_bufBlocks == NULL)
		m_bufBlocks = sb_malloc(m_bufBlockSize * m_numBufBlock);

	if (m_bufBlocks == NULL)
		return VBM_INSUFFICIENT_BUF_BLOCK;


	if (m_blockInfoList == NULL)
		m_blockInfoList = (BlockInfo*)sb_malloc(sizeof(BlockInfo)*m_numBufBlock);

	if (m_blockInfoList == NULL)
		return VBM_INSUFFICIENT_BUF_BLOCK;


	for (i = 0; i<m_numBufBlock; i++) {
		if (i != m_numBufBlock-1)
			m_blockInfoList[i].nextInfo = (void*)(m_blockInfoList+i+1);
		else
			m_blockInfoList[i].nextInfo = NULL;	
		m_blockInfoList[i].currentBlock = getPosition(i);
	}

	fbm_init(m_blockInfoList);

	return SUCCESS;
}

int VBM_initBuf(VariableBuffer *pVarBuf) {
	if ( m_freeBlockRoot == NULL ) {
		if ( VBM_initModule() != SUCCESS ) {
			error("VBM_initModule() failed");
			return FAIL;
		}
	}

	pVarBuf->size = 0;

	pVarBuf->pFirst = NULL;
	pVarBuf->pLast = NULL;
	pVarBuf->lastBlockUsedBytes = 0;

	return SUCCESS;
}

#define initBufFirstPointer(pVarBuf)	{\
	if (pVarBuf->pFirst == NULL) { \
		pTmp = fbm_popOneBlock(); \
		if (pTmp == NULL) \
			return VBM_INSUFFICIENT_BUF_BLOCK; \
		pVarBuf->pFirst = pTmp; \
		pVarBuf->pLast = pTmp; \
		pVarBuf->lastBlockUsedBytes = 0;\
	}\
}
int VBM_append(VariableBuffer *pVarBuf,int size,void* pBuf) {
	void *dest = NULL;
	void *pTmpLast = NULL;
	void *pTmp = NULL;
	char operationOnlyWithinOneBlock = TRUE;

	int totalCopiedSize = 0;
	int remainSize = 0;
	int copySize = 0;
	int tmpLastBlockUsedBytes = 0;

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	initBufFirstPointer(pVarBuf);

	remainSize = size;
	pTmpLast = pVarBuf->pLast;
	tmpLastBlockUsedBytes = pVarBuf->lastBlockUsedBytes;

	while (1) {
		copySize = m_bufBlockSize - pVarBuf->lastBlockUsedBytes;
		if (copySize > remainSize)
			copySize = remainSize;

		dest = ((BlockInfo*)pVarBuf->pLast)->currentBlock + 
					pVarBuf->lastBlockUsedBytes;

		memcpy(dest,pBuf,copySize);

		totalCopiedSize += copySize;

		if (totalCopiedSize == size)
			break;
		
		operationOnlyWithinOneBlock = FALSE;

		remainSize= size - totalCopiedSize;
		pBuf = pBuf + copySize;

		pTmp = fbm_popOneBlock();
		if (pTmp == NULL) {
			pVarBuf->pLast = pTmpLast;
			pVarBuf->lastBlockUsedBytes = tmpLastBlockUsedBytes;
			freeInfoList( (BlockInfo*)pTmpLast );
			return VBM_INSUFFICIENT_BUF_BLOCK;
		}

		pVarBuf->lastBlockUsedBytes = 0;// this has side effect,
										// look at the start of this while loop
		((BlockInfo*)pVarBuf->pLast)->nextInfo = pTmp;
		pVarBuf->pLast = pTmp;
	}

	if (operationOnlyWithinOneBlock == TRUE)
		pVarBuf->lastBlockUsedBytes += copySize;
	else	
		pVarBuf->lastBlockUsedBytes = copySize;
	pVarBuf->size += size;
	return size;
}

int VBM_get(VariableBuffer *pVarBuf, int offset, int size, void* pBuf) {
	void *src = NULL;
	void *tmp = NULL;

	int totalCopiedSize = 0;
	int remainSize = 0;
	int copySize = 0;
	int startBlockNum = 0,blockOffset = 0;
	int i = 0;

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	remainSize = size;
	startBlockNum = (int)offset/m_bufBlockSize;
	blockOffset = offset%m_bufBlockSize;

	if (pVarBuf->size < size+offset) {// XXX < or <=
		error("vbm object: size:%d, lastBlockUsedBytes:%d",
							pVarBuf->size, pVarBuf->lastBlockUsedBytes);
		error("function arguments: offset:%d, size:%d",offset,size);
		return FAIL;	
	}
	else if (size == 0) {
		error("size must be larger than 0");
		return FAIL;
	}

	tmp = pVarBuf->pFirst;
	for (i = 0; i <startBlockNum; i++) {
		tmp = (void*)((BlockInfo*)tmp)->nextInfo;
	}

	if (tmp == NULL) {
		error("vbm object: size:%d, lastBlockUsedBytes:%d",
							pVarBuf->size, pVarBuf->lastBlockUsedBytes);
		error("function arguments: offset:%d, size:%d",offset,size);
		return FAIL;
	}

	while (1) {
		copySize = m_bufBlockSize - blockOffset;
		if (copySize > remainSize)
			copySize = remainSize;

		src = (void*)(((BlockInfo*)tmp)->currentBlock) + blockOffset;

		memcpy(pBuf,src,copySize);

		totalCopiedSize += copySize;

		if (totalCopiedSize == size)
			break;

		pBuf = pBuf + copySize;
		remainSize = size - totalCopiedSize;
		tmp = (void*)(((BlockInfo*)tmp)->nextInfo);	
		if (tmp == NULL) {
			error("error while getting next info block");
			break;
		}
		blockOffset = 0;// used only first time of this loop
	}
	return totalCopiedSize;
}

int VBM_getSize(VariableBuffer *pVarBuf) {
	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	return pVarBuf->size;
}

void VBM_freeBuf(VariableBuffer *pVarBuf) {
	void* tmp=NULL;

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return;
	}

	tmp = pVarBuf->pFirst;
	freeInfoList( (BlockInfo*)tmp );

	pVarBuf->size = 0;
	pVarBuf->lastBlockUsedBytes = 0;
	pVarBuf->pFirst = NULL;
	pVarBuf->pLast = NULL;
}

int VBM_appendBuf(VariableBuffer *pVarBufDest,
					VariableBuffer *pVarBufToAppend){
	void *tmpBuf;
	unsigned long sizeOfAppendBuf=0;
	int nRet=0;

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	sizeOfAppendBuf = VBM_getSize(pVarBufToAppend);
	tmpBuf = sb_malloc(sizeOfAppendBuf);
	if (tmpBuf == NULL)
		return FAIL;

	nRet = VBM_get(pVarBufToAppend,0,sizeOfAppendBuf,tmpBuf);
	if (nRet < 0){
		sb_free(tmpBuf);
		return FAIL;
	}

	VBM_freeBuf(pVarBufToAppend);

	nRet = VBM_append(pVarBufDest,sizeOfAppendBuf,tmpBuf);
	if (nRet < 0) {
		sb_free(tmpBuf);
		return FAIL;
	}
	sb_free(tmpBuf);
	return SUCCESS;
}

int VBM_print(FILE *fp, VariableBuffer *pVarBuf) {
	char tmp[1024+1];
	int iResult, iLeft, iOffset, iSize;

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	iLeft = VBM_getSize(pVarBuf);
	iOffset = 0;
	while (iLeft > 0) {
		iSize = (iLeft > 1024) ? 1024 : iLeft;
		iResult = VBM_get(pVarBuf, iOffset, iSize, tmp);
		if (iResult < 0) {
			return FAIL;
		}
		tmp[iSize] = '\0';
		fprintf(fp, "%s", tmp);

		iLeft -= iSize;
		iOffset += iSize;
	}

	return SUCCESS;
}

int VBM_buffer(char *result, VariableBuffer *pVarBuf) {
	char tmp[1024+1];
	int iResult, iLeft, iOffset, iSize;
	char szTmp[1024];

	if ( m_freeBlockRoot == NULL ) {
		error( "call VBM_initModule() for using vbm module" );
		return FAIL;
	}

	iLeft = VBM_getSize(pVarBuf);
	iOffset = 0;
	while (iLeft > 0) {
		iSize = (iLeft > 1024) ? 1024 : iLeft;
		iResult = VBM_get(pVarBuf, iOffset, iSize, tmp);
		if (iResult < 0) {
			return FAIL;
		}
		tmp[iSize] = '\0';
		sprintf(szTmp, "%s", tmp);
		strcat(result, szTmp);

		iLeft -= iSize;
		iOffset += iSize;
	}

	return SUCCESS;
}

static void freeInfoList(BlockInfo* theRoot) {
	BlockInfo *tmp=NULL;

	while(1) {
		if (theRoot == NULL)
			break;

		tmp = theRoot->nextInfo;

		fbm_pushOneBlock(theRoot);

		theRoot = tmp;
	}
}

static void* getPosition(int index) {
	return m_bufBlocks+index*m_bufBlockSize;
}

///static void* vbm_getFreeBlock() {
///	void *tmp=NULL;

///	tmp = fbm_pop();

///	if (tmp == NULL)
///		return NULL;
///	
///	return tmp;
///}

///static void* vbm_getFreeBlock() {
///	int i = 0;

///	for (i = 0; i<m_numBufBlock; i++) {
///		if (m_blockInfoTable[i].isUsed == FALSE) {
///			m_blockInfoTable[i].isUsed = TRUE;
///			m_blockInfoTable[i].usedBytes = 0;
///			m_blockInfoTable[i].currentBlock = getPosition(i);
///			m_blockInfoTable[i].nextInfoTable = NULL;
///			return (void*)&(m_blockInfoTable[i]);
///		}
///	}

///	return NULL;
///}

/**
 *	Free Block Manager
 *
 */
static void fbm_init(BlockInfo* theRoot) {
	m_freeBlockRoot = theRoot;
}
static void fbm_pushOneBlock(BlockInfo* blockInfo) {
	BlockInfo *tmp = NULL;

	tmp = m_freeBlockRoot;
	m_freeBlockRoot = blockInfo;
	m_freeBlockRoot->nextInfo = tmp;
}
static void* fbm_popOneBlock() {
	BlockInfo *tmp = NULL;

	if (m_freeBlockRoot == NULL)
		return NULL;

	tmp = m_freeBlockRoot;
	m_freeBlockRoot = m_freeBlockRoot->nextInfo;
	tmp->nextInfo = NULL;

	return tmp;
}

static void set_bufblksize(configValue v)
{
	m_bufBlockSize = atoi(v.argument[0]);
	debug("Buffer block size is %d",m_bufBlockSize);
}
static void set_bufblknum(configValue v)
{
	m_numBufBlock = atoi(v.argument[0]);
	debug("Number of buffer block is %d",m_numBufBlock);
}

static config_t config[] = {
	CONFIG_GET("VBM_BUF_BLOCK_SIZE",set_bufblksize, 1, 
					"variable buffer one block size"),
	CONFIG_GET("VBM_NUM_BUF_BLOC",set_bufblknum, 1,
					"variable buffer total block number"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_buffer_initbuf(VBM_initBuf,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_freebuf(VBM_freeBuf,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_get(VBM_get,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_getsize(VBM_getSize,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_append(VBM_append,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_appendbuf(VBM_appendBuf,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_print(VBM_print,NULL,NULL,HOOK_MIDDLE);
	sb_hook_buffer_str(VBM_buffer,NULL,NULL,HOOK_MIDDLE);
}

module vbm_module = {
    STANDARD_MODULE_STUFF,
    config,					/* config */
    NULL,                   /* registry */
    init,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks,			/* register hook api */
};
