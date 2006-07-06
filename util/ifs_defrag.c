/* $Id$ */
#include <stdlib.h> /* free(3) */
#include <string.h> /* strcasecmp(3),strncpy(3),memset(3) */
#include <dlfcn.h>
#include <errno.h>
#include "common_core.h"
#include "common_util.h"
#include "setproctitle.h"
#include "ipc.h"
#include "mod_ifs/table.h"
#include "mod_ifs/mod_ifs_defrag.h"
#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#endif

#define DEFAULT_PATH ""
static char path[MAX_PATH_LEN] = DEFAULT_PATH;
static char* optstring = "hfpg:m:t:sr:";

#ifndef HAVE_GETOPT_LONG
struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
#else
static struct option opts[] = {
	{ "help",            0, NULL, 'h' },
	{ "fix",             0, NULL, 'f' },
	{ "pause",           0, NULL, 'p' },
	{ "group-size",      1, NULL, 'g' },
	{ "defrag-mode",     1, NULL, 'm' },
	{ "temp-alive-time", 1, NULL, 't' },
	{ "show-state",      0, NULL, 's' },
	{ "softbot-root",    1, NULL, 'r' },
	{ NULL,              0, NULL, 0 }
};
#endif

static struct option_help {
	char *long_opt;
	char *short_opt;
	char *desc;
} opts_help[] = {
	{ "--help", "-h", "Display program usage" },
	{ "--fix", "-f", "Fix physical segment state (not execute defrag)" },
	{ "--pause", "-p", "Pause before defrag start" },
	{ "--group-size", "-g", "Set defrag group size (>1)" },
	{ "--defrag-mode", "-m", "Set defrag mode [bubble|copy]" },
	{ "--temp-alive-time", "-t", "Set alive time(second) of segment in state \"TEMP\"" },
	{ "--show-state", "-s", "Show ifs state and exit" },
	{ "--softbot-root", "-r", "Set gSoftBotRoot" },
	{ NULL, NULL, NULL }
};

static void show_usage(char* exec_name)
{
	struct option_help *h;

	printf("----------------------------------------------------------------------------\n");
	printf(" usage: %s [options] [path]\n", exec_name);
	printf("----------------------------------------------------------------------------\n");
	printf(" path : ifs path. default is \"%s\"\n", DEFAULT_PATH);
	for ( h = opts_help; h->long_opt; h++ ) {
#ifdef HAVE_GETOPT_LONG
		printf(" %s, %s:  %s\n", h->long_opt, h->short_opt, h->desc);
#else
		printf(" %s:  %s\n", h->short_opt, h->desc);
#endif
	}
	printf("----------------------------------------------------------------------------\n");
}

char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
char gErrorLogFile[MAX_PATH_LEN] = DEFAULT_ERROR_LOG_FILE;
char gQueryLogFile[MAX_PATH_LEN] = DEFAULT_QUERY_LOG_FILE;
module *static_modules;

int (*ifs_init_fp)();
int (*ifs_open_fp)(index_db_t** indexdb, int opt);
int (*ifs_close_fp)(index_db_t* indexdb);
int (*ifs_append_fp)(index_db_t* indexdb, int file_id, int size, void* buf);
int (*ifs_read_fp)(index_db_t* indexdb, int file_id, int offset, int size, void* buf);
int (*ifs_getsize_fp)(index_db_t* indexdb, int file_id);
int (*_ifs_fix_physical_segment_state_fp)(ifs_t* ifs);
void (*table_print_fp)(table_t* table);
int (*ifs_defrag_fp)(ifs_t* ifs, scoreboard_t* scoreboard);

ifs_set_t **ifs_set_p;
int* defrag_group_size_p;
defrag_mode_t* defrag_mode_p;
int* temp_alive_time_p;

#define DLOPEN(handle, name) \
	snprintf(libname, sizeof(libname), "%s/lib/softbot/" name, gSoftBotRoot); \
	handle = dlopen(libname, RTLD_NOW|RTLD_GLOBAL); /* flag는 국가보훈처때처럼.. */\
	if ( handle == NULL ) { \
		error("cannot load %s/lib/softbot/" name ": %s", gSoftBotRoot, strerror(errno)); \
		goto end; \
	}

#define DLSYM(handle, ptr, name) \
	ptr = dlsym(handle, name); \
	if ( ptr == NULL ) { \
		error("cannot load symbol" name ": %s", strerror(errno)); \
		goto end; \
	}

int main(int argc, char* argv[], char* envp[])
{
	void *mod_sfs=NULL, *mod_ifs=NULL, *mod_ifs_defrag=NULL;
	index_db_t* indexdb = NULL;
	ifs_t* ifs;
	int arg;
	int exit_value = 0;
	char* defrag_mode_str[] = { "BUBBLE", "COPY" };
	int pause = 0, show_state = 0, fix = 0;
    ifs_set_t local_ifs_set[MAX_INDEXDB_SET];

	init_setproctitle(argc, argv, envp);
	log_setlevelstr("debug");

	// load dynamic library
	char libname[MAX_PATH_LEN];
	DLOPEN(mod_sfs, "mod_sfs.so");
	DLOPEN(mod_ifs, "mod_ifs.so");
	DLOPEN(mod_ifs_defrag, "mod_ifs_defrag.so");

	DLSYM(mod_ifs, ifs_init_fp, "ifs_init");
	DLSYM(mod_ifs, ifs_open_fp, "ifs_open");
	DLSYM(mod_ifs, ifs_close_fp, "ifs_close");
	DLSYM(mod_ifs, ifs_append_fp, "ifs_append");
	DLSYM(mod_ifs, ifs_read_fp, "ifs_read");
	DLSYM(mod_ifs, ifs_append_fp, "ifs_append");
	DLSYM(mod_ifs, ifs_getsize_fp, "ifs_getsize");
	DLSYM(mod_ifs, _ifs_fix_physical_segment_state_fp, "_ifs_fix_physical_segment_state");
	DLSYM(mod_ifs, table_print_fp, "table_print");
	DLSYM(mod_ifs_defrag, ifs_defrag_fp, "ifs_defrag");
	DLSYM(mod_ifs, ifs_set_p, "ifs_set");
	DLSYM(mod_ifs_defrag, defrag_group_size_p, "defrag_group_size");
	DLSYM(mod_ifs_defrag, defrag_mode_p, "defrag_mode");
	DLSYM(mod_ifs, temp_alive_time_p, "temp_alive_time");

	// make ifs_set
	memset(local_ifs_set, 0x00, sizeof(local_ifs_set));
	local_ifs_set[0].set = 1;
	local_ifs_set[0].set_segment_size = 1;
	local_ifs_set[0].segment_size = 0;
	local_ifs_set[0].set_block_size = 1;
	local_ifs_set[0].block_size = 0;
	*ifs_set_p = local_ifs_set;

	opterr = 0;
	while ( ( arg =
#ifdef HAVE_GETOPT_LONG
			getopt_long(argc, argv, optstring, opts, NULL)
#elif HAVE_GETOPT
			getopt(argc, argv, optstring)
#else
#	error at least one of getopt, getopt_long must exist.
#endif
		) != -1 )
	{
		switch( arg ) {
			case 'h':
				show_usage( argv[0] );
				return 0;
				break;

			case 'f':
				fix++;
				break;

			case 'p':
				pause++;
				break;

			case 'g':
				*defrag_group_size_p = atoi(optarg);
				if ( *defrag_group_size_p <= 1 ) {
					error("invalid group size: %s", optarg);
					exit_value = -1;
				}
				break;

			case 'm':
				if ( strcasecmp( optarg, "copy" ) == 0 )
					*defrag_mode_p = DEFRAG_MODE_COPY;
				else if ( strcasecmp( optarg, "bubble" ) == 0 )
					*defrag_mode_p = DEFRAG_MODE_BUBBLE;
				else {
					warn("unknown defrag mode: %s", optarg);
					exit_value = -1;
				}
				break;

			case 't':
				*temp_alive_time_p = atoi(optarg);
				if ( *temp_alive_time_p < 0 ) {
					error("invalid temp_alive_time: %s", optarg);
					exit_value = -1;
				}
				break;

			case 's':
				show_state++;
				break;

			case 'r':
				snprintf( gSoftBotRoot, MAX_PATH_LEN, "%s", optarg);
				break;

			case '?':
				warn("unknown option: %c", (char)optopt);
				exit_value = -1;
				break;

			default:
				error("unknown getopt return value: 0x%2x[%c]", arg, (char) arg);
				break;
		}
	} // while ( arg ... )

	if ( optind < argc ) {
		strncpy( path, argv[optind], sizeof(path) );
	}
	else {
		error("need path");
		exit_value = -1;
	}

	if ( exit_value != 0 ) {
		show_usage( argv[0] );
		return exit_value;
	}

	local_ifs_set[0].set_ifs_path = 1;
	strncpy( local_ifs_set[0].ifs_path, path, MAX_PATH_LEN-1 );

	printf("Defrag Enviroment\n");
	printf("===========================================\n");
	printf("gSoftBotRoot    : %s\n", gSoftBotRoot);
	printf("Path            : %s\n", path);
	printf("TempAliveTime   : %d\n", *temp_alive_time_p);
	printf("DefragGroupSize : %d\n", *defrag_group_size_p);
	printf("DefragMode      : %s\n", defrag_mode_str[(int)(*defrag_mode_p)]);
	printf("===========================================\n");

	if ( pause ) {
		printf("press any key to start defrag...");
		getchar();
	}

	ifs_init_fp();
	if ( ifs_open_fp(&indexdb, 0) != SUCCESS ) {
		error("ifs_open failed");
		exit_value = -1;
		goto end;
	}
	ifs = (ifs_t*) indexdb->db;

	table_print_fp( &ifs->shared->mapping_table );

	if ( fix ) {
		if ( _ifs_fix_physical_segment_state_fp( ifs ) > 0 ) {
			crit("table fixed");
			table_print_fp( &ifs->shared->mapping_table );
		}
		goto end;
	}

	if ( show_state ) {
		char buf[1];
		ifs_read_fp(indexdb, 1, 100000000, 1, buf);
		// 이렇게 하면 모든 sfs가 loading된다
	}
	else if ( ifs_defrag_fp(ifs, NULL) != SUCCESS ) {
	    error("defragment fail");
		exit_value = -1;
	}

end:
	if ( indexdb != NULL ) ifs_close_fp(indexdb);
	free_ipcs();

	if ( mod_ifs_defrag ) dlclose(mod_ifs_defrag);
	if ( mod_ifs ) dlclose(mod_ifs);
	if ( mod_sfs ) dlclose(mod_sfs);

	return exit_value; 
}

