/* $Id$ */
#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

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
#ifdef HAVE_PTHREAD_H
	pthread_t pthread;
#endif
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

#endif
