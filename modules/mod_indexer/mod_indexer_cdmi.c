/* $Id$ */
#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/vrfi.h"
#include "mod_api/mod_api.h"
#include "mod_api/register.h"

#include "mod_qp/mod_qp.h" /* XXX: STD_HITS_LEN */
#include "mod_indexer.h"

#include "hit.h"

#define STATE_INIT (0)
#define STATE_WAITING_SIGNAL (1)
#define STATE_INDEXING (2)
#define STATE_SAVING (3)

#define MONITORING_PERIOD	5

#define TOO_MANY_DOCS_TO_INDEX	(1000000)
#define MAX_INDEX_SIZE_AT_A_TIME (10000)

#define MAX_WORD_HIT_LEN		(50000) /* wordhit per document */
#define MAX_DOC_HIT_LIST_NUM	(5000000)

/* XXX: 일단은 child 1개 */
static scoreboard_t scoreboard[] = {PROCESS_SCOREBOARD(1)};

typedef struct {
	WordId *wordids; // XXX: only used in FORWARD_INDEX?
	DocId *docids; // XXX: obsolete?
	doc_hit_t *dochits;

	uint32_t idx;

	WordId * _wids_storage;
	DocId * _dids_storage;
	doc_hit_t * _hits_storage;
} mem_index_t;

// sharing with qp
REGISTRY int *rVrfSemid=NULL;
REGISTRY uint32_t *rLastIndexedDid=NULL;

/* member variables  */
extern cdm_db_t *m_cdmdb;
static char cdmdbname[MAX_DBNAME_LEN];
static char cdmdbpath[MAX_DBPATH_LEN];

static int mTimer = 10000;

static VariableRecordFile *mVRFI=NULL;

static char mInvertedIdxFile[STRING_SIZE]="dat/indexer/index";

#if FORWARD_INDEX == 1
static char mForwardIdxFile[MAX_FILE_LEN]="dat/forward_index/forward_idx";
static VariableRecordFile *mForwardVRFI=NULL;
static void save_forward_index(mem_index_t *mem_index);
static uint32_t countSuccessiveDocid(doc_hit_t *dochits,uint32_t limit);
#endif

/* private functions */
static void sort_index_by_wordid(mem_index_t *mem_index);
static uint32_t sumHitsNum
	(mem_index_t *mem_index, uint32_t startIdx,uint32_t numToSum);
static void add_to_mem_index
	(mem_index_t *memindex, word_hit_t *word_hit,uint16_t num,DocId theDid);
static int cmp_word_hit(const void* var1,const void* var2);
static uint32_t countSuccessiveWid(void *obj,uint32_t max,int8_t is_word_hit);

static uint32_t process_index(mem_index_t *mem_index, 
						DocId did_start, uint32_t num_to_index);
static void save_index(mem_index_t *mem_index);

/*** signal handler **********************************************************/
static RETSIGTYPE _do_nothing(int sig) { return; }

static RETSIGTYPE _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	scoreboard->shutdown++;
}
static RETSIGTYPE _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}

static void init_mem_index(mem_index_t *mem_index)
{
	mem_index->wordids = 
		(WordId*)malloc(sizeof(WordId) * MAX_DOC_HIT_LIST_NUM);
	mem_index->docids = 
		(DocId*)malloc(sizeof(DocId) * MAX_DOC_HIT_LIST_NUM);
	mem_index->dochits = 
		(doc_hit_t*)malloc(sizeof(doc_hit_t) * MAX_DOC_HIT_LIST_NUM);
	mem_index->idx = 0;

	mem_index->_wids_storage = 
		(WordId*)malloc(sizeof(WordId) * MAX_DOC_HIT_LIST_NUM);
	mem_index->_dids_storage = 
		(DocId*)malloc(sizeof(DocId) * MAX_DOC_HIT_LIST_NUM);
	mem_index->_hits_storage = 
		(doc_hit_t*)malloc(sizeof(doc_hit_t) * MAX_DOC_HIT_LIST_NUM);
}

static int indexer_main(slot_t *slot)
{
	DocId cdm_last_did=0L;
	DocId start_did=0L;
	uint32_t amount_to_index=0,remain_amount_to_index=0;
	uint32_t amount_to_index_thistime=0,really_indexed_amount=0;
	struct timeval timeout,ts,tf;
	double diff=0;
	mem_index_t mem_index;
	int iteration=0;
	cdm_info_t *cdm_info;

	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	/* must be called after forking because it calls malloc */
	init_mem_index(&mem_index);

	while (1) { /* endless loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) {/*{{{*/
			info("finishing");
			slot->state = SLOT_FINISH;
			break;
		}/*}}}*/

		cdm_info = sb_run_cdm_get_info(m_cdmdb);
		if (cdm_info == NULL) {
			error("error while get cdm info");
			break;
		}

		cdm_last_did = cdm_info->last_registered_intkey;

		if ( cdm_last_did < *rLastIndexedDid ) {/*{{{*/
			error("integerity err: cdm last did=[%ld] < indexer lastdid=[%d]",
											cdm_last_did, *rLastIndexedDid);
			amount_to_index = 0;/*}}}*/
		} else {
			amount_to_index = cdm_last_did - *rLastIndexedDid;
		}

		info("cdm last did[%ld], last indexed did[%d]",
						cdm_last_did, *rLastIndexedDid);

		slot->state = SLOT_PROCESS;

		if ( amount_to_index > TOO_MANY_DOCS_TO_INDEX ) {
			warn("amount to index[%d] > TOO_MANY_DOCS_TO_INDEX[%d]",
					amount_to_index, TOO_MANY_DOCS_TO_INDEX);
			warn("setting amount to index %d",amount_to_index);
			amount_to_index = TOO_MANY_DOCS_TO_INDEX - 1;
		}

		remain_amount_to_index = amount_to_index;
		gettimeofday(&ts,NULL);
		while ( remain_amount_to_index > 0 ) {

			start_did = (*rLastIndexedDid)+ 1;
			amount_to_index_thistime = 
				(remain_amount_to_index < MAX_INDEX_SIZE_AT_A_TIME) ?
					remain_amount_to_index : MAX_INDEX_SIZE_AT_A_TIME;

			setproctitle("softbotd: %s (%ld indexed, indexing %ld docs [iteration:%d])",
							__FILE__, *rLastIndexedDid, amount_to_index_thistime, iteration);

			really_indexed_amount = 
					process_index(&mem_index,start_did,amount_to_index_thistime);

			remain_amount_to_index -= really_indexed_amount;

			setproctitle("softbotd: %s (%ld indexed, saving %ld docs [iteration:%d])",
							__FILE__, *rLastIndexedDid, really_indexed_amount, iteration);
#if FORWARD_INDEX==1
			if (mem_index.idx > 0)
				save_forward_index(&mem_index);
#endif

			sort_index_by_wordid(&mem_index);

			if (mem_index.idx > 0)
				save_index(&mem_index);

			*rLastIndexedDid += really_indexed_amount;
			mem_index.idx = 0;
			iteration++;
		}
		gettimeofday(&tf,NULL);
		diff = timediff(&tf, &ts);

		setproctitle("softbotd: indexer (%u indexed," 
				"now idle [last process %4.2f sec for %u docs(%2.2f docs/sec)] [iteration:%d])",
				(*rLastIndexedDid),
				diff, amount_to_index, amount_to_index/diff, iteration);
		info("%u indexed[last process %4.2f sec for %u docs(%2.2f docs/sec)]) [iteration:%d]",
				(*rLastIndexedDid), 
				diff, amount_to_index, amount_to_index/diff, iteration);

		slot->state = SLOT_WAIT;

		timeout.tv_sec = mTimer;
		timeout.tv_usec = 0;
		select(0, NULL, NULL, NULL, &timeout); 
	} /* endless while loop */

	/* synchronize lexicon db */
	sb_run_sync_word_db(&gWordDB); //XXX: indexer should do this?

	sb_run_vrfi_close(mVRFI); // XXX
#if FORWARD_INDEX==1
	sb_run_vrfi_close(mForwardVRFI); // XXX
#endif

	slot->state = SLOT_FINISH;

	return 0;
}

static int module_main(slot_t *slot) {
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	m_cdmdb = sb_run_cdm_db_open(cdmdbname, cdmdbpath, CDM_SHARED);
	if (m_cdmdb == NULL) {
		error("cannot open cdmdb[%s, %s]", cdmdbname, cdmdbpath);
		return 1;
	}

	sb_init_scoreboard(scoreboard);
	sb_spawn_processes(scoreboard,"indexer process",indexer_main);
	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	return 0;
}

static int init() {
	// XXX: followings should be taken from configuration
	ipc_t vrf_lock;
	int rv=FAIL;

	debug("initialization");

	rv=sb_run_vrfi_alloc(&mVRFI);
	rv==SUCCESS ? /* pass */: error("error allocating vrf");

	rv = sb_run_vrfi_open(mVRFI, mInvertedIdxFile, sizeof(inv_idx_header_t), sizeof(doc_hit_t));

	if (rv == FAIL) {
		crit("cannot open vrfi. path[%s]", mInvertedIdxFile);
		return FAIL;
	}

#if FORWARD_INDEX==1
	rv=sb_run_vrfi_alloc(&mForwardVRFI);
	rv==SUCCESS ? /* pass */: error("error allocating forward vrf");
	rv=sb_run_vrfi_open(mForwardVRFI,
			mForwardIdxFile,sizeof(forwardidx_header_t), sizeof(forward_hit_t));
	rv==SUCCESS ? /* pass */: error("error opening forward vrf[%s]", mForwardIdxFile);
#endif

	vrf_lock.type = IPC_TYPE_SEM;
	vrf_lock.pid = SYS5_INDEXER;
	vrf_lock.pathname = mInvertedIdxFile;

	get_sem(&vrf_lock);
	*rVrfSemid = vrf_lock.id;

	return SUCCESS;
}

static uint32_t process_index(mem_index_t *mem_index,DocId did_start, uint32_t num_to_index)
{
	int16_t ret=0;
	word_hit_t wordhit[MAX_WORD_HIT_LEN]; 
	DocId did=0;
	uint32_t indexed_wordhit_size=0,hitidx=0;

	// for each newly created document
	for (did = did_start;did < did_start+num_to_index; did++){
		if (did % 1000 == 0) {
			setproctitle("softbotd: %s (%ld indexed, indexing %ld/%ld)",
									__FILE__, *rLastIndexedDid,
									did-did_start+1,num_to_index);
		}
		hitidx=0;

		ret = sb_run_index_one_doc(did,wordhit,MAX_WORD_HIT_LEN,&hitidx);
		if (ret != SUCCESS) {
			warn("error while indexing [%ld].",did);
			continue;
		}

		indexed_wordhit_size += (hitidx+1);

		// wordid순 정렬. stable해야 한다. 
		// 즉, 동일한 word에 대해 앞선 field에 나온 word가 우선
		mergesort(wordhit,(size_t)hitidx,
								sizeof(word_hit_t),cmp_word_hit);

		add_to_mem_index(mem_index,wordhit,hitidx,did);

		if (indexed_wordhit_size >= MAX_DOC_HIT_LIST_NUM - MAX_WORD_HIT_LEN) {
			return did - did_start + 1;
		}
	} // for each documents to process
	return num_to_index;
}

/**
 *  현재까지 indexing되어서 mem_index 에 저장된 data를 
 *  VRF db에 저장한다.
 *  io operation이 주이므로, 그 동안 또 indexing을 할 수 있다.
 */
void save_index(mem_index_t *mem_index)
{
	uint32_t ndocs=0,ntotal_hits=0;
	uint32_t idxLimit = mem_index->idx;
	uint32_t indexed_idx=0;
	WordId *wordids=mem_index->wordids;
	inv_idx_header_t inv_idx_header;
	doc_hit_t *dochits=mem_index->dochits;
	int ret=FAIL;

	acquire_lock(*rVrfSemid);
	indexed_idx=0;
	while (1) {
		// 새로 indexing된 document 몇개에서 wordid가 
		// wordid[indexed_idx]인 단어가 나왔는지 체크
		ndocs = countSuccessiveWid((void*)(wordids+indexed_idx),
										idxLimit-indexed_idx,
										/*is_word_hit=*/FALSE);
		ntotal_hits = sumHitsNum(mem_index,indexed_idx,ndocs);

		ret = sb_run_vrfi_get_fixed_dup(mVRFI,(uint32_t)wordids[indexed_idx],
										(void*)&inv_idx_header);
		if (ret < 0){ 
			alert("error while vrf_getfixed for key[%ld]",wordids[indexed_idx]);
			break;
		}

		// XXX: do not count ndocs. just append it, then vrfi will count ndata
		inv_idx_header.ndocs += ndocs;
		inv_idx_header.ntotal_hits += ntotal_hits;

		ret = sb_run_vrfi_set_fixed(mVRFI,(uint32_t)wordids[indexed_idx],
												(void*)&inv_idx_header);
		if (ret < 0) {
			alert("Error while writing fixed data, key: %ld",
										wordids[indexed_idx]);
			break;
		}

		debug("dochits[%u].id: %u", indexed_idx, dochits[indexed_idx].id);

		ret = sb_run_vrfi_append_variable(mVRFI,(uint32_t)wordids[indexed_idx],
										ndocs,(void*)(dochits+indexed_idx));

		if (ret < 0) {
			alert("Error while writing variable data to VRF db");
			break;
		}

		indexed_idx += ndocs;
		if (indexed_idx == idxLimit){
			debug("%u docs are saved to VRF",idxLimit);
			break;
		}
	}
	release_lock(*rVrfSemid);

	return;
}

#if FORWARD_INDEX==1
#define MAX_WORDS_PER_DOC 10000
static void save_forward_index(mem_index_t *mem_index)
{
	forwardidx_header_t forwardidx_header;
	forward_hit_t forwardhits[MAX_WORDS_PER_DOC];
	doc_hit_t *dochits=mem_index->dochits;
	DocId *docids=mem_index->docids;
	WordId *wordids=mem_index->wordids;
	uint32_t idxlimit= mem_index->idx;
	uint32_t idx=0;
	uint32_t nwords=0,ntotal_hits=0;
	int ret=0,i=0;

	idx=0;
	while (1) {
		// num of words in a document
		nwords = countSuccessiveDocid(dochits+idx,idxlimit-idx); 
		ntotal_hits = sumHitsNum(mem_index,idx,nwords);

		forwardidx_header.nwords = nwords;
		forwardidx_header.ntotal_hits = ntotal_hits;

		ret = sb_run_vrfi_set_fixed(mForwardVRFI,(uint32_t)docids[idx],
													&forwardidx_header);
		if (ret < 0) {
			alert("Error while writing fixed data in forward idx, key:%ld",
															docids[idx]);
		}

		nwords = (nwords > MAX_WORDS_PER_DOC)? MAX_WORDS_PER_DOC:nwords;
		for (i=idx; i<idx+nwords; i++) {
			forwardhits[i-idx]=*((forward_hit_t*)&(dochits[i]));
			forwardhits[i-idx].id=wordids[i];
		}

		ret = sb_run_vrfi_append_variable(mForwardVRFI,(uint32_t)docids[idx],
													nwords,forwardhits);
		idx += nwords;
		if (idx >= idxlimit) {
			debug("%u docs are saved to Forward VRF",idxlimit);
			break;
		}
	}
}

#undef MAX_WORDS_PER_DOC
#endif /* FORWARD_INDEX */

static uint32_t sumHitsNum(mem_index_t *mem_index,
						uint32_t startIdx,uint32_t numToSum)
{
	doc_hit_t *dochits=mem_index->dochits;
	uint32_t i=0;
	uint32_t nhits_sum=0;

	for (i=startIdx; i<startIdx+numToSum; i++) {
		nhits_sum+=dochits[i].nhits;
	}
	return nhits_sum;
}

static void add_to_mem_index(mem_index_t *mem_index,
			word_hit_t *word_hit,uint16_t nelm,DocId did)
{
	uint32_t hitnum = 0;
	uint32_t count=0;
	WordId *wordids=mem_index->wordids;
	DocId *docids=mem_index->docids;
	doc_hit_t *dochits=mem_index->dochits;

	while (count < nelm) {
		wordids[mem_index->idx] = word_hit->wordid;
		docids[mem_index->idx] = did;

		hitnum = countSuccessiveWid(word_hit,nelm-count,/*is_word_hit=*/TRUE);
		count += hitnum;

		// XXX: Does it need to made to hook/run interface? 
		fill_dochit(&(dochits[mem_index->idx]),did,hitnum,word_hit);

		if (mem_index->idx >= MAX_DOC_HIT_LIST_NUM) {
			error("mem_index->idx[%u] > MAX_DOC_HIT_LIST_NUM[%u]",
					mem_index->idx, MAX_DOC_HIT_LIST_NUM);
			return;
		}
		mem_index->idx++;
		word_hit += hitnum;
	}
}

/* TODO:  this(merge, merge_sort_by_wid) can be fastened by 
 * 		  modifying mergesort of freebsd implementation.
 */

void merge(mem_index_t *mem_index, uint32_t first, uint32_t split, uint32_t last) {
	/* Merges the two halves into a temporary array, then copies
	   the sorted elements back into the original array. */
	   
	uint32_t index=0;
	uint32_t tracer1 = first, tracer2 = split + 1;
	WordId *tempWids=mem_index->_wids_storage;
	DocId *tempDids=mem_index->_dids_storage;
	doc_hit_t *tempDocHits=mem_index->_hits_storage;
/*    WordId tempWids[last-first+1];*/
/*    DocId tempDids[last-first+1];*/
/*    doc_hit_t tempDocHits[last-first+1];*/
	
	/* Do this until one of the halves is emptied. */
	index=0;
	while (tracer1 <= split && tracer2 <= last) {
		if (mem_index->wordids[tracer1] <= mem_index->wordids[tracer2]) {
			tempWids[index] = mem_index->wordids[tracer1];
			tempDids[index] = mem_index->docids[tracer1];
			tempDocHits[index] = mem_index->dochits[tracer1];
			index++,tracer1++;
		}
		else {
			tempWids[index] = mem_index->wordids[tracer2];
			tempDids[index] = mem_index->docids[tracer2];
			tempDocHits[index] = mem_index->dochits[tracer2];
			index++,tracer2++;
		}
	} 
	
	/* Now copy the leftover elements from the non-empty half. */
	while (tracer2 <= last) {  /* right half is non-empty */
		tempWids[index] = mem_index->wordids[tracer2];
		tempDids[index] = mem_index->docids[tracer2];
		tempDocHits[index] = mem_index->dochits[tracer2];
		index++,tracer2++;
	}
	while (tracer1 <= split) {  /* left half is non-empty */
			tempWids[index] = mem_index->wordids[tracer1];
			tempDids[index] = mem_index->docids[tracer1];
			tempDocHits[index] = mem_index->dochits[tracer1];
			index++,tracer1++;
	}
		
	/* Copy from temporary array back into mem_index. */
	for (index = first; index <= last; ++index) {
		mem_index->wordids[index] = tempWids[index-first];
		mem_index->docids[index] = tempDids[index-first];
		mem_index->dochits[index] = tempDocHits[index-first];
	}
	return;
}

/* XXX: this function must be stable.
 *      because it should preserve the sequence of docid.
 *      so cannot use quick sort algorithm. */
void merge_sort_by_wid(mem_index_t *mem_index, uint32_t first, uint32_t last) {
	/* Cuts the array in half, and sorts each half recursively. */
	uint32_t split=0;
	
	if (first < last) {
		split = (first + last)/2; /* integer division truncates */
		merge_sort_by_wid(mem_index, first, split);
		merge_sort_by_wid(mem_index, split + 1, last);
		merge(mem_index, first, split, last);
	}
	return;
}

static void sort_index_by_wordid(mem_index_t *mem_index) {
	debug("mem_index->idx: %u",mem_index->idx);
	if (mem_index->idx > 0)
		merge_sort_by_wid(mem_index,0,mem_index->idx-1);
}

#if FORWARD_INDEX==1
static uint32_t countSuccessiveDocid(doc_hit_t *dochits,uint32_t limit)
{
	DocId currentDid=dochits->id;
	uint32_t count=0;

	for (count=1; count<limit; count++) {
		if (dochits[count].id != currentDid)
			return count;
	}
	return limit;
}
#endif

static uint32_t countSuccessiveWid(void *obj,uint32_t limit,int8_t is_word_hit)
{
	if (is_word_hit == TRUE) { // counting word hit
		word_hit_t *word_hit=(word_hit_t*)obj;
		WordId currentWid = word_hit->wordid;
		uint32_t count=0;

		for (count=1; count<limit; count++) {
			if (word_hit[count].wordid!= currentWid){
				return count;
			}
		}
		return limit;
	}
	else { // 
		WordId *wordids=(WordId*)obj;
		WordId currentWid = *wordids;
		uint32_t count=0;

		for (count=1; count<limit; count++) {
			if (wordids[count] != currentWid){
				return count;
			}
		}
		return limit;
	}

	return 0;
}

static int cmp_word_hit(const void* var1,const void* var2) {
	if ( ((word_hit_t*)var1)->wordid> ((word_hit_t*)var2)->wordid) 
		return 1;
	else if ( ((word_hit_t*)var1)->wordid < ((word_hit_t*)var2)->wordid ) 
		return -1;

	return 0;
}



/*
 * below here follows configuration , registry
 */
static void set_inverted_index_file(configValue v)
{
	crit("not yet implemented");
}

#if FORWARD_INDEX==1
static void set_forward_index_file(configValue v)
{
	crit("not yet implemented");
}
#endif
static void set_timer(configValue v)
{
	mTimer=atoi(v.argument[0]);
	debug("timer is set to %d",mTimer);
}
static void set_cdmdb(configValue v)
{
	strncpy(cdmdbname, v.argument[0], MAX_DBNAME_LEN);
	cdmdbname[MAX_DBNAME_LEN-1] = '\0';

	snprintf(cdmdbpath, MAX_DBPATH_LEN, "%s/%s",
			gSoftBotRoot, v.argument[1]);
	cdmdbpath[MAX_DBPATH_LEN-1] = '\0';

	info("configure: cdmdb[%s, %s]", cdmdbname, cdmdbpath);
}

/* registry related functions */
REGISTRY void init_vrf_lock(void *data)
{
	rVrfSemid = data;
	debug("called");
}
REGISTRY char* registry_get_vrflock()
{
	//XXX: maybe need more detailed information
	return NULL;
}
REGISTRY void init_last_indexed_did(void *data)
{
	rLastIndexedDid = data;
}
REGISTRY char* registry_get_last_indexed_did()
{
	static char buf[STRING_SIZE];

	snprintf(buf,STRING_SIZE,"last indexed did:%d",(*rLastIndexedDid));

	return buf;
}

static registry_t registry[] = {
	RUNTIME_REGISTRY("VRFLock","lock for vrf object",sizeof(int),
					init_vrf_lock, registry_get_vrflock,NULL),
	PERSISTENT_REGISTRY("LAST_INDEXED_DOCID","last indexed document id",sizeof(uint32_t),
					init_last_indexed_did, registry_get_last_indexed_did,NULL),
	NULL_REGISTRY
};

static config_t config[] = {
	CONFIG_GET("InvertedIndexFile",set_inverted_index_file,1,\
			"inv indexer db path (e.g: IndexDbPath dat/indexdb)"),
	CONFIG_GET("IndexTimer",set_timer,1,\
			"timer(sec) watching if there's newly assembled document"),
#if FORWARD_INDEX==1
	CONFIG_GET("ForwardIndexFile",set_forward_index_file,1,\
		"forward index db file path (e.g: ForwardIndexFile dat/forward_index/forward)"),
#endif	
	CONFIG_GET("CdmDatabase", set_cdmdb, 2,\
			"cdm database db name, path: CdmDatabase [dbname dbpath]"),
	{NULL}
};

static uint32_t last_indexed_did(void)
{
	return *rLastIndexedDid;
}

static void register_hooks(void)
{
	sb_hook_last_indexed_did(last_indexed_did,NULL, NULL, HOOK_FIRST);
}

module indexer_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	init,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,					/* scoreboard */
	register_hooks			/* register hook api */
};
