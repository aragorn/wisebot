/* $Id$ */
#include "common_core.h"
#include "scoreboard.h"
#include "memory.h"
#include "ipc.h"
#include "setproctitle.h"

#include "mod_api/cdm.h"
#include "mod_api/cdm2.h"
#include "mod_api/indexer.h"
#include "mod_api/tcp.h"
#include "mod_api/rmas.h"
#include "mod_api/protocol4.h"

#include "mod_api/http_client.h"
#include "mod_httpd/http_util.h"
#include "mod_httpd_handler/handler_util.h"

#include "mod_api/xmlparser.h"

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> // sleep
#include <time.h>

#define MAX_SERVERS			16
#define MAX_PROCESSES		64
#define POLLING_WAIT		10         // sec
#define RETRY_WAIT          5          // sec
#define MONITORING_PERIOD	2

#define NODOCUMENT			0xffffffff // document가 아닌 docid

// slot->userptr에 이 값을 저장한다.
#define INDEXER_WAITING     ((void*)1)
#define NOT_INDEXER_WAITING ((void*)2)
#define DOCID_NOTINIT       ((void*)3) // 아직 slot->desc가 초기화 안된 상태

#define INDEXER_WAIT_TIME   2000000    // usec
#define INDEXER_SIGNAL      SIGPIPE

typedef struct {
    int id;         /* field id */
    char name[SHORT_STRING_SIZE];
    int index;      /* 1 for yes, 0 for no */
    int indexer_morpid;
    int qpp_morpid;
    int type;       // enum field_type
} field_info_t;

typedef struct {
	char address[STRING_SIZE];
	char port[STRING_SIZE];
	http_client_t* http_client;
} server_t;

typedef struct {
	time_t error_time;
	int    use_cnt;
} rmas_state_t;

enum rma_protocols {
	PROT_SOFTBOT4 = 0,
	PROT_HTTP,
	PROT_LOCAL,
	PROT_UNKNOWN
};
static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_PROCESSES) };

static server_t rmas_addr[MAX_SERVERS];
static int      num_of_rmas = 0;
static char*    meta_data = NULL;
static int      meta_data_size = 0; // buffer length
static int      needed_processes=1;
static int		rma_protocol = PROT_SOFTBOT4;
static int      rmas_retry=10; // 0이면 무조건 계속 시도
static int      max_requests = 10000;

static field_info_t field_info[MAX_EXT_FIELD];

/////////////////////////////////////////////////
// registry를 통해 공유된다.
REGISTRY uint32_t     *last_fetched_docid;
REGISTRY rmas_state_t *rmas_state_table;
REGISTRY int          *last_used_rmas;

/************************************************
 * rmac2에서 사용할 semaphore
 * id 하나에 semaphore는 여러개 생성할 수 있다.
 *
 * 0: rmac lock
 * 1: server table lock
 ************************************************/
#define SEM_CNT           2

#define RMAC_LOCK         0
#define RMAS_STATE_LOCK   1

static int rmac_semid;
static int b_use_old_cdm; // 1이면 mod_cdm사용. 아니면 cdm2 api사용
// for cdm2 api
static int cdm_set = -1;
static cdm_db_t* cdm_db;

/****************************************************************************/

static uint32_t get_docid(slot_t *slot);
static void     set_docid(slot_t *slot, uint32_t docid);
static int      set_nodoc(slot_t *slot);
static int      set_state(slot_t *slot, int state);
static void     set_title(char *msg, uint32_t docid);
static int      get_docid_to_index(slot_t *slot, int *docid);
static void     init_scoreboard();

static slot_t*  get_minimum_docid_slot();
static int      wait_until_minimum_docid(slot_t *slot);
static int      signal_if_minimum_is_wait();
static int      morphological_analyze_softbot4(uint32_t docid, void *pCdmData, long cdmLength,
									  void **pRmasData, long *rmasLength);
static int      morphological_analyze_http(uint32_t docid, void *pCdmData, long cdmLength,
									  void **pRmasData, long *rmasLength);
static int      morphological_analyze_local(uint32_t docid, void *pCdmData, long cdmLength,
									  void **pRmasData, long *rmasLength);
static int      send_to_indexer(uint32_t docid, int is_normal_doc, void *pRmasData, long rmasLength);

/****************************************************************************/

static int find_a_server_to_connect();
static void mark_rmas_error(int server_id);

/****************************************************************************/
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


/****************************************************************************/
#define CHECK_SHUTDOWN() \
	if (scoreboard->shutdown || scoreboard->graceful_shutdown) { \
		info("shutting down slot[%d]", slot->id); \
		slot->state = SLOT_FINISH; \
		exit_status = EXIT_SUCCESS; \
		goto END_LOOP; \
	} 

#define CHECK_SHUTDOWN_AND_RET(msg) \
	CHECK_SHUTDOWN() \
	if ( ret != SUCCESS ) { \
		error( "slot[%d], docid[%u]: %s [%d]", slot->id, docid_to_index, msg, ret ); \
		slot->state = SLOT_RESTART; /* restart하지 않으면 slot->docid 관리가 안된다. */ \
		exit_status = EXIT_FAILURE; \
		goto END_LOOP; \
	}

// 현재 process가 kill signal을 받으면 현재 작업중인 문서는 잃어버리게 되어 있다.
// monitor가 다시 살려내더라도 해당 문서는 error document로 기록될 것이다.
// 살려내지 않으면....
// slot->desc에 현재 작업중인 문서번호가 계속 남기 때문에 다른 process가
// indexer로 문서를 보낼 수가 없게 될 것이다.
// 하지만 전체 shutdown일 경우는 괜찮다. 엔진이 다시 시작되면 index 성공한 이후부터 할 것이므로...
static int process_main (slot_t *slot)
{
	int i, ret;
	int docid_to_index;
	int is_normal_doc;
	void *pCdmData = NULL, *pRmasData = NULL;
	long cdmLength = 0, rmasLength = 0;
	char *err_str = NULL;
	int my_max_requests;
	int exit_status = EXIT_SUCCESS;

	/* rmas가 없으면 할 일도 없다. */
	if ( rma_protocol != PROT_LOCAL &&  num_of_rmas == 0 ) {
		error("No rma server address. At least one is required.");
		slot->state = SLOT_FINISH;
		return 0;
	}

	b_use_old_cdm = ( find_module("mod_cdm.c") != NULL );

	if ( b_use_old_cdm ) {
		ret = sb_run_server_canneddoc_init();
		if ( ret != SUCCESS ) {
			error( "cdm module init failed" );
			return 1;
		}
	}
	else {
		ret = sb_run_cdm_open( &cdm_db, cdm_set );
		if ( ret != SUCCESS ) {
			error( "cdm2 module open failed" );
			return 1;
		}
	}

	if ( slot->userptr == DOCID_NOTINIT ) {
		set_docid( slot, NODOCUMENT );
		docid_to_index = NODOCUMENT;
		slot->userptr = NOT_INDEXER_WAITING;
	}
	else {
		docid_to_index = get_docid( slot );
	}

	pCdmData = sb_malloc(DOCUMENT_SIZE);

    my_max_requests = max_requests + (slot->id * 1021);
	for (i = 0; i < my_max_requests; i++) {
		CHECK_SHUTDOWN()

		// NODOCUMENT가 아니면 전에 남긴 찌꺼기
		if ( docid_to_index == NODOCUMENT ) is_normal_doc = TRUE;
		else {
			warn( "slot[%d]->docid is not NODOCUMENT: %s", slot->id, slot->desc );
			is_normal_doc = FALSE;
		}

		// 색인할 문서가 있는지 찾는다.
		if ( is_normal_doc == TRUE ) {
			set_state( slot, SLOT_WAIT );
			set_title( "polling document from cdm", *last_fetched_docid+1 );

			ret = get_docid_to_index( slot, &docid_to_index );
			CHECK_SHUTDOWN_AND_RET( "get_docid_to_index() failed" )

			if ( docid_to_index < 0 ) {
				sleep( POLLING_WAIT );
				continue;
			}
			info("slot[%d]: rmac is indexing.... docid[%u]", slot->id, docid_to_index);
		}
		set_state( slot, SLOT_PROCESS );

		// CDM 에서 문서 가져오기
		if ( is_normal_doc == TRUE ) {
			set_title( "get document from cdm", docid_to_index );

			if ( b_use_old_cdm ) {
				cdmLength = sb_run_server_canneddoc_get_as_pointer( docid_to_index, pCdmData, DOCUMENT_SIZE );
				if ( cdmLength <= 0 ) {
					if ( cdmLength == CDM_NOT_EXIST ) err_str = "CDM_NOT_EXIST";
					else if (cdmLength == CDM_DELETED ) err_str = "CDM_DELETED";
					else err_str = NULL;
				}
			}
			else { // use cdm2 api
				cdmLength = sb_run_cdm_get_xmldoc(cdm_db, docid_to_index, pCdmData, DOCUMENT_SIZE);
				if ( cdmLength < 0 ) {
					if ( cdmLength ==  CDM2_GET_INVALID_DOCID ) err_str = "CDM2_GET_INVALID_DOCID";
					else if ( cdmLength == CDM_DELETED ) err_str = "CDM_DELETED";
					else err_str = NULL;
				}
			}

			if ( cdmLength < 0 ) {
				if ( err_str == NULL )
					error( "get_as_point() returned error [%d], docid[%u]",
							(int)cdmLength, docid_to_index );
				else error( "get_as_point() returned error [%s], docid[%u]",
							err_str, docid_to_index );

				is_normal_doc = FALSE;
			}
		}

		CHECK_SHUTDOWN()

		// RMAS 에서 형태소분석
		if ( is_normal_doc == TRUE ) {
			set_title( "analyze document with rmas", docid_to_index );

			switch(rma_protocol) {
				case PROT_LOCAL:
					ret = morphological_analyze_local(docid_to_index, pCdmData, cdmLength, &pRmasData, &rmasLength );
					break;
				case PROT_HTTP:
					ret = morphological_analyze_http(docid_to_index, pCdmData, cdmLength, &pRmasData, &rmasLength );
					break;
				case PROT_SOFTBOT4:
				default:
					ret = morphological_analyze_softbot4(docid_to_index, pCdmData, cdmLength, &pRmasData, &rmasLength );
					break;
			}

			if ( ret != SUCCESS ) {
				error( "morphological_analyze returned error [%d], docid[%u]", ret, docid_to_index );
				is_normal_doc = FALSE;
			}
		}

		CHECK_SHUTDOWN()

		// index 차례 기다림
		set_title( "waiting my turn to index", docid_to_index );
		set_state( slot, SLOT_WAIT );
		ret = wait_until_minimum_docid( slot );
		CHECK_SHUTDOWN_AND_RET("wait_until_minimum_docid() failed")

		// indexer로 보냄
		set_title( "send to indexer", docid_to_index );
		set_state( slot, SLOT_PROCESS );

		ret = send_to_indexer(docid_to_index, is_normal_doc, pRmasData, rmasLength);
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		if ( ret != SUCCESS ) { // indexer가 포기한 문서다
			error( "slot[%d]: indexer return [%d], docid[%d]", slot->id, ret, docid_to_index );
		}

		ret = set_nodoc( slot );
		CHECK_SHUTDOWN_AND_RET("set_nodoc() failed")
		set_title( "send to indexer completed", docid_to_index );

		// 다른 rmac을 깨운다.
		ret = signal_if_minimum_is_wait();
		CHECK_SHUTDOWN_AND_RET("signal_if_minimum_is_wait() failed")

		// next init
		docid_to_index = NODOCUMENT;
		if ( pRmasData != NULL ) {
			sb_free( pRmasData );
			pRmasData = NULL;
			rmasLength = 0;
		}
	} // for (i = 0; i < my_max_requests; i++) {
	
	/* process_main()이 종료되는 경우는 크게 3가지이다.
	 *
	 * 1) my_max_requests를 채워서 for loop를 빠져나온 경우에는 
	 *    slot->state = SLOT_RESTART가 된 후 EXIT_SUCCESS를 return해야 한다.
	 * 2) CHECK_SHUTDOWN() 에서 signal을 감지한 경우에는
	 *    slot->state = SLOT_FINISH 가 된 후 EXIT_SUCCESS를 return해야 한다.
	 * 3) CHECK_SHUTDOWN_AND_RET() 에서 오류가 발생한 경우에는
	 *    slot->state = SLOT_RESTART가 된 후 EXIT_FAILURE를 return해야 한다.
	 */

	slot->state = SLOT_RESTART; /* 위의 1) 경우이다. */

END_LOOP:

	if ( !b_use_old_cdm ) {
		ret = sb_run_cdm_close( cdm_db );
		if ( ret != SUCCESS )
			error("cdm db close failed");
	}

	if ( pCdmData != NULL ) sb_free( pCdmData );
	if ( pRmasData != NULL ) sb_free( pRmasData );

	return exit_status;

} // process_main()

#define RMAS_ERROR_WAIT 10
// 숫자가 중요하지 않으므로 (대강만 맞아도 된다?) lock이 실패해도 무시...
static int find_a_server_to_connect()
{
	int start_index, i;
	time_t current_time = time( NULL );

	if ( num_of_rmas == 0 ) {
		crit( "no rmas server" );
		return -1;
	}

	acquire_lockn( rmac_semid, RMAS_STATE_LOCK );
	if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) return -1;

	start_index = ( *last_used_rmas + 1 ) % num_of_rmas;
	i = start_index;

	do {
		// 에러난지 얼마 안됐으면 안쓴다...
		if ( current_time - rmas_state_table[i].error_time > RMAS_ERROR_WAIT )
			break;

		i = ( i + 1 ) % num_of_rmas;

		// 모든 rmas가 error 상태면 이만큼은 쉬어야 한다.
		if ( i == start_index ) {
			release_lockn( rmac_semid, RMAS_STATE_LOCK );
			warn( "all rmas returned error recently. sleep %d seconds", RMAS_ERROR_WAIT );

			sleep( RMAS_ERROR_WAIT );
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) return -1;

			acquire_lockn( rmac_semid, RMAS_STATE_LOCK );
			if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) return -1;

			current_time = time( NULL );
		}
	} while ( 1 );

	rmas_state_table[i].use_cnt++;
	*last_used_rmas = i;

	release_lockn( rmac_semid, RMAS_STATE_LOCK );

	return i;
}

static void mark_rmas_error( int server_id )
{
	acquire_lockn(rmac_semid, RMAS_STATE_LOCK);
	rmas_state_table[server_id].error_time = time( NULL );
	release_lockn(rmac_semid, RMAS_STATE_LOCK);
}

static int module_main (slot_t *slot)
{
	/* set signal handler */
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);	

	debug("mod_register.c: module_main() init");

	*last_fetched_docid = sb_run_last_indexed_did();
	*last_used_rmas     = num_of_rmas;

	/* set number of thread to spawn */
	scoreboard->size = needed_processes;

	/* set scoreboard properly */
	sb_run_init_scoreboard(scoreboard);
	init_scoreboard();

	/* spawn slave process */
	sb_run_spawn_processes(scoreboard, "rmac process", process_main);

	/* monitering slave thread */
	scoreboard->period = MONITORING_PERIOD;
	sb_run_monitor_processes(scoreboard);

	debug("monitor_processes returned");

	return 0;
}

/****************************************************************************/
// get_docid(), set_docid(), set_nodoc(), set_state(), set_title()
// get_docid_to_index(), init_docid(),
// get_minimum_docid_slot(), wait_until_minimum_docid(), signal_if_minimum_is_wait()
// morphological_analyze(), send_to_indexer(),

/***************************************************
 * slot에 docid를 어떻게 저장할 건지 잘 생각해보자
 *
 * slot->desc format:
 *   when processing - "%d: processing docid"
 *   when idle       - "idle"
 ***************************************************/

// RMAC_LOCK을 걸고 들어와야 한다.
// 자기 자신의 docid를 조사하는 경우에는 lock 없어도 된다. set은 자신만 하니까...
// slot이 CLOSE 상태면 no document로 간주한다.
// 성공이면 docid, 실패면 NODOCUMENT
static uint32_t get_docid(slot_t *slot)
{
	char *docid_end;
	char slot_desc[STRING_SIZE];
	int docid;

	// 나중에 SLOT_CLOSE가 구현되면...
	// if ( slot->state == SLOT_CLOSE ) return NODOCUMENT;

	if ( strcmp( slot->desc, "idle" ) == 0 ) return NODOCUMENT;
	else {
		strncpy( slot_desc, slot->desc, STRING_SIZE );

		docid_end = strchr( slot_desc, ':' );
		if ( docid_end == NULL ) {
			warn( "invalid slot[%d]->desc [%s]", slot->id, slot_desc );
			return NODOCUMENT;
		}

		*docid_end = '\0';
		docid = atoi( slot_desc );

		if ( docid == 0 ) {
			warn( "invalid slot[%d]->desc [%s]", slot->id, slot_desc );
			return NODOCUMENT;
		}
		else return docid;
	}
}

// RMAC_LOCK을 걸고 들어와야 한다.
static void set_docid(slot_t *slot, uint32_t docid)
{
	if ( docid == NODOCUMENT ) {
		strncpy( slot->desc, "idle", STRING_SIZE );
		slot->last_desc_updated = time( NULL );
	}
	else {
		snprintf( slot->desc, STRING_SIZE, "%u: processing docid", docid );
		slot->last_desc_updated = time( NULL );
	}
}

// slot->docid = -1
static int set_nodoc(slot_t *slot)
{
	int ret;

	ret = acquire_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	set_docid(slot, NODOCUMENT);

	ret = release_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	return SUCCESS;
}

// 일단 state가 현재 중요하지 않기 때문에 lock은 안건다.
static int set_state(slot_t *slot, int state)
{
	slot->state = state;
	return SUCCESS;
}

static void set_title(char *msg, uint32_t docid)
{
	setproctitle( "softbotd: %s (docid[%u]. %s", __FILE__, docid, msg );
}

// index 작업할 document가 있는지 조사하고, 있으면 그 docid를 준다.
// 없으면 NODOCUMENT
static int get_docid_to_index(slot_t *slot, int *docid)
{
	uint32_t last_registered_id;
	int ret;
	*docid = NODOCUMENT;

	if ( b_use_old_cdm ) {
		last_registered_id = sb_run_server_canneddoc_last_registered_id();
	}
	else {
		last_registered_id = sb_run_cdm_last_docid(cdm_db);
	}

	ret = acquire_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	if ( last_registered_id > *last_fetched_docid ) {
		*docid = ++(*last_fetched_docid);
		set_docid(slot, *docid);
	}
	else *docid = NODOCUMENT;

	ret = release_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	return SUCCESS;
}

// scoreboard의 값을 초기화한다.
// 현재 set_docid는 효과가 없는 것 같다. spawn할 때 desc를 건드리니까...
static void init_scoreboard()
{
	int i;
	for ( i=1; i <= scoreboard->size; i++ ) {
		set_docid( &scoreboard->slot[i], NODOCUMENT );
		scoreboard->slot[i].userptr = DOCID_NOTINIT;
	}
}

// slot을 return 받은 시점에서 이미 slot은 minimum이 아닐 수도 있다.
// RMAC_LOCK은 밖에서 들고 와야 한다.
static slot_t* get_minimum_docid_slot()
{
	uint32_t min_docid = 0xffffffff;
	int i, docid;
	slot_t *min_slot = NULL;

	// scoreboard->size가 runtime에 변경된다고 하면 할 말 없음.
	for ( i=1; i <= scoreboard->size; i++ ) {
		docid = get_docid( &scoreboard->slot[i] );

		/* scoreboard->slot[i].state != SLOT_CLOSE */
		if ( docid!=NODOCUMENT && min_docid>docid ) {
			min_docid = docid;
			min_slot = &scoreboard->slot[i];
		}
	}

	return min_slot;
}

static int wait_until_minimum_docid(slot_t *slot)
{
	int ret;
	slot_t *min_slot;

	while (1) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			return FAIL; 

		ret = acquire_lockn(rmac_semid, RMAC_LOCK);
		if (ret != SUCCESS) return FAIL;

		min_slot = get_minimum_docid_slot();

		ret = release_lockn(rmac_semid, RMAC_LOCK);
		if (ret != SUCCESS) return FAIL;

		if ( min_slot == slot ) return SUCCESS;

		slot->userptr = INDEXER_WAITING;
		usleep( INDEXER_WAIT_TIME );
			
		// NOT_INDEXER_WAITING은 lock을 가지고 setting해야 한다.
		// signal_if_minimum_is_wait()의 동작과 관련있다.

		ret = acquire_lockn(rmac_semid, RMAC_LOCK); // 실패해도 good
		slot->userptr = NOT_INDEXER_WAITING;
		if ( ret != SUCCESS ) {
			warn( "lock acquire failed. but it's alright." );
			continue;
		}

		// release_lockn(), 2번 시도
		ret = release_lockn(rmac_semid, RMAC_LOCK);
		if ( ret == SUCCESS ) continue;

		ret = release_lockn(rmac_semid, RMAC_LOCK);
		if ( ret != SUCCESS ) return FAIL;
	}
}

static int signal_if_minimum_is_wait()
{
	int ret;
	slot_t *min_slot;

	ret = acquire_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	min_slot = get_minimum_docid_slot();

	if ( min_slot != NULL && min_slot->userptr == INDEXER_WAITING )
		kill( min_slot->pid, INDEXER_SIGNAL );

	ret = release_lockn(rmac_semid, RMAC_LOCK);
	if (ret != SUCCESS) return FAIL;

	return SUCCESS;
}

// pCdmData의 문서를 rmas에서 분석해 pRmasData에 저장한다.
static int morphological_analyze_softbot4(uint32_t docid, void *pCdmData, long cdmLength, void **pRmasData, long *rmasLength)
{
	int svrID, sockfd;
	int i, ret;

	for( i = 0; rmas_retry <= 0 || i < rmas_retry; ) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			return FAIL;

		svrID = find_a_server_to_connect();
		if ( svrID < 0 ) return FAIL;
		info("docid[%d] will be analyzed from rmas(%d)[%s:%s]",
				docid, svrID, rmas_addr[svrID].address, rmas_addr[svrID].port);

        ret = sb_run_tcp_connect(&sockfd , rmas_addr[svrID].address , rmas_addr[svrID].port);

        if (ret == FAIL) {
            error("cannot connect to server(%d)[%s:%s], docid[%u], (%d)%s",
                    svrID, rmas_addr[svrID].address , rmas_addr[svrID].port, docid,
					errno, strerror(errno));
			mark_rmas_error( svrID );

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				return FAIL;

			/* connect 에러의 경우, 재시도한다. */
            continue;
        }

        ret = sb_run_sb4c_remote_morphological_analyze_doc( sockfd , meta_data , pCdmData,
                cdmLength , pRmasData, rmasLength);
		if ( ret != SUCCESS ) {
			error("sb4c_remote_morphological_analyze_doc return error(%d) [%s], docid[%u]",
					errno, strerror(errno), docid );
        	sb_run_tcp_close(sockfd);

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				return FAIL;

			// 계속 다시 시도하지만 일정 회수를 넘으면 loop을 빠져서 FAIL
			// 대부분.. 그냥 rmas를 먼저 종료했기 때문일 것이다.
        	sb_run_tcp_close(sockfd);
        	mark_rmas_error( svrID );
			i++;
			continue;
		}

        sb_run_tcp_close(sockfd);

		return SUCCESS;
	} // for ( i )

	// retry 해도 안되면..
	error( "rmas analyzing failed. docid[%u]", docid );
	return FAIL;
}

// pCdmData의 문서를 rmas에서 분석해 pRmasData에 저장한다.
static int morphological_analyze_http(uint32_t docid, void *pCdmData, long cdmLength, void **pRmasData, long *rmasLength)
{
	int svrID, i;
    char escaped_metadata[LONG_STRING_SIZE];
	char request_uri[LONG_STRING_SIZE];
    http_client_t *client = NULL, *client_list[1] = { NULL };
    memfile* mem_body = NULL;

	if ( snprintf(request_uri, LONG_STRING_SIZE,
			"/document/ma?metadata=%s",
			escape_path(meta_data, escaped_metadata)) <= 0 ) {
		error("query is too long, max[%d].", LONG_STRING_SIZE);
		return FAIL;
	}

	for( i = 0; rmas_retry <= 0 || i < rmas_retry; ) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			return FAIL;

		svrID = find_a_server_to_connect();
		if ( svrID < 0 ) return FAIL;
		info("docid[%d] will be analyzed from rmas(%d)[%s:%s]",
				docid, svrID, rmas_addr[svrID].address, rmas_addr[svrID].port);

		if (rmas_addr[svrID].http_client  == NULL)
			rmas_addr[svrID].http_client =
				sb_run_http_client_new(rmas_addr[svrID].address, rmas_addr[svrID].port);

        client = rmas_addr[svrID].http_client;
		if (client == NULL) {
            error("cannot connect to server(%d)[%s:%s], docid[%u], (%d)%s",
                    svrID, rmas_addr[svrID].address , rmas_addr[svrID].port, docid,
					errno, strerror(errno));
			mark_rmas_error( svrID );

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				return FAIL;

			/* connect 에러의 경우, 재시도한다. */
            continue;
        } else {
            sb_run_http_client_reset(client);
        }

        client->http->request_http_ver = 1001;
        client->http->method = "POST";
        client->http->host = rmas_addr[svrID].address;
        client->http->path = request_uri;

        mem_body = memfile_new();
        if(mem_body == NULL) {
            error("memfile_new() failed: %s", strerror(errno));
            return FAIL;
        }

        memfile_append(mem_body, "body=", strlen("body=")); 
        memfile_append(mem_body, pCdmData, cdmLength); 
        http_setMessageBody(client->http, mem_body, "x-softbotd/binary", memfile_getSize(mem_body));

		/* Do not print http header. */
        /* http_print(client->http); */
        sb_run_http_client_makeRequest(client, NULL);
		client_list[0] = client;
		if ( sb_run_http_client_retrieve(1, client_list) == SUCCESS
		    && client->parsing_status.state == PARSING_COMPLETE ) {
            ; /* SUCCESS. do nothing. */
        	memfile_free(mem_body);
		} else { /* error */
        	memfile_free(mem_body);

			error("http rma request to server(%d)[%s:%s] failed, docid[%u]: %s(%d)",
					svrID, rmas_addr[svrID].address, rmas_addr[svrID].port,
					docid, strerror(errno), errno);
			if (client->parsing_status.state != PARSING_COMPLETE)
				error("failed to parse reponse from server");

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				return FAIL;

			// 계속 다시 시도하지만 일정 회수를 넘으면 loop을 빠져서 FAIL
			// 대부분.. 그냥 rmas를 먼저 종료했기 때문일 것이다.
        	mark_rmas_error( svrID );
			i++;
			continue;
		}

        mem_body = client->http->content_buf;
        memfile_setOffset(mem_body, 0);

        *rmasLength = memfile_getSize(mem_body);
        *pRmasData = sb_malloc(*rmasLength);

        memfile_read(mem_body, *pRmasData, *rmasLength);

        //sb_run_http_client_free(client);
        //rmas_addr[svrID].client = NULL;
		//
		return SUCCESS;
	} // for ( i )

	// retry 해도 안되면..
	error( "rmas analyzing failed. docid[%u]", docid );
	return FAIL;
}

/* rmac process가 곧바로 색인어추출한다. */
static int morphological_analyze_local(uint32_t docid, void *pCdmData, long cdmLength,
										void **pRmasData, long *rmasLength)
{
	void *parser = NULL;
	sb4_merge_buffer_t merge_buffer;
	char *buffer = NULL;
	int buffer_size = 0;
	int i;

	/* NOTE: This code is copied from sb4s_remote_morphological_analyze_doc()
	 * of mod_protocol4.c .
	 * 2006-10-07 김정겸
	 */

	parser = sb_run_xmlparser_parselen("CP949" , (char *)pCdmData, cdmLength);
	if (parser == NULL) { 
		error("cannot parse document. docid[%u]", docid);
		return FAIL;
	}

	merge_buffer.data = NULL;
	merge_buffer.data_size = 0;
	merge_buffer.allocated_size = 0;

	for( i = 0; i < MAX_EXT_FIELD; i++ ) {
		char xpath[STRING_SIZE] = "/Document/";
		int field_id, morpid, r;
		char *field_value; int field_length;
		void *output_buffer; int output_size;

		if (field_info[i].index == 0) continue;

		field_id = field_info[i].id;
		strncat(xpath, field_info[i].name, SHORT_STRING_SIZE);
		morpid = field_info[i].indexer_morpid;
		debug("path[%s] fieldname[%s] id[%d] morpid[%d]",
		      xpath, field_info[i].name, field_info[i].id, field_info[i].indexer_morpid);
		r = sb_run_xmlparser_retrieve_field(parser, xpath, &field_value, &field_length);
		if (r != SUCCESS) {
			notice("cannot retrieve field[%s]", xpath);
			continue;
		}

		if (field_length == 0) {
			continue;
		} else if (field_length >= buffer_size) {
			sb_free(buffer);
			buffer = sb_calloc(sizeof(char), field_length+1);
			if (buffer == NULL) {
				error("cannot allocate buffer of %d bytes", field_length+1);
				sb_run_xmlparser_free_parser(parser);
				return FAIL;
			}
			buffer_size = field_length + 1;
		}
		memcpy(buffer, field_value, field_length);
		buffer[field_length] = '\0';

		output_buffer = NULL;
		r = sb_run_rmas_morphological_analyzer(field_id, buffer, &output_buffer, 
				&output_size, morpid);
		if (r != SUCCESS) {
			warn("failed to rmas_morphological_analyzer() for field[%s] with morpid[%d]",
					field_info[i].name, morpid);
			sb_free(output_buffer);
			continue;
		}

		r = sb_run_rmas_merge_index_word_array(&merge_buffer, output_buffer, output_size);
		if (r != SUCCESS) {
			error("failed to rmas_merge_index_word_array() for field[%s] with morpid[%d]",
					field_info[i].name, morpid);
			sb_free(output_buffer);
			sb_free(merge_buffer.data);
			sb_run_xmlparser_free_parser(parser);
			return FAIL;
		}
		sb_free(output_buffer);

	} // for( i = 0; i < MAX_EXT_FIELD; i++ ) {

	sb_run_xmlparser_free_parser(parser); parser = NULL;

	*pRmasData = merge_buffer.data;
	*rmasLength = merge_buffer.data_size;

	return SUCCESS;
}

// pRmasData의 내용을 indexer로 보낸다.
#define MAGICNUMBER 12345
static int send_to_indexer(uint32_t docid, int is_normal_doc, void *pRmasData, long rmasLength)
{
	int ret, sockfd;
    char buf[12];
	const char *indexer_socket_file;

    if (is_normal_doc == TRUE && rmasLength <= 0) {
        error("invalid rmasLength: %ld", rmasLength);
        return FAIL;
    }

	indexer_socket_file = sb_run_get_indexer_socket_file();

    while(1)
    {
        if ( scoreboard->shutdown || scoreboard->graceful_shutdown) break;

        ret = sb_run_tcp_local_connect(&sockfd , (char*)indexer_socket_file);
        if (ret != SUCCESS)
        {
            error("sb_run_tcp_connect error, retry");
            sleep(RETRY_WAIT);
            continue;
        }

        if (is_normal_doc != TRUE) rmasLength = 0;

		/* buf = 4 + 4 + 4 = 12 bytes */
		*(int*)(buf) = MAGICNUMBER;
		*(uint32_t*)(buf+4) = docid;
		*(long*)(buf+8) = rmasLength;

        /* 1. send header */
        if ( sb_run_tcp_send(sockfd, buf, sizeof(buf), sb_run_tcp_client_timeout()) != SUCCESS ) {
            error("sockfd[%d] cannot send RMA_HEAD_DATA[size:%d]: %s",
					sockfd, sizeof(buf), strerror(errno));
            sb_run_tcp_close(sockfd);
            sleep(RETRY_WAIT);
            continue;
        }

        if ( is_normal_doc == TRUE )
        {
            /* 2. send data */
            if ( sb_run_tcp_send(sockfd, pRmasData, rmasLength, sb_run_tcp_client_timeout()) != SUCCESS ) {
            	error("sockfd[%d] cannot send RMA_SRC_DATA[size:%ld,%p]: %s",
					sockfd, rmasLength, pRmasData, strerror(errno));
                sb_run_tcp_close(sockfd);
                sleep(RETRY_WAIT);
                continue;
            }
        }   
        
        /* 3. recv ret */
        if ( sb_run_tcp_recv(sockfd, &ret, sizeof(int), sb_run_tcp_client_timeout()) != SUCCESS ) {
            error("cannot recv RECV_RET");
            sb_run_tcp_close(sockfd);
            sleep(RETRY_WAIT);
            continue; 
        }

        sb_run_tcp_close(sockfd);
        
        if (ret != 1) return ret;
        else return SUCCESS;
    }
	
	return FAIL; // not reachable
}

/****************************************************************************/
REGISTRY void init_last_fetched_docid(void *data)
{
	last_fetched_docid = data;
	*last_fetched_docid = 0;
}

REGISTRY void init_rmas_state_table(void *data)
{
	rmas_state_table = data;
	memset( rmas_state_table, 0, sizeof(rmas_state_t)*MAX_SERVERS );
}

REGISTRY void init_last_used_rmas(void *data)
{
	last_used_rmas = data;
	*last_used_rmas = 0;
}

static void set_ip_and_port(configValue v)
{
	char tmp[STRING_SIZE];
	char *ptmp;
	int i;
	strncpy(tmp, v.argument[0] , STRING_SIZE);

	for(i=0; tmp[i]!=':'; i++)
		rmas_addr[num_of_rmas].address[i] = tmp[i];
	
	rmas_addr[num_of_rmas].address[i] = '\0';

	ptmp = tmp + i + 1;

	for(i=0;i<STRING_SIZE && ptmp[i];i++)
		rmas_addr[num_of_rmas].port[i] = ptmp[i];

	num_of_rmas++;

	info("rmas[%d] added: ip[%s], port[%s]", num_of_rmas - 1, 
			rmas_addr[num_of_rmas - 1].address, rmas_addr[num_of_rmas - 1].port);
}

static void set_meta_data(configValue v)
{
	char buf[STRING_SIZE];
	int buflen, left, field_id;

	field_id = atoi(v.argument[0]);
	if (field_id < 0 || field_id >= MAX_EXT_FIELD) {
        error("Invalid field id(%s).", v.argument[0]);
		return;
	}

	field_info[field_id].id = field_id;
	strncpy(field_info[field_id].name,v.argument[1],SHORT_STRING_SIZE);

	if (strncasecmp("yes", v.argument[2], 4) != 0) return;
	else field_info[field_id].index = 1;

	if (field_id >= MAX_STD_FIELD ) {
		error("max indexable field_id is %d. field[%s, id:%s] would not be indexed.",
				MAX_STD_FIELD-1, v.argument[1], v.argument[0]);
		return;
	}

    field_info[field_id].indexer_morpid=atoi(v.argument[3]);
    field_info[field_id].qpp_morpid=atoi(v.argument[4]);

	buflen = sprintf(buf, "%s#%s:%s^", v.argument[1], v.argument[0], v.argument[3]);

	if ( meta_data == NULL ) {
		meta_data_size = STRING_SIZE;
		meta_data = (char*) sb_malloc(meta_data_size*sizeof(char));
		meta_data[0] = '\0';
	}
	left = meta_data_size - strlen(meta_data);
	if (left < buflen+1) {
		meta_data_size += STRING_SIZE;
		// mod_protocol4.c 의 한계다. 꼭 검사해야 한다.
		// sb4c_remote_morphological_analyze_doc() 참고
		if ( meta_data_size >= SB4_MAX_SEND_SIZE ) {
			error("too long meta_data. field[%s, id:%s] would not be indexed.",
					v.argument[1], v.argument[0]);
			meta_data_size -= STRING_SIZE;
			return;
		}
		meta_data = (char*) sb_realloc(meta_data, meta_data_size*sizeof(char));
		left += STRING_SIZE;
	}
	strncat(meta_data, buf, left);

	debug("meta data: %s", meta_data);
}

static int init(void)
{
	ipc_t rmac_lock;
    int ret=0;

    rmac_lock.type = IPC_TYPE_SEM;
    rmac_lock.pid  = SYS5_RMAC2;
    rmac_lock.pathname = NULL;

	ret = get_nsem(&rmac_lock, SEM_CNT);
	if ( ret != SUCCESS ) return FAIL;

	rmac_semid = rmac_lock.id;

    return SUCCESS;
}

static void set_processes_num(configValue v)
{
	needed_processes = atoi(v.argument[0]);
}

static void set_protocol(configValue v)
{
	if (strncasecmp(v.argument[0], "http", 5) == 0)
		rma_protocol = PROT_HTTP;
	else if (strncasecmp(v.argument[0], "local", 5) == 0)
		rma_protocol = PROT_LOCAL;
	else
		rma_protocol = PROT_SOFTBOT4;
}

static void set_rmas_retry(configValue v)
{
	rmas_retry = atoi(v.argument[0]);
}

static void set_cdm_set(configValue v)
{
	cdm_set = atoi(v.argument[0]);
}

static void set_max_requests_per_child(configValue v)
{
	max_requests = atoi(v.argument[0]);
}

static registry_t registry[] = {
	RUNTIME_REGISTRY("LAST_FETCHED_DOCID","last fetched document id",
					 sizeof(uint32_t), init_last_fetched_docid, NULL, NULL),
	RUNTIME_REGISTRY("RMAS_STATE_TABLE", "rmas state table",
					 sizeof(rmas_state_t)*MAX_SERVERS, init_rmas_state_table, NULL, NULL),
	RUNTIME_REGISTRY("LAST_USED_RMAS", "last used rmas number",
					 sizeof(int), init_last_used_rmas, NULL, NULL),
	NULL_REGISTRY
};

static config_t config[] = {
	CONFIG_GET("AddServer",	set_ip_and_port, 1, "rmas ip:port"),
	CONFIG_GET("Field",		set_meta_data , VAR_ARG ,
            "field and mophological analizer id :  ex) title:0^author:1^ "),
	CONFIG_GET("Threads",	set_processes_num, 1, "number of processes"), // 호환성땜에...
	CONFIG_GET("Processes",	set_processes_num, 1, "number of processes"),
	CONFIG_GET("Protocol",	set_protocol,	1, "softbot4, http or local"),
	CONFIG_GET("RmasRetry",	set_rmas_retry, 1, "retry count to rmas-analyze. 0 for infinite."),
	CONFIG_GET("CdmSet",	set_cdm_set, 1, "select CDM set"),
	CONFIG_GET("MaxRequests", set_max_requests_per_child, 1, "max requests for child's lifetime"),
	{NULL}
};

module rmac2_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	init,					/* initialize */
	module_main,			/* child_main */
	scoreboard,		/* scoreboard */
	NULL					/* register hook api */
};
