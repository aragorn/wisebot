/* $Id$ */

#include "softbot.h"
#include "mod_softbot4.h"
#include "mod_api/docattr.h"
#include "mod_api/did.h"


#define MAX_THREADS		(10)
#define WAIT_TIMEOUT	(3)
#define MONITORING_PERIOD	(2)

static char mBindAddr[SHORT_STRING_SIZE] = "127.0.0.1";
static char mBindPort[SHORT_STRING_SIZE] = DEFAULT_SERVER_PORTSTR;
static int m_backlog = 8;
static int listenfd;
static int needed_threads=3;

static int accept_lock;

/* FIXME currently, needed_threads is not used. scoreboard->size is used instead. */
static int max_requests_per_child = 100000;

static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_THREADS) };

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
static int child_main (slot_t *slot)
{
	int i=0, ret=0;
	socklen_t len=0;
	int sockfd = -1;
	int max_requests=0;
	static struct sockaddr remote_addr;

	CRIT("mod_softbot4.c slot->id: %d started", slot->id);

	if ( sb_run_protocol_open() != SUCCESS ) {
		error("protocol open failed");
		return 1;
	}

	len = sizeof(remote_addr);
	max_requests = (int)max_requests_per_child * slot->id / scoreboard->size / 2 +
					max_requests_per_child;
	debug("max_requests:%d",max_requests);
	/* max_requests_per_child + slot->id * 2
	 * prevents re-spawning all threads at a time */
	for (i = 0; i < max_requests; i++) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) break;
		setproctitle("softbotd: %s(listen:%s, waiting) - [%d]",__FILE__, mBindPort, i);

		slot->state = SLOT_WAIT;
		if ( acquire_lock(accept_lock) != SUCCESS ) {
			error( "acquire_lock failed: %s", strerror(errno) );
			break;
		}

		DEBUG("slot[%d]: got mutex_lock, shutdown[%d], graceful_shutdown[%d]",
				slot->id, scoreboard->shutdown, scoreboard->graceful_shutdown);
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
			release_lock(accept_lock);
			break;
		}
		setproctitle("softbotd: %s(select_accept,listen:%s) - [%d]",
				__FILE__,mBindPort,i);
		ret = sb_run_tcp_select_accept(listenfd, &sockfd, &remote_addr, &len);
		if ( release_lock(accept_lock) != SUCCESS ) {
			error( "release_lock failed: %s", strerror(errno) );
			break;
		}

		/* if graceful_shutdown is on, replying for the request is ok */
		if ( scoreboard->shutdown ) {
			info("shutting down %s slot[%d]",__FILE__,slot->id);
			break;
		}

		if ( ret != SUCCESS ) {
			info("slot[%d]: tcp_select_accept() failed", slot->id);
			continue;
		}
		slot->state = SLOT_READ;
		time(&(slot->recent_request));

		slot->state = SLOT_PROCESS;
	   	/* FIXME SLOT_PROCESS state should be set at somewhere in mod_protocol4 */

		setproctitle("softbotd: %s(processing) - [%d]",__FILE__,i);
		sb_run_sb4s_dispatch(sockfd);
		sb_run_tcp_close(sockfd);

		debug("%d'th iteration of %d(max_requests)",i,max_requests);
		if (i % 100000 == 0) 
			info("%d'th iteration of %d(max_requests): slot[%d]",i, max_requests, slot->id);
	}

	setproctitle("softbotd: %s(shutdown)",__FILE__);
	if ( scoreboard->shutdown || scoreboard->graceful_shutdown ) {
		slot->state = SLOT_FINISH;
	}
	else {
		slot->state = SLOT_RESTART;
	}
	debug("slot[%d] exits", slot->id);

	sb_run_protocol_close();

	return EXIT_SUCCESS;
}

static int module_main (slot_t *slot)
{
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	debug("softbot4.c: module_main() init");

	if (sb_run_tcp_bind_listen(mBindAddr, mBindPort, m_backlog, &listenfd) != SUCCESS) {
		error("tcp_bind_listen: %s", strerror(errno));
		return 1;
	}
	setproctitle("softbotd: mod_softbot4.c (listening[%s:%s])",mBindAddr,mBindPort);

	scoreboard->size = needed_threads;

	debug("softbot4.c: module_main() init");
	sb_init_scoreboard(scoreboard);
	//XXX: process version needs sb_run_qp_process_init or something?
	sb_spawn_processes(scoreboard, "softbot4 process", child_main);

	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	debug("monitor_threads returned");
	info("syncronize docattr db: this should be called in new cdm register module later");
	sb_run_docattr_close();

	return 0;
}

static int init()
{
	int port=0;
	ipc_t lock;

	port=assignSoftBotPort(__FILE__, 0);
	snprintf(mBindPort,SHORT_STRING_SIZE,"%d",port);
	debug("listening port:%s",mBindPort);

	lock.type = IPC_TYPE_SEM;
	lock.pid  = SYS5_ACCEPT;
	lock.pathname = NULL;

	if ( get_sem(&lock) != SUCCESS ) return FAIL;
	accept_lock = lock.id;

	return SUCCESS;
}

/****************************************************************************/
static void set_listen_address(configValue v)
{
	strncpy(mBindAddr, v.argument[0], SHORT_STRING_SIZE);
	mBindAddr[SHORT_STRING_SIZE-1]='\0';

	debug("Listen %s", mBindAddr);
	return;
}

static void set_listen_backlog(configValue v)
{
	m_backlog = atoi(v.argument[0]);
}

static void set_threads_num(configValue v)
{
	needed_threads = atoi(v.argument[0]);
}

static void set_max_requests_per_child(configValue v)
{
	max_requests_per_child = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("Listen", set_listen_address, 1, \
				"IP-address e.g) 192.168.1.1"),
	CONFIG_GET("ListenBacklog", set_listen_backlog, 1, \
		   "maximum length of the queue of pending connections. see listen(2)"),
	CONFIG_GET("Threads", set_threads_num, 1, "number of threads"),
	CONFIG_GET("MaxRequests", set_max_requests_per_child, 1, "max requests for child's lifetime"),
	{NULL}
};

static void register_hooks(void)
{
	return;
}

module softbot4_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	init,					/* initialize */
	module_main,			/* child_main */
	scoreboard,				/* scoreboard */
	register_hooks			/* register hook api */
};

