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
 * 입력TEXT pInput으로 부터 정해진 태그(pTag)로 둘러 쌓인 부분의
 * 시작 offset을 리턴하고 pLen에는 그 길이를 저장한다.
 * pInput은 반드시 시작태그로 시작하는 문자열이어야 한다.

 * @pInput		: 입력TEXT
 * @pTag		: 입력TAG
 * @pLen		: 내용길이 출력
 * @depth       : if 1, more depth, if 0, no more depth
 * return		: pInput중에서 내용의 시작오프셋
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
 * 파라미터 aCannedDoc으로 들어온 문서의 문자열을 파싱하여 필요한 필드의 값이
 * 모두 갖추어져 있는지 체크하고 각각의 필드 내용들의 오프셋과 문자열 길이를
 * Element 구조체의 배열형태로 저장해 리턴한다. parsing error인 경우 NULL을 
 * 리턴한다.
 
 * @aCannedDoc				: 파싱하고자하는 문서의 문자열
 * @pElement				: 파싱결과를 담을 구조체의 배열
 * return					: 파싱결과
 *							  FAIL -> 요류가 있거나 파싱 실패
 *							  SUCCESS -> 파싱 성공
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
 * 파라미터 aCannedDoc으로 들어온 문서의 문자열을 파싱하여 parsing error인 경우 
 * FALSE(-1)를 리턴한다. 정상인경우 TRUE(1)를 리턴한다.
 
 * @aCannedDoc				: 파싱하고자하는 문서의 문자열
 * return					: 파싱결과 
 *							  TRUE -> 올바른 문서
 *							  FALSE -> 올바르지 않은 문서
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
