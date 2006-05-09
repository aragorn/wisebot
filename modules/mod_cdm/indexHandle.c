/* $Id$ */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "common_core.h"
#include "cannedDocServer.h"

#define JUMP_NUM		1

// extern variables
//extern unsigned long dwMaxDocNum;

static unsigned long HashFunc(uint32_t docId) {
//	return docId % dwMaxDocNum;
	return docId;
}

static int GetIndexElement(int fdIndexFile, unsigned long offset,
		IndexFileElement *pIndexElement) {
	int iResult;
	
	iResult = lseek(fdIndexFile, offset * sizeof(IndexFileElement), SEEK_SET);
	if (iResult == -1) {
		error("error in lseek(): %s", strerror(errno));
		return FAIL;
	}
	
	iResult = read(fdIndexFile, pIndexElement, sizeof(IndexFileElement));
	if (iResult == -1) {
		error("error in read(): %s", strerror(errno));
		return FAIL;
	}
	
	iResult = lseek(fdIndexFile, offset * sizeof(IndexFileElement), SEEK_SET);
	if (iResult == -1) {
		error("error in lseek(): %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

static int HashSearch(int fdIndexFile, uint32_t docId, IndexFileElement *pIndexElement) {
	int iResult;
	unsigned long currentOffset, firstOffset;
	
	currentOffset = HashFunc(docId);
	iResult = GetIndexElement(fdIndexFile, currentOffset, pIndexElement);
	if (iResult != SUCCESS) {
		error("error in GetIndexElement()");
		return FAIL;
	}
	firstOffset = currentOffset;

	if (docId == pIndexElement->docId) {
		return SUCCESS;
	}
	
	/*
	while (indexElement.docId != 0) {
		if (docId == indexElement.docId) {
			return SUCCESS;
		}
		currentOffset = (currentOffset + JUMP_NUM) % dwMaxDocNum;
		iResult = GetIndexElement(fdIndexFile, currentOffset);
		if (iResult != SUCCESS) {
			error("error in GetIndexElement()");
			return FAIL;
		}
		if (currentOffset == firstOffset) return FAIL;
	}
	*/
	
	warn("no such document[%u] ( != pIndexElement->docId[%u])", docId, pIndexElement->docId);
	return FAIL;
}

static int HashInsert(int fdIndexFile, IndexFileElement *pIndexElement) {
	int iResult;
	unsigned long currentOffset, firstOffset;
	IndexFileElement ele;
	
	currentOffset = HashFunc(pIndexElement->docId);
	iResult = GetIndexElement(fdIndexFile, currentOffset, &ele);
	if (iResult != SUCCESS) {
		error("error in GetIndexElement()");
		return FAIL;
	}
	firstOffset = currentOffset;
	
	if (ele.docId !=0) {
		warn("document[%u] is already registered. i will overwrite",
				ele.docId);
	}

#if 0
	while (indexElement.docId != 0) {
		if (pIndexElement->docId == indexElement.docId) {
			warn("document with same key is exist, update document");
/*			return CDM_ALREADY_EXIST;*/
			break;
		}
		currentOffset = (currentOffset + JUMP_NUM) % dwMaxDocNum;
		iResult = GetIndexElement(fdIndexFile, currentOffset);
		if (iResult != SUCCESS) {
			error("error in GetIndexElement()");
			return FAIL;
		}
		if (currentOffset == firstOffset) {
			error("database is full!");
			return FAIL;
		}
	}
#endif
	
	iResult = write(fdIndexFile, pIndexElement, sizeof(IndexFileElement));
	if (iResult == -1) {
		error("error in write(): %s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

static int HashDelete(int fdIndexFile, uint32_t docId) {
	int iResult;
	unsigned long currentOffset, firstOffset;
	IndexFileElement index = { 0, 0, 0, 0}, ele;

	/* find doc for docId */
	currentOffset = HashFunc(docId);
	iResult = GetIndexElement(fdIndexFile, currentOffset, &ele);
	if (iResult != SUCCESS) {
		error("error in GetIndexElement()");
		return FAIL;
	}
	firstOffset = currentOffset;

	if ( docId != ele.docId ) {
		error("index file crash!");
		return FAIL;
	}
	
#if 0
	while (ele.docId != 0) {
		if (docId == ele.docId) {
			break;
		}

		currentOffset = (currentOffset + JUMP_NUM) % dwMaxDocNum;
		iResult = GetIndexElement(fdIndexFile, currentOffset, &ele);
		if (iResult != SUCCESS) {
			error("error in GetIndexElement()");
			return FAIL;
		}
		if (currentOffset == firstOffset) return FAIL;
	}
#endif

	iResult = write(fdIndexFile, &index, sizeof(IndexFileElement));
	if (iResult == -1) {
		error("error in write(): %s", strerror(errno));
		return FAIL;
	}
// collision ó���� �ʿ� ���� - chaeyk
#if 0
/*	fwrite(&index, sizeof(IndexFileElement), 1, fdIndexFile);
	if (ferror(fdIndexFile)) {
		error("error in fwrite()");
		return FAIL;
	}
*/
	/* copy all index elements that have collison with deleted element to previous bucket */
	firstOffset = currentOffset;
	currentOffset = (currentOffset + JUMP_NUM) % dwMaxDocNum;
	iResult = lseek(fdIndexFile, currentOffset * sizeof(IndexFileElement), SEEK_SET);
	if (iResult == -1) {
		error("error in lseek(): %s", strerror(errno));
		return FAIL;
	}
/*	fseek(fdIndexFile, currentOffset * sizeof(IndexFileElement), SEEK_SET);
	if (ferror(fdIndexFile)) {
		error("error in fseek()");
		return FAIL;
	}
*/
	iResult = read(fdIndexFile, &index, sizeof(IndexFileElement));
	if (iResult == -1) {
		error("error in read(): %s", strerror(errno));
		return FAIL;
	}
/*	fread(&index, sizeof(IndexFileElement), 1, fdIndexFile);
	if (ferror(fdIndexFile)) {
		error("error in fwrite()");
		return FAIL;
	}
*/
	while (index.docId != 0 && currentOffset != HashFunc(index.docId)) {
		iResult = lseek(fdIndexFile, firstOffset * sizeof(IndexFileElement), SEEK_SET);
		if (iResult == -1) {
			error("error in lseek(): %s", strerror(errno));
			return FAIL;
		}
	
		iResult = write(fdIndexFile, &index, sizeof(IndexFileElement));
		if (iResult == -1) {
			error("error in write(): %s", strerror(errno));
			return FAIL;
		}

		firstOffset = currentOffset;
		currentOffset = (currentOffset + JUMP_NUM) % dwMaxDocNum;
		iResult = lseek(fdIndexFile, currentOffset * sizeof(IndexFileElement), SEEK_SET);
		if (iResult == -1) {
			error("error in lseek(): %s", strerror(errno));
			return FAIL;
		}
		
		iResult = read(fdIndexFile, &index, sizeof(IndexFileElement));
		if (iResult == -1) {
			error("error in read(): %s", strerror(errno));
			return FAIL;
		}
	}
#endif	
	return SUCCESS;
}

/**
 * IsExistDoc
 * docId�� DocId�� ���� ������ �̹� ��ϵǾ����� �˻��Ͽ� ����� ��ȯ�Ѵ�.
 
 * @docId		: üũ�ϰ����ϴ� ������ DocId
 * return		: TRUE -> ������ �ִ�.
 *				  FALSE -> ������ ����.
 */
int IsExistDoc(int fdIndexFile, uint32_t docId) {
	int iResult;
	IndexFileElement indexElement;
	
	iResult = HashSearch(fdIndexFile, docId, &indexElement);
	if (iResult == SUCCESS) {
		return TRUE;
	}

	return FALSE;
}

/**
 * InsertElement
 * ���ڷ� ������ ������ �ε��� ������ �ش��ϴ� ��ġ�� �����ϰ�
 * ����� ��ȯ�Ѵ�.
 
 * @pIndexFileElement	: �����ϰ����ϴ� ����
 * return				: SUCCESS -> ���忡 ����
 * 						  FAIL -> ���忡 ����
 */
int InsertIndexElement(int fdIndexFile, IndexFileElement *pIndexFileElement) {
	int iResult;
	
	iResult = HashInsert(fdIndexFile, pIndexFileElement);
	return iResult;
}

/**
 * DeleteIndexElement
 * docId�� ������ �����.
 
 * @docID				: ��������ϴ� ������ DocId
 * return				: SUCCESS -> ����� ����
 *						  FAIL -> ����� ����
 */
int DeleteIndexElement(int fdIndexFile, uint32_t docId) {
	int iResult;
	
	iResult = HashDelete(fdIndexFile, docId);
	return iResult;
}

/**
 * GetIndexElement
 * docID�� ������ �ε������� ã�� �� ������ pIndexElement�� �ǵ�����.
 
 * @docId				: ã�����ϴ� ������ DocId
 * @pIndexElement		: ������ �ǵ������� �ϴ� ���� ���
 * return				: SUCCESS -> ã�µ� ����
 *						  FAIL -> ã�µ� ����
 */
int SelectIndexElement(int fdIndexFile, uint32_t docId, IndexFileElement *pIndexElement) {
	int iResult;
	IndexFileElement indexElement;
	
	iResult = HashSearch(fdIndexFile, docId, &indexElement);
	if (iResult == FAIL) {
		return FAIL;
	}
	
	pIndexElement->docId = docId;
	pIndexElement->dwDBNo = indexElement.dwDBNo;
	pIndexElement->offset = indexElement.offset;
	pIndexElement->length = indexElement.length;
#if 0
#ifdef PARAGRAPH_POSITION
	for (i=0; i<MAX_PARA_NUM; i++) {
		pIndexElement->para[i] = indexElement.para[i];
	}
#endif
#endif
	
	return SUCCESS;
}
