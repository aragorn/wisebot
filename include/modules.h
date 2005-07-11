/* $Id$ */
#ifndef MODULES_H
#define MODULES_H

#include "softbot.h"
#include "hook.h"

#ifdef MAX_CONFIG_MOD_NAME
#	define MAX_MODULE_NAME		MAX_CONFIG_MOD_NAME
#else
#	define MAX_MODULE_NAME		40
#endif

/* very early typedef */
typedef struct module_t module;
typedef struct scoreboard_t scoreboard_t;
typedef struct slot_t slot_t;

/* module stuff ***************************************************************/
struct module_t {
    /* API version, *not* module version; check that module is 
	 * compatible with this version of the server.
     */
    int version;
    /* API minor version. Provides API feature milestones. Not checked 
	 * during module init. */
    int minor_version;
	/** Index to this modules structures in config vectors.  */
	int module_index;
	/* type of module */
    int type;

	/* The module name. the name of the module's C file */
    const char *name;
	/* The handle for the DSO. Internal use only. */
	void *dynamic_load_handle;
	/* A pointer to the next module in the list */
    module *next;
	/* magic cookie to identify a module structure. see also mod_so */
    unsigned long magic;
	/* 윗 부분은 *_MODULE_STUFF로 항상 assign 된다. */
	
    config_t    *config; /* configuration table */
    registry_t	*registry; /* registry of module */

	int	(*init)(void);	/* initialize function of module */
    int	(*main)(slot_t*);  /* main function of meta module */
    scoreboard_t  *scoreboard; /* status of processes and threads */
	void		(*register_hooks)(void);	/* register hook api */
};

#define MODULE_MAGIC_NUMBER_MAJOR	1
#define MODULE_MAGIC_NUMBER_MINOR	1
#define CORE_MODULE_TYPE			0x10
#define STANDARD_MODULE_TYPE		0x20
#define TEST_MODULE_TYPE			0x40
#define STATIC_MODULE_TYPE			0x01
#define DYNAMIC_MODULE_TYPE			0x02
#define MODULE_MAGIC_COOKIE			0x53425448 /* "SB60" 0x53 for S, 0x42 for B, ..*/

#define STANDARD_MODULE_STUFF \
            MODULE_MAGIC_NUMBER_MAJOR, \
            MODULE_MAGIC_NUMBER_MINOR, \
			-1, \
            STANDARD_MODULE_TYPE, \
            __FILE__,/* name */ \
            NULL, /* dynamic load handle */ \
            NULL, /* next module */ \
            MODULE_MAGIC_COOKIE

#define CORE_MODULE_STUFF \
            MODULE_MAGIC_NUMBER_MAJOR, \
            MODULE_MAGIC_NUMBER_MINOR, \
			-1, \
            CORE_MODULE_TYPE, \
            __FILE__,/* name */ \
            NULL, /* dynamic load handle */ \
            NULL, /* next module */ \
            MODULE_MAGIC_COOKIE

#define TEST_MODULE_STUFF \
            MODULE_MAGIC_NUMBER_MAJOR, \
            MODULE_MAGIC_NUMBER_MINOR, \
			-1, \
            TEST_MODULE_TYPE, \
            __FILE__,/* name */ \
            NULL, /* dynamic load handle */ \
            NULL, /* next module */ \
            MODULE_MAGIC_COOKIE

extern SB_DECLARE_DATA module *first_module;

/* scoreboard stuff ***********************************************************/
enum scoreboard_type {
	TYPE_THREAD = 0, /* multi-threads */
	TYPE_PROCESS /* multi-processes */
};

enum slot_state {
	SLOT_OPEN = 0,
	SLOT_START,		/* starting up process */
	SLOT_WAIT,		/* waiting for connection */
	SLOT_READ,		/* reading request */
	SLOT_PROCESS,	/* processing request */
	SLOT_WRITE,		/* sending reply */
	SLOT_KEEPALIVE,	/* keepalive (read) */
	SLOT_CLOSING,	/* closing connection */
	SLOT_RESTART,	/* restarting */
	SLOT_FINISH		/* gracefully finishing process/thread */
};

typedef struct {
	volatile int shutdown;	/* flag to indicate shutdown mode */
	volatile int graceful_shutdown;	/* flag to indicate graceful shutdown mode */
	volatile int period;		/* monitoring period in sec */
} scoreboard_core_t;

#define SLOT_NAME_SIZE (64)   /* process/thread name of slot */
struct slot_t {
	int id;
	int generation;
	int pid;		/* process id or thread id */
	pthread_t pthread;
	volatile int state;
	time_t start_time;
	time_t recent_request; /* recent request time */

	int (*main)(slot_t*);
	char name[SHORT_STRING_SIZE];

	char desc[STRING_SIZE];
	time_t last_desc_updated;
	
	void *userptr;
};


typedef struct slotcontrol_t slotcontrol_t;
typedef union slotcontrol_message_t	slotcontrol_message_t;
struct slotcontrol_t {
	volatile int action;
#define STOP_ALL_PROCESSES		(1)
#define START_ALL_PROCESSES		(2)
#define RESTART_ALL_PROCESSES	(3)
#define DECREASE_PROCESS		(4)
#define INCREASE_PROCESS		(5)
#define LOADMODULE				(6)
#define FINISH_LOADMODULE		(7)
#define SETCONFIG				(8)
	union slotcontrol_message_t {
		struct pctrl {
			char scoreboard_name[SHORT_STRING_SIZE];
			int size;
		} p;
		struct mctrl {
			char module_name[SHORT_STRING_SIZE];
			char module_path[MAX_PATH_LEN];
		} m;
		struct scfg {
			char config_path[MAX_PATH_LEN];
		} c;
	} info;
};

#define SIGNAL_SHUTDOWN	SIGTERM
#define SIGNAL_GRACEFUL_SHUTDOWN SIGHUP
#define PROCESS_SCOREBOARD(size) \
	{TYPE_PROCESS, __FILE__, size, 0, NULL, 0, 0, 0, NULL, NULL}
#define THREAD_SCOREBOARD(size) \
	{TYPE_THREAD,  __FILE__, size, 0, NULL, 0, 0, 0, NULL, NULL}

struct scoreboard_t {
	int type;	/* scoreboard type */
	char *name; /* module name */
	int size;	/* number of child slots */
	int init;	/* initialize flag */

	scoreboard_core_t *core;
	volatile int shutdown;	/* flag to indicate shutdown mode */
	volatile int graceful_shutdown;	/* flag to indicate graceful shutdown mode */
	int period;		/* monitoring period in sec */

	/* slot[0] is parent's slot */
	slot_t *slot;

	slotcontrol_t *control;
};

/* function declaration *******************************************************/
SB_DECLARE(module *)find_module(const char *mod_name);
SB_DECLARE(void) set_static_modules(module *list[]);
SB_DECLARE(int) load_static_modules();
SB_DECLARE(int) load_dynamic_modules();
SB_DECLARE(module*) add_dynamic_module(const char *mod_struct_name, 
									   const char *modulename,
									   char registry_only);

SB_DECLARE(int) init_core_modules(module *start_module);
SB_DECLARE(int) init_standard_modules(module *start_module);

SB_DECLARE(void) list_static_modules(FILE *out);
SB_DECLARE(void) list_static_modules_str(char *result);
SB_DECLARE(void) list_modules(FILE *out);
SB_DECLARE(void) list_modules_str(char *result);
SB_DECLARE(void) list_config(FILE *out, char *module_name);
SB_DECLARE(void) list_config_str(char *result, char *module_name);

/* scoreboard stuff ***********************************************************/
SB_DECLARE(scoreboard_t*) init_one_scoreboard();
SB_DECLARE(void) init_all_scoreboards();
SB_DECLARE(slot_t*) get_slot_by_pid(scoreboard_t *scoreboard, pid_t pid);
SB_DECLARE(slot_t*) get_slot_by_name(scoreboard_t *scoreboard, const char *name);
SB_DECLARE(int)  is_working_slot(slot_t *slot);
SB_DECLARE(int)  get_pids_by_name(const char *name, pid_t pids[], int *size);
                 /* get_pids_by_name: returns number of processes for given name */
SB_DECLARE(slot_t*) get_slot_by_name_global(const char *name);
SB_DECLARE(void) set_proc_desc(slot_t *slt, const char *format, ...);
SB_DECLARE(void) list_scoreboard(FILE *out, char *module_name);
SB_DECLARE(void) list_scoreboard_xml(FILE *out, char *module_name);
SB_DECLARE(void) list_scoreboard_str(char *result, char *module_name);
SB_DECLARE(void) do_unittest();

#define DEFAULT_SERVER_PORT		8605
#define DEFAULT_SERVER_PORTSTR	"8605"
extern SB_DECLARE_DATA int gSoftBotListenPort;

SB_DECLARE(int) assignSoftBotPort(const char *modname, char module_portid);
SB_DECLARE(void) show_portinfo();

SB_DECLARE(int) read_config(char *configPath, module *start_module);
#endif
