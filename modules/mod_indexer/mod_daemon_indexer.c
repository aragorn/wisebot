/* $Id$ */
#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/vrfi.h"
#include "mod_api/lexicon.h"
#include "mod_api/indexer.h"
#include "mod_api/tcp.h"
#include "mod_api/cdm.h"
#include "mod_api/index_word_extractor.h"

#include "mod_qp/mod_qp.h" /* XXX: STD_HITS_LEN */
#include "mod_daemon_indexer.h"
#include "mod_index_each_doc.h"

#include "hit.h"

#define MONITORING_PERIOD	5
#define MAGICNUMBER 	    12345
#define SKIP_DOCUMENT      -104
#define MAX_TEST_FILE_LEN	2100000000

#define SEND_RET_AND_CLOSE(ret) \
	    if ( sb_run_tcp_send(sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) \
			error("cannot send RET"); \
		sb_run_tcp_close(sockfd); \
		if (data != NULL) { sb_free(data); data = NULL; }

#define ADD_AND_SEND_AND_CLOSE(ret) \
		add_result_document(&index_result, docid, SKIP_DOCUMENT); \
        add_error_document(&index_error, docid, SKIP_DOCUMENT); \
		indexer_shared->last_indexed_docid = docid; \
        if ( sb_run_tcp_send(sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) \
		    error("cannot send RET"); \
		sb_run_tcp_close(sockfd); \
		if (data != NULL) { sb_free(data); data = NULL; }
		
uint32_t doc_hit_list_memorysize = 300*1024*1024; /* 300 Mbytes */
uint32_t max_word_hit_len=400000;
uint32_t max_doc_hit_list_num=3500000;

struct indexer_shared_t {
	uint32_t last_indexed_docid;
} *indexer_shared = NULL;
static char indexer_shared_file[MAX_FILE_LEN] = "dat/indexer/indexer.shared";

static char mSocketFile[SHORT_STRING_SIZE] = "dat/indexer/socket";
static int m_backlog = 8;
static int listenfd;

#define MAX_LOGGING_SKIP_DOCUMENTS 100
uint32_t error_documents_idx = 0;
uint32_t documents_idx = 0;
static char mErrorDocumentFile[STRING_SIZE]="dat/indexer/index.error.documents";
static char mDocumentFile[STRING_SIZE]="dat/indexer/index.documents";
static char mTestDataFile[STRING_SIZE]="dat/indexer/index.test.data";
static int mErrorFileFd=0;
static int mDocFileFd=0;
static int mTestFileFd=0;
static int mTestFileNumber=0;

/* XXX: 일단은 child 1개 */
static scoreboard_t scoreboard[] = {PROCESS_SCOREBOARD(1)};

typedef struct {
	uint32_t result[MAX_LOGGING_SKIP_DOCUMENTS];
	int		 return_code[MAX_LOGGING_SKIP_DOCUMENTS];
} index_result_t;

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

static int mWordDbSet = -1;
static word_db_t* mWordDb = NULL;

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
static int set_mem_index(void* word_db, mem_index_t *mem_index, uint32_t docid, void *data, int size);
static int recv_from_rmac(int sockfd, void **data, uint32_t *docid, int *size);

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
		(doc_hit_t*)sb_malloc(sizeof(doc_hit_t) * max_doc_hit_list_num);
	if (mem_index->dochits==NULL) { crit("malloc failed:%s", strerror(errno)); return; }
#endif

	mem_index->idx = 0;

	mem_index->_wids_storage = 
		(uint32_t*)sb_malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->_wids_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_dids_storage = 
		(uint32_t *)sb_malloc(sizeof(uint32_t) * max_doc_hit_list_num);
	if (mem_index->_dids_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_hits_storage = 
		(doc_hit_t*)sb_malloc(sizeof(doc_hit_t) * max_doc_hit_list_num);
	if (mem_index->_hits_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

	mem_index->_word_hits_storage =
		(word_hit_t*)sb_malloc(sizeof(word_hit_t) * max_word_hit_len);
	if (mem_index->_word_hits_storage==NULL) { crit("malloc failed:%s", strerror(errno)); return; }

#if 0
	mem_index->list =
		(mem_index_ptr_t*)sb_malloc(sizeof(mem_index_ptr_t) * max_doc_hit_list_num);
	if (mem_index->list==NULL) { crit("malloc failed:%s", strerror(errno)); return; }
#endif
}

static int free_mem_index(mem_index_t *mem_index)
{
	sb_free( mem_index->_wids_storage ); 
	sb_free( mem_index->_dids_storage );
	sb_free( mem_index->_hits_storage );
	sb_free( mem_index->_word_hits_storage);

	return SUCCESS;
}

static int is_full(mem_index_t *mem_index)
{
	//XXX: max_word_hit_len ???
	if (mem_index->idx >= max_doc_hit_list_num - max_word_hit_len)
		return TRUE;

	return FAIL;
}

#define DOCUMENT_LOGGING_SIZE MAX_LOGGING_SKIP_DOCUMENTS * 16 /* sizeof("error 123456\n") ~= 16 */
void save_wrong_documents(index_result_t *index_result, uint32_t nelm, int file_chk)
{
	static char result[DOCUMENT_LOGGING_SIZE]="";
	char buf[STRING_SIZE]="";
	int i=0, rv=0, size_left=DOCUMENT_LOGGING_SIZE;
	char write_done=0;
	int fileFd = 0;
	
	if (file_chk == 0)
		fileFd = mErrorFileFd;
	else
		fileFd = mDocFileFd;
	
	if (fileFd <= 0) {
		warn("error document dropped. error file not opened");
		return;
	}

	rv = lseek(fileFd, 0, SEEK_END);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return;
	}

	i=0;
	while (!write_done) {
		result[0] = '\0';
		size_left = DOCUMENT_LOGGING_SIZE;

		for ( ; i<nelm; i++) {
//			warn("i = %d, nelm = %u, docid = %u, code = %d, size_left = %d", i, nelm, index_error->result[i], index_error->return_code[i], size_left);
			sz_snprintf(buf, STRING_SIZE,
						"%u %d\n", index_result->result[i], index_result->return_code[i]);
			sz_strncat(result, buf, size_left);
			size_left -= strlen(buf);
			if (size_left < STRING_SIZE)
				break;
		}

		rv = write(fileFd, result, strlen(result));
		if (rv == -1) {
			error("write failed:%s", strerror(errno));
			return;
		}

		if (i==nelm) write_done=1;
	}
}
void save_all_wrong_documents(index_result_t *index_result, index_result_t *index_error)
{
	save_wrong_documents(index_error, error_documents_idx, 0);
    error_documents_idx=0;

	save_wrong_documents(index_result, documents_idx, 1);
    documents_idx=0;
}
void add_error_document(index_result_t *index_error , uint32_t error_docid, int error_code)
{
	if (error_documents_idx >= MAX_LOGGING_SKIP_DOCUMENTS) {
		save_wrong_documents(index_error, error_documents_idx, 0);
		error_documents_idx = 0;
	}

	index_error->result[error_documents_idx] = error_docid;
	index_error->return_code[error_documents_idx] = error_code;
	error_documents_idx++;
}
void add_result_document(index_result_t *index_result , uint32_t docid, int return_code)
{
	if (documents_idx >= MAX_LOGGING_SKIP_DOCUMENTS) {
		save_wrong_documents(index_result, documents_idx, 1);
		documents_idx = 0;
	}

	index_result->result[documents_idx] = docid;
	index_result->return_code[documents_idx] = return_code;
	documents_idx++;
}
#define OPEN_FLAG   (O_RDWR)
//#define APPEND_FLAG (O_APPEND)
//#define CREATE_APPEND_FLAG (O_APPEND|O_CREAT)
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
	mDocFileFd = sb_open(mDocumentFile, OPEN_FLAG, MODE);
	if (mDocFileFd == -1) {
		mDocFileFd = sb_open(mDocumentFile, CREATE_FLAG, MODE);
		if (mDocFileFd == -1) {
			crit("unable to open file[%s]:%s", mDocumentFile, strerror(errno));
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
//	void *test_data=0x00;
    
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
	
//	test_data = (void *)sb_malloc(size+8);
//	if (test_data == NULL)
//  {
//        error("insufficient memory");
//        return FAIL;
//    }
//	memcpy(test_data, &docid, sizeof(uint32_t));
//	memcpy(test_data+4, &size, sizeof(int));
//	memcpy(test_data+8, data, size);
	
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
	
//	sb_free(test_data);
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
static int indexer_main(slot_t *slot)
{
	uint32_t last_registered_docid=0, docid;
	mem_index_t mem_index;
	struct timeval timeout,ts,tf;
	double cumulative_time = 0.0, diff=0.0;
	uint32_t indexednum=0, cumulative_indexednum=0;
	int sockfd = -1, ret = 0, size = 0;
	socklen_t len = 0;
	static struct sockaddr remote_addr;
	void *data=0x00;
	index_result_t index_result;
	index_result_t index_error;

	slot->state = SLOT_PROCESS;

	if ( sb_run_open_word_db( &mWordDb, mWordDbSet ) != SUCCESS ) {
		error("lexicon open failed");
		slot->state = SLOT_FINISH;
		return -1;
	}

	if ( sb_run_vrfi_alloc(&mVRFI) != SUCCESS ) {
		error("vrfi alloc failed: %s", strerror(errno));
		slot->state = SLOT_FINISH;
		return -1;
	}

	if ( sb_run_vrfi_open(mVRFI, mInvertedIdxFile,
				sizeof(inv_idx_header_t), sizeof(doc_hit_t), O_RDWR) != SUCCESS ) {
		error("cannot open vrfi. path[%s]", mInvertedIdxFile);
		slot->state = SLOT_FINISH;
		return -1;
	}

#if FORWARD_INDEX==1
	if ( sb_run_vrfi_alloc(&mForwardVRFI) != SUCCESS ) {
		error("forward vrfi alloc failed: %s", strerror(errno));
		slot->state = SLOT_FINISH;
		return -1;
	}
	if ( sb_run_vrfi_open(mForwardVRFI,
			mForwardIdxFile,sizeof(forwardidx_header_t), sizeof(forward_hit_t), O_RDWR) != SUCCESS ) {
		error("error opening forward vrf[%s]", mForwardIdxFile);
		slot->state = SLOT_FINISH;
		return -1;
	}
#endif

	len = sizeof(remote_addr);

	/* must be called after forking because it allocates large junk of memory */
	init_mem_index(&mem_index);
	open_error_documents_file();

	if (sb_run_tcp_local_bind_listen(mSocketFile, m_backlog, &listenfd) != SUCCESS) {
   		error("tcp_local_bind_listen: %s", strerror(errno));
		free_mem_index(&mem_index);
   		return FAIL;
    }
	setproctitle("softbotd: %s(%u indexed, now idle)", __FILE__, (indexer_shared->last_indexed_docid) );
	
	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			break;

		mem_index.idx=0;
		indexednum=0;
		gettimeofday(&ts,NULL);
		
		// mem_index가 full이 될때까지 loop를 돈다.
		while (is_full(&mem_index) == FALSE)
		{
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
				break;

			ret = sb_run_tcp_select_accept(listenfd, &sockfd, &remote_addr, &len);
			if (ret != SUCCESS) break;
		
			// rmac으로 부터 형태소 분석된 데이타를 받아온다.
			ret = recv_from_rmac(sockfd, &data, &docid, &size);
			// ret 가 SKIP_DOCUMENT는 rmac에서 에러가 난경우로 last_indexed_docid를 증가시키고 continue
			if (ret == SKIP_DOCUMENT) 
			{
				ret = FAIL;
				error("document skip, docid = %d", docid);
				// 에러파일에 기록, last_indexed_docid 증가, rmac에 ret전송, data 널이 아닐때 free
				ADD_AND_SEND_AND_CLOSE(ret);
				continue;
			}
			// FAIL일 경우는 tcp 통신 에러로 다시 시도를 한다.
			else if (ret == FAIL)
			{
				error("recv_from_rmac error");
    		    if ( sb_run_tcp_send(sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) 
			    	error("cannot send RET"); 
				sb_run_tcp_close(sockfd); 
				continue;
			}

            if (indexer_shared->last_indexed_docid+1 != docid)
			{
				error("last_indexed_docid[%d]+1 != docid[%d]", indexer_shared->last_indexed_docid, docid);
			}

			if (indexer_shared->last_indexed_docid >= docid) 
			{
				error("last_indexed_docid[%d] is greater than docid[%d]", indexer_shared->last_indexed_docid, docid);
			    SEND_RET_AND_CLOSE(ret);
				continue;
			}
							
			if (indexednum % 10 == 0) {
				setproctitle("softbotd: %s (%u/%u indexed[avg idx spd: %.2f doc/s])",
				__FILE__, indexer_shared->last_indexed_docid, last_registered_docid,
				 (cumulative_time) ? (double)cumulative_indexednum/cumulative_time : 0);
			}

			// 형태소 분석된 결과를 mem_index에 저장
			ret = set_mem_index(mWordDb, &mem_index, docid, data, size);
			if (ret != SUCCESS) {
				error("set_mem_doc error, docid=%d", docid);
				ADD_AND_SEND_AND_CLOSE(ret);
				continue;
			}
			else { /* SUCCESS */
				indexednum++;
				add_result_document(&index_result, docid, SUCCESS);
			}
			
		    SEND_RET_AND_CLOSE(ret);
			indexer_shared->last_indexed_docid = docid;

			last_registered_docid = sb_run_server_canneddoc_last_registered_id();

			// 더이상 색인할 데이타가 없을시 break;
			if (last_registered_docid == indexer_shared->last_indexed_docid) {
				int i, no_work = 1;
				for ( i = 0; i < 10; i++ ) {
					sleep( 1 ); // 잠깐 쉬어보고 진짜 없는지 확인
					if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

					last_registered_docid = sb_run_server_canneddoc_last_registered_id();
					if (last_registered_docid != indexer_shared->last_indexed_docid) {
						no_work = 0;
						break;
					}
				} // for(i)

				if ( no_work ) break;
			}
		}  // while(is_full())

		if (indexednum == 0)
		{	
			// indexednum이 0이 아닐때는 색인할 데이타가 남아 있으므로 죽으면 안됨.
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
				break;
			
			setproctitle("softbotd: %s(%u indexed, now idle [avg idx spd:%.2f doc/s])",
                __FILE__,
                (indexer_shared->last_indexed_docid),
                (double)cumulative_indexednum/cumulative_time);

			timeout.tv_sec = mTimer;
			timeout.tv_usec = 0;
			select(0, NULL, NULL, NULL, &timeout);

			continue;
		}
	
		setproctitle("softbotd: %s (%u indexed, saving %u)",
						__FILE__, indexer_shared->last_indexed_docid, indexednum);

		save_all_wrong_documents(&index_result, &index_error);
#if FORWARD_INDEX==1
		save_forward_index(&mem_index);
#endif
		sort_index_by_wordid(&mem_index);
		save_index(&mem_index);

		/* every saving time, word db is also saved */
		sb_run_sync_word_db(mWordDb); //XXX: indexer should do this?

		sb_run_vrfi_sync(mVRFI);
#if FORWARD_INDEX==1
		sb_run_vrfi_sync(mForwardVRFI); // XXX
#endif

		gettimeofday(&tf,NULL);
		diff = timediff(&tf, &ts);
		ts = tf;

		cumulative_time =  diff;
		cumulative_indexednum = indexednum;

		setproctitle("softbotd: %s(%u/%u indexed.[avg idx spd:%.2f doc/s])",
				__FILE__, (indexer_shared->last_indexed_docid), last_registered_docid,
				(double)cumulative_indexednum/cumulative_time);
	
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			break;
	} /* endless while loop */

	sb_run_close_word_db(mWordDb);

	sb_run_vrfi_close(mVRFI);
#if FORWARD_INDEX==1
	sb_run_vrfi_close(mForwardVRFI); // XXX
#endif
	close(mErrorFileFd);
	close(mDocFileFd);
	free_mem_index(&mem_index);
	
	if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
		slot->state = SLOT_FINISH;
	else
		slot->state = SLOT_RESTART;

	return 0;
}

static int test_indexer_main(slot_t *slot)
{
	uint32_t last_registered_docid=0, docid;
	struct timeval ts,tf;
	double cumulative_time = 0.0, diff=0.0;
	uint32_t indexednum=0, cumulative_indexednum=0;
	int sockfd = -1, ret = 0, size = 0;
	socklen_t len = 0;
	static struct sockaddr remote_addr;
	void *data=0x00;
	index_result_t index_result;
	index_result_t index_error;

	len = sizeof(remote_addr);

	/* must be called after forking because it allocates large junk of memory */
	open_error_documents_file();
	open_test_data_file();

	if (sb_run_tcp_local_bind_listen(mSocketFile, m_backlog, &listenfd) != SUCCESS) {
   		error("tcp_local_bind_listen: %s", strerror(errno));
   		return FAIL;
    }
   	setproctitle("softbotd: mod_daemaon_indexer.c (listening[%s])",mSocketFile);
	
	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			break;

		indexednum=0;
		gettimeofday(&ts,NULL);
		last_registered_docid = sb_run_server_canneddoc_last_registered_id();
		
		ret = sb_run_tcp_select_accept(listenfd, &sockfd, &remote_addr, &len);
		if (ret != SUCCESS) break;
		
		// rmac으로 부터 형태소 분석된 데이타를 받아온다.
		ret = recv_from_rmac(sockfd, &data, &docid, &size);
		// ret 가 SKIP_DOCUMENT는 rmac에서 에러가 난경우로 last_indexed_docid를 증가시키고 continue
		if (ret == SKIP_DOCUMENT) 
		{
			ret = FAIL;
			error("document skip, docid = %d", docid);
			ADD_AND_SEND_AND_CLOSE(ret);
			continue;
		}
		// FAIL일 경우는 tcp 통신 에러로 다시 시도를 한다.
		else if (ret == FAIL)
		{
			error("recv_from_rmac error");
		    SEND_RET_AND_CLOSE(ret);
			continue;
		}

        if (indexer_shared->last_indexed_docid+1 != docid)
		{
			error("last_indexed_docid[%d]+1 != docid[%d]", indexer_shared->last_indexed_docid, docid);
		}

		if (indexer_shared->last_indexed_docid >= docid) 
		{
			error("last_indexed_docid[%d] is greater than docid[%d]", indexer_shared->last_indexed_docid, docid);
		    SEND_RET_AND_CLOSE(ret);
			continue;
		}
							
		if (indexednum % 10 == 0) {
			setproctitle("softbotd: %s (%u indexed[avg idx spd: %.2f doc/s], [%u] left)",
			__FILE__, indexer_shared->last_indexed_docid, 
			 (cumulative_time) ? (double)cumulative_indexednum/cumulative_time : 0,
			 last_registered_docid - indexer_shared->last_indexed_docid);
		}

		indexednum++;
		add_result_document(&index_result, docid, SUCCESS);
		// test dump 데이타에서 bytepos에 0을 넣어주기 위한 함수
		set_bytepos(data, size);
		 
		// rmac으로부터 받은 데이타를 docid + data_size + data 형태로 파일로 저장한다.
 		ret = write_test_data(docid, size, data);
		if (ret != SUCCESS)
		{	
			error("rmac_data write error, docid = %u, size = %d", docid, size);
		    SEND_RET_AND_CLOSE(ret);
			continue;
		}
		
	    SEND_RET_AND_CLOSE(ret);
		indexer_shared->last_indexed_docid = docid;

		gettimeofday(&tf,NULL);
		diff = timediff(&tf, &ts);

		cumulative_time +=  diff;
		cumulative_indexednum += indexednum;

		setproctitle("softbotd: %s(%u indexed. [%u] left.[avg idx spd:%.2f doc/s])",
				__FILE__,
				(indexer_shared->last_indexed_docid),
				last_registered_docid - indexer_shared->last_indexed_docid,
				(double)cumulative_indexednum/cumulative_time);
	
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown) 
			break;
	} /* endless while loop */

	save_all_wrong_documents(&index_result, &index_error);	
	close(mErrorFileFd);
	close(mDocFileFd);
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
static int init() {
	ipc_t ipc;

	ipc.type = IPC_TYPE_MMAP;
	ipc.pathname = indexer_shared_file;
	ipc.size = sizeof(struct indexer_shared_t);

	if ( alloc_mmap(&ipc, 0) != SUCCESS ) {
		error("alloc mmap to indexer_shared failed");
		return FAIL;
	}
	indexer_shared = (struct indexer_shared_t*) ipc.addr; 

	if ( ipc.attr == MMAP_CREATED )
		memset( indexer_shared, 0, ipc.size );

	return SUCCESS;
}

int set_mem_index(void* word_db, mem_index_t *mem_index, uint32_t docid, void *data, int size)
{
	int16_t ret=0;
	word_hit_t *wordhits = mem_index->_word_hits_storage;
	uint32_t hitidx=0;

	sb_assert(docid > 0);

	ret = sb_run_index_each_doc(word_db, docid, wordhits, max_word_hit_len, &hitidx, data, size);
	if (ret != SUCCESS) {
		return ret;
	}

	// wordid순 정렬. stable해야 한다. 
	// 즉, 동일한 word에 대해 앞선 field에 나온 word가 우선
	mergesort(wordhits,(size_t)hitidx,
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

int recv_from_rmac(int sockfd, void **data, uint32_t *docid, int *size)
{
	int magic_number = 0;
	

	if ( sb_run_tcp_recv(sockfd, &magic_number, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) 
	{
        error("cannot recv docid");
        return FAIL;
    }
	if (magic_number != 12345)
	{
		error("magic number[%d] is not correct", magic_number);
		return FAIL;
	}
	
	if ( sb_run_tcp_recv(sockfd, docid, sizeof(uint32_t), sb_run_tcp_server_timeout()) != SUCCESS ) 
	{
        error("cannot recv docid");
        return FAIL;
    }
	
	if ( sb_run_tcp_recv(sockfd, size, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) 
	{
        error("cannot recv size");
        return FAIL;
    }

	if (*size <= 0) {
		error("wrong recv_data_size : %d", *size);
		return SKIP_DOCUMENT;
	}

	if (*data != NULL)
	{
		error("*data is not NULL");
		sb_free(*data);	
	}
	
	*data = (void *)sb_malloc(*size);
	if ( *data == NULL )
	{
		error("insufficient memory");
		return FAIL;
	}

	if ( sb_run_tcp_recv(sockfd, *data, *size, sb_run_tcp_server_timeout()) != SUCCESS ) 
	{
        error("cannot recv data");
		sb_free(*data);
		*data = NULL;
        return FAIL;
    }
	
	return SUCCESS;
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

static void set_socket_file(configValue v)
{
	sz_strncpy(mSocketFile, v.argument[0], SHORT_STRING_SIZE);
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

static void set_word_db_set(configValue v)
{
	mWordDbSet = atoi( v.argument[0] );
}

static void set_shared_file(configValue v)
{
	strncpy(indexer_shared_file, v.argument[0], MAX_FILE_LEN);
	indexer_shared_file[MAX_FILE_LEN-1] = '\0';
}

static config_t config[] = {
	CONFIG_GET("SocketFile",set_socket_file,1,"set socket file"),
	CONFIG_GET("InvertedIndexFile",set_inverted_index_file,1,\
			"inv indexer db path (e.g: IndexDbPath dat/indexdb)"),
	CONFIG_GET("IndexTimer",set_timer,1,\
			"timer(sec) for watching if there's newly assembled document"),
	CONFIG_GET("IndexMemorySize", set_doc_hit_list_memorysize, 1, \
			"dochit list memory size (e.g: IndexMemorySize 200000000)"),
	CONFIG_GET("WordDbSet", set_word_db_set, 1, "WordDbSet {number}"),
	CONFIG_GET("SharedFile",set_shared_file,1,"(e.g: SharedFile dat/indexer/indexer.shared)"),
#if FORWARD_INDEX==1
	CONFIG_GET("ForwardIndexFile",set_forward_index_file,1,\
		"forward index db file path (e.g: ForwardIndexFile dat/forward_index/forward)"),
#endif	
	{NULL}
};

static uint32_t last_indexed_did(void)
{
	return indexer_shared->last_indexed_docid;
}
// 원본을 return하므로 손상하지 않도록 주의한다.
static const char* get_socket_file(void)
{
	return mSocketFile;
}

static void register_hooks(void)
{
	sb_hook_last_indexed_did(last_indexed_did,NULL, NULL, HOOK_FIRST);
	sb_hook_get_indexer_socket_file(get_socket_file,NULL,NULL,HOOK_FIRST);
}

module daemon_indexer_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,    				/* registry */
	init,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};

module daemon_indexer_test_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,    				/* registry */
	init,					/* initialize function of module */
	test_main,				/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};
