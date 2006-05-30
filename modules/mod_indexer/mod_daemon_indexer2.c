/* $Id$ */
#include <signal.h>
#include <stdlib.h> /* abort(3) */
#include <unistd.h> /* lseek(2) */
#include <fcntl.h> /* O_RDWR */
#include <sys/time.h> /* gettimeofday(2) */
#include <string.h>
#include <errno.h>
#include <sys/stat.h> /* S_IREAD,S_IWRITE */
#include "common_core.h"
#include "memory.h"
#include "ipc.h"
#include "setproctitle.h"
#include "common_util.h"
#include "mod_api/lexicon.h"
#include "mod_api/indexer.h"
#include "mod_api/indexdb.h"
#include "mod_api/tcp.h"
#include "mod_api/cdm.h"
#include "mod_api/cdm2.h"
#include "mod_api/index_word_extractor.h"

#include "mod_index_each_doc.h"
#include "mod_daemon_indexer.h"

#define MONITORING_PERIOD	5
#define MAGICNUMBER 	    12345
#define SKIP_DOCUMENT      -104
#define MAX_TEST_FILE_LEN	2100000000

static word_hit_t *wordhits_storage = NULL; // 매번 malloc, free하기 싫으니까...
static int max_word_hit = 400000;

/* XXX: 일단은 child 1개 */
static scoreboard_t scoreboard[] = {PROCESS_SCOREBOARD(1)};

#define SEND_RET_AND_CLOSE(ret) \
	    if ( sb_run_tcp_send(sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) \
			error("cannot send RET"); \
		sb_run_tcp_close(sockfd); \
		if (data != NULL) { sb_free(data); data = NULL; }

#define ADD_AND_SEND_AND_CLOSE(ret) \
		add_result_document(docid, SKIP_DOCUMENT); \
        add_error_document(docid, SKIP_DOCUMENT); \
		indexer_shared->last_indexed_docid = docid; \
        if ( sb_run_tcp_send(sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) \
		    error("cannot send RET"); \
		sb_run_tcp_close(sockfd); \
		if (data != NULL) { sb_free(data); data = NULL; }
		
static char mErrorDocumentFile[STRING_SIZE]="dat/indexer/error.documents";
static char mDocumentFile[STRING_SIZE]="dat/indexer/index.documents";
static char mTestDataFile[STRING_SIZE]="dat/indexer/test.data";
static int mErrorFileFd=0;
static int mDocFileFd=0;
static int mTestFileFd=0;
static int mTestFileNumber=0;

/* member variables  */
static char mLogDir[STRING_SIZE]="dat/indexer";
static char mSocketFile[SHORT_STRING_SIZE] = "dat/indexer/socket";
static int mCdmSet = -1;
static int mWordDbSet = -1;
static int mIndexDbSet = -1;
#define BACKLOG (3)

struct indexer_shared_t {
	uint32_t last_indexed_docid;
} *indexer_shared = NULL;
static char indexer_shared_file[MAX_FILE_LEN] = "dat/indexer/indexer.shared";

/* private functions */
static uint32_t count_successive_wordid_same_to_first_in_wordhit
							(word_hit_t *wordhits, uint32_t max);
							static int cmp_word_hit(const void* var1,const void* var2);
static int recv_from_rmac(int sockfd, void **data, uint32_t *docid, int *size);
int save_to_indexdb(index_db_t* indexdb, word_db_t* word_db,
		int docid, word_hit_t* wordhits, int wordhit_len, void* data, int size);

// indexer log
static FILE* idxlog_fp = NULL;
static char idxlog_path[MAX_PATH_LEN] = "logs/indexer_log";
static void write_indexer_log(const char* type, const char* format, ...);
static void (*sighup_handler)(int sig) = NULL;
#define IDXLOG_ERROR(format, ...) \
	error(format, ##__VA_ARGS__); \
	write_indexer_log("error", format, ##__VA_ARGS__);
#define IDXLOG_INFO(format, ...) \
	info(format, ##__VA_ARGS__); \
	write_indexer_log("info", format, ##__VA_ARGS__);

static void write_indexer_log(const char* type, const char* format, ...)
{
	va_list args;
	time_t now;
	time(&now);

	va_start(args, format);
	fprintf(idxlog_fp, "[%.24s] [%s] i.c m() ", ctime(&now), type);
	vfprintf(idxlog_fp, format, args);
	fputc('\n', idxlog_fp);
	va_end(args);
}

/*** signal handler **********************************************************/
static RETSIGTYPE _do_nothing(int sig) { return; }

static RETSIGTYPE _shutdown(int sig)
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
static RETSIGTYPE _graceful_shutdown(int sig)
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

static RETSIGTYPE reopen_idxlog(int sig)
{
	fclose(idxlog_fp);

	idxlog_fp = sb_fopen(idxlog_path, "a");
	if ( idxlog_fp == NULL ){
		error("%s open failed: %s", idxlog_path, strerror(errno));
	}
	else setlinebuf(idxlog_fp);

	if ( sighup_handler != NULL
			&& sighup_handler != SIG_DFL && sighup_handler != SIG_IGN )
		sighup_handler(sig);
}

int init_wordhits_storage()
{
	if ( wordhits_storage ) return SUCCESS;

	wordhits_storage = (word_hit_t*) sb_malloc( sizeof(doc_hit_t) * max_word_hit );
	if ( wordhits_storage == NULL ) {
		error("malloc failed: %s", strerror(errno));
		return FAIL;
	}
	else return SUCCESS;
}

void free_wordhits_storage()
{
	if ( wordhits_storage ) {
		sb_free( wordhits_storage );
		wordhits_storage = NULL;
	}
}

void save_wrong_documents(uint32_t docid, int result, int file_chk)
{
	char buf[STRING_SIZE]="";
	int fd = 0, rv;
	
	if (file_chk == 0) fd = mErrorFileFd;
	else               fd = mDocFileFd;
	
	if (fd <= 0) {
		warn("error document dropped. error file not opened");
		return;
	}

	rv = lseek(fd, 0, SEEK_END);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return;
	}

	sz_snprintf(buf, STRING_SIZE, "%u %d\n", docid, result);
	rv = write(fd, buf, strlen(buf));

	if (rv == -1) {
		error("write failed:%s", strerror(errno));
		return;
	}
}

void add_error_document(uint32_t docid, int result)
{
	save_wrong_documents(docid, result, 0);
}

void add_result_document(uint32_t docid, int result)
{
	save_wrong_documents(docid, result, 1);
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

static uint32_t last_registered_did(cdm_db_t* cdm_db)
{
	if ( cdm_db ) { // mod_cdm2.so
		return sb_run_cdm_last_docid(cdm_db);
	}
	else { // mod_cdm.so
		return sb_run_server_canneddoc_last_registered_id();
	}
}

static int indexer_main(slot_t *slot)
{
	int ret = 0;

	/* socket */
	struct sockaddr remote_addr;
	int sockfd, listenfd;
	socklen_t len;

	/* data */
	void *data = NULL; int size;
	uint32_t last_registered_docid = 0, docid;

	/* cdm db */
	cdm_db_t* cdm_db = NULL;
	int b_use_cdm;

	/* word db */
	word_db_t* word_db = NULL;

	/* indexdb(indexdb?) */
	index_db_t* indexdb = NULL;

	/* 통계 */
	int indexed_num;
	double index_time;
	struct timeval start_time, end_time;

	/* hup signal handler */
	struct sigaction act, oldact;

	slot->state = SLOT_PROCESS;

	if ( init_wordhits_storage() == FAIL ) {
		error("wordhits_storage init failed");
		goto error_return;
	}

	b_use_cdm = ( find_module("mod_cdm.c") != NULL );
	if ( b_use_cdm ) {
		if ( sb_run_server_canneddoc_init() != SUCCESS ) {
			error("cdm open failed");
			goto error_return;
		}
	}
	else {
		if ( sb_run_cdm_open( &cdm_db, mCdmSet ) != SUCCESS ) {
			error("cdm2 open failed");
			goto error_return;
		}
	}

	if ( sb_run_open_word_db( &word_db, mWordDbSet ) != SUCCESS ) {
		error("word db open failed");
		goto error_return;
	}

	ret = sb_run_indexdb_open( &indexdb, mIndexDbSet );
	if ( ret == FAIL ) {
		error("indexdb load failed");
		goto error_return;
	}

	open_error_documents_file();

	/* open indexer_log */
	idxlog_fp = sb_fopen(idxlog_path, "a");
	if ( idxlog_fp == NULL ) {
		error("%s open failed: %s", idxlog_path, strerror(errno));
		goto error_return;
	}
	else setlinebuf(idxlog_fp);

	/* register HUP signal handler */
	memset(&act, 0x0, sizeof(act));
	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);
	act.sa_handler = reopen_idxlog;
	sigaction(SIGHUP, &act, &oldact);
	sighup_handler = oldact.sa_handler;

	if (sb_run_tcp_local_bind_listen(mSocketFile, BACKLOG, &listenfd) != SUCCESS) {
   		error("tcp_local_bind_listen: %s", strerror(errno));
		goto error_return;
    }
	setproctitle("softbotd: %s(%u indexed, now idle)", __FILE__, (indexer_shared->last_indexed_docid) );

	indexed_num = 0;
	
	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			break;

		len = sizeof(remote_addr);
		ret = sb_run_tcp_select_accept(listenfd, &sockfd, &remote_addr, &len);
		if (ret != SUCCESS) break;

		if ( indexed_num == 0 )
			gettimeofday( &start_time, NULL );

		indexed_num++;
		
		// rmac으로 부터 형태소 분석된 데이타를 받아온다.
		ret = recv_from_rmac(sockfd, &data, &docid, &size);
		if (ret == SKIP_DOCUMENT) 
		{
			IDXLOG_ERROR("document skip, docid[%d]", docid);
			ADD_AND_SEND_AND_CLOSE(ret);
			continue;
		}
		// FAIL일 경우는 tcp 통신 에러로 다시 시도를 한다.
		else if (ret == FAIL)
		{
			error("recv_from_rmac error");
   		    if ( sb_run_tcp_send(
					sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) 
		    	error("cannot send RET"); 
			sb_run_tcp_close(sockfd); 
			continue;
		}

        if ( indexer_shared->last_indexed_docid+1 != docid ) {
			error("last_indexed_docid[%d]+1 != docid[%d]", indexer_shared->last_indexed_docid, docid);
		}

		if ( indexer_shared->last_indexed_docid >= docid ) {
			IDXLOG_ERROR("last_indexed_docid[%d] is greater than docid[%d]", indexer_shared->last_indexed_docid, docid);
		    SEND_RET_AND_CLOSE(ret);
			continue;
		}
							
		// 형태소 분석된 결과를 mem_index에 저장
		ret = save_to_indexdb( indexdb, word_db, docid, wordhits_storage, max_word_hit, data, size );

		if (ret != SUCCESS) {
			IDXLOG_ERROR("save_to_indexdb() error, docid[%d]", docid);
			ADD_AND_SEND_AND_CLOSE(ret);
			continue;
		}
		else { /* SUCCESS */
			/* every saving time, word db is also saved */
//			sb_run_sync_word_db(&gWordDB);  // lexicon 전체가 mmap을 쓰기 때문에 일단은..
			IDXLOG_INFO("docid[%d] indexed", docid);
			add_result_document(docid, SUCCESS);
		    SEND_RET_AND_CLOSE(ret);
		}

		// 진행상황 기록
		last_registered_docid = last_registered_did(cdm_db);
		indexer_shared->last_indexed_docid = docid;

		// 가끔씩 진행상황 출력
		gettimeofday( &end_time, NULL );
		index_time = timediff( &end_time, &start_time );
		if ( index_time > 10.0 ) { // 10초에 한 번씩 출력
			setproctitle("softbotd: %s(%u/%u indexed.[avg idx spd:%.2f doc/s])",
					__FILE__, indexer_shared->last_indexed_docid,
					last_registered_docid, (double)indexed_num/index_time);

			indexed_num = 0;
		}

		// 더이상 색인할 데이타가 없는지 살펴본다. idle 출력하는 거 하나 땜에...
		if (last_registered_docid != indexer_shared->last_indexed_docid) continue;

		// 잠깐 쉬어보고 진짜로 없는지...
		gettimeofday( &end_time, NULL );
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		sleep( 1 );

		last_registered_docid = last_registered_did(cdm_db);
		if (last_registered_docid != indexer_shared->last_indexed_docid) continue;

		index_time = timediff( &end_time, &start_time );

		if ( index_time == 0.0 || indexed_num == 0 ) {
			setproctitle("softbotd %s(%u indexed, now  idle [no spd]",
					__FILE__, indexer_shared->last_indexed_docid);
		}
		else {
			setproctitle("softbotd: %s(%u indexed, now idle [avg idx spd:%.2f doc/s])",
   			        __FILE__, indexer_shared->last_indexed_docid, (double)indexed_num/index_time);
			indexed_num = 0;
		}
	} /* endless while loop */

	if ( !b_use_cdm ) sb_run_cdm_close( cdm_db );
	sb_run_close_word_db( word_db ); //XXX: indexer should do this?
	sb_run_indexdb_close( indexdb );

	close(mErrorFileFd);
	close(mDocFileFd);
	fclose(idxlog_fp);
	
	free_wordhits_storage();
	if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
		slot->state = SLOT_FINISH;
	else
		slot->state = SLOT_RESTART;

	return 0;

error_return:
	if ( cdm_db ) sb_run_cdm_close( cdm_db );
	if ( word_db ) sb_run_close_word_db( word_db );
	if ( indexdb ) sb_run_indexdb_close( indexdb );
	if ( idxlog_fp ) fclose(idxlog_fp);
	free_wordhits_storage();
	slot->state = SLOT_FINISH;
	return -1;
}

static int test_indexer_main(slot_t *slot)
{
	int ret = 0;

	/* socket */
	struct sockaddr remote_addr;
	int sockfd, listenfd;
	socklen_t len;

	/* data */
	void *data = NULL; int size;
	uint32_t last_registered_docid = 0, docid;

	/* cdm db */
	cdm_db_t* cdm_db = NULL;
	int b_use_cdm;

	/* 통계 */
	int indexed_num;
	double index_time;
	struct timeval start_time, end_time;

	b_use_cdm = ( find_module("mod_cdm.c") != NULL );
	if ( b_use_cdm ) {
		if ( sb_run_server_canneddoc_init() != SUCCESS ) {
			error("cdm open failed");
			return FAIL;
		}
	}
	else {
		if ( sb_run_cdm_open( &cdm_db, mCdmSet ) != SUCCESS ) {
			error("cdm2 open failed");
			return FAIL;
		}
	}

	/* must be called after forking because it allocates large junk of memory */
	open_error_documents_file();
	open_test_data_file();

	if (sb_run_tcp_local_bind_listen(mSocketFile, BACKLOG, &listenfd) != SUCCESS) {
   		error("tcp_local_bind_listen: %s", strerror(errno));
		if ( !b_use_cdm ) {
			sb_run_cdm_close( cdm_db );
		}
   		return FAIL;
    }
   	setproctitle("softbotd: mod_daemon_indexer2.c (listening[%s])",mSocketFile);

	indexed_num = 0;
	
	while (1) { /* endless while loop */
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			break;

		len = sizeof(remote_addr);
		ret = sb_run_tcp_select_accept(listenfd, &sockfd, &remote_addr, &len);
		if (ret != SUCCESS) break;

		if ( indexed_num == 0 )
			gettimeofday( &start_time, NULL );

		indexed_num++;
		
		// rmac으로 부터 형태소 분석된 데이타를 받아온다.
		ret = recv_from_rmac(sockfd, &data, &docid, &size);
		if (ret == SKIP_DOCUMENT) 
		{
			error("document skip, docid = %d", docid);
			ADD_AND_SEND_AND_CLOSE(ret);
			continue;
		}
		// FAIL일 경우는 tcp 통신 에러로 다시 시도를 한다.
		else if (ret == FAIL)
		{
			error("recv_from_rmac error");
   		    if ( sb_run_tcp_send(
					sockfd, &ret, sizeof(int), sb_run_tcp_server_timeout()) != SUCCESS ) 
		    	error("cannot send RET"); 
			sb_run_tcp_close(sockfd); 
			continue;
		}

        if ( indexer_shared->last_indexed_docid+1 != docid ) {
			error("last_indexed_docid[%d]+1 != docid[%d]", indexer_shared->last_indexed_docid, docid);
		}

		if ( indexer_shared->last_indexed_docid >= docid ) {
			error("last_indexed_docid[%d] is greater than docid[%d]", indexer_shared->last_indexed_docid, docid);
		    SEND_RET_AND_CLOSE(ret);
			continue;
		}
							
		add_result_document(docid, SUCCESS);
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

		// 진행상황 기록
		last_registered_docid = last_registered_did(cdm_db);
		indexer_shared->last_indexed_docid = docid;

		// 가끔씩 진행상황 출력
		gettimeofday( &end_time, NULL );
		index_time = timediff( &end_time, &start_time );
		if ( index_time > 10.0 ) { // 10초에 한 번씩 출력
			setproctitle("softbotd: %s(%u/%u indexed.[avg idx spd:%.2f doc/s])",
					__FILE__, indexer_shared->last_indexed_docid,
					last_registered_docid, (double)indexed_num/index_time);

			indexed_num = 0;
		}

		// 더이상 색인할 데이타가 없는지 살펴본다. idle 출력하는 거 하나 땜에...
		if (last_registered_docid != indexer_shared->last_indexed_docid) continue;

		// 잠깐 쉬어보고 진짜로 없는지...
		gettimeofday( &end_time, NULL );
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		sleep( 5 );

		last_registered_docid = last_registered_did(cdm_db);
		if (last_registered_docid != indexer_shared->last_indexed_docid) continue;

		index_time = timediff( &end_time, &start_time );

		if ( index_time == 0.0 || indexed_num == 0 ) {
			setproctitle("softbotd %s(%u indexed, now  idle [no spd]",
					__FILE__, indexer_shared->last_indexed_docid);
		}
		else {
			setproctitle("softbotd: %s(%u indexed, now idle [avg idx spd:%.2f doc/s])",
   			        __FILE__, indexer_shared->last_indexed_docid, (double)indexed_num/index_time);
			indexed_num = 0;
		}

	} /* endless while loop */

	sb_run_cdm_close( cdm_db );

	close(mErrorFileFd);
	close(mDocFileFd);
	close(mTestFileFd);
	
	slot->state = SLOT_FINISH;
	return 0;
}

static int module_main(slot_t *slot) {
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	sb_run_init_scoreboard(scoreboard);
	sb_run_spawn_processes(scoreboard,"indexer process",indexer_main);
	scoreboard->period = MONITORING_PERIOD;
	sb_run_monitor_processes(scoreboard);

	return 0;
}

static int test_main(slot_t *slot) {
    sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

    sb_run_init_scoreboard(scoreboard);
    sb_run_spawn_processes(scoreboard,"indexer process",test_indexer_main);
    scoreboard->period = MONITORING_PERIOD;
    sb_run_monitor_processes(scoreboard);

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

	/* some information about some data structure */
	info("sizeof(standard_hit_t) = %d", sizeof(standard_hit_t));
	info("sizeof(hit_t) = %d", sizeof(hit_t));
	info("sizeof(doc_hit_t) = %d", sizeof(doc_hit_t));
	info("STD_HITS_LEN = %d", STD_HITS_LEN);

	return SUCCESS;
}

int save_to_indexdb(index_db_t* indexdb, word_db_t* word_db,
		int docid, word_hit_t* wordhits, int wordhit_len, void* data, int size)
{
	int ret;
	int hit_count, count = 0, same_count;

	static doc_hit_t *dochits = NULL;
	static int dochits_size = 0;
	int dochits_count; // fill_dochit()의 결과
#define MAX_DOCHITS_SIZE (1024*1024)

	ret = sb_run_index_each_doc(word_db, docid, wordhits, wordhit_len, &hit_count, data, size);
	if (ret != SUCCESS) return ret;

	// wordid순 정렬. stable해야 한다. 
	// 즉, 동일한 word에 대해 앞선 field에 나온 word가 우선
	mergesort(wordhits,(size_t)hit_count, sizeof(word_hit_t), cmp_word_hit);

	while ( count < hit_count ) {
		same_count = count_successive_wordid_same_to_first_in_wordhit(
				wordhits, hit_count - count );
		count += same_count;

		// dochits memory 재조정
		if ( dochits_size == 0 || dochits_size > MAX_DOCHITS_SIZE
				|| dochits_size <= same_count ) {
			if ( dochits != NULL ) sb_free( dochits );

			dochits = (doc_hit_t*) sb_malloc( sizeof(doc_hit_t) * same_count );
			dochits_size = same_count;

			if ( dochits == NULL ) {
				dochits_size = 0;
				error( "cannot allocate memory for dochits(%d)", same_count );
				return FAIL;
			}
		}

		// word도 dochits_count(max:100) 보다 많은 건 버린다.
		dochits_count = fill_dochit( dochits, 100, docid, wordhits, same_count );
		sb_assert( dochits_count >= 1 && dochits_count <= dochits_size );

		if ( sb_run_indexdb_append( indexdb, wordhits->wordid,
				sizeof(doc_hit_t)*dochits_count, dochits ) == FAIL ) {
			error("indexdb_append failed. wordid[%d], dochits_count[%d]", wordhits->wordid, dochits_count);
			return FAIL;
		}

		wordhits += same_count;
	} // while ( count < hit_count )

	return SUCCESS;
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
static void set_log_dir(configValue v)
{

	sz_strncpy(mLogDir, v.argument[0], STRING_SIZE);
	sz_snprintf(mDocumentFile, STRING_SIZE,
				"%s%s", mLogDir, "/index.documents");
	sz_snprintf(mErrorDocumentFile, STRING_SIZE, 
				"%s%s", mLogDir, "/error.documents");
}

static void set_socket_file(configValue v)
{
	sz_strncpy(mSocketFile, v.argument[0], SHORT_STRING_SIZE);
}

static void set_max_word_hit(configValue v)
{
	max_word_hit = atoi( v.argument[0] );
}

static void set_cdm_set(configValue v)
{
	mCdmSet = atoi( v.argument[0] );
}

static void set_indexdb_set(configValue v)
{
	mIndexDbSet = atoi( v.argument[0] );
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
	CONFIG_GET("LogDir",set_log_dir,1, \
			"indexer log path (e.g: LogDir dat/indexer)"),
	CONFIG_GET("MaxWordHit",set_max_word_hit,1, \
			"maximum word hit count per document (e.g: MaxWordHit 400000)"),
	CONFIG_GET("CdmSet",set_cdm_set,1,"select CDM db set"),
	CONFIG_GET("IndexDbSet",set_indexdb_set,1,"<e.g: IndexDbSet 1>"),
	CONFIG_GET("WordDbSet",set_word_db_set,1,"<e.g: WordDbSet 1>"),
	CONFIG_GET("SharedFile",set_shared_file,1,"(e.g: SharedFile dat/indexer/indexer.shared)"),
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

module daemon_indexer2_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,    				/* registry */
	init,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};

module daemon_indexer2_test_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,   				/* registry */
	init,					/* initialize function of module */
	test_main,				/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};
