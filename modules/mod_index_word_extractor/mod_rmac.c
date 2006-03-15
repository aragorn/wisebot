/* $Id$ */
#include "softbot.h"

#include "mp_api.h"
#include "mod_api/mod_api.h"
#include "mod_api/spool.h"

#include "mod_api/index_word_extractor.h"

#define MAX_SERVERS							16
#define MAX_THREADS							64
#define WAIT_TIMEOUT						30
#define MONITORING_PERIOD					2
#define MAX_WAIT_REGISTER_PROCESS			10

typedef struct {
	char address[STRING_SIZE];
	char port[STRING_SIZE];
} server_t;

static int spool_queue_size = SPOOL_QUEUESIZE;
static int spool_mpool_size = SPOOL_MPOOLSIZE;

static spool_t *spl = NULL;
static char spoolpath[MAX_SPOOL_PATH_LEN] = "dat/indexer/indexer.spl";

static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };

static server_t rmas_addr[MAX_SERVERS];

static int num_of_rmas = 0;

static int rmas_state_table[MAX_SERVERS];

static pthread_mutex_t listmutex;
static pthread_mutex_t last_morph_anal_docid_mutex;

static char meta_data[STRING_SIZE];

static uint32_t *last_morph_anal_docid;
static uint32_t *last_piled_docid;

static int needed_threads=1;

static int spool_semid;

/****************************************************************************/

static int find_a_server_to_connect();
static void mark_server_state_table( int server_id , int value);

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
static int thread_main (slot_t *slot)
{
	int i=0, len;
	void *doc, *ptr;
	void *recv_data;
	long recv_data_size=0;
	int svrID, nret, sockfd;
	uint32_t docid = 0;
	registry_t *reg;
	int try = 0; 
	int timeout_try = 0;
	int error_num;

	reg = registry_get("LAST_INDEXER_PILED_DOCID");
	if (reg == NULL) {crit("cannot registry of LAST_INDEXER_PILED_DOCID"); return 1;}

	last_piled_docid = (uint32_t *)reg->data;
	while (1) {
		if (scoreboard->shutdown || 
				scoreboard->graceful_shutdown) {
			info("shutting down %s slot[%d]",__FILE__,slot->id);
			break;
		}

		/**************************** RMAC process ***********************/
		

		pthread_mutex_lock(&last_morph_anal_docid_mutex);
			if (*last_morph_anal_docid >= *last_piled_docid) {
				pthread_mutex_unlock(&last_morph_anal_docid_mutex);
				usleep(1000000);
				continue;
			}

			if (sb_run_spool_is_empty(spl)) {
				pthread_mutex_unlock(&last_morph_anal_docid_mutex);
				warn("spool is empty. sleep for a while");
				usleep(5000000);
				continue;
			}

			(*last_morph_anal_docid)++;
			setproctitle("softbotd: %s (analyzing document of docid[%u])",
							__FILE__, *last_morph_anal_docid);

			docid = *last_morph_anal_docid;
		pthread_mutex_unlock(&last_morph_anal_docid_mutex);


		nret = sb_run_spool_get_by_key(spl, docid, &ptr, &len);
		INFO("get document[%d]: rmac[%d] ", docid, slot->id);

		if (nret == -1) {
			warn("cannot get document of docid[%d]", docid);
			continue;
		}

		if ( ((char *)ptr)[0] == 'D' && ((char *)ptr)[1] == 'E' &&
				((char *)ptr)[2] == 'L' && ((char *)ptr)[3] == ' ' ) {
			warn("document of docid[%d] is deleted document", docid);
			continue;
		}
		else if ( !(((char *)ptr)[0] == 'C' && ((char *)ptr)[1] == 'D' &&
				((char *)ptr)[2] == ' ' && ((char *)ptr)[3] == ' ')) {
			crit("document of docid[%d] is not canned document format", docid);
			continue;
		}
		doc = ptr + 4;
		len -= 4;

#define MAX_TRY					(16)
		try = 0;
#define MAX_RETRY_TCP_TIMEOUT	(1)
		timeout_try = 0;
CONNECT_RETRY:
		svrID = find_a_server_to_connect();
		mark_server_state_table( svrID , 1 );

		try++;
		nret = sb_run_tcp_connect(&sockfd , rmas_addr[svrID].address , rmas_addr[svrID].port);

		if (nret == FAIL) {
			if (try < MAX_TRY) {
				sleep(2);
				goto CONNECT_RETRY;
			}

			error("cannot connect to server[%s:%s] %d-tries", 
					rmas_addr[svrID].address , rmas_addr[svrID].port,
					try);
			((char *)ptr)[0] = 'E';
			((char *)ptr)[1] = 'R';
			((char *)ptr)[2] = 'R';
			((char *)ptr)[3] = ' ';
			continue;
		}

		
		nret = sb_run_sb4c_remote_morphological_analyze_doc( sockfd , meta_data , doc , 
				len ,&recv_data, &recv_data_size);
		error_num = errno;	

		sb_run_tcp_close(sockfd);
		mark_server_state_table( svrID , -1 );

		if (nret == FAIL) {
			if (error_num == ETIMEDOUT) {
				if (timeout_try < MAX_RETRY_TCP_TIMEOUT) {
					warn("TCP_TIMEOUT is return from RMAS server [%s:%s] \
						  retry[%d] document[%d]",
						  rmas_addr[svrID].address , rmas_addr[svrID].port, try, docid);
					sleep(2);
					timeout_try++;
					goto CONNECT_RETRY;
				} else {
					((char *)ptr)[0] = 'E';
					((char *)ptr)[1] = 'R';
					((char *)ptr)[2] = 'R';
					((char *)ptr)[3] = ' ';

					warn("TCP_TIMEOUT is return from RMAS server [%s:%s] \
						  SKIP document[%d]",
						  rmas_addr[svrID].address , rmas_addr[svrID].port, docid);
					continue;
				}
			} else {
				((char *)ptr)[0] = 'E';
				((char *)ptr)[1] = 'R';
				((char *)ptr)[2] = 'R';
				((char *)ptr)[3] = ' ';
				warn("error is returrned from RMAS server [%s:%s] for document[%d]", 
						rmas_addr[svrID].address , rmas_addr[svrID].port, docid);
				continue;
			}
		}

		acquire_lock(spool_semid);
		sb_run_spool_free(spl, ptr);
		release_lock(spool_semid);

		try = 0;
#define MAX_SPOOL_TRY				(24)
SPOOL_MALLOC_RETRY:
		if ((ptr = sb_run_spool_malloc(spl, recv_data_size + 4)) == NULL) {
			warn("cannot malloc spool: rmac[%d] for document[%d] waiting 1 sec; "
							"it could be dead lock.", slot->id, docid);
			sleep(1);
			try++;

			if (try < MAX_SPOOL_TRY) {
				goto SPOOL_MALLOC_RETRY;
			}
			error("cannot allocate mpool to insert index word list of doc[%d]: "
					"abandon to index", docid);

			ptr = sb_run_spool_malloc(spl, 4);
			if (ptr == NULL) {
				crit("there is no space just for 4 byte of state code in spool");
				abort();
			}

			((char *)ptr)[0] = 'E';
			((char *)ptr)[1] = 'R';
			((char *)ptr)[2] = 'R';
			((char *)ptr)[3] = ' ';

			sb_run_spool_put_by_key(spl, docid, ptr, 4);
			sb_free(recv_data);

			continue;
//			spl_print_tags(stderr, spl);
		}

		((char *)ptr)[0] = 'I';
		((char *)ptr)[1] = 'D';
		((char *)ptr)[2] = 'X';
		((char *)ptr)[3] = 'W';
		doc = ptr + 4;

		memcpy(doc, recv_data , recv_data_size);

		sb_run_spool_put_by_key(spl, docid, ptr, recv_data_size + 4);

		sb_free(recv_data);

		/**********************************************************************/
		time(&(slot->recent_request));
		i++;
		DEBUG("%d'th iteration",i);
	}

	slot->state = SLOT_FINISH;
	debug("slot[%d] exits", slot->id);
	return EXIT_SUCCESS;
}

static int find_a_server_to_connect()
{
	int i ,j;
	int min = MAX_SERVERS;

    j = (int)(rand() / (RAND_MAX+1.0) * num_of_rmas);

	for(i=j;i<num_of_rmas;i++)
	{
		if ( min > rmas_state_table[i]) {
			min = rmas_state_table[i];
			min = i;
		}
	}

	for(i=0;i<j;i++)
	{
		if ( min > rmas_state_table[i]) {
			min = rmas_state_table[i];
			min = i;
		}
	}
	return i;
}

static void mark_server_state_table( int server_id , int value)
{
	pthread_mutex_lock(&listmutex);
	rmas_state_table[server_id] += value;
    pthread_mutex_unlock(&listmutex);
}

static int module_main (slot_t *slot)
{
	/* set signal handler */
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);	

	debug("mod_register.c: module_main() init");
	spl = sb_run_spool_open(spoolpath, spool_queue_size, spool_mpool_size);
	if (spl == NULL) {
		return 1;
	}

	/* set number of thread to spawn */
	scoreboard->size = needed_threads;

	/* set scoreboard properly */
	sb_run_init_scoreboard(scoreboard);

	/* spawn slave thread */
	sb_run_spawn_threads(scoreboard,
			"rmac process", thread_main);

	/* monitering slave thread */
	scoreboard->period = MONITORING_PERIOD;
	sb_run_monitor_threads(scoreboard);

	debug("monitor_processes returned");

	return 0;
}

/****************************************************************************/
REGISTRY void init_last_morph_anal_docid(void *data)
{
	last_morph_anal_docid = data;
	*last_morph_anal_docid = 0;
}

REGISTRY char* registry_get_last_morph_anal_docid()
{
	static char buf[STRING_SIZE];
	snprintf(buf,STRING_SIZE, "last_piled_docid:%u",(*last_morph_anal_docid));
	return buf;
}

static void set_spool(configValue v)
{
	snprintf(spoolpath, MAX_SPOOL_PATH_LEN, "%s", v.argument[0]);
	spoolpath[MAX_SPOOL_PATH_LEN-1] = '\0';
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
	int left;

	if (strcmp("yes", v.argument[2]) != 0) return;
	sprintf(buf, "%s#%s:%s^", v.argument[1], v.argument[0], v.argument[3]);
	left = STRING_SIZE - strlen(meta_data);
	if (left < 0) left = 0;
	strncat(meta_data, buf, left);

//	strncpy(meta_data, v.argument[0], STRING_SIZE);
//	meta_data[STRING_SIZE-1] = '\0';
	info("meta data: %s", meta_data);
}

static int init(void)
{
	ipc_t spool_lock;
    int ret=0;
    ret=pthread_mutex_init(&listmutex, NULL);
    if (ret != 0) {
        error("error initializing list mutex: %s",strerror(errno));
    }

    ret=pthread_mutex_init(&last_morph_anal_docid_mutex, NULL);
    if (ret != 0) {
        error("error initializing last morph anal docid mutex: %s",strerror(errno));
    }

	spool_lock.type = IPC_TYPE_SEM;
	spool_lock.pid = SYS5_INDEXER_SPOOL;
	spool_lock.pathname = NULL;

	get_sem(&spool_lock);
	spool_semid = spool_lock.id;

    return SUCCESS;
}

static void set_threads_num(configValue v)
{
	needed_threads = atoi(v.argument[0]);
}

static registry_t registry[] = {
	PERSISTENT_REGISTRY("LAST_MORPH_ANAL_DOCID","last morphological analyzed document id",
					sizeof(uint32_t),
					init_last_morph_anal_docid, registry_get_last_morph_anal_docid, NULL),
	NULL_REGISTRY
};
static void set_spool_queue_size(configValue v)
{
	spool_queue_size = atoi(v.argument[0]);
}

static void set_spool_mpool_size(configValue v)
{
	spool_mpool_size = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("SpoolPath", set_spool, 1,
			"path of spool database db file: Spool [spool db path]"),
	CONFIG_GET("AddServer", set_ip_and_port, 1,
			"rmas ip:port"),
//	CONFIG_GET("Metadata", set_meta_data , 1 ,
 //           "field and mophological analizer id :  ex) title:0^author:1^ "),
	CONFIG_GET("Field", set_meta_data , VAR_ARG ,
            "field and mophological analizer id :  ex) title:0^author:1^ "),
	CONFIG_GET("Threads", set_threads_num, 1, "number of threads"),
	CONFIG_GET("IndexerSpoolQueueSize",set_spool_queue_size,1,\
			""),
	CONFIG_GET("IndexerSpoolMpoolSize",set_spool_mpool_size,1,\
			""),
	{NULL}
};

module rmac_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	init,					/* initialize */
	module_main,			/* child_main */
	scoreboard,		/* scoreboard */
	NULL					/* register hook api */
};
