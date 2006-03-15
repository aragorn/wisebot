/* $Id$ */
#include "softbot.h"
#include "mp_api.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/udp.h"
#include "mod_api/did_daemon.h"

#include <sched.h>
#include <pthread.h>

#include "mod_did_daemon.h"
#include "mod_did_message.h"

#define NUM_OF_CHILD	1

/******************************************************************************/
static uint32_t DSgetPrefix(char *szKey);
static uint32_t DSgetPrefixWithPrefixBit(char *szKey, int16_t byPrefixBit);

/*static int16_t DSsortDidBK(CDidBK *bucket);*/
static did_slot_t* DSfindDid(char *szKey);
static did_slot_t* DSfindDid_BS ( CDidBK *szBK , char *szKey , int16_t StartPos , 
								int16_t FinishPos );
static CDidBK* DSfindBK ( char *szKey );

static did_slot_t* DSaddDidSL(CDidBK *bucket, did_slot_t slot);
/*static did_slot_t* DSresolveURLfind ( char *szUrl );*/
/*static did_slot_t* DSresolveURLupdate ( char *szUrl , CDidState szState);*/
/*static did_slot_t* DSresolveURLnew (char *szUrl);*/
static did_slot_t* DSnewSL ( char *szUrl , DocId Did );

static int16_t DSdoubleDidBKTable(void);
static int16_t DSpartitionDidBKTable(CDidBK *bucket);

static int16_t DSmd5 ( char* szUrl, char* key );

static void DSsaveData (void);
static void load_data (void);
static did_slot_t* load_slot	(did_slot_t tmpSL);
/******************************************************************************/

static DocId gDidCount;
static int32_t DSsaveCount=0;
static did_slot_t DSsaveSL[MAX_LOAD_BLOCK];

static int slave_thread_num = MAX_PC;
static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(NUM_OF_CHILD) };

static pthread_mutex_t sock_lock;
static pthread_mutex_t slot_lock;

static int thread_main(slot_t *slot);
static void did_server_kill (int);

/*static int8_t DSDidState;*/
/*static int8_t DSDidOperation;*/
/*static int8_t UDP_LOST_TEST=0;*/

static char m_didSlotPath[256] = DB_PATH"/did/docid.slots";
static char mBindAddr[SHORT_STRING_SIZE] = "127.0.0.1";
static char mBindPort[SHORT_STRING_SIZE] = "";
static int udp_sock;

// internal function definitions
static int isProperProtocol(ClientMsg *msg); 
static int executeClientMsg(ServerMsg *svrmsg, ClientMsg *climsg);
static int findId(char* pKey, ServerMsg *msg);
static int makeId(char* pKey, ServerMsg *msg);

static void _do_nothing(int sig);
static void _shutdown(int sig);
static void _graceful_shutdown(int sig);

/*************************** find, make DocId *******************************/
static int findId(char* pKey, ServerMsg *m_svrMsg){
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t *didSlot;

	DSmd5(pKey, tmpKey);
	didSlot = DSfindDid(tmpKey);

	m_svrMsg->result = 0;
	if (didSlot != NULL) {	// if founded
		m_svrMsg->status = DI_OLD_REGISTERED;
		m_svrMsg->result = didSlot->docId;
		return SUCCESS;
	}

	// not found (not registered key)
	m_svrMsg->status = DI_NOT_REGISTERED;
	return FAIL;
}

static int findIdandDelete(char* pKey, ServerMsg *m_svrMsg){
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t *didSlot;

	DSmd5(pKey, tmpKey);
	didSlot = DSfindDid(tmpKey);

	m_svrMsg->result = 0;
	if (didSlot != NULL) {	// if founded
		m_svrMsg->status = DI_OLD_REGISTERED;
		m_svrMsg->result = didSlot->docId;
		memset(didSlot, 0x00, sizeof(did_slot_t));
		return SUCCESS;
	}

	// not found (not registered key)
	m_svrMsg->status = DI_NOT_REGISTERED;
	return FAIL;
}

static int makeId(char *pKey, ServerMsg *m_svrMsg){
	DocId docId=0;
	did_slot_t *pTmpSL;
	char oldregistered=0;

	m_svrMsg->olddid = 0;
	if (findIdandDelete(pKey, m_svrMsg) == SUCCESS) {
		m_svrMsg->olddid = m_svrMsg->result;
		m_svrMsg->status = DI_OLD_REGISTERED;
		oldregistered = 1;
	}

	// 제일 큰 수는 만약을 위해 사용하지 않는다.
	docId = gDidCount+1;
	if (docId == 0) { // id overflow
		m_svrMsg->status = DI_ID_OVERFLOW;
	}
	else {		// if not id overflow ( gDidCount<=2^sizeof(int)-1 )
/*		debug("here !!!! : gDidCount %ld", gDidCount);*/
		docId = gDidCount;
		gDidCount++;
		if (!oldregistered)
			m_svrMsg->status = DI_NEW_REGISTERED;
		m_svrMsg->result = docId;

		pTmpSL = DSnewSL(pKey, docId);
		if (pTmpSL == NULL) {
			return FAIL;
		}

		memcpy(&DSsaveSL[DSsaveCount++] , pTmpSL , sizeof(did_slot_t));

		if (!(DSsaveCount%MAX_LOAD_BLOCK))
		{
			DSsaveData();
			DSsaveCount=0;
		}
	}


	return SUCCESS;
}
/****************************************************************************/

//m_cliMsg.majorVersion == CURRENT_MAJOR_VER (need difinition at the head file)
static int isProperProtocol(ClientMsg *m_cliMsg)
{
	if (strcmp(m_cliMsg->aProtocolId, "DITP") != 0)
		return FALSE;
/*	if (m_cliMsg.majorVersion != 0)*/
/*		return FALSE;*/
/*	if (m_cliMsg.minorVersion != 0)*/
/*		return FALSE;		*/

	return TRUE;
}

// execute the method for which client calls,
// and prepare for the server message
static int executeClientMsg(ServerMsg *m_svrMsg, ClientMsg *m_cliMsg)
{
	char *pKey;
/*	did_slot_t *didSlot;*/

	pKey = m_cliMsg->mesg.aKey;
	if (m_cliMsg->mesg.nKeyLen == MAX_DID_KEY_LEN) {
		pKey[MAX_DID_KEY_LEN-1] = '\0';
	}
	else {
		pKey[m_cliMsg->mesg.nKeyLen] = '\0';
	}
	debug("key: %s",pKey);
	m_svrMsg->nMethod = m_cliMsg->mesg.nMethod;
	m_svrMsg->session = m_cliMsg->session;

	switch (m_cliMsg->mesg.nMethod) {
		case DITP_GET_ID:
			findId(pKey, m_svrMsg);
			debug("method DITP_GET_ID: %ld", m_svrMsg->result);
			break;
		case DITP_GET_NEW_ID:
			makeId(pKey, m_svrMsg);
			debug("method DITP_GET_NEW_ID: %ld", m_svrMsg->result);
			break;
		case DITP_GET_NUM_DOC:
			m_svrMsg->result = gDidCount-1;
			m_svrMsg->status = SUCCESS;
			break;
		default:
			return FAIL;
	}
	return SUCCESS;
}

/*	Daemon main loop
 *	@author gramo @modifier: jiwon, aragorn
 */
static int thread_main(slot_t *slot)
{
	int ret;
	struct sockaddr from;
	socklen_t fromlen = sizeof(struct sockaddr);
	char buf[20];
	char test[30];
	ClientMsg m_cliMsg = {
		/*proctocolId*/"DITP", /*majorVer*/0, /*minorVer*/0, /*check*/0, 
		/*ClientRequest*/ { /*key*/"", /*keyLen*/0, /*method*/0 } };
	ServerMsg m_svrMsg = { /*stat*/0, /*method*/0, /*result*/0, /*check*/0 };

	while ( 1 )
	{
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

		slot->state = SLOT_WAIT;
		sprintf(test, "i'm slot[%d]", slot->id);

		if ( pthread_mutex_lock(&sock_lock) != 0 ) {
			error("slot[%d]: pthread_mutex_lock: %s",
					slot->id, strerror(errno));
			continue;
		}

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
			pthread_mutex_unlock(&sock_lock);
			break;
		}

		ret = sb_run_udp_recvfrom(udp_sock,
			(void *)&m_cliMsg, sizeof(ClientMsg),&from, &fromlen, 0);
		pthread_mutex_unlock(&sock_lock);

		/* if graceful_shutdown is on, replying for the request is ok */
		if ( scoreboard->shutdown ) break;
  
		if (ret <= 0) {      // if packet length is 0 or error occurs,
			info("slot[%d]: UCP_accept() failed", slot->id);
            continue;        // receive again
        }

		if ( inet_ntop(AF_INET,
				(void *)&((struct sockaddr_in *)(&from))->sin_addr, buf, 20)
				== NULL ) {
			error("inet_ntop returned null errno:%s",strerror(errno));
		}
		else{
			debug("slot[%d]: from: %s:%d", slot->id, buf,
					ntohs(((struct sockaddr_in *)(&from))->sin_port));
		}

		slot->state = SLOT_READ;
		time(&(slot->recent_request));

        if (isProperProtocol(&m_cliMsg) < 0) {
			// if protocol signature is wrong,
			info("slot[%d]: wrong procotol signature", slot->id);
			slot->state = SLOT_FINISH;
            continue; // accept another message
        }

		slot->state = SLOT_PROCESS;
		if ( pthread_mutex_lock(&slot_lock) != 0 ) {
			error("slot[%d]: pthread_mutex_lock: %s",
					slot->id, strerror(errno));
			continue;
		}

        if (executeClientMsg(&m_svrMsg, &m_cliMsg) < 0) {
			// if requesting unknown method
			info("slot[%d]: unknown method", slot->id);
			slot->state = SLOT_FINISH;
			pthread_mutex_unlock(&slot_lock);
            continue; // accept another message
        }
		pthread_mutex_unlock(&slot_lock);

		sb_run_udp_sendto(udp_sock,
			&m_svrMsg, sizeof(ServerMsg), &(from), fromlen, UDP_TIMEOUT);

    } // endless while loop

	slot->state = SLOT_FINISH;
	debug("slot[%d] exits", slot->id);
	return SUCCESS;
}


/** get prefix by key.
 *
 *	@author	woosong
 */	
static uint32_t DSgetPrefix(char *szKey) {
    uint32_t divisor, remainder;
    uint32_t prefix;
    uint32_t i;
	
    divisor = gDidBKTable->byPrefixBit / 8;
    remainder = gDidBKTable->byPrefixBit % 8;

	for ( i=0,prefix=0 ;i<divisor; i++ ) {
		prefix = (prefix << 8) + (uint8_t)szKey[i];
	}

	if (remainder) {
		prefix = (prefix << remainder) + (((uint8_t)szKey[i]) >> ( 8 - remainder ));
	}

    return prefix;
}

static uint32_t DSgetPrefixWithPrefixBit(char *szKey, int16_t byPrefixBit) {
	uint32_t divisor, remainder;
	uint32_t prefix;
	uint32_t i;

	divisor = byPrefixBit / 8;
	remainder = byPrefixBit % 8;

    prefix = 0;

	for(i=0; i<divisor; i++) {
		prefix = (prefix << 8) + (uint8_t)szKey[i];
	}

	if (remainder) {
		prefix = (prefix << remainder) + ((uint8_t)szKey[i] >> ( 8 - remainder ));
	}

	return prefix;
}

/** 
 *	compare slots with key.
 *	compare function for qsort.
 *	
 *	@author woosong
 *	@param	*a	did_slot_t
 *	@param	*b	did_slot_t
 *	@return	memcmp(a->szKey, b->szKey, SYS_DID_KEYSIZE);
 */	
static int DScompareSL(const void *a, const void *b) {
    return memcmp(((did_slot_t *)a)->szKey, ((did_slot_t *)b)->szKey, SYS_DID_KEYSIZE);
}

/** sort bucket.

	@author woosong
	@param *bucket	CDidBK
	@return int16_t
	*/
#if 0
static int16_t DSsortDidBK(CDidBK *bucket) {
    qsort(bucket->aDidSL, bucket->nSlotCnt, sizeof(did_slot_t), DScompareSL);
	return SUCCESS;
}
#endif

/**	add a slot to bucket.

    @author woosong
    @param  *bucket CDidBK
    @param  *slot   did_slot_t
    @return NULL - Bucket FULL 
    */
static did_slot_t *DSaddDidSL(CDidBK *bucket, did_slot_t slot) {
	int i, j, k;

    if ((bucket) && (uint32_t)((bucket)->nSlotCnt) < MAX_DID_SL) {
		i = 0;

		if ((bucket)->nSlotCnt > 0) {
			k = j = (bucket)->nSlotCnt - 1;
			while (j > i) {
				k = ( i + j ) / 2;
				if (DScompareSL(&((bucket)->aDidSL[k]), &slot)>0)
					j = k;
				else
					i = k + 1;
			}
			if (DScompareSL(&((bucket)->aDidSL[i]), &slot)<0)
				i++;
		}

		j = (bucket)->nSlotCnt;
		if (i != j) 
			memmove(&((bucket)->aDidSL[i+1]), 
					&((bucket)->aDidSL[i]), 
					(j-i) * sizeof(did_slot_t));
        memcpy(&((bucket)->aDidSL[i]), &slot, sizeof(did_slot_t));
        (bucket)->nSlotCnt++;
		return (&((bucket)->aDidSL[i]));
    }
    else {
        return (did_slot_t *)0;
    }
}

/** increases prefixbit by 1 and resets bucket table.

    @author woosong
    @param  void
    @return 0 - OK.
    */
static int16_t DSdoubleDidBKTable(void) {
    int16_t i;
    int8_t byPrefixBit;

    byPrefixBit = (++gDidBKTable->byPrefixBit);

	debug("DSdouble! %d %ld\n", byPrefixBit, gDidCount);

    for(i = (1 << byPrefixBit) - 1; i >= 0; i--)
        gDidBKTable->pDidBK[i] = gDidBKTable->pDidBK[i/2];

    return 0;
}

/** partition Bucket.
	@author	woosong
	@param	*bucket	CDidBK
	@return	0 - OK. 1 - Not partitionable. 2 - memory alloc error.
	*/
static int16_t DSpartitionDidBKTable(CDidBK *bucket) {
	int i, j;
	int depth;
	int8_t byPrefixBit;
	CDidBK *bucket2;

	depth = gDidBKTable->byPrefixBit - bucket->byPrefixBit;

	if (depth <= 0) return 1;

	bucket2 = (CDidBK *)sb_calloc(1, sizeof(CDidBK));
	if(bucket2 == 0x00) {
		crit("cannot allocate for bucket:%s", strerror(errno));
		return 2;
	}
	
	bucket2->nSlotCnt = 0;
	byPrefixBit = bucket2->byPrefixBit = bucket->byPrefixBit + 1;

	for(i = 0; i < bucket->nSlotCnt; i++)
		if(DSgetPrefixWithPrefixBit(bucket->aDidSL[i].szKey, byPrefixBit) % 2) break;

	j = i;
	for(; i < bucket->nSlotCnt; i++)
	{
		DSaddDidSL(bucket2, bucket->aDidSL[i]);
	}
	bucket->nSlotCnt = j;
	bucket->byPrefixBit++;

	for(i = 0; i < (1 << gDidBKTable->byPrefixBit); i++)
	{
		if (gDidBKTable->pDidBK[i] == bucket)
			if ((i >> (depth - 1)) % 2)
				gDidBKTable->pDidBK[i] = bucket2;
	}
	return 0;
}

/****************************** MD5 related *********************************/
/** url to md5 key of SYS_DID_KEYSIZE.

	@author	woosong
	@param szUrl	url
	@return char*	SYS_DID_KEYSIZE bytes of md5 key digest
	*/
static int16_t DSmd5( char* szUrl, char *key ) {
	MD5_CTX context;
	unsigned char digest[16];
	unsigned int len = strlen(szUrl);

	MD5Init( &context );
	MD5Update( &context, szUrl, len );
	MD5Final( digest, &context );

	memcpy(key, digest, SYS_DID_KEYSIZE);

	return 0;
}
/****************************************************************************/

/** find Did. 

        @author gramo
        @param  char* szKey
        @return , *did_slot_t - Success , NULL - Fail
                 */ 
static did_slot_t* DSfindDid ( char *szKey ) {

        CDidBK *currentBK;


        currentBK = DSfindBK( szKey );

		if (currentBK != 0 )
        	return DSfindDid_BS (currentBK , szKey , 0 , (currentBK)->nSlotCnt);

		return (did_slot_t*)NULL;
}

#if 0
static void printBK(CDidBK *a, int16_t t) {
	int i;
	info("Bucket : %d \n************",t);
	for(i=0;i<((a->nSlotCnt>100) ? 100 : a->nSlotCnt);i++)
		info("slot[%d]:%x %x %x %x %x %x",
				i,
				(unsigned char)a->aDidSL[i].szKey[0],
				(unsigned char)a->aDidSL[i].szKey[1],
				(unsigned char)a->aDidSL[i].szKey[2],
				(unsigned char)a->aDidSL[i].szKey[3],
				(unsigned char)a->aDidSL[i].szKey[4],
				(unsigned char)a->aDidSL[i].szKey[5]);
	info("*************");
}
#endif

/** Find bucket.

    @author gramo
    @param  char* szKey
    @return *CDidBK
 */
static CDidBK* DSfindBK ( char *szKey ) {
        int32_t prefix;

        prefix = DSgetPrefix ( szKey );
		if (prefix >= MAX_DID_BK) {
			crit("prefix[%d] is larger than MAX_DID_BK[%d]",
					prefix, MAX_DID_BK);
			return NULL;
		}

        return (gDidBKTable->pDidBK[prefix]);
}

/** 
 *	Bynary Search for did_slot_t 
 *	if new then create DID
 *	else find DID
 *
 *	@author gramo
 *	@param CDidBK* szBK
 *	@param char* szKey
 *	@param int16_t StartPos
 *	@param int16_t FinishPos
 *	@return , *did_slot_t - Success , NULL - Fail
 */
static did_slot_t* DSfindDid_BS ( CDidBK *szBK , char *szKey , int16_t StartPos , int16_t FinishPos ) {
	int16_t MiddlePos;
	int16_t flag;
/*	did_slot_t empty;*/

	while (1) {
		MiddlePos = (StartPos + FinishPos) / 2; 

		flag = memcmp(szBK->aDidSL[MiddlePos].szKey , szKey, SYS_DID_KEYSIZE); 

		if ( ( StartPos == MiddlePos || MiddlePos == FinishPos ) && flag ) {
			return (did_slot_t*)NULL;
		}

		if ( flag < 0 ) {
			StartPos = MiddlePos;
		}
		else if ( flag > 0 ){
			FinishPos = MiddlePos;
		}
		else {
			return &(szBK->aDidSL[MiddlePos]);
		}
	}

	return (did_slot_t*)NULL ;
}

static void DSsaveData (void) {
/*	DocId Did;*/
	FILE *fp;
/*	int16_t i=0;*/
	int32_t size;

	debug("Saving Did slot from [%s]", m_didSlotPath);

	if ( (fp=sb_fopen(m_didSlotPath,"ab")) ==NULL )
		error("fail file open:%s", sys_errlist[errno]);

	if (fseek(fp,0L,SEEK_END))
		error("file file seek:%s", sys_errlist[errno]);

	size = fwrite(DSsaveSL , sizeof(did_slot_t) , MAX_LOAD_BLOCK , fp);

	if (size==MAX_LOAD_BLOCK)
		debug("Sucess!");
	else
		debug("Fail!");

	fclose(fp);
}

static void load_data (void) {
	did_slot_t pTmpSL[MAX_LOAD_BLOCK];
/*	DocId Did;*/
	FILE *fp;
	int i=0;
	int size;

	debug("Loading Did slot from [%s]", m_didSlotPath);
/*	CRIT("Loading Did slot from [%s]", m_didSlotPath);*/

	if ( (fp=sb_fopen(m_didSlotPath,"rb")) == NULL ) {
		info("there is no did database exist");
		debug("Fail file open!");
		return;
	}

	while (!feof(fp)) {
		size = fread(pTmpSL , sizeof(did_slot_t) , MAX_LOAD_BLOCK , fp);

		for (i=0; i<size; i++) {
			load_slot( pTmpSL[i] );
			gDidCount = pTmpSL[i].docId+ 1;

		}
	}
}

/** convert URL into MD5 hash key, and Find slot. 
        if don't find slot , create new slot.

                @author gramo
                @param char* szUrl
                @return *did_slot_t 
*/
#if 0
static did_slot_t* DSresolveURLnew(char *szUrl) {
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t *pTmpSL;
	DocId Did;

	DSmd5 ( szUrl, tmpKey );

	if ( (pTmpSL=DSfindDid(tmpKey))  ) {
		DSDidState = DID_STATE_OLD;
		return pTmpSL;
	}

	Did = gDidCount++;

	DSDidState = DID_STATE_NEW;
	pTmpSL = DSnewSL( szUrl , Did);

	memcpy(&DSsaveSL[DSsaveCount++] , pTmpSL , sizeof(did_slot_t));
	
	if (!(DSsaveCount%MAX_LOAD_BLOCK)) {
		DSsaveData();
		DSsaveCount=0;
	}

	return pTmpSL;
}
#endif

/** update slot

	@author gramo
	@param char* szUrl
	@return *did_slot_t 
*/
#if 0
static did_slot_t* DSresolveURLupdate ( char *szUrl , CDidState szState) {
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t *pTmpSL;
/*	DocId Did;*/

	DSmd5 ( szUrl, tmpKey );

	if ( (pTmpSL=DSfindDid(tmpKey)) ) {
		DSDidState = DID_STATE_OLD;

		pTmpSL->byState.bDoc = szState.bDoc;
		pTmpSL->byState.bAnc = szState.bAnc;

		return pTmpSL;
	}

	return (did_slot_t*)NULL;
}
#endif

/** convert URL into MD5 hash key, and only Find slot . 

                @author gramo
                @param char* szUrl
                @return *did_slot_t 
*/
#if 0
static did_slot_t* DSresolveURLfind ( char *szUrl ) {
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t *pTmpSL;
/*	DocId Did;*/

	DSmd5 ( szUrl, tmpKey );

	if ( (pTmpSL=DSfindDid(tmpKey))  ) {
		DSDidState = DID_STATE_OLD;
		return pTmpSL;
	}

	return (did_slot_t*)NULL;
}
#endif


/** create new slot. 

	@author gramo
	@param char* szUrl
	@return *did_slot_t
*/
static did_slot_t* DSnewSL ( char *szUrl , DocId Did) {
	CDidBK* pBK;
	char tmpKey[SYS_DID_KEYSIZE];
	did_slot_t tmpSL;
	did_slot_t *pTmpSL;

	DSmd5 ( szUrl, tmpKey );
	pBK = DSfindBK ( tmpKey );
	if (pBK == NULL) {
		crit("pBK(return value of DSfindBK) is NULL! what's up?");
//		return NULL;
	}

	/* create slot */
	memcpy(tmpSL.szKey , tmpKey,SYS_DID_KEYSIZE);
	tmpSL.docId= Did;

	if (!(pTmpSL=DSaddDidSL ( pBK , tmpSL ) ) ) {
		if (!pBK) {
			DSdoubleDidBKTable();
			pBK = DSfindBK(tmpKey);
		} else if ((pBK)->byPrefixBit == gDidBKTable->byPrefixBit)
			DSdoubleDidBKTable();

		if (DSpartitionDidBKTable( pBK )) 
			return NULL;

		pBK = DSfindBK ( tmpKey );
		pTmpSL = DSnewSL ( szUrl , Did);
	}

	return pTmpSL;
}


static did_slot_t* load_slot ( did_slot_t tmpSL) {
	CDidBK* pBK;
	did_slot_t *pTmpSL;

	pBK = DSfindBK ( tmpSL.szKey );

	if ( !(pTmpSL=DSaddDidSL(pBK, tmpSL)) ) {
		if ((pBK)->byPrefixBit == gDidBKTable->byPrefixBit)
			DSdoubleDidBKTable();
		if (DSpartitionDidBKTable( pBK ))	
			return NULL;
		pBK = DSfindBK ( tmpSL.szKey );
		pTmpSL = load_slot ( tmpSL );
	}
	return pTmpSL;
}

static void private_init()
{
	gDidCount = 1; // will be overriden when loading data from file
	gDidBKTable= (CDidBKTable*)sb_calloc(1,sizeof(CDidBKTable));
	gDidBKTable->pDidBK[0] = (CDidBK*)sb_calloc(1,sizeof(CDidBK));
	gDidBKTable->pDidBK[0]->nSlotCnt = 0;
}

static int module_main(slot_t *slot)
{

	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	debug("docid server started");
	private_init();
	debug("memory allocation is done.");
	CRIT("start to load_data()...");
	load_data();
	info("load_data() is now done.");
	CRIT("you may register new docs.");

	if ( sb_run_udp_bind(&udp_sock, mBindAddr, mBindPort) != SUCCESS ) {
		error("udp_bind: %s", strerror(errno));
		return 1;
	}
	
	if ( pthread_mutex_init(&sock_lock, NULL) != 0 ) {
		error("pthread_mutex_init: %s", strerror(errno));
		return 1;
	}
	pthread_mutex_init(&slot_lock, NULL);

	sb_run_init_scoreboard(scoreboard);
	sb_run_spawn_threads(scoreboard, "docid server thread(child)", thread_main);

	scoreboard->period = 20;
	sb_run_monitor_threads(scoreboard);
	did_server_kill(0);

	if ( pthread_mutex_destroy(&sock_lock) != 0 ) {
		error("pthread_mutex_destroy: %s", strerror(errno));
		return 1;
	}

	return 0;
}

static void did_server_kill (int signum) {
	FILE *fp;
	int16_t size;

	debug("saving extra did slot to [%s]", m_didSlotPath);

	if ( (fp=sb_fopen(m_didSlotPath,"ab")) ==NULL ) {
		error("fail file open");
		return;
	}

	if (fseek(fp,0L,SEEK_END)) {
		error("fail file seek");
		return;
	}

	size = fwrite(DSsaveSL , sizeof(did_slot_t) , DSsaveCount , fp);

	if (size==DSsaveCount)
		debug("save extra sucessfully");
	else
		debug("save extra fail");

	fclose(fp);
	
	return;
}


// XXX -- 라이브러리로 사용할 경우
/*static void DSinit(int32_t wPort) {*/
/*	gDidCount = 1;*/
/*	gDidBKTable= (CDidBKTable*)sb_calloc(1,sizeof(CDidBKTable));*/
/*	gDidBKTable->pDidBK[0] = (CDidBK*)sb_calloc(1,sizeof(CDidBK));*/
/*	gDidBKTable->pDidBK[0]->nSlotCnt = 0;*/

/*	load_data();*/
/*}*/

/*****************************************************************************/
static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

//	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	scoreboard->shutdown++;
}

static void _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

//	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing; 
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL); 

	scoreboard->graceful_shutdown++;
}

static int init()
{
	int port=0;
	port = assignSoftBotPort(__FILE__,PORT_ID_DID);
	snprintf(mBindPort,SHORT_STRING_SIZE,"%d",port);
	return SUCCESS;
}

/*************************************************************************/
/* api: same interface with mod_did_client.c */

static int DI_init()
{
	private_init();
	load_data();
	return SUCCESS;
}

static void DI_finish()
{
	did_server_kill(0);
}

static int DI_getId(char *pKey, DocId *docid)
{
	ServerMsg svrMsg;
	svrMsg.nMethod = DITP_GET_ID;
	findId(pKey, &svrMsg);
	*docid = svrMsg.result;
	return (int)svrMsg.status; 
}

int DI_getNewId(char *pKey, DocId *docid, DocId *olddocid)
{
	ServerMsg svrMsg;
	svrMsg.nMethod = DITP_GET_NEW_ID;
	makeId(pKey, &svrMsg);
	*docid = svrMsg.result;
	*olddocid = svrMsg.olddid;
	return (int)svrMsg.status;
}

static void register_hooks(void)
{
	sb_hook_local_get_docid(DI_getId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_local_get_new_docid(DI_getNewId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_load_docid_db(DI_init, NULL, NULL, HOOK_MIDDLE);
	sb_hook_unload_docid_db(DI_finish, NULL, NULL, HOOK_MIDDLE);
}

/*************************************************************************/
/* configuration stuff here */

static void set_bind_address(configValue v)
{
	strncpy(mBindAddr, v.argument[0], SHORT_STRING_SIZE);
	mBindAddr[SHORT_STRING_SIZE-1] = '\0';
}

static void get_did_slot_path(configValue v) {
	strncpy(m_didSlotPath, v.argument[0], 256);
}

static void get_slave_thread_num(configValue v) {
	slave_thread_num = atoi(v.argument[0]);
}
/*****************************************************************************/
static config_t config[] = {
	CONFIG_GET("BindIP", set_bind_address, 1, \
		   "IP-address e.g) 192.168.1.1"),
	CONFIG_GET("DidSlotPath",get_did_slot_path, 1, "did slot db local path"),
	CONFIG_GET("SlaveThreadNum",get_slave_thread_num, 1, "slave thread number"),
	{NULL}
};

module did_daemon_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	init,               /* initialize function */
	module_main,		/* child_main */
	scoreboard,			/* scoreboard */
	register_hooks		/* register hook api */
};

