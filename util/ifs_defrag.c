#include "../modules/mod_ifs/table.h"
#include "../modules/mod_ifs/mod_ifs_defrag.h"

#define DEFAULT_PATH ""
static char path[MAX_PATH_LEN] = DEFAULT_PATH;
static char* optstring = "hpg:m:t:sr:";

#ifndef HAVE_GETOPT_LONG
struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
#endif

static struct option opts[] = {
	{ "help",            0, NULL, 'h' },
	{ "pause",           0, NULL, 'p' },
	{ "group-size",      1, NULL, 'g' },
	{ "defrag-mode",     1, NULL, 'm' },
	{ "temp-alive-time", 1, NULL, 't' },
	{ "show-state",      0, NULL, 's' },
	{ "softbot-root",    1, NULL, 'r' },
	{ NULL,              0, NULL, 0 }
};

static struct option_help {
	char *long_opt;
	char *short_opt;
	char *desc;
} opts_help[] = {
	{ "--help", "-h", "Display program usage" },
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

int main(int argc, char* argv[], char* envp[])
{
	index_db_t* indexdb;
	ifs_t* ifs;
	int arg;
	int exit_value = 0;
	char* defrag_mode_str[] = { "BUBBLE", "COPY" };
	int pause = 0, show_state = 0;
    ifs_set_t local_ifs_set[MAX_INDEXDB_SET];

	init_set_proc_title(argc, argv, envp);
	log_setlevelstr("debug");

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

			case 'p':
				pause++;
				break;

			case 'g':
				defrag_group_size = atoi(optarg);
				if ( defrag_group_size <= 1 ) {
					error("invalid group size: %s", optarg);
					exit_value = -1;
				}
				break;

			case 'm':
				if ( strcasecmp( optarg, "copy" ) == 0 )
					defrag_mode = DEFRAG_MODE_COPY;
				else if ( strcasecmp( optarg, "bubble" ) == 0 )
					defrag_mode = DEFRAG_MODE_BUBBLE;
				else {
					warn("unknown defrag mode: %s", optarg);
					exit_value = -1;
				}
				break;

			case 't':
				temp_alive_time = atoi(optarg);
				if ( temp_alive_time < 0 ) {
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

	printf("Defrag Enviroment\n");
	printf("===========================================\n");
	printf("gSoftBotRoot    : %s\n", gSoftBotRoot);
	printf("Path            : %s\n", path);
	printf("TempAliveTime   : %d\n", temp_alive_time);
	printf("DefragGroupSize : %d\n", defrag_group_size);
	printf("DefragMode      : %s\n", defrag_mode_str[(int)defrag_mode]);
	printf("===========================================\n");

	if ( pause ) {
		printf("press any key to start defrag...");
		getchar();
	}

	// make ifs_set
	memset(local_ifs_set, 0x00, sizeof(local_ifs_set));
	ifs_set = local_ifs_set;
	ifs_set[0].set = 1;
	ifs_set[0].set_ifs_path = 1;
	strncpy( ifs_set[0].ifs_path, path, MAX_PATH_LEN-1 );
	ifs_set[0].set_segment_size = 1;
	ifs_set[0].segment_size = 0;
	ifs_set[0].set_block_size = 1;
	ifs_set[0].block_size = 0;

	ifs_init();
	if ( ifs_open(&indexdb, 0) != SUCCESS ) {
		error("ifs_open failed");
		exit_value = -1;
		goto end;
	}
	ifs = (ifs_t*) indexdb->db;

	table_print( &ifs->shared->mapping_table );

    if ( !show_state && ifs_defrag(ifs, NULL) != SUCCESS ) {
	    error("defragment fail");
		exit_value = -1;
	}

end:
	ifs_close(indexdb);
	free_ipcs();

	return exit_value; 
}

