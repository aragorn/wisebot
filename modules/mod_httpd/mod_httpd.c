/* $Id$ */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_core.h"
#include "common_util.h"
#include "mod_httpd.h"
#include "timelog.h"
#include "connection.h"
#include "listen.h"
#include "conf.h" /* ugly workaround for name collision */
#include "apr_hooks.h" /* for apr_global_hook_pool */
#include "apr_poll.h"
#include "apr_lib.h"

/* XXX:should run process version for the time being. --jiwon */
#undef THREAD_VER
#define MAX_THREADS		(100)
#define WAIT_TIMEOUT	(30)
#define MONITORING_PERIOD	(2)
#define APACHE_STYLE_CONF "etc/mod_httpd.conf"

#ifndef DEFAULT_LOCKFILE
#  define DEFAULT_LOCKFILE "/tmp/softbot_httpd_lock"
#endif

static char mApacheStyleConf[SHORT_STRING_SIZE] = APACHE_STYLE_CONF;
static char mListenAddr[SHORT_STRING_SIZE] = "8000";
static int mBackLog = 8;
static int mMaxRequests = 10000;
static int mExtendedStatus = 1;
#ifdef THREAD_VER
static pthread_mutex_t accept_lock;
static scoreboard_t scoreboard[] = { THREAD_SCOREBOARD(MAX_THREADS) };
#else
static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_THREADS) };
#endif
static int num_listensocks = 0;

static int process_num = 1;

static process_rec *process;
static server_rec *server_conf;
static apr_pool_t *pool; /* pool for httpd child stuff */
static apr_pool_t *pconf;
/*****************************************************************************/
static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

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

	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}

/*****************************************************************************/
int graceful_stop_signalled(void)
{
	return (scoreboard->graceful_shutdown || scoreboard->shutdown);
}

int update_slot_state(slot_t *slot, int state, request_rec *r)
{
	int old_state;

	old_state = slot->state;
	slot->state = state;

	// FIXME fix up include/modules.h and uncomment codes below
	if (mExtendedStatus) {
#if 0
		if (state == SLOT_START || SLOT_OPEN) {
			/* reset individual counters */

			if (state == SLOT_OPEN) {
				slot->my_access_count = 0L;
				slot->my_bytes_served = 0L;
			}
			slot->conn_count = 0;
			slot->conn_bytes = 0;
		}
		if (r) {
			conn_rec *c = r->connection;
			strncpy(slot->client, get_remote_host(c), sizeof(slot->client));
			if (r->the_request == NULL) {
				strncpy(slot->request, "NULL", sizeof(slot->request));
			} else {
				strncpy(slot->request, r->the_request, sizeof(slot->request));
			}
		}
#endif
	}
	return old_state;
}

/*****************************************************************************/
static int process_socket(apr_socket_t *sock, slot_t *slot, 
		apr_bucket_alloc_t *bucket_alloc, apr_pool_t *p)
{
	conn_rec *current_conn;

	current_conn =
		sb_run_create_connection(sock, server_conf, slot, bucket_alloc, p);
	if (current_conn) {
		debug("a new connection is established.");
		sb_run_pre_connection(current_conn, sock);
		if (!current_conn->aborted)
			sb_run_process_connection(current_conn);
		sb_run_lingering_close_connection(current_conn);
		debug("connection is closed.");
	}

	return SUCCESS;
}

static int thread_main (slot_t *slot)
{
	apr_pool_t *tpool; /* thread's pool */
	apr_pool_t *ptrans; /* pool for per-transaction stuff */
	apr_allocator_t *allocator;
	apr_bucket_alloc_t *bucket_alloc;
	listen_rec *lr, *last_lr = listeners;
	apr_pollfd_t *pollset;
	apr_socket_t *csd = NULL;
	apr_status_t rv;
	int max_requests=0;
	//FIXME : configuration must set this value 
	apr_uint32_t ap_max_mem_free = 1024;

	sb_run_handler_init();

	apr_pool_create(&tpool, pool);
	bucket_alloc = apr_bucket_alloc_create(tpool);

	debug("slot[%d]: thread started", slot->id);
	apr_allocator_create(&allocator);
	apr_allocator_max_free_set(allocator, ap_max_mem_free);
	apr_pool_create_ex(&ptrans, NULL, NULL, allocator);
	apr_allocator_owner_set(allocator, ptrans);
/*	apr_pool_create_ex(&ptrans, pool, NULL, allocator);*/
/*	bucket_alloc = apr_bucket_alloc_create(ptrans);*/

	debug("slot[%d]: pool created", slot->id);

/*	apr_poll_setup(&pollset, num_listensocks, ptrans);*/
	apr_poll_setup(&pollset, num_listensocks, tpool);
	for (lr = listeners; lr != NULL; lr = lr->next)
		apr_poll_socket_add(pollset, lr->sd, APR_POLLIN);

	/* max_requests_per_child + slot->id * 2
	 * prevents re-spawning all threads at a time */
	max_requests = (int)mMaxRequests * slot->id / scoreboard->size;
	debug("slot[%d]: max_requests:%d", slot->id, max_requests);

	while (1) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;

		slot->state = SLOT_WAIT;
#ifdef THREAD_VER
		debug("going to wait for accept_lock");
		if ( pthread_mutex_lock(&accept_lock) != 0 ) {
			error("slot[%d]: pthread_mutex_lock: %s", slot->id, strerror(errno));
			continue;
		}
		debug("accept lock held successfully");
#endif

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
#ifdef THREAD_VER
			pthread_mutex_unlock(&accept_lock);
#endif
			break;
		}
#ifndef THREAD_VER
		/*set_proc_desc(slot, "softbotd: %s(select_accept,listenfd:%d)",
						__FILE__,listenfd); XXX: listenfd? */
#endif
		while (1) {
			apr_status_t ret;
			apr_int16_t event;
			int n;

			if ( scoreboard->shutdown
					|| scoreboard->graceful_shutdown ) break;

			/* old apr interface */
			// ret = apr_poll(pollset, &n, -1);
			ret = apr_poll(pollset, num_listensocks, &n, -1);
			if (ret != APR_SUCCESS) {
				if (APR_STATUS_IS_EINTR(ret)) continue;

				/* apr_pool() will only return errors in catastrophic
				 * circumstances. let's try exiting gracefully, for now. */
				error("apr_poll: (listen)");
				kill(getpid(), SIGNAL_GRACEFUL_SHUTDOWN);
			}

			if ( scoreboard->shutdown
					|| scoreboard->graceful_shutdown ) break;
			
			/* find a listener */
			lr = last_lr;
			do {
				lr = lr->next;
				if (lr == NULL)
					lr = listeners;

				apr_poll_revents_get(&event, lr->sd, pollset);
				if (event & APR_POLLIN) {
					last_lr = lr;
					goto got_fd;
				}

				if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
			} while (lr != last_lr);
		}

		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
			info("slot[%d]: shutting down %s",slot->id,__FILE__);
#ifdef THREAD_VER
			pthread_mutex_unlock(&accept_lock);
#endif
			break;
		}
	got_fd:
		debug("slot[%d]: got fd", slot->id);
		rv = lr->accept_func(&csd, lr, ptrans);
		debug("slot[%d]: accept func", slot->id);
#ifdef THREAD_VER
		pthread_mutex_unlock(&accept_lock);
#endif
		if (rv == APR_EGENERAL) {
			/* E[NM]FILE, ENOMEN, etc */
			error("accept(): %s", strerror(errno));
			kill(getpid(), SIGNAL_GRACEFUL_SHUTDOWN);
		}

	    if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		timelog("http_start");
		process_socket(csd, slot, bucket_alloc, ptrans);
		apr_pool_clear(ptrans);
	}

	apr_bucket_alloc_destroy(bucket_alloc);

	apr_pool_destroy(tpool);

#ifndef THREAD_VER
	sb_run_handler_finalize();
#endif

	slot->state = SLOT_FINISH;
	debug("slot[%d] exits", slot->id);
	return 0;
}

/*****************************************************************************/

static process_rec *create_process_rec(int argc, const char * const *argv)
{
	process_rec *proc;
	apr_pool_t *cntx;
	apr_status_t stat;

	stat = apr_pool_create(&cntx, NULL);
	if (stat != APR_SUCCESS) {
		error("apr_pool_create() failed to create initial context");
		return NULL;
	}

	apr_pool_tag(cntx, "process");

	proc = apr_palloc(cntx, sizeof(process_rec));
	proc->pool = cntx;

	apr_pool_create(&proc->pconf, proc->pool);
	apr_pool_tag(proc->pconf, "pconf");
	proc->argc = argc;
	proc->argv = argv;
	proc->short_name = apr_filename_of_pathname(argv[0]);

	return proc;
}

static int module_init ()
{
	int argc = 1;
	const char *argv[] = {"mod_httpd", NULL};
	const char *envp[] = {NULL};

    /* initialization for apr */
    apr_app_initialize(&argc,
        (char const *const **)&argv,
        (char const *const **)&envp); /* see "apr_general.h" */
    atexit(apr_terminate); /* will be called at exit. see "apr_general.h" */

	process = create_process_rec(argc, argv);
	if ( process == NULL ) return FAIL;

	pool = process->pool;
	pconf = process->pconf;

	/* FIXME we have to clean up this messy code of apr pool.
	 * which is gonna use apr_global_hook_pool?? */
	apr_global_hook_pool = pool;

	return SUCCESS;
}

static int module_main (slot_t *slot)
{
	apr_pool_t *ptemp; /* Pool for temporary config stuff, reset often */

	debug("mod_httpd.c: module_main() init");

	/* creating pool : now moved to module_init() */
	sb_run_set_default_sighandlers(_shutdown, _graceful_shutdown);

	/* read apache style configuration file */
	apr_pool_create(&ptemp, pconf);
	apr_pool_tag(ptemp, "ptemp");
	server_conf = ap_read_config(process, ptemp, mApacheStyleConf, &ap_conftree);
	if (sb_run_pre_config(pconf, ptemp) != SUCCESS) {
		error("pre-configuration failed");
		return 1;
	}
	
	/* set_listener() can be called multiple times for each listen address. */
	if (set_listener(mListenAddr, mBackLog, pool) != SUCCESS) return 1;

	/* actually bind and listen */
	num_listensocks = setup_listeners(pool);
	if (num_listensocks < 1) {
		error("failed to bind and listen: %s", strerror(errno));
		return 1;
	}
	set_proc_desc(slot, "softbotd: mod_httpd.c (listening[%s])",mListenAddr);

#ifdef THREAD_VER
	if ( pthread_mutex_init(&accept_lock, NULL) != 0 ) {
		error("pthread_mutex_init: %s", strerror(errno));
		return 1;
	}
#endif

	scoreboard->size = (process_num < MAX_THREADS) ? process_num : MAX_THREADS;

	sb_run_init_scoreboard(scoreboard);

#ifdef THREAD_VER
	sb_run_spawn_threads(scoreboard, "httpd thread", thread_main);
#else
	sb_run_spawn_processes(scoreboard, "httpd process", thread_main);
#endif

	scoreboard->period = MONITORING_PERIOD;
#ifdef THREAD_VER
	sb_run_monitor_threads(scoreboard);
#else
	sb_run_monitor_processes(scoreboard);
#endif
	debug("monitor_threads returned");

#ifdef THREAD_VER
	if ( pthread_mutex_destroy(&accept_lock) != 0 ) {
		error("pthread_mutex_destroy: %s", strerror(errno));
		return 1;
	}
#endif

#ifdef THREAD_VER
	sb_run_handler_finalize();
#endif

	return 0;
}


/****************************************************************************/
static void set_apache_style_conf(configValue v)
{
	strncpy(mApacheStyleConf, v.argument[0], SHORT_STRING_SIZE);
	mApacheStyleConf[SHORT_STRING_SIZE-1] = '\0';

	return;
}

static void set_listen_address(configValue v)
{
	strncpy(mListenAddr, v.argument[0], SHORT_STRING_SIZE);
	mListenAddr[SHORT_STRING_SIZE-1] = '\0';

	return;
}

static void set_listen_backlog(configValue v)
{
	mBackLog = atoi(v.argument[0]);
}

static void set_max_requests_per_child(configValue v)
{
	mMaxRequests = atoi(v.argument[0]);
}

static void set_threads_num(configValue v)
{
	process_num = atoi(v.argument[0]);
	if (process_num > MAX_THREADS)
		warn("You should not set Threads value more than MAX_THREADS(%d).", MAX_THREADS);
}

static config_t config[] = {
	CONFIG_GET("Threads", set_threads_num, 1, "number of threads"),
	CONFIG_GET("ApacheStyleConf", set_apache_style_conf, 1, \
				"path to apache style configuration file"),
	CONFIG_GET("Listen", set_listen_address, 1, \
				"[IP-address:]Port ex) 192.168.1.1:8604"),
	CONFIG_GET("ListenBacklog", set_listen_backlog, 1, \
		   "maximum length of the queue of pending connections. see listen(2)"),
	CONFIG_GET("MaxRequests", set_max_requests_per_child, 1, "max requests for child's lifetime"),
	{NULL}
};

static void register_hooks(void)
{
	return;
}

module httpd_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	module_init,				/* initialize */
	module_main,				/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks				/* register hook api */
};

