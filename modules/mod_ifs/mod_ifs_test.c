#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_ifs.h"

#define MAX_PROCESSES (20)
#define MONITORING_PERIOD (2)

static scoreboard_t read_scoreboard[] = { PROCESS_SCOREBOARD(MAX_PROCESSES) };
static scoreboard_t append_scoreboard[] = { PROCESS_SCOREBOARD(MAX_PROCESSES) };

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

	read_scoreboard->shutdown++;
	append_scoreboard->shutdown++;
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

	read_scoreboard->graceful_shutdown++;
	append_scoreboard->graceful_shutdown++;
}

/*******************************************
 * global variable & functions
 *******************************************/

static int indexdb_set = -1;

static int read_process = 1;
static int append_process = 1;
static int append_loop = 50000;

static int file_count = 2000;

/*********** config functions ************/

static void set_indexdb_set(configValue a)
{
	indexdb_set = atoi( a.argument[0] );
}

static void set_file_count(configValue a)
{
	file_count = atoi( a.argument[0] );
}

static void set_read_process(configValue v)
{
	read_process = atoi(v.argument[0]);
}

static void set_append_process(configValue v)
{
	append_process = atoi(v.argument[0]);
	if ( append_process > 1 ) {
		warn("not supported value of append_process[%d]", append_process);
		append_process = 1;
	}
}

static void set_append_loop(configValue v)
{
	append_loop = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("IndexDbSet", set_indexdb_set, 1, "indexdb set"),

	CONFIG_GET("ReadProcess", set_read_process, 1, "process count"),
	CONFIG_GET("AppendProcess", set_append_process, 1, "process count"),
	CONFIG_GET("AppendLoop", set_append_loop, 1, "max loop count of append process"),

	CONFIG_GET("FileCount", set_file_count, 1, "set file count ( or max file number )"),
	{NULL}
};

static int random_number(int min, int max)
{
	return ( rand() % (max-min+1) ) + min;
}

/*******************************************
 * read test
 *******************************************/

#define BUFFER_SIZE 1024*1024

static int read_main(slot_t *slot)
{
	ifs_t *ifs;
	int file_id = 1, file_size, current_number, read_size;
	int i, errcnt = 0, retry = 0;
	int *buffer;
	time_t start, end;

	srand( time(NULL) + slot->id );
	buffer = sb_malloc( sizeof(int) * BUFFER_SIZE );
	if ( buffer == NULL ) {
		crit("unable to allocate memory");
		return -1;
	}

	ifs = ifs_create();
	if ( ifs_open( ifs, indexdb_set ) != SUCCESS ) {
		error( "ifs_open failed." );
		return -1;
	}

	while (1) {
		if ( read_scoreboard->shutdown || read_scoreboard->graceful_shutdown ) break;

		if ( !retry ) file_id = random_number( 1, file_count );
		else retry = 0;
		current_number = file_id;

		read_size = 0;
		file_size = 0;

		do {
			memset( buffer, -1, BUFFER_SIZE * sizeof(int) );

			time(&start);
			read_size = ifs_read( ifs, file_id, read_size, BUFFER_SIZE * sizeof(int), buffer );
			time(&end);

			if ( end - start > 5 ) {
				warn("slot[%d] file[%d] time[%d] is too long", slot->id, file_id, (int)(end-start));
			}

			if ( read_size == -2 ) {
				info("slot[%d] file[%d] not exists", slot->id, file_id);
				break; // 파일없음
			}
			else if ( read_size < 0 ) {
				error("slot[%d] file[%d] cannot read", slot->id, file_id);
				sleep(1);
				break;
			}

			file_size += read_size;

			for ( i = 0; i < BUFFER_SIZE && i < read_size/sizeof(int); i++ ) {
				if ( buffer[i] != current_number ) {
					error("slot[%d] file[%d] invalid value[%d], expected[%d], read_size[%d]",
							slot->id, file_id, buffer[i], current_number, read_size);
					if ( ++errcnt > 10 ) {
						int physical_count, start_segment, start_offset;
						int physical_segment_array[MAX_LOGICAL_COUNT];

						error("slot[%d] file[%d] test stop. too many errors", slot->id, file_id);
						errcnt = 0;
						retry++;

						/** 해당 segment 정보 읽기 **/
						physical_count = table_get_read_segments(
								&ifs->shared->mapping_table, physical_segment_array );
						__get_start_segment( ifs, physical_segment_array, physical_count,
								file_id, i, &start_segment, &start_offset);

						__view_sfs_info( ifs->local.sfs[physical_segment_array[start_segment]] );

						sleep(5);
						break;
					}
				}
				current_number++;
			}
		} while( read_size == BUFFER_SIZE );

		if ( errcnt == 0 && retry == 0 )
			info("slot[%d] reading test ok. file[%d], size[%d]", slot->id, file_id, file_size);
		else {
			table_print( &ifs->shared->mapping_table );
		}

		if ( retry > 5 ) retry = 0;
	} // while (1)

	ifs_close( ifs );
	ifs_destroy( ifs );
	sb_free( buffer );

	return EXIT_SUCCESS;
}

static int read_module_main(slot_t *slot)
{
	if ( read_process <= 0 ) {
		warn("read_process is [%d]. exit now", read_process);
		return 0;
	}

	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	read_scoreboard->size = read_process;

	sb_init_scoreboard(read_scoreboard);
	sb_spawn_processes(read_scoreboard, "[IFS] reading test", read_main);

	read_scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(read_scoreboard);

	return 0;
}

/*********** module structure ************/

module ifs_read_test_module = {
	STANDARD_MODULE_STUFF,
	config,                 /* config */
	NULL,                   /* registry */
	NULL,                   /* initialize */
	read_module_main,       /* child_main */
	read_scoreboard,        /* scoreboard */
	NULL                    /* register hook api */
};

/*******************************************
 * append test
 *******************************************/

static int *append_file_array = NULL;

static int append_init()
{
	int i;

	append_file_array = (int*) sb_malloc( (file_count+1) * sizeof(int) );
	if ( append_file_array == NULL ) return FAIL;

	for ( i = 0; i <= file_count; i++ ) {
		append_file_array[i] = i-1;
	}

	return SUCCESS;
}

static int append_main(slot_t *slot)
{
	ifs_t *ifs;
	int file_id, current_size, current_number;
	int errcnt = 0, written_size, i;
	static int *buffer = NULL;
	static int buffer_size = 0;

	srand( time(NULL) + slot->id );

	ifs = ifs_create();
	if ( ifs_open( ifs, indexdb_set ) != SUCCESS ) {
		error( "ifs_open failed." );
		return -1;
	}

	// 처음 4byte는 써놓고 시작하자.
	for ( i = 1; i <= file_count; i++ ) {
//		info("init %d", i);

		written_size = ifs_append( ifs, i, sizeof(int), &i );
		if ( written_size != sizeof(int) ) {
			error("init failed. file[%d]", i);
		}
		else append_file_array[i] = i;
	}

	while ( append_loop-- ) {
		if ( append_scoreboard->shutdown || append_scoreboard->graceful_shutdown ) break;

		file_id = random_number( 1, file_count );
		current_number = append_file_array[file_id] + 1;
		current_size = random_number( 1, 1024*20 );

		info("appending test... file[%d], size[%d], remains[%d]",
				file_id, current_size, append_loop);
		if ( append_loop % 100 == 0 )
			setproctitle( "softbotd: [IFS] appending test... [%d]", append_loop );

		if ( buffer == NULL || buffer_size < current_size * sizeof(int) ) {
			buffer = (int*) sb_realloc( buffer, current_size * sizeof(int) );
			if ( buffer == NULL ) {
				crit("unable to allocate memory");
				return -1;
			}
			else buffer_size = current_size * sizeof(int);
		}

		for ( i = 0; i < current_size; i++ ) {
			buffer[i] = current_number++;
		}

		written_size = ifs_append( ifs, file_id, current_size * sizeof(int), buffer );
		if ( written_size < 0 ) {
			error("unable to append. file[%d], current_size[%d], current_number[%d]",
					file_id, current_size, current_number);
			if ( ++errcnt > 10 ) goto error_return;

			sleep(1);
			continue;
		}

		if ( written_size != current_size * sizeof(int) ) {
			error("invalid written size[%d], current_size[%d], sizeof(int): %d, file_id[%d]",
					written_size, current_size, (int)sizeof(int), file_id);

			if ( ++errcnt > 10 ) goto error_return;
		}

		append_file_array[file_id] += written_size/sizeof(int);
	}

	ifs_close( ifs );
	ifs_destroy( ifs );
	sb_free( buffer );

	return EXIT_SUCCESS;

error_return:
	crit("too many errors... [%d]", errcnt);
	sb_free( buffer );
	return -1;
}

static int append_module_main(slot_t *slot)
{
	if ( append_process <= 0 ) {
		warn("append_process is [%d]. exit now", append_process);
		return 0;
	}

	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	append_scoreboard->size = append_process;

	sb_init_scoreboard(append_scoreboard);
	sb_spawn_processes(append_scoreboard, "[IFS] appending test", append_main);

	append_scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(append_scoreboard);

	return 0;
}

module ifs_append_test_module = {
	STANDARD_MODULE_STUFF,
	config,                 /* config */
	NULL,                   /* registry */
	append_init,            /* initialize */
	append_module_main,     /* child_main */
	append_scoreboard,      /* scoreboard */
	NULL                    /* register hook api */
};

