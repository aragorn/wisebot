/* $Id$ */
#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/spool.h"
#include "mod_api/vbm.h"
#include "mod_api/cdm.h"
#include "mod_pile_doc.h"

#define MONITORING_PERIOD	5

REGISTRY uint32_t *last_piled_docid=NULL;
static char spoolpath[STRING_SIZE]="dat/indexer/indexer.spl";

static int spool_queue_size = SPOOL_QUEUESIZE;
static int spool_mpool_size = SPOOL_MPOOLSIZE;

static scoreboard_t scoreboard[] = {THREAD_SCOREBOARD(1)};

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

#define IDLE_TIME	500000 /* 0.5 sec */
#define LONG_IDLE_TIME 1000000  /* 1.0 sec */
static int thread_main(slot_t *slot)
{
	char *doc = NULL;
//	uint32_t last_registered_docid; //, *last_indexed_docid;
	spool_t *spl;
	int rv=0, size=0, piled_num=0, try=0;
	VariableBuffer var;
	struct timeval timeout;
	uint32_t *last_registered_docid=NULL;
	registry_t *reg;

	spl = sb_run_spool_open(spoolpath, spool_queue_size, spool_mpool_size);
	if (spl == NULL) {
		crit("cannot open spool[%s]",spoolpath);
		slot->state = SLOT_FINISH;
		return 1;
	}

	reg = registry_get("LastRegisteredDocId");
	if (reg == NULL) { 
		crit("cannot get LastRegisteredDocId registry"); 
		abort();
	}
	last_registered_docid = (uint32_t*)(reg->data);


	while (1) {  // endless while loop
		if (scoreboard->shutdown || scoreboard->graceful_shutdown) {
			info("shutting down %s slot[%d]",__FILE__,slot->id);
			goto FINISH;
		}

		piled_num = 0;
		for ( ;*last_piled_docid < *last_registered_docid && 
				piled_num < SPOOL_QUEUESIZE; ) {

			if (scoreboard->shutdown || scoreboard->graceful_shutdown) {
				info("shutting down %s slot[%d]",__FILE__,slot->id);
				goto FINISH;
			}

			if (sb_run_spool_is_full(spl)) {
				warn("spool is full. sleeping for a while : check spool ");
				break;
			}

			// XXX: variable buffer should be vanished in the future
			sb_run_buffer_initbuf(&var);
			rv = sb_run_server_canneddoc_get((DocId)(*last_piled_docid) + 1, &var);

			if (rv == CDM_DELETED) {
				info("trying to get deleted document(docid[%u])",*last_piled_docid + 1);
				size = 0;
			}
			//FIXME: temporarily.. CDM_NOT_EXIST는 CDM에서 return하면 안된다.
			else if (rv == CDM_NOT_EXIST) {
				info("there is no registered document of docid[%u]", *last_piled_docid + 1);
				size = 0;
				rv = CDM_DELETED;
			}
			else if (rv < 0) {
				//XXX: "ERR " code must be inserted into spool like "DEL "
				error("cannot get document[%d] from cdm: error code [%d]", 
						*last_piled_docid + 1, rv);
				sb_run_buffer_freebuf(&var);
				(*last_piled_docid)++;
				continue;
			}
			else {
				size = sb_run_buffer_getsize(&var);
				if (size == 0) rv = CDM_DELETED;
			}

			try = 0;
			while ((doc = (char *)sb_run_spool_malloc(spl, size + 4)) == NULL) {
				if (scoreboard->shutdown || scoreboard->graceful_shutdown) 
					goto FINISH;

				if (try > 2) {
					error("cannot allocate spool, skipping document[%d]",
							*last_piled_docid);
					sb_run_buffer_freebuf(&var);
					goto SKIP;
				}

				warn("cannot malloc(size:%d) mpool for docid[%u], sleep IDLE_TIME[%.2f]",
							size, *last_piled_docid + 1, (float)IDLE_TIME/1000000);
				usleep(LONG_IDLE_TIME);
				try++;
			}

			if (rv == CDM_DELETED) {
				doc[0] = 'D';
				doc[1] = 'E';
				doc[2] = 'L';
				doc[3] = ' ';
			}
			else {
				doc[0] = 'C';
				doc[1] = 'D';
				doc[2] = ' ';
				doc[3] = ' ';

				sb_run_buffer_get(&var, 0, size, doc + 4);
			}

			sb_run_buffer_freebuf(&var);

RETRY_ENQUEUE:
			if ((rv = sb_run_spool_enqueue(spl, *last_piled_docid + 1, doc, size + 4)) == -1) {
				warn("spool is full. sleeping for a while : not enqueued ");
				usleep(LONG_IDLE_TIME);
				goto RETRY_ENQUEUE;
			}
			info("document[%d] is inserted into spool by piler", *last_piled_docid + 1);
			piled_num++;
SKIP:
			(*last_piled_docid)++;
		}

		timeout.tv_sec = LONG_IDLE_TIME/1000000;
		timeout.tv_usec = LONG_IDLE_TIME % 1000000;
		select(0, NULL, NULL, NULL, &timeout); 
	}

FINISH:
	sb_run_spool_close(spl);
	if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
		slot->state = SLOT_FINISH;
	}
	else {
		slot->state = SLOT_RESTART;
	}
	debug("slot[%d] exits as a state[%d]", slot->id, slot->state);
	return 0;
}

static int module_main(slot_t *slot) {
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	if ( sb_run_server_canneddoc_init() != SUCCESS ) {
		error( "cdm module init failed" );
		return 1;
	}

	/* store pid of self in registry */

	sb_init_scoreboard(scoreboard);
	sb_spawn_threads(scoreboard,"indexer piling process",thread_main);

	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_threads(scoreboard);

	return 0;
}

/*****************************************************************************/
/*
 * below here follows configuration , registry
 */
REGISTRY void init_last_piled_docid(void *data)
{
	last_piled_docid = data;
	*last_piled_docid = 0;
}
REGISTRY char* registry_get_last_piled_docid()
{
	static char buf[STRING_SIZE];
	snprintf(buf,STRING_SIZE, "last_piled_docid:%u",(*last_piled_docid));
	return buf;
}

void set_spool_path(configValue v)
{
	strncpy(spoolpath, v.argument[0], STRING_SIZE);
	spoolpath[STRING_SIZE-1] = '\0';
}

static registry_t registry[] = {
	PERSISTENT_REGISTRY("LAST_INDEXER_PILED_DOCID","last piled (to spool) document id",
					sizeof(uint32_t),
					init_last_piled_docid, registry_get_last_piled_docid, NULL),
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
	CONFIG_GET("IndexerSpoolPath",set_spool_path,1,\
			"spool path which indexer uses. (e.g: IndexerSpoolPath dat/indexer/indexer.spl)"),
	CONFIG_GET("IndexerSpoolQueueSize",set_spool_queue_size,1,\
			""),
	CONFIG_GET("IndexerSpoolMpoolSize",set_spool_mpool_size,1,\
			""),
	{NULL}
};

module indexer_piling_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	registry,				/* registry */
	NULL,					/* initialize function of module */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	NULL					/* register hook api */
};
