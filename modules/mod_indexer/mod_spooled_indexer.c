/* $Id$ */
#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/vrfi.h"
#include "mod_api/oldlexicon.h"
#include "mod_api/indexer.h"
#include "mod_api/spool.h"

#include "mod_qp/mod_qp.h" /* XXX: STD_HITS_LEN */
#include "mod_spooled_indexer.h"
#include "mod_index_each_spooled_doc.h"

#include "hit.h"

#define MONITORING_PERIOD	5
#define MAX_TEST_FILE_LEN   2100000000


uint32_t doc_hit_list_memorysize = 300*1024*1024; /* 300 Mbytes */
uint32_t max_word_hit_len=300000;
uint32_t max_doc_hit_list_num=3500000;

/* sharing with qp XXX: no need to lock???  */
REGISTRY int *rVrfSemid=NULL;
REGISTRY uint32_t *last_indexed_docid=NULL;

static char spoolpath[STRING_SIZE]="dat/indexer/indexer.spl";
static spool_t *spool = NULL;

static int spool_queue_size = SPOOL_QUEUESIZE;
static int spool_mpool_size = SPOOL_MPOOLSIZE;

static int spool_semid;

#define MAX_LOGGING_ERROR_DOCUMENTS 5000
uint32_t deleted_documents[MAX_LOGGING_ERROR_DOCUMENTS];
uint32_t deleted_documents_idx = 0;
uint32_t error_documents[MAX_LOGGING_ERROR_DOCUMENTS];
uint32_t error_documents_idx = 0;
uint32_t failed_documents[MAX_LOGGING_ERROR_DOCUMENTS];
uint32_t failed_documents_idx = 0;
static char mErrorDocumentFile[STRING_SIZE]="dat/indexer/index.error.documents";
static int mErrorFileFd=0;
static char mTestDataFile[STRING_SIZE]="dat/indexer/index.test.spooldata";
static int mTestFileFd=0;
static int mTestFileNumber=0;


/* XXX: 일단은 child 1개 */
static scoreboard_t scoreboard[] = {PROCESS_SCOREBOARD(1)};

typedef struct {
    char word[MAX_WORD_LEN];
    unsigned long pos;
    unsigned long bytepos;
    unsigned long attribute;
    unsigned short field;
    unsigned short len;
} index_word_t;

typedef struct {
	uint32_t *wordid;
	uint32_t *docid;
	doc_hit_t *dochit;
} mem_index_ptr_t;
 
typedef struct {
#if 0
	uint32_t *wordids; // XXX: only used in FORWARD_INDEX?
	uint32_t *docids;
	doc_hit_t *dochits;
#endif

	uint32_t idx;

//	mem_index_ptr_t *list;
 
	uint32_t *_wids_storage;
	uint32_t *_dids_storage;
	doc_hit_t *_hits_storage;
	word_hit_t *_word_hits_storage;
} mem_index_t;

/* member variables  */
static int mTimer = 1;

static VariableRecordFile *mVRFI=NULL;
static char mInvertedIdxFile[STRING_SIZE]="dat/indexer/index";

#if FORWARD_INDEX == 1
static char mForwardIdxFile[MAX_FILE_LEN]="dat/forward_index/forward_idx";
static VariableRecordFile *mForwardVRFI=NULL;
static void save_forward_index(mem_index_t *mem_index);
static uint32_t count_successive_docid_same_to_first
						(doc_hit_t *dochits, uint32_t max);
#endif

/* private functions */
static void sort_index_by_wordid(mem_index_t *mem_index);
static void add_to_mem_index
	(mem_index_t *memindex, word_hit_t *wordhits,uint32_t nelm,uint32_t docid);
static uint32_t count_successive_wordid_same_to_first_in_wordids
							(uint32_t wordids[], uint32_t max);
static uint32_t count_successive_wordid_same_to_first_in_wordhit
							(word_hit_t *wordhits, uint32_t max);
static int cmp_word_hit(const void* var1,const void* var2);
static uint32_t sum_nhits(doc_hit_t dochits[], uint32_t nelm);
static void save_index(mem_index_t *mem_index);
static int index_one_spooled_doc(mem_index_t *mem_index, uint32_t docid);

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
	max_doc_hit_list_num = doc_hit_list_memorysize / sizeof(doc_hit_t);
	notice("max_doc_hit_list_num:%u", max_doc_hit_list_num);
	sb_assert(max_doc_hit_list_num > max_word_hit_len);

#if 0
	mem_index->wordids = 
		(uint32_t *)sb_malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->wordids==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->docids = 
		(uint32_t *)sb_malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->docids==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->dochits = 
		(doc_hit_t*)malloc(sizeof(doc_hit_t) * max_doc_hit_list_num);
	if (mem_index->dochits==NULL) { crit("malloc failed:%s", strerror(errno)); return; }
#endif

	mem_index->idx = 0;

	mem_index->_wids_storage = 
		(uint32_t*)malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->_wids_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_dids_storage = 
		(uint32_t *)malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->_dids_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_hits_storage = 
		(doc_hit_t*)malloc(sizeof(doc_hit_t) * max_doc_hit_list_num);
	if (mem_index->_hits_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_word_hits_storage =
		(word_hit_t*)malloc(sizeof(word_hit_t) * max_word_hit_len);
	if (mem_index->_word_hits_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

#if 0
	mem_index->list =
		(mem_index_ptr_t*)malloc(sizeof(mem_index_ptr_t) * max_doc_hit_list_num);
	if (mem_index->list==NULL) { crit("malloc failed:%s", strerror(errno)); return; }
#endif
}

static int is_full(mem_index_t *mem_index)
{
	//XXX: max_word_hit_len ???
	if (mem_index->idx >= max_doc_hit_list_num - max_word_hit_len)
		return TRUE;

	return FAIL;
}

#define DOCUMENT_LOGGING_SIZE MAX_LOGGING_ERROR_DOCUMENTS * 16 /* sizeof("error 123456\n") ~= 16 */
void save_wrong_documents(uint32_t document_array[], uint32_t nelm, const char *prefix)
{
	static char result[DOCUMENT_LOGGING_SIZE]="";
	char buf[STRING_SIZE]="";
	int i=0, rv=0, size_left=DOCUMENT_LOGGING_SIZE;
	char write_done=0;

	if (mErrorFileFd <= 0) {
		warn("error document[%s] dropped. error file not opened", prefix);
		return;
	}

	rv = lseek(mErrorFileFd, 0, SEEK_END);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return;
	}

	i=0;
	while (!write_done) {
		result[0] = '\0';
		size_left = DOCUMENT_LOGGING_SIZE;

		for ( ; i<nelm; i++) {
			sz_snprintf(buf, STRING_SIZE,
						"%s %u\n", prefix, document_array[i]);
			sz_strncat(result, buf, size_left);
			size_left -= strlen(buf);
			if (size_left < STRING_SIZE)
				break;
		}

		rv = write(mErrorFileFd, result, strlen(result));
		if (rv == -1) {
			error("write failed:%s", strerror(errno));
			return;
		}

		if (i==nelm) write_done=1;
	}
}
void save_all_wrong_documents()
{
	save_wrong_documents(deleted_documents, deleted_documents_idx, "deleted");
	deleted_documents_idx=0;

	save_wrong_documents(error_documents, error_documents_idx, "error");
	error_documents_idx=0;

	save_wrong_documents(failed_documents, failed_documents_idx, "failed");
	failed_documents_idx=0;
}
void add_deleted_document(uint32_t deleted_docid)
{
	if (deleted_documents_idx >= MAX_LOGGING_ERROR_DOCUMENTS) {
		save_wrong_documents(deleted_documents, deleted_documents_idx, "deleted");
		deleted_documents_idx = 0;
	}

	deleted_documents[deleted_documents_idx] = deleted_docid;
	deleted_documents_idx++;
}
void add_error_document(uint32_t error_docid)
{
	if (error_documents_idx >= MAX_LOGGING_ERROR_DOCUMENTS) {
		save_wrong_documents(error_documents, error_documents_idx, "error");
		error_documents_idx = 0;
	}

	error_documents[error_documents_idx] = error_docid;
	error_documents_idx++;
}
void add_failed_document(uint32_t failed_docid)
{
	if (failed_documents_idx >= MAX_LOGGING_ERROR_DOCUMENTS) {
		save_wrong_documents(failed_documents, failed_documents_idx, "failed");
		failed_documents_idx = 0;
	}

	failed_documents[failed_documents_idx] = failed_docid;
	failed_documents_idx++;
}

void exception_documention_handling(uint32_t docid_to_handle, int errorcode)
{
	if (errorcode == DELETED_DOCUMENT) {
		add_deleted_document(docid_to_handle);
	}
	else if (errorcode == ERROR_DOCUMENT) {
		add_error_document(docid_to_handle);
	}
	else if (errorcode == FAIL) {
		add_failed_document(docid_to_handle);
	}
	else {
		crit("index_one_spooled_doc returned wrong return value[%d]", errorcode);
		crit(" not INDEXER_WAIT[%d], not FAIL[%d], not ERROR_DOCUMENT[%d], not SUCCESS[%d]",
								INDEXER_WAIT, FAIL, ERROR_DOCUMENT, SUCCESS);
	}
}

#define OPEN_FLAG   (O_RDWR)
#define CREATE_FLAG (O_RDWR|O_CREAT)
#define MODE        (S_IREAD|S_IWRITE)
void open_error_documents_file()
{
	mErrorFileFd = sb_open(mErrorDocumentFile, OPEN_FLAG, MODE);
	if (mErrorFileFd == -1) {
		mErrorFileFd = sb_open(mErrorDocumentFile, CREATE_FLAG, MODE);
		if (mErrorFileFd == -1) {
			crit("unable to open file[%s]:%s", mErrorDocumentFile, strerror(errno));
			return;
		}
	}
	return;
}

void open_test_data_file()
{
    char buf[STRING_SIZE];

    sprintf(buf, "%s%02d", mTestDataFile, mTestFileNumber);

    mTestFileFd = sb_open(buf, OPEN_FLAG, MODE);
    if (mTestFileFd == -1) {
        mTestFileFd = sb_open(buf, CREATE_FLAG, MODE);
        if (mTestFileFd == -1) {
            crit("unable to open file[%s]:%s", buf, strerror(errno));
            return;
        }
    }
    return;
}

int write_test_data(uint32_t docid, int size, void *data)
{
    int rv = 0;
    long dwCurrentFileOffset = 0;
//  void *test_data=0x00;

    if (mTestFileFd <= 0) {
        warn("error document dropped. error file not opened");
        return FAIL;
    }

    dwCurrentFileOffset = (long)lseek(mTestFileFd, 0, SEEK_END);
    if (dwCurrentFileOffset == -1) {
        error("lseek failed:%s", strerror(errno));
        return FAIL;
    }

    if ((dwCurrentFileOffset + size + 8) > MAX_TEST_FILE_LEN)
    {
        mTestFileNumber++;
        close(mTestFileFd);
        open_test_data_file();
    }

//  test_data = (void *)sb_malloc(size+8);
//  if (test_data == NULL)
//  {
//        error("insufficient memory");
//        return FAIL;
//    }
//  memcpy(test_data, &docid, sizeof(uint32_t));
//  memcpy(test_data+4, &size, sizeof(int));
//  memcpy(test_data+8, data, size);

    rv = write(mTestFileFd, &docid, sizeof(uint32_t));
    if (rv == -1) {
        error("write failed:%s", strerror(errno));
        return FAIL;
    }

    rv = write(mTestFileFd, &size, sizeof(int));
    if (rv == -1) {
        error("write failed:%s", strerror(errno));
        return FAIL;
    }

    rv = write(mTestFileFd, data, size);
    if (rv == -1) {
        error("write failed:%s", strerror(errno));
        return FAIL;
    }

//  sb_free(test_data);
    return SUCCESS;
}

void set_bytepos(void *data, int size)
{
    index_word_t *indexwords=NULL;
    int32_t num_of_indexwords=0; 
    int i = 0;

    indexwords = (index_word_t *)data;
    num_of_indexwords = size / sizeof(index_word_t);
    
    if (size % sizeof(index_word_t)) error("index_word_t size error");

    for (i=0; i<num_of_indexwords; i++) {
        indexwords[i].bytepos = 0;
    }
}

static int discard_document(spool_t *spool, void *ptr, int len)
{
    uint32_t docid;

    if (strncmp((char*)ptr, "DEL ", 4) == 0) {
        if (sb_run_spool_delete_front(spool, &docid) == -1) {
            crit("cannot delete front of spool");
            sb_run_spool_free(spool, ptr);
            return FAIL;
        }
        sb_run_spool_free(spool, ptr);
        return DELETED_DOCUMENT;
    }

    if (strncmp((char*)ptr, "ERR ", 4) == 0) {
        if (sb_run_spool_delete_front(spool, &docid) == -1) {
            crit("cannot delete front of spool");
            sb_run_spool_free(spool, ptr);
            return FAIL;
        }
        sb_run_spool_free(spool, ptr);
        warn("invalid document: docid[%u]", docid);
        return ERROR_DOCUMENT;
    }

    if (strncmp((char*)ptr, "CD  ", 4) == 0) {
        INFO("returning INDEXER_WAIT");
        return INDEXER_WAIT;
    }

    if (strncmp((char*)ptr, "IDXW", 4) != 0) {
        crit("wrong data type was inserted to spool '%c%c%c%c': doc[%d]",
                ((char*)ptr)[0], ((char*)ptr)[1], ((char*)ptr)[2], ((char*)ptr)[3],
                docid);
        if (sb_run_spool_delete_front(spool, &docid) == -1) {
            crit("cannot delete front of spool");
            sb_run_spool_free(spool, ptr);
            return FAIL;
        }
        sb_run_spool_free(spool, ptr);
        return INDEXER_WAIT;
    }

    if (sb_run_spool_delete_front(spool, &docid) == -1) {
        crit("cannot delete front of spool");
        sb_run_spool_free(spool, ptr);
        return FAIL;
    }

    if ((len - 4) % sizeof(index_word_t) != 0) {
        crit("index word list array is not proper length");
        sb_run_spool_free(spool, ptr);
        return ERROR_DOCUMENT;
    }

    return SUCCESS;
}
                                                    
static int get_first_from_spool(uint32_t docid,
            void **spooled_memory, int *spooled_memory_size)
{
    uint32_t retrieved_docid=0;
    void *ptr = NULL;
    int len=0, rv=0;
	
    /* lock spool */
    acquire_lock(spool_semid);
    if (sb_run_spool_get_front(spool, &retrieved_docid, &ptr, &len) == -1) {
        warn("spool is empty");
        rv = INDEXER_WAIT;
        goto UNLOCK_RETURN_ERROR;
    }
	
    if (docid > retrieved_docid) {
        crit("docid[%u] > retrieved_docid[%u]", docid, retrieved_docid);
        abort();
    }
    else if (docid < retrieved_docid) {
        /* FIXME: should be changed to warn after "DEL " is dealt */
        error("docid[%u] < retrieved_docid[%u]", docid, retrieved_docid);
        rv = FAIL;
        goto UNLOCK_RETURN_ERROR;
    }

    rv = discard_document(spool, ptr, len);
    if (rv < 0) { /* INDEXER_WAIT */
        goto UNLOCK_RETURN_ERROR;
    }

    /* unlock spool */
    release_lock(spool_semid);

    *spooled_memory = ptr;
    *spooled_memory_size = len;

    return SUCCESS;

UNLOCK_RETURN_ERROR:
    release_lock(spool_semid);
    return rv;
}

static int indexer_main(slot_t *slot)
{
	uint32_t *last_registered_docid=NULL, docid_to_index_this_time=0;
	registry_t *reg=NULL;
	mem_index_t mem_index;
	struct timeval timeout,ts,tf;
	double cumulative_time = 0.0, diff=0.0;
	uint32_t indexednum=0, cumulative_indexednum=0;
	int rv=0;


	/* must be called after forking because it allocates large junk of memory */
	init_mem_index(&mem_index);
	open_error_documents_file();
	
	reg = registry_get("LastRegisteredDocId");
	if (reg == NULL) { 
		crit("cannot get LastRegisteredDocId registry"); 
		abort();
	}
	last_registered_docid = (uint32_t*)(reg->data);

	// XXX: waiting for piling process to pile something to spool
	usleep(1000000);

	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			goto FINISH;

		/* for each one complete stages of index process
		 *    -> get morphologically analyzed documnet
		 *      -> make (word, position) pair
		 *      -> sort it
		 *    -> do above again for next document
		 *    -> if indexer memory is full, sort it.
		 *    -> save to vrfi
		 */
		while ( *last_indexed_docid < *last_registered_docid) {
			mem_index.idx=0;
			indexednum=0;

			gettimeofday(&ts,NULL);

			while ( *last_indexed_docid < *last_registered_docid ) { 
				if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
					break;

				if (indexednum % 10 == 0) {
					setproctitle("softbotd: %s (%u indexed[avg indexing speed: %.2f docs/sec], "
								 "[%u] docs more to index)",
								 __FILE__, *last_indexed_docid, 
								 (cumulative_time) ? (double)cumulative_indexednum/cumulative_time : 0,
								 *last_registered_docid - *last_indexed_docid);
				}

				docid_to_index_this_time = (*last_indexed_docid) + 1;
				rv = index_one_spooled_doc(&mem_index, docid_to_index_this_time);
				if (rv == INDEXER_WAIT) {
					usleep(200000);
					continue;
				}
				else if (rv != SUCCESS) {
					exception_documention_handling(docid_to_index_this_time, rv);
					(*last_indexed_docid)++;
					continue;
				}
				else { /* SUCCESS */
					indexednum++;
					(*last_indexed_docid)++;
				}

				if ( is_full(&mem_index) == TRUE ) 
					break;
			}

			setproctitle("softbotd: %s (%u indexed, saving %u)",
							__FILE__, *last_indexed_docid, indexednum);

			save_all_wrong_documents();
#if FORWARD_INDEX==1
			save_forward_index(&mem_index);
#endif
			sort_index_by_wordid(&mem_index);
			save_index(&mem_index);

			/* every saving time, word db is also saved */
			sb_run_sync_word_db(&gWordDB); //XXX: indexer should do this?

			sb_run_vrfi_sync(mVRFI);
#if FORWARD_INDEX==1
			sb_run_vrfi_sync(mForwardVRFI); // XXX
#endif

			gettimeofday(&tf,NULL);
			diff = timediff(&tf, &ts);

			cumulative_time +=  diff;
			cumulative_indexednum += indexednum;

			setproctitle("softbotd: %s"
					"(%u indexed. [%u] docs more to index.[avg indexing speed:%.2f docs/sec])",
					__FILE__,
					(*last_indexed_docid),
					*last_registered_docid - *last_indexed_docid,
					(double)cumulative_indexednum/cumulative_time);

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
				goto FINISH;
		}
		setproctitle("softbotd: %s"
				"(%u indexed, now idle [avg indexing speed:%.2f docs/sec])",
				__FILE__,
				(*last_indexed_docid),
				(double)cumulative_indexednum/cumulative_time);

		timeout.tv_sec = mTimer;
		timeout.tv_usec = 0;
		select(0, NULL, NULL, NULL, &timeout); 
	} /* endless while loop */

FINISH:
	sb_run_sync_word_db(&gWordDB); //XXX: indexer should do this?

	sb_run_vrfi_close(mVRFI);
#if FORWARD_INDEX==1
	sb_run_vrfi_close(mForwardVRFI); // XXX
#endif

	slot->state = SLOT_FINISH;
	return 0;
}


static int test_indexer_main(slot_t *slot)
{
	uint32_t *last_registered_docid=NULL, docid_to_index_this_time=0;
	registry_t *reg=NULL;
	struct timeval timeout,ts,tf;
	double cumulative_time = 0.0, diff=0.0;
	uint32_t indexednum=0, cumulative_indexednum=0;
	int rv=0, len=0;
	void *ptr = NULL;
	
	/* must be called after forking because it allocates large junk of memory */
	open_error_documents_file();
	open_test_data_file();
	
	reg = registry_get("LastRegisteredDocId");
	if (reg == NULL) { 
		crit("cannot get LastRegisteredDocId registry"); 
		abort();
	}
	last_registered_docid = (uint32_t*)(reg->data);

	// XXX: waiting for piling process to pile something to spool
	usleep(1000000);

	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			goto FINISH;

		/* for each one complete stages of index process
		 *    -> get morphologically analyzed documnet
		 *      -> make (word, position) pair
		 *      -> sort it
		 *    -> do above again for next document
		 *    -> if indexer memory is full, sort it.
		 *    -> save to vrfi
		 */
		indexednum=0;

		while ( *last_indexed_docid < *last_registered_docid) {

			gettimeofday(&ts,NULL);

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
				break;

			if (indexednum % 10 == 0) {
				setproctitle("softbotd: %s (%u indexed[avg indexing speed: %.2f docs/sec], "
							 "[%u] docs more to index)",
							 __FILE__, *last_indexed_docid, 
							 (cumulative_time) ? (double)cumulative_indexednum/cumulative_time : 0,
							 *last_registered_docid - *last_indexed_docid);
			}

			docid_to_index_this_time = (*last_indexed_docid) + 1;

		 	rv = get_first_from_spool(docid_to_index_this_time, &ptr, &len);
			if (rv == INDEXER_WAIT) {
				usleep(200000);
				continue;
			}
			else if (rv != SUCCESS) {
				exception_documention_handling(docid_to_index_this_time, rv);
				(*last_indexed_docid)++;
				continue;
			}
			else { /* SUCCESS */
				indexednum++;
				(*last_indexed_docid)++;
			}
        		
        	// test dump 데이타에서 bytepos에 0을 넣어주기 위한 함수
       		set_bytepos(ptr+4, len-4);
			rv = write_test_data(docid_to_index_this_time, len-4, ptr+4);
        	if (rv != SUCCESS)
        	{
           		error("rmac_data write error, docid = %u, size = %d", docid_to_index_this_time, len);
        	}				

			sb_run_spool_free(spool, ptr);

			setproctitle("softbotd: %s (%u indexed, saving %u)",
							__FILE__, *last_indexed_docid, indexednum);

			save_all_wrong_documents();

			gettimeofday(&tf,NULL);
			diff = timediff(&tf, &ts);

			cumulative_time +=  diff;
			cumulative_indexednum = indexednum;

			setproctitle("softbotd: %s"
					"(%u indexed. [%u] docs more to index.[avg indexing speed:%.2f docs/sec])",
					__FILE__,
					(*last_indexed_docid),
					*last_registered_docid - *last_indexed_docid,
					(double)cumulative_indexednum/cumulative_time);

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
				goto FINISH;
		}
		setproctitle("softbotd: %s"
				"(%u indexed, now idle [avg indexing speed:%.2f docs/sec])",
				__FILE__,
				(*last_indexed_docid),
				(double)cumulative_indexednum/cumulative_time);

		timeout.tv_sec = mTimer;
		timeout.tv_usec = 0;
		select(0, NULL, NULL, NULL, &timeout); 
	} /* endless while loop */

FINISH:
	close(mTestFileFd);

	slot->state = SLOT_FINISH;
	return 0;
}

static int module_main(slot_t *slot) {
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	sb_init_scoreboard(scoreboard);
	sb_spawn_processes(scoreboard,"indexer process",indexer_main);
	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	return 0;
}

static int test_main(slot_t *slot) {
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	sb_init_scoreboard(scoreboard);
	sb_spawn_processes(scoreboard,"indexer process",test_indexer_main);
	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	return 0;
}
/*FIXME: should be removed from module structure and called in indexer_main() */
static int test_init() {

    ipc_t spool_lock;

    spool = sb_run_spool_open(spoolpath, spool_queue_size, spool_mpool_size);
    if (spool == NULL) {
        crit("cannot open spool[%s]", spoolpath);
        return FAIL;
    }

    spool_lock.type = IPC_TYPE_SEM;
    spool_lock.pid = SYS5_INDEXER_SPOOL;
    spool_lock.pathname = spoolpath;

    get_sem(&spool_lock);
    spool_semid = spool_lock.id;

	return SUCCESS;
}

/*FIXME: should be removed from module structure and called in indexer_main() */
static int init() {
    ipc_t vrf_lock;
    int rv=FAIL;

    DEBUG("initialization");

    rv=sb_run_vrfi_alloc(&mVRFI);
    rv==SUCCESS ? /* pass */: error("error allocating vrf");

    rv =sb_run_vrfi_open(mVRFI, mInvertedIdxFile,
                sizeof(inv_idx_header_t), sizeof(doc_hit_t), O_RDWR);
    if (rv == FAIL) {
        crit("cannot open vrfi. path[%s]", mInvertedIdxFile);
        return FAIL;
    }

#if FORWARD_INDEX==1
    rv=sb_run_vrfi_alloc(&mForwardVRFI);
    rv==SUCCESS ? /* pass */: error("error allocating forward vrf");
    rv=sb_run_vrfi_open(mForwardVRFI,
            mForwardIdxFile,sizeof(forwardidx_header_t), sizeof(forward_hit_t), O_RDWR);
    rv==SUCCESS ? /* pass */: error("error opening forward vrf[%s]", mForwardIdxFile);
#endif

    vrf_lock.type = IPC_TYPE_SEM;
    vrf_lock.pid = SYS5_INDEXER;
    vrf_lock.pathname = mInvertedIdxFile;

    get_sem(&vrf_lock);
    *rVrfSemid = vrf_lock.id;

    return SUCCESS;
}

int index_one_spooled_doc(mem_index_t *mem_index, uint32_t docid)
{
	int16_t ret=0;
	word_hit_t *wordhits = mem_index->_word_hits_storage;
	uint32_t hitidx=0;

	sb_assert(docid > 0);

	ret = sb_run_index_each_spooled_doc(docid, wordhits, max_word_hit_len, &hitidx);
	if (ret != SUCCESS) {
		return ret;
	}

	// wordid순 정렬. stable해야 한다. 
	// 즉, 동일한 word에 대해 앞선 field에 나온 word가 우선
	ret = mergesort(wordhits,(size_t)hitidx,
								sizeof(word_hit_t),cmp_word_hit);

	add_to_mem_index(mem_index, wordhits, hitidx, docid);
	INFO("docid[%u] is added to index", docid);

	return SUCCESS;
}

uint32_t count_documents_no_overlapping
				(doc_hit_t dochits[], uint32_t nelm)
{
	uint32_t current_docid = 0;
	uint32_t count = 0;
	uint32_t i=0;

	for (i=0; i<nelm; i++) {
		if (dochits[i].id != current_docid) {
			count++;
			current_docid = dochits[i].id;
		}
	}

	return count;
}

/**
 *  현재까지 indexing되어서 mem_index 에 저장된 data를 
 *  VRF db에 저장한다.
 *  io operation이 주이므로, 그 동안 또 indexing을 할 수 있다.
 */
void save_index(mem_index_t *mem_index)
{
	uint32_t ndocs=0,ndochits=0, ntotal_hits=0;
	uint32_t current_idx=0;
	uint32_t total_indexed_count = mem_index->idx;
	uint32_t *wordids=mem_index->_wids_storage;
	inv_idx_header_t inv_idx_header;
	doc_hit_t *dochits=mem_index->_hits_storage;
	int ret=FAIL;

	if (mem_index->idx <=0) return;

	CRIT("saving index to file");
	acquire_lock(*rVrfSemid); /* XXX: locking needed? */

	current_idx=0;
	while (1) {
		ndochits = count_successive_wordid_same_to_first_in_wordids(wordids+current_idx,
												total_indexed_count-current_idx);
		if (ndochits == 0) {crit("ndochits:0");abort();}
		ndocs = count_documents_no_overlapping(dochits+current_idx, ndochits);
		ntotal_hits = sum_nhits(dochits+current_idx, ndochits);

		//XXX: make it not duplicate.
		ret = sb_run_vrfi_get_fixed_dup(mVRFI,(uint32_t)wordids[current_idx],
													(void*)&inv_idx_header);
		if (ret < 0){ 
			alert("error while vrf_getfixed for key[%u]",wordids[current_idx]);
			break;
		}

		inv_idx_header.ndocs += ndocs;
		inv_idx_header.ntotal_hits += ntotal_hits;

		ret = sb_run_vrfi_set_fixed(mVRFI,(uint32_t)wordids[current_idx],
												(void*)&inv_idx_header);
		if (ret < 0) {
			alert("Error while writing fixed data, key: %u",
										wordids[current_idx]);
			break;
		}

		DEBUG("dochits[%u].id: %u", current_idx, dochits[current_idx].id);

		ret = sb_run_vrfi_append_variable(mVRFI,(uint32_t)wordids[current_idx],
											ndochits,(void*)(dochits+current_idx));

		if (ret < 0) {
			alert("Error while writing variable data to VRF db");
			break;
		}

		current_idx += ndochits;
		if (current_idx == total_indexed_count){
			DEBUG("%u docs are saved to VRF",total_indexed_count);
			break;
		}
	}
	release_lock(*rVrfSemid);

	return;
}

#if FORWARD_INDEX==1
#define MAX_WORDS_PER_DOC 1000000
static void save_forward_index(mem_index_t *mem_index)
{
	forwardidx_header_t forwardidx_header;
	static forward_hit_t forwardhits[MAX_WORDS_PER_DOC];
	doc_hit_t *dochits=mem_index->_hits_storage; /* XXX, FIXME: sort전에는 storage를 쓴다 */
	uint32_t *docids=mem_index->_dids_storage;
	uint32_t *wordids=mem_index->_wids_storage;
	uint32_t idxlimit= mem_index->idx;
	uint32_t idx=0;
	uint32_t nwords=0,ntotal_hits=0, orig_nwords=0;
	int ret=0,i=0;

	if (mem_index->idx <= 0)
		return;

	CRIT("saving forward index");
	INFO("mem_index->idx:%d", mem_index->idx);
	idx=0;

	while (1) {
		// num of words in a document
		nwords = orig_nwords = 
				count_successive_docid_same_to_first(dochits+idx,idxlimit-idx); 
		ntotal_hits = sum_nhits(dochits+idx, nwords);

		forwardidx_header.nwords = nwords;
		forwardidx_header.ntotal_hits = ntotal_hits;

		ret = sb_run_vrfi_set_fixed(mForwardVRFI,(uint32_t)docids[idx],
													&forwardidx_header);
		if (ret < 0) {
			alert("Error while writing fixed data in forward idx, key:%u",
															docids[idx]);
		}

		if (nwords > MAX_WORDS_PER_DOC) {
			warn("document[%u] has nwords[%u] > MAX_WORDS_PER_DOC[%u], so dropping excessive words",
					docids[idx], nwords, MAX_WORDS_PER_DOC);
			nwords = MAX_WORDS_PER_DOC;
		}

		for (i=idx; i<idx+nwords; i++) {
			forwardhits[i-idx]=*((forward_hit_t*)&(dochits[i]));
			forwardhits[i-idx].id=wordids[i];
		}

		ret = sb_run_vrfi_append_variable(mForwardVRFI,(uint32_t)docids[idx],
														nwords,forwardhits);
		if (ret < 0) {
			error("error vrfi_append_variable ret[%d]", ret);
		}

		idx += orig_nwords;
		if (idx >= idxlimit) {
			DEBUG("%u docs are saved to Forward VRF",idxlimit);
			break;
		}
	}
}

#undef MAX_WORDS_PER_DOC
#endif /* FORWARD_INDEX */

static uint32_t sum_nhits(doc_hit_t dochits[], uint32_t nelm)
{
	uint32_t i=0;
	uint32_t count=0;

	for (i=0; i<nelm; i++) {
		count += dochits[i].nhits;
	}
	return count;
}

static uint32_t count_successive_wordid_same_to_first_in_wordhit
						(word_hit_t *wordhits, uint32_t max)
{
	uint32_t first_wordid = wordhits->wordid;
	uint32_t count = 0;

	for (count = 1; count<max; count++) {
		if (wordhits[count].wordid != first_wordid) {
			return count;
		}
	}
	return max;
}

/* FIXME: should be configurable */
int max_dochit_per_doc = 100;

static void add_to_mem_index(mem_index_t *mem_index,
			word_hit_t *wordhits,uint32_t nelm, uint32_t docid)
{
	uint32_t nhitelm = 0;
	uint32_t count = 0;
	uint32_t *wordids = mem_index->_wids_storage;
	uint32_t *docids = mem_index->_dids_storage;
	doc_hit_t *dochits = mem_index->_hits_storage;
	int ndochits = 0, i=0;

	/* XXX: should be moved to somewhere else.. */
	sb_assert(max_dochit_per_doc < MAX_DOCHITS_PER_DOCUMENT);
	sb_assert(docid > 0);

	while (count < nelm) {
		nhitelm = count_successive_wordid_same_to_first_in_wordhit(wordhits, nelm-count);
		count += nhitelm;

		// XXX: Does it need hook/run interface? 
		ndochits = 
			fill_dochit(&(dochits[mem_index->idx]), 
						max_dochit_per_doc, docid, wordhits, nhitelm);
		sb_assert(ndochits >= 1);

		for (i=0; i<ndochits; i++) {
			wordids[mem_index->idx+i] = wordhits->wordid;
			docids[mem_index->idx+i] = docid;
		}

		mem_index->idx += ndochits;
		wordhits += nhitelm;

		if (mem_index->idx >= max_doc_hit_list_num) {
			error("mem_index->idx[%u] > max_doc_hit_list_num[%u]",
					mem_index->idx, max_doc_hit_list_num);
			return;
		}
	}
}

int cmp_by_wordid(const void *a, const void *b)
{
	mem_index_ptr_t *pa = (mem_index_ptr_t *)a;
	mem_index_ptr_t *pb = (mem_index_ptr_t *)b;

	if ((*(pa->wordid)) < (*(pb->wordid))) {
		return -1;
	}
	else if ((*(pa->wordid)) > (*(pb->wordid))) {
		return 1;
	}

	return 0;
}

/* qsort stuff */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t  data1_t;
typedef uint32_t  data2_t;
typedef doc_hit_t data3_t;

#define COMPARE(a, b)  \
	(int32_t)( (data1[a] - data1[b]) ? : (data2[a] - data2[b]) ? : (data3[a].hits[0].std_hit.position - data3[b].hits[0].std_hit.position) )

#define SWAP(a, b)    \
  do {                \
  { register data1_t tmp = data1[a]; data1[a] = data1[b]; data1[b] = tmp; } \
  { register data2_t tmp = data2[a]; data2[a] = data2[b]; data2[b] = tmp; } \
  { register data3_t tmp = data3[a]; data3[a] = data3[b]; data3[b] = tmp; } \
    } while (0)

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
  {
    int lo;
    int hi;
  } stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define STACK_SIZE      (CHAR_BIT * sizeof(size_t))
#define PUSH(low, high) ((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high)  ((void) (--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY  (stack < top)

/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
      next array partition to sort.  To save time, this maximum amount
      of space required to store an array of SIZE_MAX is allocated on the
      stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
      only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
      Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (total_elems)
      stack size is needed (actually O(1) in this case)!  */
void
qsort_index(data1_t *data1, data2_t *data2, data3_t *data3, size_t total_elems)
{
  const int base_ptr = 0;

  const size_t max_thresh = MAX_THRESH;

  if (total_elems == 0)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;

  if (total_elems > MAX_THRESH)
    {
      int lo = 0;
      int hi = total_elems - 1;
      stack_node stack[STACK_SIZE];
      stack_node *top = stack + 1;

      while (STACK_NOT_EMPTY)
        {
          int left_ptr;
          int right_ptr;

          /* Select median value from among LO, MID, and HI. Rearrange
             LO and HI so the three values are sorted. This lowers the
             probability of picking a pathological pivot value and
             skips a comparison for both the LEFT_PTR and RIGHT_PTR in
             the while loops. */

          int mid = lo + ((hi - lo) / 2);

          if (COMPARE(mid, lo) < 0)
              SWAP(mid, lo);
          if (COMPARE(hi, mid) < 0)
              SWAP(mid, hi);
          else
              goto jump_over;
          if (COMPARE(mid, lo) < 0)
              SWAP (mid, lo);
        jump_over:;

        left_ptr  = lo + 1;
        right_ptr = hi - 1;

        /* Here's the famous ``collapse the walls'' section of quicksort.
           Gotta like those tight inner loops!  They are the main reason
           that this algorithm runs much faster than others. */
        do
          {
            while (COMPARE(left_ptr, mid) < 0)
              left_ptr++;

            while (COMPARE(mid, right_ptr) < 0)
              right_ptr--;

            if (left_ptr < right_ptr) {
                SWAP(left_ptr, right_ptr);
                if (mid == left_ptr)
                  mid = right_ptr;
                else if (mid == right_ptr)
                  mid = left_ptr;
                left_ptr++;
                right_ptr--;
            } else if (left_ptr == right_ptr) {
                left_ptr++;
                right_ptr--;
                break;
                }
          }
          while (left_ptr <= right_ptr);

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size.  If so,
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if (right_ptr - lo <= max_thresh)
            {
              if (hi - left_ptr <= max_thresh)
                /* Ignore both small partitions. */
                POP (lo, hi);
              else
                /* Ignore small left partition. */
                lo = left_ptr;
            }
          else if (hi - left_ptr <= max_thresh)
            /* Ignore small right partition. */
            hi = right_ptr;
          else if ((right_ptr - lo) > (hi - left_ptr))
            {
              /* Push larger left partition indices. */
              PUSH (lo, right_ptr);
              lo = left_ptr;
            }
          else
            {
              /* Push larger right partition indices. */
              PUSH (left_ptr, hi);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */

  {
#define min(x, y) ((x) < (y) ? (x) : (y))
    int end_ptr = total_elems - 1;
    int tmp_ptr = base_ptr;
    int thresh = min(end_ptr, base_ptr + max_thresh);
    register int run_ptr;

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr++)
      if (COMPARE(run_ptr, tmp_ptr) < 0)
         tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
        SWAP(tmp_ptr, base_ptr);

    /* Insertion sort, running from left-hand-side up to right-hand-side.  */

    run_ptr = base_ptr + 1;
    while (++run_ptr <= end_ptr) {
        tmp_ptr = run_ptr - 1;
        while (COMPARE(run_ptr, tmp_ptr) < 0)
          tmp_ptr--;

        tmp_ptr++;
        if (tmp_ptr != run_ptr)
          {
            int trav;

            trav = run_ptr+1;
            while (--trav >= run_ptr)
              {
                data1_t c1 = data1[trav];
                data2_t c2 = data2[trav];
                data3_t c3 = data3[trav];
                int hi, lo;

                for (hi = lo = trav; --lo >= tmp_ptr; hi = lo) {
                  data1[hi] = data1[lo];
                  data2[hi] = data2[lo];
                  data3[hi] = data3[lo];
  				}
                data1[hi] = c1;
                data2[hi] = c2;
                data3[hi] = c3;
              }
          }
      }
  }
}

/* qsort stuff end */

static void sort_index_by_wordid(mem_index_t *mem_index)
{
	qsort_index(mem_index->_wids_storage, mem_index->_dids_storage, mem_index->_hits_storage, mem_index->idx);
}

#if FORWARD_INDEX==1
static uint32_t count_successive_docid_same_to_first
					(doc_hit_t *dochits, uint32_t max)
{
	uint32_t first_docid = dochits[0].id;
	uint32_t count = 0;

	for (count = 1; count < max; count++) {
		if (dochits[count].id != first_docid)
			return count;
	}

	return max;
}
#endif

static uint32_t count_successive_wordid_same_to_first_in_wordids
					(uint32_t wordids[], uint32_t max)
{
	uint32_t first_wordid = wordids[0];
	uint32_t count = 0;

	for (count=1; count < max; count++) {
		if (wordids[count] != first_wordid) {
			return count;
		}
	}

	return max;
}

static int cmp_word_hit(const void* var1,const void* var2) {
	if ( ((word_hit_t*)var1)->wordid > ((word_hit_t*)var2)->wordid ) 
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

	sz_strncpy(mInvertedIdxFile, v.argument[0], STRING_SIZE);
	sz_snprintf(mErrorDocumentFile, STRING_SIZE, 
				"%s%s", mInvertedIdxFile, ".error.documents");
}

#if FORWARD_INDEX==1
static void set_forward_index_file(configValue v)
{
	sz_strncpy(mForwardIdxFile, v.argument[0], STRING_SIZE);
}
#endif
static void set_timer(configValue v)
{
	mTimer=atoi(v.argument[0]);
	DEBUG("timer is set to %d",mTimer);
}
static void set_doc_hit_list_memorysize(configValue v)
{
	doc_hit_list_memorysize = atoi(v.argument[0]);
	INFO("doc_hit_list_memorysize:%u", doc_hit_list_memorysize);
}

static void set_spool_path(configValue v)
{
    strncpy(spoolpath, v.argument[0], STRING_SIZE);
    spoolpath[STRING_SIZE-1] = '\0';
}

static void set_spool_queue_size(configValue v)
{
    spool_queue_size = atoi(v.argument[0]);
}

static void set_spool_mpool_size(configValue v)
{
    spool_mpool_size = atoi(v.argument[0]);
}

/* registry related functions */
REGISTRY void init_vrf_lock(void *data)
{
	rVrfSemid = data;
	DEBUG("called");
}
REGISTRY char* registry_get_vrflock()
{
	//XXX: maybe need more detailed INFOrmation
	return NULL;
}
REGISTRY void init_last_indexed_docid(void *data)
{
	last_indexed_docid = data;
	*last_indexed_docid = 0;
}
REGISTRY char* registry_get_last_indexed_did()
{
	static char buf[STRING_SIZE];

	snprintf(buf,STRING_SIZE,"last indexed did:%d",(*last_indexed_docid));

	return buf;
}

static registry_t registry[] = {
	RUNTIME_REGISTRY("VRFLock","lock for vrf object",sizeof(int),
					init_vrf_lock, registry_get_vrflock,NULL),
	PERSISTENT_REGISTRY("LAST_INDEXED_DOCID","last indexed document id",sizeof(uint32_t),
					init_last_indexed_docid, registry_get_last_indexed_did,NULL),
	NULL_REGISTRY
};


static config_t config[] = {
    CONFIG_GET("IndexerSpoolPath",set_spool_path,1,\
            "spool path which indexer uses. (e.g: IndexerSpoolPath dat/indexer/indexer.spl)"),
    CONFIG_GET("IndexerSpoolQueueSize",set_spool_queue_size,1,\
            ""),
    CONFIG_GET("IndexerSpoolMpoolSize",set_spool_mpool_size,1,\
            ""),
	CONFIG_GET("InvertedIndexFile",set_inverted_index_file,1,\
			"inv indexer db path (e.g: IndexDbPath dat/indexdb)"),
	CONFIG_GET("IndexTimer",set_timer,1,\
			"timer(sec) for watching if there's newly assembled document"),
	CONFIG_GET("IndexMemorySize", set_doc_hit_list_memorysize, 1, \
			"dochit list memory size (e.g: IndexMemorySize 200000000)"),
#if FORWARD_INDEX==1
	CONFIG_GET("ForwardIndexFile",set_forward_index_file,1,\
		"forward index db file path (e.g: ForwardIndexFile dat/forward_index/forward)"),
#endif	
	{NULL}

};

static uint32_t last_indexed_did(void)
{
	return *last_indexed_docid;
}

static void register_hooks(void)
{
	sb_hook_last_indexed_did(last_indexed_did,NULL, NULL, HOOK_FIRST);
}

module spooled_indexer_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	init,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};

module spooled_indexer_test_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	test_init,				/* initialize function of module */
	test_main,				/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};
