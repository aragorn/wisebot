/*
 * $Id$
 */
#include <stdio.h>
#include <string.h>

#include "softbot.h"
#include "mod_qpp.h"

#include "tokenizer.h"

#define MAX_QPP_OP_NUM			(128)
#define MAX_QPP_OP_STR			(32)

static char  m_aUselessChars[256];	// useless chars like ` ~ ! @ .., etc.
static int   m_isUselessCharsSet = FALSE;

static char  m_aStopChars[256]="&|!:\"()*";
static int   m_isStopCharsSet = TRUE;
static int16_t  m_numStopChars = 8;

static char m_opAND [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"AND","&"};
static int  m_numAND = 2;

static char m_opOR [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"OR","|"};
static int  m_numOR = 2;

static char m_opNOT [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"NOT","!"};
static int  m_numNOT = 2;

static char m_opPARA [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"PARA"};
static int  mParaNum = 1;
static int  m_paraDefaultParam=1;

static char m_opWITHIN [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"/"};
static int  m_numWITHIN = 1;
/*static int  m_withinDefaultParam = 0;*/

static char m_opFUZZY [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"FUZZY"};
static int  m_numFUZZY = 1;

static int  m_numFIELD = 1;
static char m_opFIELD [MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={":"};

static int mNumOfField = 0;
static char mFieldName[MAX_EXT_FIELD][SHORT_STRING_SIZE] = { "" };

static int mVirtualFieldNum = 0;
static char mVirtualFieldName[MAX_EXT_FIELD][SHORT_STRING_SIZE]={"ALL"};
static uint32_t mVirtualFieldIds[MAX_EXT_FIELD] = {0x0E00}; /* 10 11 12 field set*/

static char m_opSTAR[MAX_QPP_OP_NUM][MAX_QPP_OP_STR]={"*"};
static int  m_numSTAR = 1;

static char m_opBEGIN_PHRASE='\"'; 
static char m_opEND_PHRASE='\"'; 
static int  m_isPhraseOperatorSame = TRUE;

static char m_opBEGIN_PAREN = '(';	// begin parenthesis
static char m_opEND_PAREN = ')';

static char m_opNaturalBegin = '[';
static char m_opNaturalEnd= ']';
// Operator setting end

// Private functions
static void skipUselessChars(TokenObj *pTkObj);
static int16_t getNextTokenLength(TokenObj *pTkObj,int16_t offset);
static void copyToken(TokenObj *pTkObj,char dest[],int16_t startIdx,int16_t length);
static int  isUselessChar(char ch);
static int  isEscapeChar(char ch);
static int  isStopChar(TokenObj *pTkObj,char ch);
static int  isPhraseEndChar(char ch);
/*static void setUselessChar(char aGarbage[]);*/
static int16_t copyTranslateEscChar(char strTo[],char strFrom[]);

static void addStopChar(char ch);
/*static void setStopChar(char aStopChar[]);*/
static void addStopCharFromOperator(char *strOperator);

static int  isAlphaNumericOperator(char *opStr);
static int  isNumParamFollowing(char *theString);
static int16_t getParameter(char *theString,int32_t *pNumber);
//static int  isFilterParam(char *tmpStr);
static void pairMatchingProcess(TokenObj *pTkObj);
static void removePhrase(TokenObj *pTkObj);
static void removeParen(TokenObj *pTkObj);
static void removeNatural(TokenObj *pTkObj);

static void operatorProcess(TokenObj *pTkObj,QueryNode *pQueryNode);
static int16_t operatornCmp(TokenObj *pTkObj,char* src, char* opcodeStr,
						 int isParamFollowing);
static int16_t prefixCmp(char* src, char* cmpStr);
static int16_t fieldOpCmp(TokenObj *pTkObj,char* src,int32_t *pOpParam);
static int16_t virtualfieldOpCmp(TokenObj *pTkObj,char* src,int32_t *pOpParam);

static int  isOperator(TokenObj *pTkObj);
static int  isOpAND(TokenObj *pTkObj);
static int  isOpOR(TokenObj *pTkObj);
static int  isOpNOT(TokenObj *pTkObj);
static int  isOpPARA(TokenObj *pTkObj);
static int  isOpWITHIN(TokenObj *pTkObj);
static int  isOpFUZZY(TokenObj *pTkObj);
static int  isOpFIELD(TokenObj *pTkObj);
static int  isOpVirtualFIELD(TokenObj *pTkObj);
static int  isOpSTAR(TokenObj *pTkObj);
static int  isOpBEGIN_PHRASE(TokenObj *pTkObj);
static int  isOpEND_PHRASE(TokenObj *pTkObj);
static int  isOpBEGIN_PAREN(TokenObj *pTkObj);
static int  isOpEND_PAREN(TokenObj *pTkObj);
static int  isNaturalBegin(TokenObj *pTkObj);
static int  isNaturalEnd(TokenObj *pTkObj);

typedef struct {
	int (*isOperator)(TokenObj *pTkObj);
} OpFuncs;

static OpFuncs m_eachOperator[] = {
	{isOpAND},
	{isOpOR},
	{isOpNOT},
	{isOpPARA},
	{isOpWITHIN},
	{isOpFUZZY},
	{isOpFIELD},
	{isOpVirtualFIELD},
	{isOpSTAR},
	{isOpBEGIN_PHRASE},
	{isOpEND_PHRASE},
	{isOpBEGIN_PAREN},
	{isOpEND_PAREN},
	{isNaturalBegin},
	{isNaturalEnd},
	{NULL}
};


#define isEndOfString(par) 	( ((par) == (char)NULL) ? TRUE:FALSE )

int tk_init(){ 
	
	m_opBEGIN_PAREN = '(';
	m_opEND_PAREN = ')';
	addStopChar('(');
	addStopChar(')');
	
	return SUCCESS;
}

void tk_setString(TokenObj *pTkObj,char* theString){
	strncpy(pTkObj->inputStr,theString,MAX_INPUTSTR_LEN);
	pTkObj->inputStr[MAX_INPUTSTR_LEN-1] = '\0';
	pTkObj->idx = 0;
	
	pairMatchingProcess(pTkObj);

	pTkObj->isBeginPhraseTurn = TRUE;
	pTkObj->isPositionWithinPhrase = FALSE;
	pTkObj->lastOperator = QPP_OP_OPERAND;	// not any operator

	skipUselessChars(pTkObj); // get rid of first useless part of string
}

#define MAX_TMP_STR 512
int tk_getNextToken(TokenObj *pTkObj,QueryNode *pQueryNode,int maxLen){
	int16_t length = 0;
	int16_t retVal = 0;
	char tmpStr[MAX_TMP_STR];

	skipUselessChars(pTkObj);
	
	if (isEndOfString(pTkObj->inputStr[pTkObj->idx]) == TRUE){
		return END_OF_STRING;
	}

	if (isOperator(pTkObj) == TRUE) {
		int max=0;
		operatorProcess(pTkObj,pQueryNode);
		pTkObj->lastOperator = pTkObj->currentOperator;

		// operator 다음의 white space등.. 은 없앤다
		skipUselessChars(pTkObj);		
		if (pTkObj->currentOperator != QPP_OP_STAR){
			return TOKEN_OPERATOR;
		}

		// pTkObj->currentOperator is surely QPP_OP_TRUNC
		if (pTkObj->starStrBufferLen >= maxLen)
			return TOKEN_OVERFLOW;

		max = maxLen > MAX_TMP_STR ? MAX_TMP_STR: maxLen;
		sz_strncpy(tmpStr,pTkObj->starStrBuffer,max);
		//copyTranslateEscChar(pQueryNode->word_st.string, tmpStr);
		copyTranslateEscChar(pQueryNode->original_word, tmpStr);

		return TOKEN_STAR;
	}
	
	length = getNextTokenLength(pTkObj,0);
	debug("length of token %d",length);
	
	if (length >= maxLen) {
		warn("token length(%d) is larger than maxLen(%d)",length,maxLen);
		copyToken(pTkObj,tmpStr,pTkObj->idx,maxLen-1);
		pTkObj->idx = pTkObj->idx + length;
	} else if (length == 0) {
		info("length zero occur! ignore [%c]",pTkObj->inputStr[pTkObj->idx]);
		pTkObj->idx++;
		return 0;
	}
	else { // normal case
		copyToken(pTkObj,tmpStr,pTkObj->idx,length);
		pTkObj->idx = pTkObj->idx + length;
	}

//	retVal = copyTranslateEscChar(pQueryNode->word_st.string, tmpStr);
	retVal = copyTranslateEscChar(pQueryNode->original_word, tmpStr);
	pTkObj->lastOperator = QPP_OP_OPERAND;

	debug("retVal [%d]",retVal);
	return retVal;
}


/***************************************************************** 
 *                                                               *
 *                                                               *
 *	below here follows only private functions                    *
 *                                                               *
 *                                                               *
 *****************************************************************/
static void operatorProcess(TokenObj *pTkObj,QueryNode *pQueryNode){
	pQueryNode->operator = pTkObj->currentOperator;
	pQueryNode->num_of_operands = pTkObj->currentNumOperand;
	pQueryNode->opParam = pTkObj->currentOpParam;

	pTkObj->idx = pTkObj->idx + pTkObj->currentOpLen;
}

static int isOperator(TokenObj *pTkObj){
	int i=0;

	// phrase 안에 있을 경우에는 phrase_end operator만 체크한다 AND/OR .. 무시
	if (pTkObj->isPositionWithinPhrase == TRUE){
		return isOpEND_PHRASE(pTkObj);
	}

	i=0;
	while(m_eachOperator[i].isOperator != NULL) {
		if (m_eachOperator[i].isOperator(pTkObj) == TRUE)
			return TRUE;
		i++;
	}

	return FALSE;
}

static int  isAlphaNumericOperator(char *opStr){
	if (opStr[0] >= '!' && opStr[0] <= '/')
		return FALSE;
	if (opStr[0] >= ':' && opStr[0] <= '@')
		return FALSE;
	if (opStr[0] >= '[' && opStr[0] <= '`')
		return FALSE;
	if (opStr[0] >= '{' && opStr[0] <= '~')
		return FALSE;

	return TRUE;	
}
static int16_t prefixCmp(char* src, char* cmpStr){
	int i = 0;
	while (1) {
		if (cmpStr[i] == (char)NULL)
			break;

		if (src[i] == (char)NULL)
			return 0;
				
		if (src[i] != cmpStr[i]){
			return 0;
		}
		i++;
	}
	return i;
}
static int16_t operatornCmp(TokenObj *pTkObj,char* src, char* opcodeStr, int isParamFollowing){
	int16_t len = 0;

	len = prefixCmp(src,opcodeStr);
	if (len <= 0)
		return 0;

	if (isAlphaNumericOperator(opcodeStr) == FALSE)
		return len;

	// non-alphanumeric operator is cutted above..
	// alphanumeric operator like AND, OR, NOT, 의 경우
	// 뒤에 white space가 있어야 함
	if (isUselessChar(src[len]) == TRUE)
		return len;

	if (isEndOfString(src[len]) == TRUE)
		return len;	

	if (isStopChar(pTkObj,src[len]) == TRUE){
		return len;
	}

	if (isParamFollowing == FALSE)
		return 0;

	// parameter를 가진 operator (NEAR10, NEAR/10, WITHIN9, WITHIN/9, ...)
	// 의 경우..
	if (isNumParamFollowing(src+len) == TRUE){
		return len;
	}

	return 0;
}
static int16_t virtualfieldOpCmp(TokenObj *pTkObj,char* src,int32_t *pOpParam){
	int16_t i = 0;
	int16_t opLen = 0;
	int16_t paramLen = 0;

	// field search name like "body", "title" matching
	for (i = 0; i<mVirtualFieldNum; i++){
		paramLen = prefixCmp(src,mVirtualFieldName[i]);
		if (paramLen > 0)
			break;
	}
	if (paramLen == 0)
		return 0;
	
	*pOpParam = mVirtualFieldIds[i];
		
	// field operator like ":" matching	
	for (i = 0; i<m_numFIELD; i++){
		opLen = operatornCmp(pTkObj,src+paramLen, m_opFIELD[i],FALSE);
		if (opLen > 0)
			break;
	}
	if (opLen == 0)
		return 0;

	return paramLen + opLen;
}

static int16_t fieldOpCmp(TokenObj *pTkObj,char* src,int32_t *pOpParam){
	int16_t i = 0;
	int16_t opLen = 0;
	int16_t paramLen = 0;

	// field search name like "body", "title" matching
	for (i = 0; i<mNumOfField; i++) {
		int len = strlen(mFieldName[i]);

		if (len == 0) continue;
        if (strncasecmp(src, mFieldName[i], len) == 0 && src[len] == ':') {
			INFO("src = [%s], field[%d] = [%s]", src, i, mFieldName[i]);
			paramLen = len;
			break;
        }
	}
	if (paramLen == 0)
		return 0;
	
	*pOpParam = i;
		
	// field operator like ":" matching	
	for (i = 0; i<m_numFIELD; i++){
		opLen = operatornCmp(pTkObj,src+paramLen, m_opFIELD[i],FALSE);
		if (opLen > 0)
			break;
	}
	if (opLen == 0)
		return 0;

	return paramLen + opLen;
}

static int  isNumParamFollowing(char *theString){
	if (theString[0] == (char)NULL)
		return FALSE;	

	if (theString[0] >= '0' && theString[0] <= '9')
		return TRUE;

	if (theString[1] >= '0' && theString[1] <= '9')
		return TRUE;

	return FALSE;	
}
static int16_t getParameter(char *theString, int32_t *pNumber){
	int16_t startIdx,endIdx,len,idx;
	char strNum[256];
	int32_t resultNum = 0;

	if (isNumParamFollowing(theString) == FALSE){
		return 0;
	}

	if (theString[0] >= '0' && theString[0] <= '9')
		startIdx = 0;
	else
		startIdx = 1;	 
	for (idx = startIdx; ; idx++){
		if (theString[idx] < '0')
			break;
		if (theString[idx] > '9')
			break;
	}
	endIdx = idx;

	len = endIdx - startIdx;
	strncpy(strNum,theString+startIdx,len);
	strNum[len] = '\0';
	resultNum = atoi(strNum);

	*pNumber = resultNum;

	return endIdx;
}
/*
static int  isFilterParam(char *theStr) {
	int16_t i = 0;
	char isRangeOperatorFound = FALSE;

	for (i = 0; theStr[i] != '\0'; i++) {
		if (isRangeOperatorFound == TRUE && theStr[i] == '-')
			return FALSE;
		else if (isRangeOperatorFound == FALSE && theStr[i] == '-') {
			isRangeOperatorFound = TRUE;
		}
		else if (theStr[i] < '0') {
			return FALSE;
		}
		else if (theStr[i] > '9') {
			return FALSE;
		}
	}

	return TRUE;
}
*/

static int isOpAND(TokenObj *pTkObj){
	int  i;
	int16_t opLen=0;
	for (i=0; i<m_numAND; i++) {
		opLen = operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opAND[i],FALSE);
		if (opLen > 0){
			pTkObj->currentOperator = QPP_OP_AND;
			pTkObj->currentOpLen = opLen;
			pTkObj->currentNumOperand = 2;
			pTkObj->currentOpParam = 0;
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpOR(TokenObj *pTkObj){
	int  i;
	int16_t opLen=0;

	for (i=0; i<m_numOR; i++) {
		opLen = operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opOR[i],FALSE);
		if (opLen > 0){
			pTkObj->currentOperator = QPP_OP_OR;
			pTkObj->currentOpLen = opLen;
			pTkObj->currentNumOperand = 2;
			pTkObj->currentOpParam = 0;
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpNOT(TokenObj *pTkObj){
	int  i;
	int16_t opLen=0;
	for (i=0; i<m_numNOT; i++) {
		opLen = operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opNOT[i],FALSE);
		if (opLen > 0){
			pTkObj->currentOperator = QPP_OP_NOT;
			pTkObj->currentOpLen = opLen;
			pTkObj->currentNumOperand = 1;
			pTkObj->currentOpParam = 0;
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpPARA(TokenObj *pTkObj){
	int  i;
	int16_t opLen = 0;
	int16_t paramLen = 0;
	int32_t opParam = 0;

	for (i=0; i<mParaNum; i++) {
		opLen = 
			operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opPARA[i],TRUE);
		if (opLen > 0){
			paramLen = 
				getParameter(pTkObj->inputStr+pTkObj->idx+opLen,&opParam);
			if (opParam == 0)
				opParam = m_paraDefaultParam;

			pTkObj->currentOperator = QPP_OP_PARA;
			pTkObj->currentOpLen = opLen + paramLen;
			pTkObj->currentNumOperand = 2;
			pTkObj->currentOpParam = opParam;
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpWITHIN(TokenObj *pTkObj){
	int  i;
	int16_t opLen = 0;
	int16_t paramLen = 0;
	int32_t opParam = 0;
	
	debug("is op within operator check section");
	
	for (i=0; i<m_numWITHIN; i++) {
		opLen = operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opWITHIN[i],TRUE);
		debug("opLen(%d)",opLen);
		
		if (opLen > 0){
			paramLen = getParameter(pTkObj->inputStr+pTkObj->idx+opLen,&opParam);
			
/*			opLen = operatornCmp(pTkObj, pTkObj->inputStr+ (pTkObj->idx + paramLen)
								 , m_opWITHIN[i],TRUE);
			debug("in opLen(%d)",opLen);
			
			if(opLen <= 0)
				return FALSE;*/
			if (paramLen <= 0) /* XXX: added to check "/" without number */
				return FALSE;
			
			WARN("checking after /");
			if (pTkObj->inputStr[pTkObj->idx+opLen+paramLen] != '/') {
				WARN("pTkObj->inputStr[pTkObj->idx+opLen+paramLen]:%c is not /",
									pTkObj->inputStr[pTkObj->idx+opLen+paramLen]);
				return FALSE;
			}

			pTkObj->currentOperator = QPP_OP_WITHIN;

			// +1 for '/'
			pTkObj->currentOpLen = opLen + paramLen + 1;
			
			pTkObj->currentNumOperand = 2;
			pTkObj->currentOpParam = opParam;
			
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpFUZZY(TokenObj *pTkObj){
	int  i;
	int16_t opLen=0;
	for (i=0; i<m_numFUZZY; i++) {
		opLen = operatornCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,m_opFUZZY[i],FALSE);
		if (opLen > 0){
			pTkObj->currentOperator = QPP_OP_FUZZY;
			pTkObj->currentOpLen = opLen;
			pTkObj->currentNumOperand = 2;
			pTkObj->currentOpParam = 0;
			return TRUE;
		}
	}
	return FALSE;
}
static int isOpFIELD(TokenObj *pTkObj){
	int16_t opLen = 0;
	int32_t opParam = 0;

	//if opLen == 0 then field operator name not match with (title, body)..
	opLen = fieldOpCmp(pTkObj,pTkObj->inputStr+pTkObj->idx,&opParam);
	
	if (opLen > 0){
		pTkObj->currentOperator = QPP_OP_FIELD;
		pTkObj->currentOpLen = opLen;
		pTkObj->currentNumOperand = 1;
		pTkObj->currentOpParam = opParam;
		return TRUE;
	}
	return FALSE;
}
static int isOpVirtualFIELD(TokenObj *token)
{
	int16_t opLen = 0;
	int32_t opParam = 0;

	opLen = virtualfieldOpCmp(token, token->inputStr+ token->idx, &opParam);

	if (opLen > 0) {
		token->currentOperator = QPP_OP_VIRTUAL_FIELD;
		token->currentOpLen = opLen;
		token->currentNumOperand = 1;
		token->currentOpParam = opParam; /* virtualfield */
		return TRUE;
	}

	return FALSE;
}
static int isOpSTAR(TokenObj *pTkObj){
	int16_t opLen = 0;
	int16_t length = 0;
	int16_t i,offset;

	// *SomeThing 의 경우
	for (i=0; i<m_numSTAR; i++) {
		opLen = prefixCmp(pTkObj->inputStr+pTkObj->idx,m_opSTAR[i]); 
		if (opLen > 0){
			pTkObj->currentOperator = QPP_OP_STAR;
			pTkObj->currentNumOperand = 1;
			length = getNextTokenLength(pTkObj,opLen);
	
			copyToken(pTkObj,pTkObj->starStrBuffer,pTkObj->idx+opLen,length);
			pTkObj->currentOpLen = length + opLen;
			pTkObj->currentOpParam = STAR_BEGIN;
			pTkObj->starStrBufferLen = length;
			return TRUE;
		}
	}

	// SomeThing* 의 경우
	for (offset=0; pTkObj->inputStr[pTkObj->idx+offset] != '\0'; offset++){
		if (isUselessChar(pTkObj->inputStr[pTkObj->idx+offset]) == TRUE)
			return FALSE;

		for (i=0; i<m_numSTAR; i++) {
			opLen = prefixCmp(pTkObj->inputStr+pTkObj->idx+offset,m_opSTAR[i]); 
			if (opLen > 0){
				pTkObj->currentOperator = QPP_OP_STAR;
				pTkObj->currentNumOperand = 1;
				length = getNextTokenLength(pTkObj,0);
		
				copyToken(pTkObj,pTkObj->starStrBuffer,pTkObj->idx,length);
				pTkObj->currentOpLen = opLen + length;
				pTkObj->currentOpParam = STAR_END;
				pTkObj->starStrBufferLen = length;
				return TRUE;
			}
		}
		if (isStopChar(pTkObj,pTkObj->inputStr[pTkObj->idx+offset]) == TRUE)
			return FALSE;
	}

	return FALSE;
}
static int isNaturalBegin(TokenObj *pTkObj) {
	if (pTkObj->inputStr[pTkObj->idx] == m_opNaturalBegin) {
		pTkObj->currentOperator = QPP_OP_NATURAL_BEGIN;
		pTkObj->currentOpLen = 1; // '['  's length is 1
		return TRUE;
	}
	return FALSE;
}
static int isNaturalEnd(TokenObj *pTkObj) {
	if (pTkObj->inputStr[pTkObj->idx] == m_opNaturalEnd) {
		pTkObj->currentOperator = QPP_OP_NATURAL_END;
		pTkObj->currentOpLen = 1; // ']'  's length is 1
		return TRUE;
	}
	return FALSE;
}

static int isOpBEGIN_PHRASE(TokenObj *pTkObj){
	if (pTkObj->inputStr[pTkObj->idx] != m_opBEGIN_PHRASE)
		return FALSE;
	else if (m_isPhraseOperatorSame == FALSE){
		pTkObj->isPositionWithinPhrase = TRUE;

		pTkObj->currentOperator = QPP_OP_BEG_PHRASE;
		pTkObj->currentOpLen = 1;
		return TRUE;
	}
	else if (pTkObj->isBeginPhraseTurn == TRUE){
		pTkObj->isPositionWithinPhrase = TRUE;

		pTkObj->currentOperator = QPP_OP_BEG_PHRASE;
		pTkObj->currentOpLen = 1;
		pTkObj->isBeginPhraseTurn = FALSE;
		return TRUE;
	}
	else
		return FALSE;

	return FALSE;
}
static int isOpEND_PHRASE(TokenObj *pTkObj){
	if (pTkObj->inputStr[pTkObj->idx] != m_opEND_PHRASE)
		return FALSE;

	if (m_isPhraseOperatorSame == FALSE){
		pTkObj->isPositionWithinPhrase = FALSE;

		pTkObj->currentOperator = QPP_OP_END_PHRASE;
		pTkObj->currentOpLen = 1;
		return TRUE;
	}

	if (pTkObj->isBeginPhraseTurn == FALSE){
		pTkObj->isPositionWithinPhrase = FALSE;

		pTkObj->currentOperator = QPP_OP_END_PHRASE;
		pTkObj->currentOpLen = 1;
		pTkObj->isBeginPhraseTurn = TRUE;
		return TRUE;
	}
	return FALSE;
}
static int isOpBEGIN_PAREN(TokenObj *pTkObj){
	if (pTkObj->inputStr[pTkObj->idx] == m_opBEGIN_PAREN){
		pTkObj->currentOperator = QPP_OP_BEG_PAREN;
		pTkObj->currentOpLen = 1;
		return TRUE;
	}
	return FALSE;
}
static int isOpEND_PAREN(TokenObj *pTkObj){
	if (pTkObj->inputStr[pTkObj->idx] == m_opEND_PAREN){
		pTkObj->currentOperator = QPP_OP_END_PAREN;
		pTkObj->currentOpLen = 1;
		return TRUE;
	}
	return FALSE;
}

/*void setUselessChar(char aGarbage[]){// aGarbage should be terminated with NULL*/
/*	strncpy(m_aUselessChars,aGarbage,255);*/
/*	m_aUselessChars[255] = (char)NULL;*/
/*	m_isUselessCharsSet = TRUE;*/
/*}*/

/*void setStopChar(char aStopChar[]){// aGarbage should be terminated with NULL*/
/*	strncpy(m_aStopChars,aStopChar,255);*/
/*	m_aStopChars[255] = (char)NULL;*/
/*	m_isStopCharsSet = TRUE;*/
/*}*/

void addStopCharFromOperator(char *strOperator){
	if (strOperator[0] >= 'a' && strOperator[0] <= 'z')
		return;
	if (strOperator[0] >= 'A' && strOperator[0] <= 'Z')
		return;

	addStopChar(strOperator[0]);
}
void addStopChar(char ch){
	if (m_numStopChars < 256){
		m_aStopChars[m_numStopChars] = ch;
		m_numStopChars++;
		m_aStopChars[m_numStopChars] = '\0';
		m_isStopCharsSet = TRUE;
	}
}

int isStopChar(TokenObj *pTkObj,char ch){
	int i=0;
	
	if (pTkObj->isPositionWithinPhrase == TRUE)
		return FALSE;	
	
	if (ch == m_opBEGIN_PAREN || ch == m_opEND_PAREN
			|| ch == m_opBEGIN_PHRASE || ch == m_opEND_PHRASE){
		return TRUE;
	}

	if (ch == m_opNaturalBegin || ch == m_opNaturalEnd)
		return TRUE;

	if (m_isStopCharsSet == FALSE)
		return FALSE;

	for (i=0; m_aStopChars[i]!=(char)NULL; i++) {
		if (m_aStopChars[i] == ch)
			return TRUE;
	}
	return FALSE;
}

int isPhraseEndChar(char ch){
	if (ch == m_opEND_PHRASE)
		return TRUE;

	return FALSE;	
}

int isUselessChar(char ch){
	int i=0;
	if (ch == ' '||ch == '\t'){
		return TRUE;
	}
	if (m_isUselessCharsSet == FALSE)
		return FALSE;

	for (i=0; m_aUselessChars[i]!=(char)NULL; i++) {
		if (m_aUselessChars[i] == ch)
			return TRUE;
	}
	return FALSE;
}

int isEscapeChar(char ch){
	if (ch == '%')
		return TRUE;
	return FALSE;	
}

void skipUselessChars(TokenObj *pTkObj){
	while (isEndOfString(pTkObj->inputStr[pTkObj->idx]) != TRUE) {
		if ( isUselessChar(pTkObj->inputStr[pTkObj->idx]) == TRUE ) {
			pTkObj->idx++;
		}
		else
			break;
	}
}
int16_t copyTranslateEscChar(char strTo[],char strFrom[]){
	int16_t toIdx = 0;
	int16_t fromIdx = 0;
	int16_t specialCharLen = 0;
	int32_t tmpNum = 0;
	
	while (1){
		// %로 encoding된 문자들을 decode
		if (isEscapeChar(strFrom[fromIdx]) == TRUE){
			specialCharLen  = getParameter(strFrom+fromIdx,&tmpNum);
		
			//susia inser: encoding 과 관련없는 % 처리 
			if (specialCharLen == 0) {
				strTo[toIdx++] = strFrom[fromIdx++];
				continue;
			}
			
			strTo[toIdx] = (char)tmpNum;
			fromIdx = fromIdx + specialCharLen;
			toIdx++;
			continue;
		}
		else
			strTo[toIdx] = strFrom[fromIdx];
		
		if (strFrom[fromIdx] == '\0')
			break;

		toIdx++;
		fromIdx++;
	}
	return toIdx;
}

int16_t getNextTokenLength(TokenObj *pTkObj,int16_t offset){
	int16_t length = 0;

	for (length=0; ; length++) {
		if (isPhraseEndChar(pTkObj->inputStr[pTkObj->idx+offset+length]) == TRUE)
			break;
		
		if (isStopChar(pTkObj,pTkObj->inputStr[pTkObj->idx+offset+length]) == TRUE)
			break;
		
		if (isUselessChar(pTkObj->inputStr[pTkObj->idx+offset+length]) == TRUE)
			break; 
		
		if (isEndOfString(pTkObj->inputStr[pTkObj->idx+offset+length]) == TRUE)
			break;
	}
	return length;
}

void copyToken(TokenObj *pTkObj,char dest[],int16_t startIdx,int16_t length){
	strncpy(dest,&(pTkObj->inputStr[startIdx]),length);
	dest[length] = '\0';
}

// phrase("), parenthesis(괄호)의 짝이 맞는지 체크하고, 맞지 않으면
// " 나, 괄호를 지워버린다.
void pairMatchingProcess(TokenObj *pTkObj){
	int16_t numBeginParen = 0, numEndParen = 0;
	int isParenMatchSuccess = TRUE;
	int16_t numBeginPhrase = 0, numEndPhrase = 0;
	int isPhraseMatchSuccess = TRUE;
	int16_t numNaturalBegin = 0, numNaturalEnd = 0;

	pTkObj->isBeginPhraseTurn = TRUE;
	pTkObj->isPositionWithinPhrase = FALSE;

	for (pTkObj->idx = 0; pTkObj->inputStr[pTkObj->idx] != '\0'; pTkObj->idx++) {
		if (isOpBEGIN_PHRASE(pTkObj) == TRUE)
			numBeginPhrase++;
		else if (isOpEND_PHRASE(pTkObj) == TRUE)
			numEndPhrase++;

		if (isNaturalBegin(pTkObj) == TRUE)
			numNaturalBegin++;
		else if (isNaturalEnd(pTkObj) == TRUE)
			numNaturalEnd++;

		if (isOpBEGIN_PAREN(pTkObj) == TRUE)
			numBeginParen++;
		else if (isOpEND_PAREN(pTkObj) == TRUE)
			numEndParen++;

		if (numBeginParen < numEndParen)
			isParenMatchSuccess = FALSE;
		if (numBeginPhrase < numEndPhrase)
			isPhraseMatchSuccess = FALSE;
	}

	if (numBeginPhrase != numEndPhrase)
		isPhraseMatchSuccess = FALSE;

	if (numBeginParen != numEndParen)
		isParenMatchSuccess = FALSE;

	if (numNaturalBegin != numNaturalEnd)
		removeNatural(pTkObj);

	if (isPhraseMatchSuccess == FALSE)
		removePhrase(pTkObj);		

	if (isParenMatchSuccess == FALSE)
		removeParen(pTkObj);

///	get rid of side effect
	pTkObj->idx = 0;
}

void removeNatural(TokenObj *pTkObj){
	int16_t i=0;

	for (i=0; pTkObj->inputStr[i] != '\0'; i++) {
		if (pTkObj->inputStr[i] == m_opNaturalBegin || 
				pTkObj->inputStr[i] == m_opNaturalEnd)
			pTkObj->inputStr[i] = ' ';
	}
}

void removePhrase(TokenObj *pTkObj){
	int16_t i = 0;
		
	for (i = 0; pTkObj->inputStr[i] != '\0'; i++){

		if (pTkObj->inputStr[i] == m_opBEGIN_PHRASE ||
				pTkObj->inputStr[i] == m_opEND_PHRASE){
			pTkObj->inputStr[i] = ' ';
		}
	}
}

void removeParen(TokenObj *pTkObj){
	int16_t i = 0;

	for (i = 0; pTkObj->inputStr[i] != '\0'; i++){
		if (pTkObj->inputStr[i] == m_opBEGIN_PAREN ||
				pTkObj->inputStr[i] == m_opEND_PAREN){
			pTkObj->inputStr[i] = ' ';
		}
	}
}

// configuration related functions
void set_op_and(configValue v)
{
	int i=0;
	m_numAND = v.argNum;

	if (m_numAND > MAX_QPP_OP_NUM) {
		warn("Maximum possible AND operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numAND = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numAND; i++) {
		strncpy(m_opAND[i],v.argument[i],MAX_QPP_OP_STR);
		m_opAND[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opAND[i]);
		debug("%dth AND operator: %s",i,m_opAND[i]);
	}
}
void set_op_or(configValue v)
{
	int i=0;
	m_numOR = v.argNum;

	if (m_numOR > MAX_QPP_OP_NUM) {
		warn("Maximum possible OR operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numOR = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numOR; i++) {
		strncpy(m_opOR[i],v.argument[i],MAX_QPP_OP_STR);
		m_opOR[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opOR[i]);
		debug("%dth OR operator: %s",i,m_opOR[i]);
	}
}
void set_op_not(configValue v)
{
	int i=0;
	m_numNOT = v.argNum;

	if (m_numNOT > MAX_QPP_OP_NUM) {
		warn("Maximum possible NOT operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numNOT = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numNOT; i++) {
		strncpy(m_opNOT[i],v.argument[i],MAX_QPP_OP_STR);
		m_opNOT[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opNOT[i]);
		debug("%dth NOT operator: %s",i,m_opNOT[i]);
	}
}
void set_op_paragraph(configValue v)
{
	int i=0;
	mParaNum = v.argNum;

	if (mParaNum > MAX_QPP_OP_NUM) {
		warn("Maximum possible NEAR operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		mParaNum = MAX_QPP_OP_NUM;
	}

	for (i=0; i<mParaNum; i++) {
		strncpy(m_opPARA[i],v.argument[i],MAX_QPP_OP_STR);
		m_opPARA[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opPARA[i]);
		debug("%dth NEAR operator: %s",i,m_opPARA[i]);
	}
}
void set_op_within(configValue v)
{
	int i=0;
	m_numWITHIN = v.argNum;

	if (m_numWITHIN > MAX_QPP_OP_NUM) {
		warn("Maximum possible WITHIN operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numWITHIN = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numWITHIN; i++) {
		strncpy(m_opWITHIN[i],v.argument[i],MAX_QPP_OP_STR);
		m_opWITHIN[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opWITHIN[i]);
		debug("%dth WITHIN operator: %s",i,m_opWITHIN[i]);
	}
}
void set_op_fuzzy(configValue v)
{
	int i=0;
	m_numFUZZY = v.argNum;

	if (m_numFUZZY > MAX_QPP_OP_NUM) {
		warn("Maximum possible FUZZY operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numFUZZY = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numFUZZY; i++) {
		strncpy(m_opFUZZY[i],v.argument[i],MAX_QPP_OP_STR);
		m_opFUZZY[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opFUZZY[i]);
		debug("%dth FUZZY operator: %s",i,m_opFUZZY[i]);
	}
}
void set_op_field(configValue v)
{
	int i=0;
	m_numFIELD = v.argNum;

	if (m_numFIELD > MAX_QPP_OP_NUM) {
		warn("Maximum possible FIELD operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numFIELD = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numFIELD; i++) {
		strncpy(m_opFIELD[i],v.argument[i],MAX_QPP_OP_STR);
		m_opFIELD[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opFIELD[i]);
		debug("%dth FIELD operator: %s",i,m_opFIELD[i]);
	}
}

void set_virtual_fieldname(configValue v)
{
	int i=0, id=0;

	strncpy(mVirtualFieldName[mVirtualFieldNum], v.argument[0], SHORT_STRING_SIZE);
	mVirtualFieldName[mVirtualFieldNum][SHORT_STRING_SIZE-1] = '\0';
	
	mVirtualFieldIds[mVirtualFieldNum] = 0;
	for (i=1; i<v.argNum; i++) {
		id = atol(v.argument[i]);
		mVirtualFieldIds[mVirtualFieldNum] |= (1 << id);
	}
	mVirtualFieldNum++;
	return;
}

void set_fieldname(configValue v)
{
	int fieldid=0;

	if (v.argNum < 6) {
		error("Field directive must have 5 args at least.");
		error("\t e.g) Field 1 Court yes(index) 0(morp_id) 0(morp_id) yes(paragraph search)");
		error("\t but argNum:%d for Field %s",v.argNum,v.argument[0]);
		return ;
	}

	if (strcasecmp("YES",v.argument[2]) != 0) {
		debug("Field %s does not need indexing",v.argument[1]);
		return;
	}

	fieldid = atoi(v.argument[0]);
	if (fieldid >= MAX_EXT_FIELD) {
		error("fieldid(%d) for Field:%s >= MAX_EXT_FIELD(%d)",
					fieldid, v.argument[1], MAX_EXT_FIELD);
		return ;
	}

	strncpy(mFieldName[fieldid],v.argument[1],MAX_FIELD_STRING);
	mFieldName[fieldid][MAX_FIELD_STRING-1] = '\0';
	warn("mFieldName[%d] = %s", fieldid, mFieldName[fieldid]);
	
	mMorpAnalyzerId[fieldid]=atoi(v.argument[4]);

	if (mNumOfField != fieldid)
		warn("Field should be sorted by fieldid (ascending)");

	mNumOfField = (fieldid+1 > mNumOfField) ? fieldid+1 : mNumOfField;

	return;
}

void set_op_star(configValue v)
{
	int i=0;
	m_numSTAR= v.argNum;

	if (m_numSTAR > MAX_QPP_OP_NUM) {
		warn("Maximum possible FIELD operator is %d, but configured %d",
											MAX_QPP_OP_NUM,	v.argNum);
		warn("overflows are ignored");
		m_numSTAR = MAX_QPP_OP_NUM;
	}

	for (i=0; i<m_numSTAR; i++) {
		strncpy(m_opSTAR[i],v.argument[i],MAX_QPP_OP_STR);
		m_opSTAR[i][MAX_QPP_OP_STR-1] = '\0';

		addStopCharFromOperator(m_opSTAR[i]);
		debug("%dth STAR operator: %s",i,m_opSTAR[i]);
	}
}
void set_op_phrase(configValue v)
{
	if (v.argNum == 1) {
		m_isPhraseOperatorSame = TRUE;
		m_opBEGIN_PHRASE = v.argument[0][0];
		m_opEND_PHRASE = m_opBEGIN_PHRASE;

		addStopChar(m_opBEGIN_PHRASE);
	}
	else if (v.argNum == 2) {
		m_isPhraseOperatorSame = FALSE;
		m_opBEGIN_PHRASE = v.argument[0][0];
		m_opEND_PHRASE = v.argument[1][0];

		addStopChar(m_opBEGIN_PHRASE);
		addStopChar(m_opEND_PHRASE);
	}
	else {
		warn("phrase operator can be wrongly set");
		m_isPhraseOperatorSame = TRUE;
		m_opBEGIN_PHRASE = v.argument[0][0];
		m_opEND_PHRASE = m_opBEGIN_PHRASE;

		addStopChar(m_opBEGIN_PHRASE);
	}
}

void set_begin_op_phrase(configValue v)
{
	m_opBEGIN_PHRASE = v.argument[0][0];
}

void set_end_op_phrase(configValue v)
{
	m_opEND_PHRASE = v.argument[0][0];
}
