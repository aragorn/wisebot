#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_vrfi/mod_vrfi.h"
#include "mod_qp/mod_qp.h"
#include "mod_api/indexdb.h"
#include "mod_api/lexicon.h"
#include "hit.h"

#define MAX_PROCESSES         10
#define MONITORING_PERIOD     5

#define TYPE_VRFI             1
#define TYPE_INDEXDB          2

static scoreboard_t scoreboard[] = { PROCESS_SCOREBOARD(MAX_PROCESSES+1) };
static int m_processes   = 1;
static int m_type        = TYPE_INDEXDB;
static int m_word_db_set = -1;

/******************************************************
 * signal handler
 ******************************************************/

static void _do_nothing(int sig)
{
	return;
}

static void _shutdown(int sig)
{
	struct sigaction act;
	
	memset(&act, 0x00, sizeof(act));

//	act.sa_flags = SA_RESTART;
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

//	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}

/*********************************************************
 * vrfi
 *********************************************************/

static char m_index_db_path[MAX_PATH_LEN] = "dat/indexer/index";

/*********************************************************
 * indexdb
 *********************************************************/

static int m_indexdb_set = -1;

/*********************************************************
 * main functions
 *********************************************************/

static int init()
{
	return SUCCESS;
}

#define RETURN_ERROR(format, ...) \
	error( format, ##__VA_ARGS__ ); \
	slot->state = SLOT_FINISH; \
	return -1;

static int vrfi_child_main(slot_t* slot)
{
	uint32_t start, end, current, expected_count, vrfi_count;
	word_db_t* word_db;
	VariableRecordFile *vrfi;
	word_t word;
	doc_hit_t *doc_hits;

	time_t start_time, old_time, current_time;
	uint32_t old_current;

	slot->state = SLOT_PROCESS;

	if ( sb_run_open_word_db( &word_db, m_word_db_set ) != SUCCESS ) {
		RETURN_ERROR( "word db[set:%d] open failed", m_word_db_set );
	}

	if ( sb_run_vrfi_alloc( &vrfi ) != SUCCESS ) {
		RETURN_ERROR( "vrfi alloc failed: %s", strerror(errno) );
	}

	if ( sb_run_vrfi_open( vrfi, m_index_db_path,
			sizeof(inv_idx_header_t), sizeof(doc_hit_t), O_RDONLY ) != SUCCESS ) {
		RETURN_ERROR( "vrfi open failed: %s", strerror(errno) );
	}

	if ( sb_run_get_num_of_word( word_db, &end ) != SUCCESS ) {
		RETURN_ERROR( "get_num_of_wordid failed: %s", strerror(errno) );
	}

	doc_hits = (doc_hit_t*) sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
	if ( doc_hits == NULL ) {
		RETURN_ERROR( "sb_malloc failed: %s", strerror(errno) );
	}

	start = slot->id;
	current = start;

	crit("[%u~%u] check start", start, end);

	old_current = start;
	old_time = time(NULL);
	start_time = old_time;

	for( current = start; current <= end; current+=m_processes ) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			break;

		current_time = time(NULL);
		if ( current_time - old_time >= 5 ) {
			int percentage, speed, remain_time;

			percentage = (current-start)*100/(end-start+1);
			speed = (current-old_current)/(current_time-old_time);
			remain_time = (speed)?(end-current+1)/speed:(-1);

			setproctitle( "softbotd: [%u]/[%u~%u] %d%%, %dw/s, %dsec remain",
					current, start, end, percentage, speed, remain_time );

			old_current = current;
			old_time = current_time;
		}

		word.id = current;

		if ( sb_run_get_word_by_wordid( word_db, &word ) != SUCCESS ) {
			error("lexicon failed to get word");
			continue;
		}

		if ( sb_run_vrfi_get_num_of_data( vrfi, current, &expected_count ) != SUCCESS ) {
			error("word[%u, %s]: vrfi_get_num_of_data failed", current, word.string);
			continue;
		}

		if ( expected_count > MAX_DOC_HITS_SIZE ) {
			warn("word[%u, %s]: expected_count[%u] > MAX_DOC_HITS_SIZE[%u]",
					current, word.string, expected_count, MAX_DOC_HITS_SIZE);
			expected_count = MAX_DOC_HITS_SIZE;
		}
		else if ( expected_count == 0 ) {
			warn("word[%u, %s]: count is 0", current, word.string);
			continue;
		}

		vrfi_count = sb_run_vrfi_get_variable( vrfi, current, 0L,
											expected_count, (void*)doc_hits );
		if ( (int)vrfi_count < 0 ) {
			error("word[%u, %s]: vrfi_get_variable failed", current, word.string);
			continue;
		}

		if ( expected_count != vrfi_count ) {
			error("word[%u, %s]: expected_count[%u] != vrfi_count[%u]",
					current, word.string, expected_count, vrfi_count);
		}
		else info("word[%u, %s]: ok [%u]", current, word.string, vrfi_count);
	} // for (current)

	if ( current > end ) crit("[%u~%u] check finished", start, end);
	else crit("[%u~%u] check aborted[%u]", start, end, current);

	sb_run_vrfi_close( vrfi );
	sb_run_close_word_db( word_db );

	slot->state = SLOT_FINISH;
	return EXIT_SUCCESS;
}

static int indexdb_child_main(slot_t* slot)
{
	uint32_t start, end, current, expected_count;
	int indexdb_count, indexdb_size;
	word_db_t* word_db;
	index_db_t* indexdb;
	word_t word;
	doc_hit_t *doc_hits;

	time_t start_time, old_time, current_time;
	uint32_t old_current;

	slot->state = SLOT_PROCESS;

	if ( sb_run_open_word_db( &word_db, m_word_db_set ) != SUCCESS ) {
		RETURN_ERROR( "word db[set:%d] open failed", m_word_db_set );
	}

	if ( sb_run_indexdb_open( &indexdb, m_indexdb_set ) != SUCCESS ) {
		RETURN_ERROR( "indexdb load failed: %s", strerror(errno) );
	}

	if ( sb_run_get_num_of_word( word_db, &end ) != SUCCESS ) {
		RETURN_ERROR( "get_num_of_wordid failed: %s", strerror(errno) );
	}

	doc_hits = (doc_hit_t*) sb_calloc(MAX_DOC_HITS_SIZE, sizeof(doc_hit_t));
	if ( doc_hits == NULL ) {
		RETURN_ERROR( "sb_malloc failed: %s", strerror(errno) );
	}

	start = slot->id;
	current = start;

	crit("[%u~%u] check start", start, end);

	old_current = start;
	old_time = time(NULL);
	start_time = old_time;

	for( current = start; current <= end; current+=m_processes ) {
		if ( scoreboard->shutdown || scoreboard->graceful_shutdown )
			break;

		current_time = time(NULL);
		if ( current_time - old_time >= 5 ) {
			int percentage, speed, remain_time;

			percentage = (current-start)*100/(end-start+1);
			speed = (current-old_current)/(current_time-old_time);
			remain_time = (speed)?(end-current+1)/speed:(-1);

			setproctitle( "softbotd: [%u]/[%u~%u] %d%%, %dw/s, %dsec remain",
					current, start, end, percentage, speed, remain_time );

			old_current = current;
			old_time = current_time;
		}

		word.id = current;

		if ( sb_run_get_word_by_wordid( word_db, &word ) != SUCCESS ) {
			error("lexicon failed to get word");
			continue;
		}

		indexdb_size = sb_run_indexdb_getsize( indexdb, current );
		expected_count = indexdb_size / sizeof(doc_hit_t);

		if ( expected_count < 0 ) {
			error("word[%u, %s]: indexdb_getsize failed", current, word.string);
			continue;
		}

		if ( indexdb_size % sizeof(doc_hit_t) != 0 ) {
			error("word[%u, %s]: invalid size[%d]", current, word.string, indexdb_size);
		}

		if ( expected_count > MAX_DOC_HITS_SIZE ) {
			warn("word[%u, %s]: expected_count[%u] > MAX_DOC_HITS_SIZE[%u]",
					current, word.string, expected_count, MAX_DOC_HITS_SIZE);
			expected_count = MAX_DOC_HITS_SIZE;
		}
		else if ( expected_count == 0 ) {
			warn("word[%u, %s]: count is 0", current, word.string);
			continue;
		}

		indexdb_size = sb_run_indexdb_read( indexdb, current, 0,
									expected_count*sizeof(doc_hit_t), (void*)doc_hits );
		indexdb_count = indexdb_size / sizeof(doc_hit_t);

		if ( indexdb_size < 0 ) {
			error("word[%u, %s]: vrfi_get_variable failed", current, word.string);
			continue;
		}

		if ( expected_count != indexdb_count ) {
			error("word[%u, %s]: expected_count[%u] != indexdb_count[%u]",
					current, word.string, expected_count, indexdb_count);
		}
		else info("word[%u, %s]: ok [%u]", current, word.string, indexdb_count);
	} // for (current)

	if ( current > end ) crit("[%u~%u] check finished", start, end);
	else crit("[%u~%u] check aborted[%u]", start, end, current);

	sb_run_indexdb_close( indexdb );
	sb_run_close_word_db( word_db );

	slot->state = SLOT_FINISH;
	return EXIT_SUCCESS;
}

static int module_main(slot_t* slot)
{
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown);

	scoreboard->size = m_processes;
	sb_init_scoreboard(scoreboard);

	switch( m_type ) {
		case TYPE_VRFI:
			sb_spawn_processes(scoreboard, "vrfi data check process", vrfi_child_main);
			break;
		case TYPE_INDEXDB:
			sb_spawn_processes(scoreboard, "indexdb data check process", indexdb_child_main);
			break;
		default:
			error( "error in m_type: %d", m_type );
	}

	scoreboard->period = MONITORING_PERIOD;
	sb_monitor_processes(scoreboard);

	return 0;
}

/********************************************************
 * config
 ********************************************************/
static void set_processes(configValue v)
{
	m_processes = atoi(v.argument[0]);

	if ( m_processes > MAX_PROCESSES ) {
		warn("value[%s] is too big. assign %d to Processes", v.argument[0], MAX_PROCESSES);
		m_processes = MAX_PROCESSES;
	}
	else if ( m_processes <= 0 ) {
		warn("value[%s] is invalid. assign 1 to Processes", v.argument[0]);
	}
}

static void set_type(configValue v)
{
	if ( strcasecmp( v.argument[0], "vrfi" ) == 0 ) m_type = TYPE_VRFI;
	else if ( strcasecmp( v.argument[0], "indexdb" ) == 0 ) m_type = TYPE_INDEXDB;
	else error( "unknown Type value (must be \"vrfi\" or \"indexdb\")" );
}

static void set_word_db_set(configValue v)
{
	m_word_db_set = atoi( v.argument[0] );
}

static void set_index_db_path(configValue v)
{
	strncpy(m_index_db_path, v.argument[0], sizeof(m_index_db_path));
	m_index_db_path[sizeof(m_index_db_path)-1] = '\0';
}

static void set_indexdb_set(configValue v)
{
	m_indexdb_set = atoi( v.argument[0] );
}

static config_t config[] = {
	CONFIG_GET("Processes", set_processes, 1, "Number of Check Process"),
	CONFIG_GET("Type", set_type, 1, "vrfi or indexdb"),
	CONFIG_GET("WordDbSet", set_word_db_set, 1, "WordDbSet {number}"),

	CONFIG_GET("IndexDbPath", set_index_db_path, 1,
			"inv indexer db path (e.g: IndexDbPath dat/indexer/index)"),
	CONFIG_GET("IndexDbSet", set_indexdb_set, 1, "e.g> IndexDbSet 1"),
	{NULL}
};

module index_check_module = {
	STANDARD_MODULE_STUFF,
	config,                 /* config */
	NULL,                   /* registry */
	init,                   /* initialize */
	module_main,            /* child_main */
	scoreboard,             /* scoreboard */
	NULL,                   /* register hook api */
};

