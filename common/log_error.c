/* $Id$ */
#include "common_core.h"
/* moved to common_core.h due to precompiled header 
#define CORE_PRIVATE 1
#include "ipc.h"
#include "modules.h"
#include "ansi_color.h"
#include "log_error.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
*/

int   gLogLevel = MAX_LOG_LEVEL;
char *gLogLevelStr[] = {
    "emerg", /* red background */
    "alert", /* red background */
    "crit",  /* red background */
    "error", /* red */
    "warn",  /* yellow */
    "notice", /* bold blue */
    "info",  /* bold blue */
    "debug", /* white */
	NULL
};

//#include <pthread.h>

#define DEBUG_MODULE_NUM 	(20)

#define NO_POLICY			(0)
#define INCLUDE 			(1)
#define EXCLUDE				(2)

static int  debug_module_policy=NO_POLICY;
static int  debug_module_number=0;
static char debug_module_name[DEBUG_MODULE_NUM][SHORT_STRING_SIZE];

static char* color_format[] = {
	/*         module.c     function     */
	CLEAR GREEN "%s" CLEAR " %s() " BLINK BOLD WHITE ON_RED " ", /* emerg */
	CLEAR GREEN "%s" CLEAR " %s() " BLINK BOLD WHITE ON_RED " ", /* alert */
	CLEAR GREEN "%s" CLEAR " %s() " BOLD WHITE ON_RED " ", /* crit */
	CLEAR GREEN "%s" CLEAR " %s() " BOLD RED     " ", /* error */
	CLEAR GREEN "%s" CLEAR " %s() " YELLOW       " ", /* warn */
	CLEAR GREEN "%s" CLEAR " %s() " CYAN         " ", /* notice */
	CLEAR GREEN "%s" CLEAR " %s() " CYAN         " ", /* info */
	CLEAR GREEN "%s" CLEAR " %s()  "              , /* debug */
};

static FILE *fplog  = NULL; // error_log
static FILE *fpqlog = NULL; // query_log
static int  screen_log = 0;

#ifndef HAVE_SETLINEBUF
void setlinebuf(FILE *stream)
{
	setvbuf(stream, (char*)NULL, _IOLBF, 0);
}
#endif

#define DEBUG_LOG_ERROR
//#undef DEBUG_LOG_ERROR

static int  semid = -1;
static void init_loglock(void);

void set_screen_log(void) {
	screen_log = 1;
}

void open_error_log(const char* error_log, const char* query_log) {
	init_loglock();

	if ((fpqlog = sb_fopen(query_log,"a")) == NULL){
		crit("cannot open query log file %s: %s", query_log, strerror(errno));
	}
	setvbuf(fpqlog, (char*)NULL, _IOLBF, 0);

	/* screen_log > 0이면 파일을 열지 않는다. 열지 않으면 기본으로 stderr로 찍음. */
	if (screen_log) return;

	if((fplog = sb_freopen(error_log,"a",stderr)) == NULL){
		crit("cannot open error log file %s: %s", error_log, strerror(errno));
		crit("server exits(1)");
		exit(1);
	}
	setvbuf(stderr, (char*)NULL, _IOLBF, 0);

    return;
}

void close_error_log() {
	int ignored = 0;

	if (fplog) {
		fclose(fplog);
		fplog = NULL;
	}

	if (fpqlog) {
		fclose(fpqlog);
		fpqlog = NULL;
	}
	
	if (semid != -1 && semctl(semid, ignored, IPC_RMID) != 0)
		fprintf(stderr, "error while removing semaphore: %s", strerror(errno));
}

void reopen_error_log(const char* error_log, const char* query_log)
{
	if (fpqlog) {
		fclose(fpqlog);
		fpqlog = NULL;
	}
	
	if ((fpqlog = sb_fopen(query_log,"a")) == NULL){
		crit("cannot open query log file %s: %s", query_log, strerror(errno));
	}
	setvbuf(fpqlog, (char*)NULL, _IOLBF, 0);

	if (screen_log) return;

	if (fplog) {
		fclose(fplog);
		fplog = NULL;
	}

	if((fplog = sb_freopen(error_log,"a",stderr)) == NULL){
		crit("cannot open error log file %s: %s", error_log, strerror(errno));
		crit("server exits(1)");
		exit(1);
	}
	setvbuf(stderr, (char*)NULL, _IOLBF, 0);

	return;
}

void save_pid(const char *file) {
	FILE *log;

	if((log = sb_fopen(file,"w")) == NULL) {
		crit("cannot open pid log file %s: %s", file, strerror(errno));
		crit("server exits(1)");
		exit(1);
	}
	fprintf(log, "%d", getpid());
	fclose(log);

    return;
}

void log_setdebuglevel(int level)
{
	if (level < 0 || level > MAX_LOG_LEVEL) {
		warn("Invalid log level[%d]",level);
		level = MAX_LOG_LEVEL;
	}
	gLogLevel = level;
	info("log level is set to level[%d]",gLogLevel);
}

int log_setlevelstr(const char* levelstr)
{
	int i=0;

	debug("setting loglevel to [%s]", levelstr);
	for (i=0; gLogLevelStr[i]!=NULL; i++) {
		if (strcasecmp(gLogLevelStr[i],levelstr)==0) {
			gLogLevel = i;
			return SUCCESS;
		}
	}
	warn("Loglevel[%s] does not exist.",levelstr);
	return FAIL;
}

static void acquire_loglock(int semid)
{
	if (semid < 0) return;

	if (acquire_lock(semid) != SUCCESS) {
		perror("acquire_loglock error: ");
	}
}
static void release_loglock(int semid)
{
	if (semid < 0) return;

	if (release_lock(semid) != SUCCESS) {
		perror("release_loglock error: ");
	}
}

/* check log condition by name of module and log level */
static int check_log_condition(const char *module, int level)
{
	if (level <= gLogLevel){
		return SUCCESS;
	}

#ifdef DEBUG_LOG_ERROR
	if (debug_module_policy == INCLUDE) {
		int i;
		for (i=0; i<debug_module_number; i++) {
			if (strncmp(debug_module_name[i],module,STRING_SIZE) == 0)
				return SUCCESS;
		}

		return FAIL;
	}
	else if (debug_module_policy == EXCLUDE) {
		int i;
		for (i=0; i<debug_module_number; i++) {
			if (strncmp(debug_module_name[i],module,STRING_SIZE) == 0)
				return FAIL;
		}

		return SUCCESS;
	}
#endif

	return FAIL; // in case of NO_POLICY
}


/* Log format = (data , Module, Caller , level , content) */
void log_error(int          level,
			   const char  *aModule,
			   const char  *aCaller,
			   const char  *format, ...)
{
	va_list args;

	va_start(args, format);
	log_error_core(level, aModule, aCaller, format, args);
	va_end(args);
}

/* Log format = (data , Module, Caller , level , content) */
void log_error_core(int          level,
					const char  *aModule,
					const char  *aCaller,
					const char  *format, va_list args)
{
	time_t now;
	
	if (check_log_condition(aModule, level)==FAIL)	
		return;

    if (strrchr(aModule,'/'))
        aModule = 1+ strrchr(aModule,'/');
    if (strrchr(aModule,'\\'))
        aModule = 1+ strrchr(aModule,'\\');

	acquire_loglock(semid);
	if (fplog == NULL) {
		fprintf (stderr, color_format[level], aModule, aCaller);
		vfprintf(stderr, format, args);
		fprintf(stderr, " " CLEAR "\n");
	} else {
		time(&now);
		fprintf (fplog, "[%.24s] [%s] %s %s() ",
					ctime(&now),
					gLogLevelStr[level],
					aModule,
					aCaller);
		vfprintf(fplog, format, args);
		fputc('\n', fplog);
	}
	release_loglock(semid);

	return;
}

void log_query(const char* query)
{
	time_t now;
	
	acquire_loglock(semid);

	time(&now);
	if ( fpqlog )
		fprintf (fpqlog, "[%.24s] %s\n", ctime(&now), query);

	release_loglock(semid);

	return;
}

void log_assert(const char *exp,const char *file,int line,const char *func)
{
	log_error(LEVEL_CRIT, file, func,
			"line %d, assertion \"%s\" failed", line, exp);

#if defined(WIN32)
	DebugBreak();
#else
	/* unix assert does an abort leading to a core dump */
	abort();
#endif
}

void _sb_abort(const char *file, const char *caller)
{
	crit("%s: %s() called abort()", file, caller);
	abort();
}


static void init_loglock(void)
{
	key_t key;
	union semun semarg;

	key = IPC_PRIVATE;
	semid = semget(key,1,IPC_CREAT|IPC_EXCL|0600);
	if (semid == -1 && errno == EEXIST) {
		fprintf(stderr,"use existing semaphore for log [key=%d,semid=%d]\n", (int)key, semid);
		semid = semget(key,1,0);
	}

	if (semid == -1) {
		perror("error while getting sema for log: ");
	} else {
		semarg.val = 1;
		if (semctl(semid,0,SETVAL,semarg) == -1) {
		    perror("semctl SETVAL error: ");
		}
	}

	return;
}

double timediff(struct timeval *later, struct timeval *first)
{
	return (double)(later->tv_sec - first->tv_sec +
			(double)(later->tv_usec - first->tv_usec)/1000000);
}

/*****************************************************************************/
/* config section */

int set_debug_module_policy(const char *policy)
{
	if (strcasecmp(policy,"include") == 0) {
		debug_module_policy = INCLUDE;
	} else
	if (strcasecmp(policy,"exclude") == 0) {
		debug_module_policy = EXCLUDE;
	} else {
		warn("policy[%s] should be either \"include\" or \"exclude\".", policy);
		return FAIL;
	}

	return SUCCESS;
}

int add_debug_module(const char *name)
{
	if (debug_module_policy == NO_POLICY) {
		/* The policy should be either INCLUDE or EXCLUDE. */
		debug_module_policy = INCLUDE;
	}

	if (debug_module_number >= DEBUG_MODULE_NUM)
	{
		alert("You cannot set more than %d modules for debug.", DEBUG_MODULE_NUM);
		return FAIL;
	}
	
	if (find_module(name) == NULL) {
		error("module[%s] is not loaded.", name);
		return FAIL;
	}

	strncpy(debug_module_name[debug_module_number], name, SHORT_STRING_SIZE);
	debug_module_name[debug_module_number][SHORT_STRING_SIZE-1] = '\0';
	debug_module_number++;
	
	return SUCCESS;
}

