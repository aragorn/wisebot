/* $Id$ */
#include "cannedDocServer.h"

extern char aDocument[MAX_TAG_NAME];
extern FieldInfo *pFieldInfos;
extern int iNumOfDocumentField;

/**
 * Quick Search algorithm
 * c code from 'http://www-igm.univ-mlv.fr/~lecroq/string/'
 */

#define ASIZE 256

static void preQsBc(const unsigned char *x, int m, int qsBc[]) {
	int i;
	
	for (i = 0; i < ASIZE; ++i)
		qsBc[i] = m + 1;
	for (i = 0; i < m; ++i)
		qsBc[x[i]] = m - i;
}

static int QS(const unsigned char *x, int m, const unsigned char *y, int n) {
	int j, qsBc[ASIZE];

	/* Preprocessing */
	preQsBc(x, m, qsBc);

	/* Searching */
	j = 0;
	while (j <= n - m) {
		if (strncmp(y + j, x, m) == 0)
			return j;
		j += qsBc[y[j + m]];               /* shift */
	}

	return -1;
}

/**
 * GetContent
 * �Է�TEXT pInput���� ���� ������ �±�(pTag)�� �ѷ� ���� �κ���
 * ���� offset�� �����ϰ� pLen���� �� ���̸� �����Ѵ�.
 * pInput�� �ݵ�� �����±׷� �����ϴ� ���ڿ��̾�� �Ѵ�.

 * @pInput		: �Է�TEXT
 * @pTag		: �Է�TAG
 * @pLen		: ������� ���
 * @depth       : if 1, more depth, if 0, no more depth
 * return		: pInput�߿��� ������ ���ۿ�����
 */
// edited by kim young hoon for attribute at 2002. 3. 5.
static int GetContent(const char *pInput, const char *pTag, int *pLen, int depth) {
	int len, iResult, gt;
	char tmp[MAX_TAG_NAME];
//	memset(tmp, 0, MAX_TAG_NAME);

	/* calculate length of pattern */
	len = strlen(pTag);
/*	printf("%s len = %d\n", pTag, len);*/

	/* make open tag string */
	tmp[0] = '<';
	strncpy(tmp+1, pTag, MAX_TAG_NAME - 1);
	tmp[len+1] = '\0';
/*	tmp[len+1] = '>';*/
/*	tmp[len+2] = '\0';*/

	/* compare with first (len+2) charators */
	iResult = strncmp(pInput, tmp, len+1);
/*	iResult = strncmp(pInput, tmp, len+2);*/
	if (iResult != 0) {
		error("parse error : cannot find open tag");
		return FAIL;
	}

	gt = len + 1;
	while (pInput[gt] != '\0' && pInput[gt] != '>') ++gt;
	++gt;

	if (depth) {
		*pLen = -1;
		return gt;
/*		return len+2;*/
	}

	/* make close tag string */
	tmp[0] = '<';
	tmp[1] = '/';
	strncpy(tmp+2, pTag, MAX_TAG_NAME - 3);
	tmp[len+2] = '>';
	tmp[len+3] = '\0';
	
/*	iResult = strlen(pInput);*/
/*	iResult = QS(tmp, len+3, pInput+len+3, strlen(pInput+len+3));*/
	iResult = QS(tmp, len+3, pInput+gt, strlen(pInput+gt));
	if (iResult < 0) {
		error("parse error : cannot find close tag");
		return FAIL;
	}
	
	*pLen = iResult;
	
	return gt;
/*	return len+2;*/
}	

/**
 * ParseCannedDoc
 * �Ķ���� aCannedDoc���� ���� ������ ���ڿ��� �Ľ��Ͽ� �ʿ��� �ʵ��� ����
 * ��� ���߾��� �ִ��� üũ�ϰ� ������ �ʵ� ������� �����°� ���ڿ� ���̸�
 * Element ����ü�� �迭���·� ������ �����Ѵ�. parsing error�� ��� NULL�� 
 * �����Ѵ�.
 
 * @aCannedDoc				: �Ľ��ϰ����ϴ� ������ ���ڿ�
 * @pElement				: �Ľ̰���� ���� ����ü�� �迭
 * return					: �Ľ̰��
 *							  FAIL -> ����� �ְų� �Ľ� ����
 *							  SUCCESS -> �Ľ� ����
 */
int ParseCannedDoc(const char *aCannedDoc, Element *pElements) {
	int iResult, iLength, nextOffset, iCount;
	char *aTagName;
	
	printf ("%s", aCannedDoc);
	/* parse document tag */
	iResult = GetContent(aCannedDoc, aDocument, &iLength, 1);
	if (iResult < 0) {
		error("error in GetContent();");
		return FAIL;
	}
	nextOffset = iResult;
	
	/* trim white space next of tag */
	while(aCannedDoc[nextOffset] == '\n' || 
		  aCannedDoc[nextOffset] == ' ' || 
		  aCannedDoc[nextOffset] == '\t') {
		++nextOffset;
	}

	/* parse each field tag */
	for (iCount=1; iCount<=iNumOfDocumentField; iCount++) {
		aTagName = pFieldInfos[iCount].aName;
		iResult = GetContent(aCannedDoc+nextOffset, aTagName, &iLength, 0);
		if (iResult < 0) {
			error("error in GetContent();");
			return FAIL;
		}
		pElements[iCount].offset = nextOffset + iResult;
		pElements[iCount].length = iLength;
		nextOffset += iResult + iLength + strlen(aTagName) + 3;

		/* trim white space next of tag */
		while(aCannedDoc[nextOffset] == '\n' || 
			  aCannedDoc[nextOffset] == ' ' || 
			  aCannedDoc[nextOffset] == '\t') {
			++nextOffset;
		}
	}

	return SUCCESS;
}

/**
 * CheckCannedDoc
 * �Ķ���� aCannedDoc���� ���� ������ ���ڿ��� �Ľ��Ͽ� parsing error�� ��� 
 * FALSE(-1)�� �����Ѵ�. �����ΰ�� TRUE(1)�� �����Ѵ�.
 
 * @aCannedDoc				: �Ľ��ϰ����ϴ� ������ ���ڿ�
 * return					: �Ľ̰�� 
 *							  TRUE -> �ùٸ� ����
 *							  FALSE -> �ùٸ��� ���� ����
 */
int CheckCannedDoc(const char *aCannedDoc) {
	int iResult, iLength, nextOffset, iCount;
	char *aTagName;
	
	/* parse document tag */
	iResult = GetContent(aCannedDoc, aDocument, &iLength, 1);
	if (iResult < 0) {
		return FALSE;
	}
	nextOffset = iResult;
	
	/* trim white space next of tag */
	while(aCannedDoc[nextOffset] == '\n' || 
		  aCannedDoc[nextOffset] == ' ' || 
		  aCannedDoc[nextOffset] == '\t') {
		++nextOffset;
	}
	
	/* parse each field tag */
	for (iCount=1; iCount<=iNumOfDocumentField; iCount++) {
		aTagName = pFieldInfos[iCount].aName;
		iResult = GetContent(aCannedDoc+nextOffset, aTagName, &iLength, 0);
		if (iResult < 0) {
			return FALSE;
		}
		nextOffset += iResult + iLength + strlen(aTagName) + 3;

		/* trim white space next of tag */
		while(aCannedDoc[nextOffset] == '\n' || 
			  aCannedDoc[nextOffset] == ' ' || 
			  aCannedDoc[nextOffset] == '\t') {
			++nextOffset;
		}
	}

	return TRUE;
}
