/* $Id$ */
#include "common_core.h"
#include "common_util.h"
#include "memory.h"
#include "setproctitle.h"
#include "server.h"
#include "ipc.h"

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#ifdef HAVE_GETOPT_LONG
#  include <getopt.h>
#endif

#include <stdlib.h> /* exit(3) */
#include <unistd.h> /* setsid(P) */
#include <errno.h>
#include <string.h>
#include <signal.h>

static char mConfigFile[MAX_PATH_LEN] = DEFAULT_CONFIG_FILE;
static char mPidFile[MAX_PATH_LEN] = DEFAULT_PID_FILE;
#ifdef USE_TIMELOG
static char mTimeLogFile[MAX_PATH_LEN] = DEFAULT_TIME_LOG_FILE;
#endif
static int  *mServerUptime;

static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(10) };
static int nodaemon = 0;
static int unittest = 0;

static int clc_log_level=0; /* command line configuration log level */
static int clc_listen_port=0;
static int clc_server_root=0; /* command line configuration server root */
static char debug_module[MAX_MODULE_NAME] = "";

static void detach()
{
	int pid;
	pid_t pgrp;

	debug("detaching from terminal");

	/* 1. chdir to the root */
	// if we do chdir to root, we should have write permission in root 
	// for we do write msg socket in currently running directory.
	/*	if ( chdir("/") != 0 )
		warn("chdir(\"/\") failed: %s", strerror(errno));*/

	/* 2. fork and run in background */
	if ((pid = sb_fork()) > 0 )
		exit(0);
	else if (pid == -1) {
		perror("fork");
		error("unable to fork new process\n");
		exit(1);
	}

	/* 3. setsid or setpgid */
	if ((pgrp = setsid()) == -1)
		error("setsid() failed. see setsid(2)");

	/* 4. close out the standard file descriptors */
	if (freopen("/dev/null", "r", stdin) == NULL)
		warn("freopen stdin failed: %s", strerror(errno));
	if (freopen("/dev/null", "w", stdout) == NULL)
		warn("freopen stdout failed: %s", strerror(errno));

	/*
	if (freopen("/dev/null", "w", stderr) == NULL)
		warn("freopen stderr failed: %s", strerror(errno));
	*/
	/* stderr will be reopened to be error_log by log_error.c.
	 * so leave this alone for now.
	 */
}

/* signal handlers : see sigaction(2) ********************************************************/

static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
	scoreboard->shutdown++;
}

static void _graceful_shutdown(int sig)
{
	scoreboard->graceful_shutdown++;
//	list_scoreboard(stdout, NULL); /* XXX : for debugging, delete soon */
}

static void _reopen_log_error(int sig)
{
	reopen_error_log(gErrorLogFile, gQueryLogFile);
	return;
}

static void set_signal_handlers()
{
    struct sigaction act;

    memset(&act, 0x00, sizeof(act));
//    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);
    
    act.sa_handler = _do_nothing;
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGXFSZ, &act, NULL); // log파일이 2G 되면 이 signal이 발생한다.
    
    act.sa_handler = _shutdown;
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    
    act.sa_handler = _graceful_shutdown;
    sigaction(SIGINT, &act, NULL);

	act.sa_handler = _reopen_log_error;
	act.sa_flags = SA_RESTART;
	sigaction(SIGHUP, &act, NULL);
}


/* end signal handlers */

/* getopt related stuff: see getopt(3) *******************************************************/
extern char *optarg;
extern int optind,opterr,optopt;

#ifndef HAVE_GETOPT_LONG
struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
#endif

/* d,m argument stuff is dealt in switch statement. */
#define OPTION_COMMANDS		"hnp:g:m:ukltc:r:v"

#ifdef HAVE_GETOPT_LONG
static struct option opts[] = {
	{ "help",			0, NULL, 'h' },
	{ "nodaemon",		0, NULL, 'n' },
	{ "port",			1, NULL, 'p' },
	{ "log",			1, NULL, 'g' },
	{ "debug-module",	1, NULL, 'm' },
	{ "unittest",       0, NULL, 'u' },
	{ "show-hook",		0, NULL, 'k' },
	{ "list",			0, NULL, 'l' },
	{ "configtest",		0, NULL, 't' },
	{ "configpath",		1, NULL, 'c' },
	{ "root",			1, NULL, 'r' },
	{ "version",		0, NULL, 'v' },
	{ "version-status",	0, NULL, 1 },
	{ NULL,				0, NULL, 0 }
};
#endif

static struct option_help {
	char *long_opt, *short_opt, *desc;
} opts_help[] = {
	{ "--help", "-h",
	  "Display softbotd usage" },
	{ "--nodaemon", "-n",
	  "Disable background daemon mode(output goes to tty, instead of logfile)" },
	{ "--port", "-p [port]",
	  "Set server port of SoftBot. all other ports are decided relavant to it)"},
	{ "--log", "-g [level]",
	  "Set error log level (debug, info, notice,.. -d help for more)" },
	{ "--debug-module", "-m [mod_name.c]",
	  "Run on foreground and execute the module's main function" },
	{ "--unittest", "-u", 
	  "Do unittest for each module."},
	{ "--show-hook", "-k",
	  "Show hooking process" },
	{ "--list", "-l",
	  "List all compiled-in modules" },
	{ "--configtest", "-t",
	  "Test the syntax of the specified config" },
	{ "--configpath", "-c",
	  "Set configuration path for the server" },
	{ "--root", "-r",
	  "Set root path for the server" },
	{ "--version", "-v",
	  "Print version number and exit" },
	{ "--version-status", "-vv",
	  "Print extended version information and exit" },
	{ NULL, NULL, NULL }
};

static void show_usage(int exit_code)
{
	struct option_help *h;

	printf("usage: softbotd [options]\n");
	for (h = opts_help; h->long_opt; h++) {
#ifdef HAVE_GETOPT_LONG
		printf(" %s, %s\n ", h->long_opt, h->short_opt);
#else
		printf(" %s\n ", h->short_opt);
#endif
		printf("    %s\n", h->desc);
	}

	exit(exit_code);
}

/* end getopt related stuff */

#define CHILD_MONITORING_PERIOD (2)

/* server_main()
 * -> sb_run_* APIs are enabled
 * -> all registries are enabled
 * -> all configs are enabled. config should follow and overwrite registry.
 */
int server_main()
{
	module *mod=NULL;

  /***********************************************************************
   * first-pass config 
   */

	set_proc_desc(NULL, "softbotd: master - loading static modules");
	/* load_static_modules()
	 *  -> hooking APIs. we cannot call sb_run_* APIs before this
	 */
	load_static_modules();

    /* FIXME why should i load kbo module?? */
	//load_kbo_module();

	if (read_config(mConfigFile, NULL) != SUCCESS) {
		crit("failed to read config file");
		exit(1);
	}
	/* 1. load dynamic modules -> make linked list of whole modules
	 * 2. setup each modules's configuration -> call each config function
	 */

	open_error_log(gErrorLogFile, gQueryLogFile);
#ifdef USE_TIMELOG
    if(sb_tstat_log_init(mTimeLogFile) != SUCCESS) {
        exit(1);
    }
#endif

	save_pid(mPidFile);

	load_dynamic_modules();
  /***********************************************************************
   * all the modules has been loaded.
   */

	sb_sort_hook();

	load_each_registry();
	/* 1. register each module to registry
	 * 2. do the shared memory related stuff...
	 * 3. XXX registry module does not guarantee shared memory locking...
	 */

	restore_registry_file(gRegistryFile);

	/* TODO second-pass config */

  /***********************************************************************
   * runtime configuration is done.
   */
	init_all_scoreboards();

	/* method 3 of doc/README.init */
	set_proc_desc(NULL, "softbotd: master - init each module");
	if ( init_core_modules(first_module) != SUCCESS ) goto STOP;
	if ( init_standard_modules(first_module) != SUCCESS ) goto STOP;

	if (clc_listen_port >= 2) { /* show using ports */
		show_portinfo();
		goto STOP;
	}
	set_proc_desc(NULL, "softbotd: master - spawning child");
	/* check debug_module */
	if ( strlen(debug_module) > 0 ) {
		mod = find_module(debug_module);
		if ( mod == NULL ) {
			error("no such module [%s]", debug_module);
			return FAIL;
		}
		else {
			info("debugging module [%s]", debug_module);
			// XXX: in debugging mode, isn't it better not to fork? is it?
			if (mod->main) 
				sb_run_spawn_process_for_module(scoreboard, mod);
			else {
				error("module[%s] has no main",debug_module);
				exit(1);
			}
		}
	}
	else if (unittest) {
		crit("unittesting started");
		do_unittest();
		crit("unittesting ended");
		goto STOP;
	}
	else
		sb_run_spawn_processes_for_each_module(scoreboard, first_module);

	CRIT("*** master monitoring loop ***");
	setproctitle("softbotd: master - monitoring");
	set_proc_desc(NULL, "softbotd: master - monitoring");
	scoreboard->period = CHILD_MONITORING_PERIOD;
	sb_run_monitor_processes_for_modules(scoreboard, first_module);

  /***********************************************************************
   * stopping state.
   */
STOP:	
	if ( save_registry_file(gRegistryFile) != SUCCESS )
		error("save_registry_file(%s) failed: %s",
			gRegistryFile, strerror(errno));
	free_ipcs(); /* release shared memory and semaphore */
	close_error_log(); /* close error_log file and semaphore */

#ifdef USE_TIMELOG
    sb_tstat_log_destroy();
#endif

	return SUCCESS;
}

static void show_version_and_exit (void)
{
	printf("SoftBot Server Version: %s\n", PACKAGE_VERSION);
	printf("          Release Date: %s\n", RELEASE_DATE);
	printf("   Module Magic Number: %d:%d\n",
			MODULE_MAGIC_NUMBER_MAJOR, MODULE_MAGIC_NUMBER_MINOR);
	printf("          Architecture: %ld-bit\n", 8*(long)sizeof(void *));
	printf("Server compiled with....\n");
#ifdef DEBUG_SOFTBOTD
	printf(" -D DEBUG_SOFTBOTD\n");
#endif
#ifdef SERVER_ROOT
	printf(" -D SERVER_ROOT=\"" SERVER_ROOT "\"\n");
#endif
#ifdef DEFAULT_CONFIG_FILE
	printf(" -D DEFAULT_CONFIG_FILE=\"" DEFAULT_CONFIG_FILE "\"\n");
#endif
#ifdef DEFAULT_ERROR_LOG_FILE
	printf(" -D DEFAULT_ERROR_LOG_FILE=\"" DEFAULT_ERROR_LOG_FILE "\"\n");
#endif
#ifdef DEFAULT_PID_FILE
	printf(" -D DEFAULT_PID_FILE=\"" DEFAULT_PID_FILE "\"\n");
#endif
#ifdef DEFAULT_REGISTRY_FILE
	printf(" -D DEFAULT_REGISTRY_FILE=\"" DEFAULT_REGISTRY_FILE "\"\n");
#endif

	exit(0);
}

int
main(int argc, char *argv[], char *envp[])
{
	int c;
	int show_version = 0;
	const char *cmdopts = OPTION_COMMANDS;
	char SoftBotRoot[MAX_PATH_LEN], *tmp;

	// set gSoftBotRoot
	if ( realpath(argv[0], SoftBotRoot) ) {
		tmp = strstr( SoftBotRoot, "/bin/softbotd" );
		if ( tmp ) {
			*tmp = '\0';

			strcpy( gSoftBotRoot, SoftBotRoot );
		}
	}
	info("server has started in [%s].", gSoftBotRoot );


#ifdef AIX5
	{ char *env = getenv("EXTSHM");
	  if (!env || strcmp(env, "ON")) {
		/* we need EXTSHM=ON in our environment to make the shmat and mmaps work */
		setenv("EXTSHM", "ON", 1);
		fprintf(stderr, "re-executing self with EXTSHM=ON exported to fix AIX shmat problem.\n");
		execv(argv[0], argv);
		fprintf(stderr, "re-execution failed - terminating.\n");
		exit(1);
	  }
	}
#endif

	init_setproctitle(argc, argv, envp);
	set_static_modules(server_static_modules);

	/* getopt stuff */
	opterr = 0;
	while ( (c = 
#ifdef HAVE_GETOPT_LONG
		getopt_long(argc, argv, cmdopts, opts, NULL)
#elif HAVE_GETOPT
		getopt(argc, argv, cmdopts)
#else
#	error at least one of getopt, getopt_long must exist.
#endif /* HAVE_GETOPT_LONG */
		) != -1 )
	{
		int i = 0;

		switch ( c ) {
			case 'n':
				nodaemon++;
				set_screen_log();
				break;
			case 'p':
				clc_listen_port++;
				if (strcmp(optarg,"show")==0) { /* show using ports info */
					clc_listen_port++; 
					nodaemon++;
					log_setlevelstr("error");
					clc_log_level++;
					break;
				}

				i=atoi(optarg);
				if (i<1024) gSoftBotListenPort += i;
				else gSoftBotListenPort = i;

				info("Setting gSoftBotListenPort to %d",gSoftBotListenPort);

				break;
			case 'g':
				if ( !optarg ) {
					error("Fatal: -g requires error log level argument.");
					exit(1);
				}

				if (log_setlevelstr(optarg) == SUCCESS) {
					clc_log_level++;
				} else {
					error("Invalid log level.");
					info("  arg for -d is like following");
					for (i=0; gLogLevelStr[i]; i++) {
						info("    %s",gLogLevelStr[i]);
					}
					exit(1);
				}
				break;
			case 'm':
				if ( !optarg ) {
					error("Fatal: -m requires module name.");
					exit(1);
				}

				strncpy(debug_module, optarg, MAX_MODULE_NAME);
				debug_module[MAX_MODULE_NAME-1] = '\0';

				/* nodaemon is on by default when debugging a module */
				nodaemon++;
				set_screen_log();
				debug("debugging module[%s]", debug_module);
				break;
			case 'u':
				if (unittest==0) {
					info("When doing unittest,");
					info("always check that modules to be tested are loaded");
				}
				nodaemon++;
				set_screen_log();
				unittest++;
				break;
			case 'k':
				/* refer to hook.c, hook.h, modules.c */
				debug_module_hooks++;
				break;
			case 'l':
				list_static_modules(stdout);
				exit(0);
				break;
			case 'c':
				if ( !optarg ) {
					error("Fatal: -c requires configuration path");
					exit(1);
				}
				strncpy(mConfigFile, optarg, MAX_PATH_LEN);
				mConfigFile[MAX_PATH_LEN-1] = '\0';
				debug("configuration file path set to %s",mConfigFile);
				break;
			case 't':
				check_config_syntax = 1;

				clc_log_level++;
				gLogLevel = LEVEL_INFO;

				set_screen_log(); // syntax checking 시 error를 화면으로..
				printf("Checking syntax of configuration file\n");
				fflush(stdout);
				break;
			case 'r':
				if ( !optarg ) {
					error("Fatal: -r requires server root path");
					exit(1);
				}
				strncpy(gSoftBotRoot, optarg, MAX_PATH_LEN);
				gSoftBotRoot[MAX_PATH_LEN-1] = '\0';
				clc_server_root++;
				debug("gSoftBotRoot is set to %s by -r option.",gSoftBotRoot);
				break;
			case 'v':
				show_version++;
				break;
			case 'h':
				show_usage(0);
			case '?':
				error("Unknown option: %c", (char)optopt);
				show_usage(1);
		} /* switch ( c ) */
	} 
	/* end of getopt stuff */

	if ( show_version ) show_version_and_exit();


	/* initializing */

	/* we're only doing a syntax check of the configuration file.
	 */
	if ( check_config_syntax ) {
		int ret=FAIL;
		// TODO: is this enough? -aragorn
		debug("checking syntax");
		debug("configuration file:%s",mConfigFile);
		load_static_modules();
		ret = read_config(mConfigFile, NULL);

		printf("Syntax check completed.\n");
		printf("Configuration module returned %d\n",ret);
		exit(0);
	}

	/* setuid, setgid stuff */

	/* signal handler stuff */
	set_signal_handlers();
	/* resource limits */

	if ( nodaemon == 0 ) detach();

/*  RESTART:*/

	/* set pid */
	gRootPid = getpid();

	server_main();
	/* if ( restart ) goto RESTART; */

	return 0;
}

/*****************************************************************************/
static void clearModuleList(configValue a)
{
	debug("clear module list");
	clear_module_list();
}

static void doAddModule(configValue a)
{
	char registry_only = 0;

	if (load_module(a.argument[0], NULL, registry_only) 
			== NULL) {
		error("cannot load module[%s]", a.argument[1]);
	}
} 

static void doLoadModule(configValue a)
{
	char registry_only = 0;

	if (a.argNum == 3 && !strcasecmp("READ_REG", a.argument[2])) {
		registry_only++;
	} else if (a.argNum == 3) {
		info("%s should be READ_REG or nothing", a.argument[2]);
	}

	if (load_module(a.argument[0], a.argument[1], registry_only) 
			== NULL) {
		error("cannot load dynamic module[%s]", a.argument[1]);
	}
}

static void doLoadTestModule(configValue a)
{
	if (!unittest)
		return;

	if (load_module(a.argument[0], a.argument[1], 0) == NULL) {
		error("cannot load test module[%s]",a.argument[1]);
	}
}

static void setLogLevel(configValue a)
{
	int i;

	/* cannot overwrite command line configuration */
	if ( clc_log_level ) return;

	gLogLevel = MAX_LOG_LEVEL;
	for(i = 0; i < MAX_LOG_LEVEL; i++) {
		if (strcasecmp(a.argument[0], gLogLevelStr[i]) == 0) {
			gLogLevel = i;
			break;
		}
	}
	if (gLogLevel == MAX_LOG_LEVEL)
		info("set LogLevel default value, MAX_LOG_LEVEL");
}

static void setErrorLog(configValue a)
{
	strncpy(gErrorLogFile, a.argument[0], MAX_PATH_LEN);
}

static void setQueryLog(configValue a)
{
	strncpy(gQueryLogFile, a.argument[0], MAX_PATH_LEN);
}

static void setDebugModulePolicy(configValue a)
{
	set_debug_module_policy(a.argument[0]);
}

static void setDebugModuleName(configValue a)
{
	int i = 0;
	for ( i = 0; i < a.argNum; i++ )
	{
		add_debug_module(a.argument[i]);
	}
}

static void setMemoryDebug(configValue a)
{
	if (strncasecmp("yes", a.argument[0], 3) == 0 ||
	    strncasecmp("on",  a.argument[0], 3) == 0 ) sb_memory_debug_on();
	else sb_memory_debug_off();
}

static void setPidFile(configValue a)
{
	debug("pid file. argument[0]:%s",a.argument[0]);
	strncpy(mPidFile, a.argument[0], MAX_PATH_LEN);
}

static void setRegistryFile(configValue a)
{
	debug("registry file. argument[0]:%s",a.argument[0]);
	strncpy(gRegistryFile, a.argument[0], MAX_PATH_LEN);
}

static void setServerRoot(configValue a)
{
	debug("gSoftBotRoot is %s",a.argument[0]);
	strncpy(gSoftBotRoot, a.argument[0], MAX_PATH_LEN);
}

static void setListenPort(configValue a)
{
	/* cannot overwrite command line configuration */
	if ( clc_listen_port ) return;

	gSoftBotListenPort = atoi(a.argument[0]);
	if (gSoftBotListenPort < 1024) {
		error("gSoftBotListenPort(%d) is less than 1024",gSoftBotListenPort);
		error("setting it default value(%d)", DEFAULT_SERVER_PORT);
		gSoftBotListenPort = DEFAULT_SERVER_PORT;
	}
}

static void setName(configValue a)
{
	setproctitle_prefix(a.argument[0]);
}

/***** registry *****/

static void init_server_uptime(void *data)
{
	mServerUptime = data;
	*mServerUptime = time(NULL);
}

static char *set_server_uptime(void *timestr)
{
	*mServerUptime = atoi(timestr);
	return "OK";
}

static char* get_server_uptime()
{
	//static char buf[11]; /* "4294967295" + '\0' */
	static char buf[SHORT_STRING_SIZE]; /* "4294967295" + '\0' */
	sprintf(buf, "%d[%p]", *mServerUptime,mServerUptime);
	return buf;
}

/** config stuff **/
static config_t config[] = {
	CONFIG_GET("ClearModuleList", clearModuleList, 1,
		"a module name and the name of a shared object file to load it from"),
	CONFIG_GET("AddModule", doAddModule, 1,
		"a module name and the name of a shared object file to load it from"),
	CONFIG_GET("LoadModule", doLoadModule, VAR_ARG,
		"a module name and the name of a shared object file to load it from"),
	CONFIG_GET("TestModule", doLoadTestModule, 2,\
		"a module name and the name of a shared object file to load it from"),
	CONFIG_GET("LogLevel", setLogLevel, 1, \
				"write error log messages of this level or above"),
	CONFIG_GET("ErrorLog", setErrorLog, 1, \
				"path to error log file"),
	CONFIG_GET("QueryLog", setQueryLog, 1, \
				"path to query log file"),
	CONFIG_GET("DebugModulePolicy", setDebugModulePolicy, 1, \
				"debug message only with these modules or only without these modules"),
	CONFIG_GET("DebugModuleName", setDebugModuleName, VAR_ARG, \
				"lists module names for DebugModulePolicy"),
	CONFIG_GET("MemoryDebug", setMemoryDebug, 1, \
				"turn on or off(default) MemoryDebug mode"),
	CONFIG_GET("PidFile", setPidFile, 1, \
				"save process id of softbot in this file"),
	CONFIG_GET("RegistryFile", setRegistryFile, 1, \
				"save registry in this file and restore from this file"),
	CONFIG_GET("ServerRoot", setServerRoot, 1, \
				"server root directory"),
	CONFIG_GET("Listen", setListenPort, 1, "set global listen port for server"),
	CONFIG_GET("Name", setName, 1, "set engine name"),
	{NULL}
};

static registry_t registry[] = {
	RUNTIME_REGISTRY("ServerUptime", "epoch time when server started",\
		sizeof(int), init_server_uptime, get_server_uptime, set_server_uptime),
	NULL_REGISTRY
};

module server_module = {
	CORE_MODULE_STUFF,
	config,				/* config */
	registry,			/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	scoreboard,			/* scoreboard */
	NULL,				/* register hook api */
};

