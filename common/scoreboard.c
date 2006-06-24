/* $Id$ */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define CORE_PRIVATE 1
#include "common_core.h"
#include "ipc.h"
#include "log_error.h"
#include "modules.h"
#include "hook.h"
#include "setproctitle.h"
#include "scoreboard.h"


HOOK_STRUCT(
	HOOK_LINK(set_default_sighandlers)
	HOOK_LINK(init_scoreboard)

	HOOK_LINK(spawn_processes)
	HOOK_LINK(monitor_processes)

	HOOK_LINK(spawn_processes_for_each_module)
	HOOK_LINK(spawn_process_for_module)
	HOOK_LINK(monitor_processes_for_modules)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, set_default_sighandlers,\
		(void (*shutdown_handler)(int), void (*graceful_shutdown_handler)(int)),\
		(shutdown_handler,graceful_shutdown_handler),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, init_scoreboard, (scoreboard_t *s),(s),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_processes,\
		(scoreboard_t *s, const char *name, int (*main)(slot_t *)),(s, name, main),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, monitor_processes, (scoreboard_t *s),(s),DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_processes_for_each_module,\
		(scoreboard_t *s, module *mod),(s, mod),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, spawn_process_for_module,\
		(scoreboard_t *s, module *mod),(s, mod),DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, monitor_processes_for_modules,\
		(scoreboard_t *s, module *first_mod),(s, first_mod),DECLINE)


static char *slot_state_str[] = {
	"OPEN",
	"START",
	"WAIT",
	"READ",
	"PROCESS",
	"WRITE",
	"KEEPALIVE",
	"CLOSING",
	"FINISH"
};

//#define DEBUG_SCOREBOARD_MEMORY
ipc_t g_scoreboard_shm;

static void alloc_shm_for_scoreboard(int size)
{
	g_scoreboard_shm.type = IPC_TYPE_SHM;
	g_scoreboard_shm.pathname = NULL;
	/* guarantee no shm collision with client.
	 * NULL pathname means key IPC_PRIVATE.
	 * see ipc.c->alloc_shm() */
	g_scoreboard_shm.pid = SYS5_SCOREBOARD;
	g_scoreboard_shm.size = size;

	if ( size == 0 ) {
		warn("invalid scoreboard size: %d", size);
		g_scoreboard_shm.addr = NULL;
	}
	else if ( alloc_shm(&g_scoreboard_shm) != SUCCESS ) {
		// it's critical if error occured while getting shared memory.
		// so we exit.
		crit("error while allocating shm for scoreboard");
		exit(1);
	}

#ifdef DEBUG_SCOREBOARD_MEMORY
	debug("key file for shared memory:%s",gSoftBotRoot);
	debug("shared memory key:%d",g_scoreboard_shm.key);
	debug("shared memory shmid:%d",g_scoreboard_shm.id);
	debug("shared memory size:%ld",g_scoreboard_shm.size);
#endif

	/* memset(NULL) for newly created shm will be done by alloc_shm().
	 * you should not memset(NULL) if the shm is attached by another process. */
/*	memset(g_scoreboard_shm.addr, 0x00, size);*/
}

scoreboard_t* init_one_scoreboard(module *mod)
{
	void *current_addr;
	scoreboard_t *score=NULL;
	int i, mem_size = 0;

	if (mod->scoreboard == NULL) return NULL;

	score = mod->scoreboard;
	mem_size = sizeof(slotcontrol_t) + sizeof(slot_t) * (score->size + 1);

	alloc_shm_for_scoreboard(mem_size);
	current_addr = g_scoreboard_shm.addr;

	score->control = (slotcontrol_t*)current_addr;
	score->control->action = 0;
	current_addr += sizeof(slotcontrol_t);

	score->slot = (slot_t*)current_addr;
	score->init = 1; /* indicates initialized scoreboard */
	for (i = 0; i <= score->size; i++) {
		score->slot[i].id = i;
		score->slot[i].state = SLOT_OPEN;
		score->slot[i].pthread = -1;
	}
	return score;
}

void init_all_scoreboards()
{
	void *current_addr;
	module *mod;
	scoreboard_t *score=NULL;
	int i, mem_size = 0;

	/* 1. figure out needed memory size for scoreboards */
	for (mod=first_module; mod; mod=mod->next) {
		if (mod->scoreboard == NULL) continue;

		score = mod->scoreboard;
		mem_size += sizeof(slotcontrol_t) + sizeof(slot_t) * (score->size + 1);
		debug("mem_size = %d", mem_size);
	}

	if (mem_size == 0) {
		alert("shared memory for scoreboard is 0");
		alert("you really don't need scoreboard?");
		return ;
	}

	alloc_shm_for_scoreboard(mem_size);

	current_addr = g_scoreboard_shm.addr;
	/* 2. assign pointers to shared memory */
	for (mod=first_module; mod; mod=mod->next) {
		if (mod->scoreboard == NULL) continue;

		score = mod->scoreboard;
		score->control = (slotcontrol_t*)current_addr;
		score->control->action = 0;
		current_addr += sizeof(slotcontrol_t);

		score->slot = (slot_t*)current_addr;
		score->init = 1; /* indicates initialized scoreboard */
		for (i = 0; i <= score->size; i++) {
			score->slot[i].id = i;
			score->slot[i].state = SLOT_OPEN;
			score->slot[i].pthread = -1;
		}

		current_addr += sizeof(slot_t) * (score->size + 1);
	}
	return;
}

slot_t *get_slot_by_pid(scoreboard_t *s, pid_t pid)
{
	int i;

	if ( pid == 0 ) return NULL;

	for (i = 1; i <= s->size; i++) {
		if (s->slot[i].state == SLOT_OPEN) continue;
		if (s->slot[i].pid == pid)
			return &(s->slot[i]);
	}
	return NULL;
}

slot_t *get_slot_by_name(scoreboard_t *s, const char *name)
{
	int i;

	if ( name == NULL || strlen(name) == 0 ) return NULL;

	for (i = 1; i <= s->size; i++) {
		if (s->slot[i].state == SLOT_OPEN) continue;
		if (strncmp(s->slot[i].name, name, SHORT_STRING_SIZE) == 0)
			return &(s->slot[i]);
	}
	return NULL;
}

int is_working_slot(slot_t *slot)
{
	return (slot->state >= SLOT_START &&
			slot->state <= SLOT_RESTART);
}

/*XXX: returns number of processes for given name */
int get_pids_by_name(const char *name, pid_t pids[], int *size /* value return parameter */)
{
	module *m;
	scoreboard_t *scoreboard;
	int i;

	if ( *size <= 0 ) {
		warn("weird size value [%d]", *size);
	}

	for (m=first_module; m; m=m->next) {
		scoreboard = m->scoreboard;

		if (scoreboard && get_slot_by_name(scoreboard, name)) {
			int working_slotsize=0;

			for (i=0; i<scoreboard->size; i++) {
				if (is_working_slot(&scoreboard->slot[i+1])) {

					if (i < *size) 
						pids[i] = scoreboard->slot[i+1].pid;
					
					working_slotsize++;
				}
			}
			*size = working_slotsize;
			
			return working_slotsize;
		}
	}

	return -1;
}

slot_t *get_slot_by_name_global(const char *name)
{
	module *m=NULL;
	scoreboard_t *scoreboard=NULL;
	slot_t *slot=NULL;

	for (m=first_module; m && !slot; m=m->next) {
		scoreboard = m->scoreboard;

		if (scoreboard) 
			slot = get_slot_by_name(scoreboard, name);
	}

	return slot;
}

// FIXME see modules/mod_httpd/mod_httpd.c
/*
void update_slot_state(slot_t *slot, int state)
{
	slot->state = state;
}
*/


void set_proc_desc(slot_t *in_slt, const char *format, ... ){
	va_list msg;
	
	char buf[STRING_SIZE];
	module *m = NULL;
	slot_t *slt = in_slt;

	va_start(msg, format);
	memset(buf, 0, STRING_SIZE);
	vsnprintf(buf, STRING_SIZE, format, msg);
	va_end(msg);
	
	setproctitle(buf);

	if ( !slt ) {
		pid_t pid = getpid();
		for ( m=first_module; m && m->scoreboard; m=m->next ) {
			slt = get_slot_by_pid(m->scoreboard, pid);
			if ( slt ) break;
		}
	}
	if ( slt ) {
		// desc는 건드리지 않는 것으로 한다.
//		memcpy(slt->desc, buf, STRING_SIZE);
//		slt->last_desc_updated = time(NULL);
	}
}


void list_scoreboard(FILE *out, char *module_name)
{
	int i, printed = 0, list_all = 0;
	time_t now;
	module *m = NULL;
	scoreboard_t *s = NULL;

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		fprintf(out, "Scoreboard of all modules:\n");
	else
		fprintf(out, "Scoreboard of %s:\n", module_name);

	time(&now);
	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		s = m->scoreboard;

		if ( list_all == 1 ) {
			if ( s == NULL ) continue;
			fprintf(out, "%s\n",m->name);
		} else if ( s == NULL ) {
			fprintf(out, "  (no scoreboard)\n");
			continue;
		}

		/* ctime(3) returns string with '\n' */
		fprintf(out, " start time: %s", ctime(&(s->slot[0].start_time)));
		fprintf(out, " shutdown/graceful: %d/%d\n",
						s->shutdown, s->graceful_shutdown);
		fprintf(out, "  ID     PID GEN   SS  STATE    NAME\n");

		for (i = 0; i <= s->size; i++) {
			if ( s->slot[i].state == SLOT_OPEN )
				fprintf(out, " [%2d] %6d %2d %5ld  %-8s %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					0l, /* long int */
					slot_state_str[s->slot[i].state],
					s->slot[i].name);
			else if ( s->slot[i].state > SLOT_OPEN &&
					  s->slot[i].state <= SLOT_FINISH )
				fprintf(out, " [%2d] %6d %2d %5ld  %-8s %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					(long int) now - s->slot[i].start_time, /* long int */
					slot_state_str[s->slot[i].state],
					s->slot[i].name);
			else
				fprintf(out, " [%2d] %6d %2d %5ld  %-8d %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					(long int) now - s->slot[i].start_time, /* long int */
					s->slot[i].state,
					s->slot[i].name);
		}
	}
	if ( printed == 0 )
		fprintf(out, "  (no such module exists)\n");
	fflush(out);
}



void list_scoreboard_xml(FILE *out, char *module_name)
{
	int i, printed = 0, list_all = 0;
	time_t now;
	module *m = NULL;
	scoreboard_t *s = NULL;

	fprintf(out, "<?xml version=\"1.0\" encoding=\"euc-kr\" ?>\n"
				"<xml>\n" );

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		fprintf(out, "<!-- Scoreboard of all modules: -->\n");
	else
		fprintf(out, "<!-- Scoreboard of %s: -->\n", module_name);

	time(&now);
	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		s = m->scoreboard;

		if ( list_all == 1 ) {
			if ( s == NULL ) continue;
		} else if ( s == NULL ) {
			fprintf(out, "<!--  (module %s has no scoreboard) -->\n", m->name);
			continue;
		}
		fprintf(out, "<module name=\"%s\" >\n",m->name);

		/* ctime(3) returns string with '\n' */
		fprintf(out, "<!-- start time : %s --><start_time>%ld</start_time>", 
				ctime(&(s->slot[0].start_time)), (long int)s->slot[0].start_time);
		fprintf(out, "<shutdown>%d</shutdown><graceful>%d</graceful>\n",
						s->shutdown, s->graceful_shutdown);

		for (i = 0; i <= s->size; i++) {
			if ( s->slot[i].state == SLOT_OPEN )
				fprintf(out, "<slot id=\"%d\" >\n"
						"<pid>%d</pid>\n"
						"<generation>%d</generation>\n"
						"<!-- working for '%d hour %d min %d sec' -->\n"
						"<working_time>%ld</working_time>"
						"<state>%s</state>"
						"<name>%s</name>"
						"<description>%s</description>\n"
						"<!-- last description updated '%d hour %d min %d sec' ago -->\n"
						"<last_desc_updated>%ld</last_desc_updated>"
						"</slot>\n", 
						s->slot[i].id, s->slot[i].pid, s->slot[i].generation,
						0, 0, 0, 0l, 
						slot_state_str[s->slot[i].state],
						s->slot[i].name,
						s->slot[i].desc,
						(int)((now - s->slot[i].last_desc_updated)/3600), 
						(int)((now - s->slot[i].last_desc_updated) % 3600)/60, 
						(int)((now-s->slot[i].last_desc_updated) % 60),
						(long int)s->slot[i].last_desc_updated );
			else if ( s->slot[i].state > SLOT_OPEN &&
					  s->slot[i].state <= SLOT_FINISH )
				fprintf(out, "<slot id=\"%d\" >\n"
						"<pid>%d</pid>\n"
						"<generation>%d</generation>\n"
						"<!-- working for '%d hour %d min %d sec' -->\n"
						"<working_time>%ld</working_time>"
						"<state>%s</state>"
						"<name>%s</name>"
						"<description>%s</description>\n"
						"<!-- last description updated '%d hour %d min %d sec' ago -->\n"
						"<last_desc_updated>%ld</last_desc_updated>"
						"</slot>\n",
						s->slot[i].id, s->slot[i].pid, s->slot[i].generation,
						(int)((now - s->slot[i].start_time)/3600), (int)((now - s->slot[i].start_time) % 3600)/60,
						(int)((now - s->slot[i].start_time)%60), (long int)(now - s->slot[i].start_time),
						slot_state_str[s->slot[i].state],
						s->slot[i].name,
						s->slot[i].desc,
						(int)((now - s->slot[i].last_desc_updated)/3600), 
						(int)((now - s->slot[i].last_desc_updated) % 3600)/60, 
						(int)((now-s->slot[i].last_desc_updated) % 60),
						(long int)s->slot[i].last_desc_updated );
			else
				fprintf(out, "<slot id=\"%d\" >\n"
						"<pid>%d</pid>\n"
						"<generation>%d</generation>\n"
						"<!-- working for '%d hour %d min %d sec' -->\n"
						"<working_time>%ld</working_time>"
						"<state>%d</state>"
						"<name>%s</name>"
						"<description>%s</description>\n"
						"<!-- last description updated '%d hour %d min %d sec' ago -->\n"
						"<last_desc_updated>%ld</last_desc_updated>"
						"</slot>\n", 
						s->slot[i].id, s->slot[i].pid, s->slot[i].generation,
						(int)((now - s->slot[i].start_time)/3600), (int)((now - s->slot[i].start_time) % 3600)/60,
						(int)((now - s->slot[i].start_time)%60),
						(long int)(now - s->slot[i].start_time),
						s->slot[i].state,
						s->slot[i].name,
						s->slot[i].desc,
						(int)(now - s->slot[i].last_desc_updated)/3600, 
						(int)((now - s->slot[i].last_desc_updated) % 3600)/60, 
						(int)((now-s->slot[i].last_desc_updated) % 60), (long int)s->slot[i].last_desc_updated );
		}
		fprintf(out, "</module>\n");

	}
	if ( printed == 0 )
		fprintf(out, "<error>no such module exists</error>\n");

	fprintf(out, "</xml>");
	fflush(out);
}


void list_scoreboard_str(char *result, char *module_name)
{
	int i, printed = 0, list_all = 0;
	time_t now;
	module *m = NULL;
	scoreboard_t *s = NULL;
	char tmp[1024];

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		sprintf(result, "Scoreboard of all modules:\n");
	else
		sprintf(result, "Scoreboard of %s:\n", module_name);

	time(&now);
	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		s = m->scoreboard;

		if ( list_all == 1 ) {
			if ( s == NULL ) continue;
			sprintf(tmp, "%s\n",m->name);
			strcat(result, tmp);
		} else if ( s == NULL ) {
			strcat(result, "  (no scoreboard)\n");
			
			continue;
		}

		/* ctime(3) returns string with '\n' */
		sprintf(tmp, " start time: %s", ctime(&(s->slot[0].start_time)));
		strcat(result, tmp);
		sprintf(tmp, " shutdown/graceful: %d/%d\n",
						s->shutdown, s->graceful_shutdown);
		strcat(result, tmp);						
		strcat(result, "  ID     PID GEN   SS  STATE    NAME\n");

		for (i = 0; i <= s->size; i++) {
			if ( s->slot[i].state == SLOT_OPEN )
			{
				sprintf(tmp, " [%2d] %6d %2d %5ld  %-8s %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					0l, /* long int */
					slot_state_str[s->slot[i].state],
					s->slot[i].name);
				strcat(result, tmp);
			}
			else if ( s->slot[i].state > SLOT_OPEN &&
					  s->slot[i].state <= SLOT_FINISH )
			{					  
				sprintf(tmp, " [%2d] %6d %2d %5ld  %-8s %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					(long int) now - s->slot[i].start_time, /* long int */
					slot_state_str[s->slot[i].state],
					s->slot[i].name);
				strcat(result, tmp);
			}
			else
			{
				sprintf(tmp, " [%2d] %6d %2d %5ld  %-8d %s\n",
					s->slot[i].id,
					s->slot[i].pid,
					s->slot[i].generation,
					(long int) now - s->slot[i].start_time, /* long int */
					s->slot[i].state,
					s->slot[i].name);
				strcat(result, tmp);
			}
		}
	}
	if ( printed == 0 )
		strcat(result, "  (no such module exists)\n");

}

