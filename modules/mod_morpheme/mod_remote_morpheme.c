/* $Id$ */
#include "softbot.h"

#include <pthread.h>

#include "mod_mp/mod_mp.h"
#include "mod_api/mod_api.h"
#include "mod_api/spool.h"
#include "mod_api/xmlparser.h"

#define MAX_THREADS							2
#define WAIT_TIMEOUT						30
#define MONITORING_PERIOD					2
#define MAX_WAIT_REGISTER_PROCESS			10

typedef struct {
	char address[STRING_SIZE];
	char port[STRING_SIZE];
} server_t;

static spool_t *spl = NULL;
static char spoolpath[MAX_SPOOL_PATH_LEN] = "dat/indexer/indexer.spl";

static scoreboard_t remote_morph_scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };

static server_t rmas_addr[MAX_THREADS];

static int num_of_rmas = 0;

static int rmas_state_table[MAX_THREADS];

static pthread_mutex_t listmutex;
static pthread_mutex_t last_morph_anal_docid_mutex;

static char meta_data[STRING_SIZE];

static int max_requests_per_child = 100000;

static int *last_morph_anal_docid;
static int *last_piled_docid;

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

	remote_morph_scoreboard->shutdown++;
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

	remote_morph_scoreboard->graceful_shutdown++;
}


/****************************************************************************/
static int thread_main (slot_t *slot)
{
	int i=0, max_requests, len;
	void *doc, *ptr;
	void *recv_data;
	long recv_data_size=0;
	int svrID, nret, sockfd;
	uint32_t docid = 0;

	/* max_requests_per_child + slot->id * 2
	 * prevents re-spawning all threads at a time */
	max_requests = max_requests_per_child + 
			(int)(max_requests_per_child * slot->id / remote_morph_scoreboard->size);
	debug("max_requests:%d",max_requests);
	for (i=0; i<max_requests; ) {
		if (remote_morph_scoreboard->shutdown || 
				remote_morph_scoreboard->graceful_shutdown) {
			info("shutting down %s slot[%d]",__FILE__,slot->id);
			break;
		}

		/**************************** RMAC process ***********************/
		while(1) {
			if (*last_morph_anal_docid > *last_piled_docid) {
				usleep(1000000);
				continue;
			}

			pthread_mutex_lock(&last_morph_anal_docid_mutex);
			docid = *last_morph_anal_docid;
			(*last_morph_anal_docid)++;
			pthread_mutex_unlock(&last_morph_anal_docid_mutex);

			nret = sb_run_spool_get_by_key(spl, docid, &ptr, &len);

			if (nret == -1) {
				warn("cannot get document of docid[%d]", docid);
				continue;
			}

			if ( !(((char *)ptr)[0] == 'C' && ((char *)ptr)[1] == 'D' &&
					((char *)ptr)[2] == ' ' && ((char *)ptr)[3] == ' ')) {
				warn("document of docid[%d] is not canned document format", docid);
				sb_run_spool_free(spl, ptr);
				continue;
			}
			doc = ptr + 4;
			len -= 4;

			svrID = find_a_server_to_connect();

			mark_server_state_table( svrID , 1 );

			nret = sb_run_tcp_connect(&sockfd , rmas_addr[svrID].address , rmas_addr[svrID].port);

			if (nret == FAIL) {
				sb_run_spool_free(spl, ptr);
				continue;
			}

			nret = sb_run_sb4c_remote_morphological_analyze_doc( sockfd , meta_data , doc , 
					len ,&recv_data, &recv_data_size);

			sb_run_tcp_close(sockfd);
			if (nret == FAIL) {
				sb_run_spool_free(spl, ptr);
				continue;
			}

			mark_server_state_table( svrID , -1 );

			sb_run_spool_free(spl, ptr);

			ptr = sb_run_spool_malloc(spl, recv_data_size + 4);
			if (ptr == 0x00) {
				warn("cannot malloc spool");
				continue;
			}

			((char *)ptr)[0] = 'I';
			((char *)ptr)[1] = 'D';
			((char *)ptr)[2] = 'X';
			((char *)ptr)[3] = 'W';
			doc = ptr + 4;

			memcpy(doc, recv_data , recv_data_size);

			sb_run_spool_put_by_key(spl, docid, ptr, recv_data_size + 4);

			sb_free(recv_data);

		}

		/**********************************************************************/
		time(&(slot->recent_request));
		i++;
		debug("%d'th iteration of %d(max_requests)",i,max_requests);
	}

	slot->state = SLOT_FINISH;
	debug("slot[%d] exits", slot->id);
	return 0;
}

static int find_a_server_to_connect()
{
	int i ,j;
	int min = MAX_THREADS;

    j = (int)(rand() / (RAND_MAX+1.0) * 10);

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
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);	

	debug("mod_register.c: module_main() init");

	spl = sb_run_spool_open(spoolpath, SPOOL_QUEUESIZE, SPOOL_MPOOLSIZE);
	if (spl == NULL) {
		return 1;
	}

	/* set scoreboard properly */
	sb_init_scoreboard(remote_morph_scoreboard);

	/* spawn slave processes */
	sb_spawn_processes(remote_morph_scoreboard,
			"rmac process", thread_main);

	/* monitering slave processes */
	remote_morph_scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(remote_morph_scoreboard);

	debug("monitor_processes returned");

	return 0;
}

/****************************************************************************/
REGISTRY void init_last_piled_docid(void *data)
{
	last_piled_docid = data;
}

REGISTRY char* registry_get_last_piled_docid()
{
	static char buf[STRING_SIZE];
	snprintf(buf,STRING_SIZE, "last_piled_docid:%u",(*last_piled_docid));
	return buf;
}

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

	for(i=0;tmp[i]==':';i++)
		rmas_addr[num_of_rmas].address[i] = tmp[i];
	
	rmas_addr[num_of_rmas].address[i] = 0x00;

	ptmp = tmp + i;

	for(i=0;i<STRING_SIZE && ptmp[i];i++)
		rmas_addr[num_of_rmas].port[i] = ptmp[i];

	num_of_rmas++;
}

static void set_meta_data(configValue v)
{
	char tmp[STRING_SIZE];
	int i,j;
	strncpy(tmp, v.argument[0] , STRING_SIZE);

	sscanf(v.argument[0], "%s %d", tmp , &i);

	j = strlen(meta_data);

	if (j < STRING_SIZE - 20)
	{
		sprintf(meta_data+j , "%s=%d^" , tmp , i);
	}
}

static int init(void)
{
    int ret=0;
    ret=pthread_mutex_init(&listmutex, NULL);
    if (ret != 0) {
        error("error initializing list mutex: %s",strerror(errno));
    }

    ret=pthread_mutex_init(&last_morph_anal_docid_mutex, NULL);
    if (ret != 0) {
        error("error initializing last morph anal docid mutex: %s",strerror(errno));
    }

	meta_data[0] = 0x00;

    return SUCCESS;
}

static registry_t registry[] = {
	PERSISTENT_REGISTRY("LAST_INDEXER_PILED_DOCID","last piled (to spool) document id",
					sizeof(uint32_t),
					init_last_piled_docid, registry_get_last_piled_docid, NULL),
	PERSISTENT_REGISTRY("LAST_MORPH_ANAL_DOCID","last morphological analyzed document id",
					sizeof(uint32_t),
					init_last_morph_anal_docid, registry_get_last_morph_anal_docid, NULL),
	NULL_REGISTRY
};

static config_t config[] = {
	CONFIG_GET("SpoolPath", set_spool, 1,
			"path of spool database db file: Spool [spool db path]"),
	CONFIG_GET("Connect", set_ip_and_port, 1,
			"rmas ip:port"),
	CONFIG_GET("Field2MAID", set_meta_data , 1 ,
            "field and mophological analizer id :  ex) title 0 "),
	{NULL}
};

module remote_morph_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	init,					/* initialize */
	module_main,			/* child_main */
	remote_morph_scoreboard,		/* scoreboard */
	NULL					/* register hook api */
};
