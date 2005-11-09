/* $Id$ */
#include "softbot.h"

#include "mod_mp/mod_mp.h"
#include "mod_api/cdm.h"
#include "mod_api/indexer.h"
#include "mod_api/tcp.h"
#include "mod_api/protocol4.h"
#include "mod_indexer/hit.h" // MAX_STD_FIELD

#define MAX_SERVERS			16
#define MAX_PROCESSES		64
#define POLLING_WAIT		10         // sec
#define RETRY_WAIT          5          // sec
#define MONITORING_PERIOD	2

#define NODOCUMENT			0xffffffff // document�� �ƴ� docid

// slot->userptr�� �� ���� �����Ѵ�.
#define INDEXER_WAITING     ((void*)1)
#define NOT_INDEXER_WAITING ((void*)2)
#define DOCID_NOTINIT       ((void*)3) // ���� slot->desc�� �ʱ�ȭ �ȵ� ����

#define INDEXER_WAIT_TIME   2000000    // usec
#define INDEXER_SIGNAL      SIGPIPE

typedef struct {
	char address[STRING_SIZE];
	char port[STRING_SIZE];
} server_t;

typedef struct {
	time_t error_time;
	int    use_cnt;
} rmas_state_t;

static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_PROCESSES) };

static server_t rmas_addr[MAX_SERVERS];
static int      num_of_rmas = 0;
static char*    meta_data = NULL;
static int      meta_data_size = 0; // buffer length
static int      needed_processes=1;
static int      rmas_retry=10; // 0�̸� ������ ��� �õ�

/////////////////////////////////////////////////
// registry�� ���� �����ȴ�.
REGISTRY uint32_t     *last_fetched_docid;
REGISTRY rmas_state_t *rmas_state_table;
REGISTRY int          *last_used_rmas;

/************************************************
 * rmac2���� ����� semaphore
 * id �ϳ��� semaphore�� ������ ������ �� �ִ�.
 *
 * 0: rmac lock
 * 1: server table lock
 ************************************************/
#define SEM_CNT           2

#define RMAC_LOCK         0
#define RMAS_STATE_LOCK   1

static int rmac_semid;

/****************************************************************************/

static uint32_t get_docid(slot_t *slot);
static void     set_docid(slot_t *slot, uint32_t docid);
static int      set_nodoc(slot_t *slot);
static int      set_state(slot_t *slot, int state);
static void     set_title(char *msg, uint32_t docid);
static int      get_docid_to_index(slot_t *slot, uint32_t *docid);
static void     init_scoreboard();

static slot_t*  get_minimum_docid_slot();
static int      wait_until_minimum_docid(slot_t *slot);
static int      signal_if_minimum_is_wait();
static int      morphological_analyze(uint32_t docid, void *pCdmData, long cdmLength,
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

	act.sa_flags = SA_RESTART;
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

	act.sa_flags = SA_RESTART;
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
		if ( pCdmData != NULL ) sb_free( pCdmData ); \
		if ( pRmasData != NULL ) sb_free( pRmasData ); \
		slot->state = SLOT_FINISH; \
		return EXIT_SUCCESS; \
	} 

#define CHECK_SHUTDOWN_AND_RET(msg) \
	CHECK_SHUTDOWN() \
	if ( ret != SUCCESS ) { \
		error( "slot[%d], docid[%u]: %s [%d]", slot->id, docid_to_index, msg, ret ); \
		if ( pCdmData != NULL ) sb_free( pCdmData ); \
		if ( pRmasData != NULL ) sb_free( pRmasData ); \
		slot->state = SLOT_RESTART; /* restart���� ������ slot->docid ������ �ȵȴ�. */ \
		return 1; \
	}

// ���� process�� kill signal�� ������ ���� �۾����� ������ �Ҿ������ �Ǿ� �ִ�.
// monitor�� �ٽ� ��������� �ش� ������ error document�� ��ϵ� ���̴�.
// ������� ������....
// slot->desc�� ���� �۾����� ������ȣ�� ��� ���� ������ �ٸ� process��
// indexer�� ������ ���� ���� ���� �� ���̴�.
// ������ ��ü shutdown�� ���� ������. ������ �ٽ� ���۵Ǹ� index ������ ���ĺ��� �� ���̹Ƿ�...
static int process_main (slot_t *slot)
{
	int ret;
	int docid_to_index;
	int is_normal_doc;
	void *pCdmData = NULL, *pRmasData = NULL;
	long cdmLength = 0, rmasLength = 0;
	char *err_str;

	// rmas�� ������ �� �ϵ� ����.
	if ( num_of_rmas == 0 ) {
		slot->state = SLOT_FINISH;
		return 0;
	}

	ret = sb_run_server_canneddoc_init();
	if ( ret != SUCCESS ) {
		error( "cdm module init failed" );
		return 1;
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

	while (1) {
		CHECK_SHUTDOWN()

		// NODOCUMENT�� �ƴϸ� ���� ���� ���
		if ( docid_to_index == NODOCUMENT ) is_normal_doc = TRUE;
		else {
			warn( "slot[%d]->docid is not NODOCUMENT: %s", slot->id, slot->desc );
			is_normal_doc = FALSE;
		}

		// ������ ������ �ִ��� ã�´�.
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

		// CDM ���� ���� ��������
		if ( is_normal_doc == TRUE ) {
			set_title( "get document from cdm", docid_to_index );

			cdmLength = sb_run_server_canneddoc_get_as_pointer( docid_to_index, pCdmData, DOCUMENT_SIZE );
			if ( cdmLength <= 0 ) {
				if ( cdmLength == CDM_NOT_EXIST ) err_str = "CDM_NOT_EXIST";
				else if (cdmLength == CDM_DELETED ) err_str = "CDM_DELETED";
				else err_str = NULL;

				if ( err_str == NULL )
					error( "get_as_point() returned error [%d], docid[%u]",
							(int)cdmLength, docid_to_index );
				else error( "get_as_point() returned error [%s], docid[%u]",
							err_str, docid_to_index );

				is_normal_doc = FALSE;
			}
		}

		CHECK_SHUTDOWN()

		// RMAS ���� ���¼Һм�
		if ( is_normal_doc == TRUE ) {
			set_title( "analyze document with rmas", docid_to_index );

			ret = morphological_analyze(
					docid_to_index, pCdmData, cdmLength, &pRmasData, &rmasLength );
			if ( ret != SUCCESS ) {
				error( "morphological_analyze returned error [%d], docid[%u]", ret, docid_to_index );
				is_normal_doc = FALSE;
			}
		}

		CHECK_SHUTDOWN()

		// index ���� ��ٸ�
		set_title( "waiting my turn to index", docid_to_index );
		set_state( slot, SLOT_WAIT );
		ret = wait_until_minimum_docid( slot );
		CHECK_SHUTDOWN_AND_RET("wait_until_minimum_docid() failed")

		// indexer�� ����
		set_title( "send to indexer", docid_to_index );
		set_state( slot, SLOT_PROCESS );

		ret = send_to_indexer(docid_to_index, is_normal_doc, pRmasData, rmasLength);
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		if ( ret != SUCCESS ) { // indexer�� ������ ������
			error( "slot[%d]: indexer return [%d], docid[%d]", slot->id, ret, docid_to_index );
		}

		ret = set_nodoc( slot );
		CHECK_SHUTDOWN_AND_RET("set_nodoc() failed")
		set_title( "send to indexer completed", docid_to_index );

		// �ٸ� rmac�� �����.
		ret = signal_if_minimum_is_wait();
		CHECK_SHUTDOWN_AND_RET("signal_if_minimum_is_wait() failed")

		// next init
		docid_to_index = NODOCUMENT;
		if ( pRmasData != NULL ) {
			sb_free( pRmasData );
			pRmasData = NULL;
			rmasLength = 0;
		}
	} // while (1)

	if ( pCdmData != NULL ) sb_free( pCdmData );
	if ( pRmasData != NULL ) sb_free( pRmasData );

	return EXIT_SUCCESS;
} // main()

#define RMAS_ERROR_WAIT 10
// ���ڰ� �߿����� �����Ƿ� (�밭�� �¾Ƶ� �ȴ�?) lock�� �����ص� ����...
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
		// �������� �� �ȵ����� �Ⱦ���...
		if ( current_time - rmas_state_table[i].error_time > RMAS_ERROR_WAIT )
			break;

		i = ( i + 1 ) % num_of_rmas;

		// ��� rmas�� error ���¸� �̸�ŭ�� ����� �Ѵ�.
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
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);	

	debug("mod_register.c: module_main() init");

	*last_fetched_docid = sb_run_last_indexed_did();
	*last_used_rmas     = num_of_rmas;

	/* set number of thread to spawn */
	scoreboard->size = needed_processes;

	/* set scoreboard properly */
	sb_init_scoreboard(scoreboard);
	init_scoreboard();

	/* spawn slave process */
	sb_spawn_processes(scoreboard, "rmac process", process_main);

	/* monitering slave thread */
	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	debug("monitor_processes returned");

	return 0;
}

/****************************************************************************/
// get_docid(), set_docid(), set_nodoc(), set_state(), set_title()
// get_docid_to_index(), init_docid(),
// get_minimum_docid_slot(), wait_until_minimum_docid(), signal_if_minimum_is_wait()
// morphological_analyze(), send_to_indexer(),

/***************************************************
 * slot�� docid�� ��� ������ ���� �� �����غ���
 *
 * slot->desc format:
 *   when processing - "%d: processing docid"
 *   when idle       - "idle"
 ***************************************************/

// RMAC_LOCK�� �ɰ� ���;� �Ѵ�.
// �ڱ� �ڽ��� docid�� �����ϴ� ��쿡�� lock ��� �ȴ�. set�� �ڽŸ� �ϴϱ�...
// slot�� CLOSE ���¸� no document�� �����Ѵ�.
// �����̸� docid, ���и� NODOCUMENT
static uint32_t get_docid(slot_t *slot)
{
	char *docid_end;
	char slot_desc[STRING_SIZE];
	int docid;

	// ���߿� SLOT_CLOSE�� �����Ǹ�...
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

// RMAC_LOCK�� �ɰ� ���;� �Ѵ�.
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

// �ϴ� state�� ���� �߿����� �ʱ� ������ lock�� �ȰǴ�.
static int set_state(slot_t *slot, int state)
{
	slot->state = state;
	return SUCCESS;
}

static void set_title(char *msg, uint32_t docid)
{
	setproctitle( "softbotd: %s (docid[%u]. %s", __FILE__, docid, msg );
}

// index �۾��� document�� �ִ��� �����ϰ�, ������ �� docid�� �ش�.
// ������ NODOCUMENT
static int get_docid_to_index(slot_t *slot, uint32_t *docid)
{
	uint32_t last_registered_id;
	int ret;
	*docid = NODOCUMENT;

	last_registered_id = sb_run_server_canneddoc_last_registered_id();

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

// scoreboard�� ���� �ʱ�ȭ�Ѵ�.
// ���� set_docid�� ȿ���� ���� �� ����. spawn�� �� desc�� �ǵ帮�ϱ�...
static void init_scoreboard()
{
	int i;
	for ( i=1; i <= scoreboard->size; i++ ) {
		set_docid( &scoreboard->slot[i], NODOCUMENT );
		scoreboard->slot[i].userptr = DOCID_NOTINIT;
	}
}

// slot�� return ���� �������� �̹� slot�� minimum�� �ƴ� ���� �ִ�.
// RMAC_LOCK�� �ۿ��� ��� �;� �Ѵ�.
static slot_t* get_minimum_docid_slot()
{
	uint32_t min_docid = 0xffffffff;
	int i, docid;
	slot_t *min_slot = NULL;

	// scoreboard->size�� runtime�� ����ȴٰ� �ϸ� �� �� ����.
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
			
		// NOT_INDEXER_WAITING�� lock�� ������ setting�ؾ� �Ѵ�.
		// signal_if_minimum_is_waiting()�� ���۰� �����ִ�.

		ret = acquire_lockn(rmac_semid, RMAC_LOCK); // �����ص� good
		slot->userptr = NOT_INDEXER_WAITING;
		if ( ret != SUCCESS ) {
			error( "lock acquire failed. but it's alright." );
			continue;
		}

		// release_lockn(), 2�� �õ�
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

// pCdmData�� ������ rmas���� �м��� pRmasData�� �����Ѵ�.
static int morphological_analyze(uint32_t docid, void *pCdmData, long cdmLength, void **pRmasData, long *rmasLength)
{
	int svrID, sockfd;
	int i, ret;

#define RMAS_RETRY 5

	for( i = 0; rmas_retry <= 0 || i < rmas_retry; ) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			return FAIL;

		svrID = find_a_server_to_connect();
		info("docid[%d] will be analyzed from rmas[%s:%s]", docid, rmas_addr[svrID].address,
				rmas_addr[svrID].port);
		if ( svrID < 0 ) return FAIL;

        ret = sb_run_tcp_connect(&sockfd , rmas_addr[svrID].address , rmas_addr[svrID].port);

        if (ret == FAIL) {
            error("cannot connect to server[%s:%s], docid[%u], (%d)%s",
                    rmas_addr[svrID].address , rmas_addr[svrID].port, docid,
					errno, strerror(errno));
			mark_rmas_error( svrID );

			if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
				return FAIL;

			/* connect ������ ���, ��õ��Ѵ�. */
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

			// ��� �ٽ� �õ������� ���� ȸ���� ������ loop�� ������ FAIL
			// ��κ�.. �׳� rmas�� ���� �����߱� ������ ���̴�.
        	sb_run_tcp_close(sockfd);
        	mark_rmas_error( svrID );
			i++;
			continue;
		}

        sb_run_tcp_close(sockfd);

		return SUCCESS;
	} // for ( i )

	// retry �ص� �ȵǸ�..
	error( "rmas analyzing failed. docid[%u]", docid );
	return FAIL;
}

// pRmasData�� ������ indexer�� ������.
#define MAGICNUMBER 12345
static int send_to_indexer(uint32_t docid, int is_normal_doc, void *pRmasData, long rmasLength)
{
	int ret, sockfd;
    char buf[12];
	const char *indexer_socket_file;

    if (is_normal_doc == TRUE && rmasLength <= 0) {
        error("wrong send_data_size : %ld", rmasLength);
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

		*(int*)(buf) = MAGICNUMBER;
		*(uint32_t*)(buf+4) = docid;
		*(long*)(buf+8) = rmasLength;

        /* 1. send header */
        if ( sb_run_tcp_send(sockfd, buf, sizeof(buf), sb_run_tcp_client_timeout()) != SUCCESS ) {
            error("cannot send RMA_HEAD_DATA");
            sb_run_tcp_close(sockfd);
            sleep(RETRY_WAIT);
            continue;
        }

        if ( is_normal_doc == TRUE )
        {
            /* 2. send data */
            if ( sb_run_tcp_send(sockfd, pRmasData, rmasLength, sb_run_tcp_client_timeout()) != SUCCESS ) {
                error("cannot send RMA_SRC_DATA");
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
	int buflen, left;

	if (strcmp("yes", v.argument[2]) != 0) return;
	buflen = sprintf(buf, "%s#%s:%s^", v.argument[1], v.argument[0], v.argument[3]);

	if ( atoi(v.argument[0]) >= MAX_STD_FIELD ) {
		error("max indexable fieldid is %d. field[%s, id:%s] would not be indexed.",
				MAX_STD_FIELD-1, v.argument[1], v.argument[0]);
		return;
	}

	if ( meta_data == NULL ) {
		meta_data_size = STRING_SIZE;
		meta_data = (char*) sb_malloc(meta_data_size*sizeof(char));
		meta_data[0] = '\0';
	}
	left = meta_data_size - strlen(meta_data);
	if (left < buflen+1) {
		meta_data_size += STRING_SIZE;
		// mod_protocol4.c �� �Ѱ��. �� �˻��ؾ� �Ѵ�.
		// sb4c_remote_morphological_analyze_doc() ����
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

	info("meta data: %s", meta_data);
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

static void set_rmas_retry(configValue v)
{
	rmas_retry = atoi(v.argument[0]);
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
	CONFIG_GET("AddServer", set_ip_and_port, 1,
			"rmas ip:port"),
	CONFIG_GET("Field", set_meta_data , VAR_ARG ,
            "field and mophological analizer id :  ex) title:0^author:1^ "),
	CONFIG_GET("Threads", set_processes_num, 1, "number of processes"), // ȣȯ������...
	CONFIG_GET("Processes", set_processes_num, 1, "number of processes"),
	CONFIG_GET("RmasRetry", set_rmas_retry, 1, "retry rmas analyze"),
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
