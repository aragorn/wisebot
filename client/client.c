/* $Id$ */
#define CLIENT /* includes client only APIs */
#include "common_core.h"
#include "ipc.h"
#include "util.h"
#include "setproctitle.h"
#include "commands.h"
#include "client.h"
#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* <getopt.h> */
#endif
#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#endif
#include <signal.h>
#include <unistd.h> /* close(2) */
#include <stdlib.h> /* free(3) */
#include <errno.h>

char mServerAddr[SHORT_STRING_SIZE] = "localhost";
char mServerPort[SHORT_STRING_SIZE] = "8605";
static char mConfigFile[MAX_PATH_LEN] = "etc/softbot.conf";
static char mErrorLogFile[MAX_PATH_LEN] = "etc/client_log";

static int clc_log_level=0;
static int clc_listen_port=0;
static int clc_server_root=0;

int mCdmSet = -1;
cdm_db_t* mCdmDb = NULL;
int mWordDbSet = -1;
word_db_t* mWordDb = NULL;
int mDidSet = -1;
did_db_t* mDidDb = NULL;

COMMAND commands[] = {
	{ "log" , com_log_level, "setting log level"},
	{ "repeat", com_repeat, "repeat commands"},
	{ "xrepeat", com_xrepeat, "repeat commands(with increasing number)"},
	{ "quit",	com_quit,	"quit softbot client" },
	{ "exit",	com_quit,	"quit softbot client" },
	{ "help",	com_help,	"display this text" },
	{ "?",		com_help,	"synonym for `help'" },
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Misc"HIDE, NULL, RESET},
	{ "rmas" , com_rmas_run, "rmas morphological analyzer without protocol4"},
	{ "rmac" , com_rmac_run, "rmac morphological analyzer without protocol4 through tcp"},
	{ "indexwords", com_index_word_extractor, "see how string is divided by index_word_extractor"},
	{ "morpheme", com_morpheme, "result of morpheme analysis" },
	{ "tokenizer", com_tokenizer, "result of tokenizing" },
	{ "qpp" , com_qpp, "result of preprocess" },
	{ "client_memstat" , com_client_memstat, "memory status of client" },
	{ "connect",com_connect,"hostname and port to connect" },
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Canned Doc"HIDE, NULL, RESET},
//	{ "register",com_register_doc,"register canned document" },
//	{ "registeri",com_register_doc_i,"register canned document iteratively" },
	{ "register4",com_register4_doc,"register document of softbot4.x" },
	{ "get",com_get_doc,"get document e.g) get 1" },
	{ "getsize",com_get_doc_size,"get size of document e.g) getsize 1" },
	{ "getabstract",com_get_abstracted_doc,"get abstracted document " },
	{ "lastregidoc",com_last_regi,"get last registered document id" },
	{ "getfield",com_get_field,"get field of document e.g) getfield 1 Body [filename]" },
	{ "getdocattr",com_get_docattr,"get docattr of document" },
	{ "setdocattr",com_set_docattr,"set docattr of document" },
	{ "setdocattr_by_oid",com_set_docattr_by_oid,"set docattr of document" },
	{ "rebuild_docattr",com_rebuild_docattr,"rebuild docattr db" },
	{ "rebuild_rid",com_rebuild_rid,"rebuild docattr db" },
	{ "getabstractfield",com_get_abstracted_field,
									"get field of abstracted document" },
	{ "updatefield",com_update_field, "update field value of cdm2 document" },
	{ BLANKLINE, NULL, ""},

	{ "connectdb",com_connectdb,"connect cdm db e.g) connect dbname dbpath" },
	{ "selectdoc",com_selectdoc,"get field of document e.g) selectdoc fieldname from docid" },
	{ BLANKLINE, NULL, ""},
	{ "delete", com_delete,"delete document by did" },
	{ "undelete", com_undelete,"undelete document by did" },
	{ "del", com_delete_doc,"delete document by did through softbot4" },
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"QueryProcessor"HIDE, NULL, RESET},
	{ "search",	com_search,	"search document by query string" },
	{ "search_setting",com_search_setting,"setting LC,PG.default LC:15,PG:0"},
	{ "query_test",com_query_test,"test query from file"},
	{ "docattr_query_test", com_docattr_query_test, "test docattr query (not search). valid when using general docattr module"},
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Indexer"HIDE, NULL, RESET},
/*	{ "indexwordstat",  com_index_word_stat, "indexing word statistic(nhits by doc size)"},*/
	{ "forward_index", com_forward_index, "see forward index of document"},
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Status"HIDE, NULL, RESET},
	{ "config", com_config,	"show configuration value" },
	{ "registry", com_registry,	"show registry value" },
	{ "status",	com_status,	"show scoreboard status" },
	{ "status_test",	com_status_test,	"test repeated status query" },
/*	{ "drop_cdm", com_drop_cdm, "drop all canned document db"},*/
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Lexicon"HIDE, NULL, RESET},
	{ "get_wordid", com_get_wordid , "get wordid for given word"},
	{ "get_word_by_wordid", com_get_word_by_wordid , "get word for given wordid"},
	{ "get_new_wordid", com_get_new_wordid , "get new wordid for given word"},
	{ "sync_word_db", com_sync_word_db , "synchronize word db"},
	{ "get_num_of_wordid", com_get_num_of_wordid , "get maked wordid number"},
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"Docid"HIDE, NULL, RESET},
	{ "getdocid", com_get_docid, "retreive document id"},
	{ "getnewdocid", com_get_new_docid, "generate new document id"},
	{ BLANKLINE, NULL, ""},
	
	{ RESET GREEN"Test related stuffs"HIDE, NULL, RESET},
	{ "test_tokenizer", com_test_tokenizer, "tests tokenizer module"},
	{ "strcmp", com_strcmp, "hangul compare function"},
	{ BLANKLINE, NULL, ""},

	{ RESET GREEN"benchmark"HIDE, NULL, RESET},
	{ "benchmark", com_benchmark, "benchmark [option] query_list_file"},
	{ BLANKLINE, NULL, ""},

	{ NULL,		NULL,		NULL }
};

/*****************************************************************************/
char *stripwhite(char *line)
{
	char *s, *t;

	for (s = line; whitespace(*s); s++)
		;
	if (*s == '\0') return s;

	t = s + strlen (s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';

	return s;
}
	
char *dupstr(char *s) {
	char *r;

	r = (char*)malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

/*****************************************************************************/
/* signal handler (ignore SIGINT) */

static void _signal_handler(int sig)
{
	// stdin 이 없어지면 종료된다.
	close(0);
}

static void set_signal_handler()
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _signal_handler;
	sigaction(SIGINT, &act, NULL);
}

/*****************************************************************************/
/* readline related stuff */
static char *command_generator(const char *, int);
static char **softbot_completion(const char *, int, int);
static COMMAND *find_command();

#define HISTORY_FILE  ".history"
#define MAX_HISTORY   (1000)
static char history_file[MAX_PATH_LEN];
static void init_readline()
{
	rl_readline_name = "softbot";
	rl_attempted_completion_function = softbot_completion;

	sb_server_root_relative(history_file, HISTORY_FILE);

    if (read_history(history_file) != 0) { 
        FILE* fp=sb_fopen(HISTORY_FILE,"a+"); 
        fclose(fp); 
    } 
    
    stifle_history(MAX_HISTORY); 
} 
    
static void finish_readline() 
{ 
    if (write_history(history_file)) { 
        error("error writing history file[%s]: %s", history_file, strerror(errno)); 
    } 
}

int execute_line(char *line)
{
	int i;
	COMMAND *command;
	char *word;

	/* isolate the command word. */
	for ( i = 0; line[i] && whitespace(line[i]); i++ )
		; /* do nothing */
	word = line + i;

	for ( ; line[i] && !whitespace(line[i]); i++ )
		; /* do nothing */

	if ( line[i] )
		line[i++] = '\0';

	command = find_command(word);

	if (command == NULL) {
		printf("%s: No such command for softbot. Type help.\n", word);
		return FAIL;
	}
	/* get argument to command, if any. */
	for ( ; whitespace(line[i]); i++ )
		; /* do nothing */

	word = line + i;
	
	/* call the function */
	if (command->func == NULL)
		return 1; // XXX: ?? refer to readline and FIXME

	return ((*(command->func)) (word));
}

static COMMAND *find_command(char *name)
{
	register int i;

    /* TODO: use history_search_prefix to do things like "!ls"  */
    //history_search_prefix(); 

	for ( i = 0; commands[i].name; i++ )
		if (strcmp (name, commands[i].name) == 0)
			return (&commands[i]);

	return NULL;
}

/* interface to readline completion */

static char **softbot_completion(const char *text, int start, int end)
{
	char **matches;

	matches = (char **)NULL;

	if (start == 0)
		matches = rl_completion_matches(text, command_generator);

	return matches;
}

static char *command_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	if (!state){
	   list_index = 0;
	   len = strlen (text);
	 }

	/* Return the next name which partially matches from the command list. */
	while ( (name = commands[list_index].name) ){
		list_index++;

		if (strncmp (name, text, len) == 0) {
			return (dupstr(name));
		}
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}
/* end readline related stuff */


/*****************************************************************************/
int client_main()
{
	static char prompt[] = "softbot> ";
	char *line, *s;
	int r = SUCCESS;

	set_signal_handler();

	load_static_modules();
	read_config(mConfigFile,NULL);
	/* 1. load dynamic modules -> make linked list of whole modules
	 * 2. setup each modules's configuration -> call each config function
	 */

	load_dynamic_modules();
  /***********************************************************************
   * all the modules has been loaded.
   */
	sb_sort_hook();

	load_each_registry();
	
	init_core_modules(first_module);
	init_standard_modules(first_module);

	//spawn_child_for_each_module();
	r = sb_run_server_canneddoc_init();
	if ( r != SUCCESS && r != DECLINE ) {
		error( "cdm module init failed" );
		return 1;
	}

	sb_run_cdm_open( &mCdmDb, mCdmSet );
	sb_run_open_word_db( &mWordDb, mWordDbSet );
	sb_run_open_did_db( &mDidDb, mDidSet );

	init_readline();

	printf("SoftBot Client Version: %s\n", PACKAGE_VERSION);
	printf("          Release Date: %s\n", RELEASE_DATE);
	printf("   Module Magic Number: %u:%u\n",
			MODULE_MAGIC_NUMBER_MAJOR, MODULE_MAGIC_NUMBER_MINOR);
	printf("          Architecture: %ld-bit\n", 8*(long)sizeof(void *));

	for ( ; ; ) {
		line = readline(prompt);
		if ( line == NULL ) break; /* ^D input */

		/* remote leading and trailing whitespace from the line. */
		s = stripwhite(line);

		if ( *s ) {
			add_history(s);
			r = execute_line(s);
		}

		free(line);
		if (r == DECLINE) break;
	}
	finish_readline();
	printf("bye!\n");

	if ( mDidDb ) sb_run_close_did_db( mDidDb );
	if ( mWordDb ) sb_run_close_word_db( mWordDb );

	return SUCCESS;
}

/* getopt(3) related stuff ***************************************************/
extern char *optarg;
extern int optind,opterr,optopt;

#ifndef HAVE_GETOPT_LONG
struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
#else
static struct option opts[] = {
	{ "help",			0, NULL, 'h' },
	{ "port",			1, NULL, 'p' },
	{ "log",			1, NULL, 'g' },
	{ "show-hook",		0, NULL, 'k' },
	{ "list",			0, NULL, 'l' },
	{ "configtest",		0, NULL, 't' },
	{ "configpath",		0, NULL, 'c' },
	{ "version",		0, NULL, 'v' },
	{ "version-status",	0, NULL, 1 },
	{ NULL,				0, NULL, 0 }
};
#endif

#define OPTION_COMMANDS		"hp:g:kltc:v"

static struct option_help {
	char *long_opt, *short_opt, *desc;
} opts_help[] = {
	{ "--help", "-h",
	  "Display softbot usage" },
	{ "--port", "-p [port]",
	  "Set server port of SoftBot. all other ports are decided relavant to it)"},
	{ "--log", "-g [level]",
	  "Set error log level (debug, info, notice,.. -d help for more)" },
	{ "--show-hook", "-k",
	  "Show hooking process" },
	{ "--list", "-l",
	  "List all compiled-in modules" },
	{ "--configtest", "-t",
	  "Test the syntax of the specified config" },
	{ "--configpath", "-c",
	  "Set configuration path for the server" },
	{ "--version", "-v",
	  "Print version number and exit" },
	{ "--version-status", "-vv",
	  "Print extended version information and exit" },
	{ NULL, NULL, NULL }
};

static void show_usage(int exit_code)
{
	struct option_help *h;

	printf("usage: softbotcli [options]\n");
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

int
main(int argc, char *argv[], char *envp[])
{
	int show_version = 0;
	int check_config_syntax = 0;
	/* get opt stuff */
	int c;
	const char *cmdopts = OPTION_COMMANDS;
	int i=0;

	char SoftBotRoot[MAX_PATH_LEN], *tmp;

	// set gSoftBotRoot
	if ( realpath(argv[0], SoftBotRoot) ) {
		tmp = strstr( SoftBotRoot, "/bin/softbotcli" );
		if ( tmp ) {
			*tmp = '\0';

			strcpy( gSoftBotRoot, SoftBotRoot );
		}
	}
	info("gSoftBotRoot is %s", gSoftBotRoot );

	/* FIXME: will be removed when released officially.
	 * all error messages should go into error_log file */
	set_screen_log();

	init_setproctitle(argc, argv, envp);
	set_static_modules(client_static_modules);

	// getopt stuff
	opterr = 0;
	while ( (c = 
#ifdef HAVE_GETOPT_LONG
		getopt_long(argc, argv, cmdopts, opts, NULL)
#elif HAVE_GETOPT
		getopt(argc, argv, cmdopts)
#else
#	error
#endif /* HAVE_GETOPT_LONG */
		) != -1 )
	{
		switch ( c ) {
			case 'p':
				clc_listen_port++;

				i=atoi(optarg);
				if (i<1024) {
					gSoftBotListenPort += i;
				}
				else gSoftBotListenPort = i;

				snprintf(mServerPort,SHORT_STRING_SIZE,"%d",gSoftBotListenPort);

				CRIT("Setting gSoftBotListenPort to %s",mServerPort);
				break;
			case 'g':
				if ( !optarg ) {
					error("Fatal: -d requires error log level argument.");
					exit(1);
				}
				for (i=0; gLogLevelStr[i]; i++) {
					if (strcmp(gLogLevelStr[i],optarg)==0) {
						gLogLevel = i;
						clc_log_level++;
						break;
					}
				}
				if (gLogLevelStr[i] == NULL) {
					debug("Invalid log level.");
					info("  arg for -d is like following");
					for (i=0; gLogLevelStr[i]; i++) {
						info("    %s",gLogLevelStr[i]);
					}
					exit(1);
				}
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
				debug("configuration file path is set to %s", mConfigFile);
				break;
			case 't':
				check_config_syntax = 1;
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
				debug("server root path set to %s",gSoftBotRoot);
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
	// end of getopt stuff

	if ( show_version ) {
		printf("SoftBot Client Version: %s\n", PACKAGE_VERSION);
		printf("          Release Date: %s\n", RELEASE_DATE);
		printf("   Module Magic Number: %d:%d\n",
			MODULE_MAGIC_NUMBER_MAJOR, MODULE_MAGIC_NUMBER_MINOR);
		printf("          Architecture: %ld-bit\n", 8*(long)sizeof(void *));

		exit(0);
	}

	/* initializing */

	if ( check_config_syntax ) {
		int ret=FAIL;
		debug("checking syntax");
		debug("configuration file:%s",mConfigFile);
		load_static_modules();
		read_config(mConfigFile,NULL);

		printf("Syntax check complete.\n");
		printf("Configuration module returned %d\n",ret);
		exit(0);
	}

	client_main();
	close_error_log();
	free_ipcs();

	return 0;
}

/****************************************************************************/
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

static void setLogLevel(configValue a)
{
	int i;

	/* cannot overwite command line configuration */
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
	strncpy(mErrorLogFile, a.argument[0], MAX_PATH_LEN);
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

static void setServerRoot(configValue a)
{
	debug("server root. argument[0]:%s",a.argument[0]);
	strncpy(gSoftBotRoot, a.argument[0], MAX_PATH_LEN);
}

static void setServer(configValue v)
{
	char host[SHORT_STRING_SIZE];
    char *colon;

    debug("1.Server %s:%s", mServerAddr, mServerPort);

    strncpy(host, v.argument[0], SHORT_STRING_SIZE);
    host[SHORT_STRING_SIZE-1] = '\0';
    colon = strchr(host, ':');

    if ( colon == NULL ) { /* (null):port */
		// let mServerAddr left with default value
		if (clc_listen_port) /* currently, commandline can only override port */
			return;

        strncpy(mServerPort, host, SHORT_STRING_SIZE);
    } else {               /* host:port */
        *colon = '\0';
        strncpy(mServerAddr, host, SHORT_STRING_SIZE);

		if (clc_listen_port) /* currently, commandline can only override port */
			return;
        strncpy(mServerPort, colon+1, SHORT_STRING_SIZE);
    }
    debug("2.Server %s:%s", mServerAddr, mServerPort);
}


static void setRmacServer(configValue v)
{
	char host[SHORT_STRING_SIZE];
    	char *colon;

    CRIT("1.rmac Server %s:%s", mRmacServerAddr, mRmacServerPort);

    strncpy(host, v.argument[0], SHORT_STRING_SIZE);
    host[SHORT_STRING_SIZE-1] = '\0';
    colon = strchr(host, ':');

    if ( colon == NULL ) { /* (null):port */
		// let mServerAddr left with default value
		if (clc_listen_port) /* XXX: currently, commandline can only override port */
			return;

        strncpy(mRmacServerPort, host, SHORT_STRING_SIZE);
    } else {               /* host:port */
        *colon = '\0';
        strncpy(mRmacServerAddr, host, SHORT_STRING_SIZE);

		if (clc_listen_port) /* XXX: currently, commandline can only override port */
			return;
        strncpy(mRmacServerPort, colon+1, SHORT_STRING_SIZE);
    }
    CRIT("2.Ramc Server %s:%s", mRmacServerAddr, mRmacServerPort);
}

static void setRebuildDocAttrField(configValue v) {
    int iCount;
    static char docattr_fields[MAX_FIELD_NUM][MAX_FIELD_NAME_LEN];

    for (iCount=0; iCount<MAX_FIELD_NUM && docattrFields[iCount]; iCount++);

    if (iCount == MAX_FIELD_NUM) {
        error("too many fields are setted");
        return;
    }
    strncpy(docattr_fields[iCount], v.argument[0], MAX_FIELD_NAME_LEN);
    docattr_fields[iCount][MAX_FIELD_NAME_LEN-1] = '\0';
    docattrFields[iCount] = docattr_fields[iCount];
    info("read RebuidDocAttrField[%s]", docattrFields[iCount]);
}

static void setCdmSet(configValue v) {
	mCdmSet = atoi( v.argument[0] );
}

static void setDidSet(configValue v) {
	mDidSet = atoi( v.argument[0] );
}

static void setWordDbSet(configValue v) {
	mWordDbSet = atoi( v.argument[0] );
}

static config_t config[] = {
    CONFIG_GET("LoadModule", doLoadModule, VAR_ARG,
        "a module name and the name of a shared object file to load it from"),
	CONFIG_GET("LogLevel", setLogLevel, 1, \
				"write error log messages of this level or above"),
	CONFIG_GET("ErrorLog", setErrorLog, 1, "path to client_log file"),
	CONFIG_GET("DebugModulePolicy", setDebugModulePolicy, 1, \
				"log Out_of_this or Inside_of_this"),
	CONFIG_GET("DebugModuleName", setDebugModuleName, VAR_ARG, \
				"lists module names for DebugModulePolicy"),
	CONFIG_GET("ServerRoot", setServerRoot, 1, \
				"server root directory"),
	CONFIG_GET("Server", setServer, 1, "[IP-address:]Port ex) 192.168.1.1:8604"),
	
	CONFIG_GET("AddServer", setRmacServer, 1, "[IP-address:]Port ex) 192.168.1.1:8604"),
	
	CONFIG_GET("RebuildDocAttrField", setRebuildDocAttrField, VAR_ARG, \
				"fields inserted into docattr db"),

	CONFIG_GET("CdmSet", setCdmSet, 1, "CdmSet 0~..."),
	CONFIG_GET("DidSet", setDidSet, 1, "DidSet 0~..."),
	CONFIG_GET("WordDbSet", setWordDbSet, 1, "WordDbSet 0~..."),
	{ NULL }
};


module client_module = {
	CORE_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	NULL					/* register hook api */
};

