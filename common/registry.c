/* $Id$ */
#include <stdlib.h>
#include <search.h> /* for hash table function like hcreate.. */
#include <string.h>
#include <errno.h>
#include <time.h>
#include "common_core.h"
#include "ipc.h"
#include "modules.h"
#include "log_error.h"
#include "registry.h"

#undef DEBUG_REGISTRY_MEMORY

char gRegistryFile[MAX_PATH_LEN] = DEFAULT_REGISTRY_FILE;
static ipc_t registry_shm;

static char *bin2hex(char *s, void *bin, int size);
static void *hex2bin(void *bin, char *s, int size);

static void alloc_shm_for_registry(int size) {
	registry_shm.type = IPC_TYPE_SHM;
	registry_shm.pathname = NULL;
    /* guarantee no shm collision with client.
	 * NULL pathname means key IPC_PRIVATE.
	 * see ipc.c->alloc_shm() */
	registry_shm.pid = SYS5_REGISTRY;
	registry_shm.size = size;
	if ( alloc_shm(&registry_shm) != SUCCESS ) {
		exit(1);
	}

#ifdef DEBUG_REGISTRY_MEMORY
	debug("key file for shared memory:%s",gSoftBotRoot);
	debug("shared memory key:%d",registry_shm.key);
	debug("shared memory shmid:%d",registry_shm.id);
	debug("shared memory size:%ld",registry_shm.size);
#endif

}

static int count_total_registry_size()
{
	int total_size = 0;
	int alignment_size = 8;
	module *mod;
	registry_t *reg = NULL;

	for (mod=first_module; mod; mod=mod->next) {
		if (mod->registry == NULL) continue;

		for (reg = mod->registry; reg->name; reg++) {
			/* see Bug20041125-1.
			 * unaligned memory access makes bus error on Sparc Solaris.
			 */
			int padded = reg->size % alignment_size;
			if (padded != 0) {
				padded = alignment_size - padded;
				debug("%d bytes are padded for registry[%s] of module[%s]",
						padded, reg->name, mod->name);
			}
			
			total_size += reg->size + padded;
		}
	}

	return total_size;
}

void load_each_registry() {
	int registry_size = 0;
	int alignment_size = 8;
	module *mod;
	registry_t *reg=NULL;
	void *current_shmaddr=NULL;
	ENTRY e,*ep;

	/* 1. first count the size of registry data.
	 * 2. allocate shared memory.
	 * 3. assign registry data pointer.
	 */

	registry_size = count_total_registry_size();
	alloc_shm_for_registry(registry_size);
	current_shmaddr = registry_shm.addr;
	hcreate(registry_size); // see hcreate(3)

	debug("linking registry"); 
	for (mod=first_module; mod; mod=mod->next) {
		if (mod->registry == NULL)continue;

		for (reg = mod->registry; reg->name; reg++) {
			/* see Bug20041125-1.
			 * unaligned memory access makes bus error on Sparc Solaris.
			 */
			int padded = reg->size % alignment_size;
			if (padded != 0) {
				padded = alignment_size - padded;
				debug("%d bytes are padded for registry[%s] of module[%s]",
						padded, reg->name, mod->name);
			}

			e.key = reg->name;
			e.data = (void *)reg;
			ep = hsearch(e,ENTER);
			if (ep == NULL)
				warn("registry %s is not registered due to hash error", reg->name);

			reg->data = current_shmaddr;
			reg->data_relative = (void*)(registry_shm.addr - current_shmaddr);

			current_shmaddr += reg->size + padded;

			if ( reg->init == NULL )
				warn("registry %s has NULL init()", reg->name);
			else
				(*(reg->init))(reg->data);
#ifdef DEBUG_REGISTRY_MEMORY
			debug("module:%s[%p]/registry:%s[%p]/data[%p]",
					mod->name,mod,reg->name,reg,reg->data);
#endif
		}
	}
	return;
}

void list_registry(FILE *out, char *module_name)
{
	int i, printed = 0, list_all = 0;
	char save_flag;
	module *m = NULL;
	registry_t *r = NULL;

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		fprintf(out, "Registry of all modules:\n");
	else
		fprintf(out, "Registry of %s:\n", module_name);
	fprintf(out, "*) Permanent Registry\n");

	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		r = m->registry;

		if ( list_all == 1 ) {
			if ( r == NULL ) continue;
			fprintf(out, "%s\n",m->name);
		} else if ( r == NULL ) {
			fprintf(out, "  (no registry)\n");
			continue;
		}

		for (i = 0; r[i].module != NULL; i++) {
			if ( r[i].flag == DONT_SAVE ) save_flag = ' ';
			else save_flag = '*';

			if ( r[i].get == NULL ) {
				warn("registry %s has NULL get()", r[i].name);
				fprintf(out, "  %s%c[%p] = '%s'\n",
					r[i].name, save_flag, r[i].data, "(null)");
			} else {
				fprintf(out, "  %s%c[%p] = '%s'\n",
					r[i].name, save_flag, r[i].data, r[i].get());
			}
			fprintf(out, "    - %s\n", r[i].desc);
		}
	}
	if ( printed == 0 )
		fprintf(out, "  (no such module exists)\n");
	fflush(out);
}

void list_registry_str(char *result, char *module_name)
{
	int i, printed = 0, list_all = 0;
	char save_flag;
	module *m = NULL;
	registry_t *r = NULL;
	char tmp[1024];

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		sprintf(result, "Registry of all modules:\n");
	else
		sprintf(result, "Registry of %s:\n", module_name);
	strcat(result, "*) Permanent Registry\n");

	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		r = m->registry;

		if ( list_all == 1 ) {
			if ( r == NULL ) continue;
			sprintf(tmp, "%s\n",m->name);
			strcat(result, tmp);
		} else if ( r == NULL ) {
			strcat(result, "  (no registry)\n");
			continue;
		}

		for (i = 0; r[i].module != NULL; i++) {
			if ( r[i].flag == DONT_SAVE ) save_flag = ' ';
			else save_flag = '*';

			if ( r[i].get == NULL ) {
				warn("registry %s has NULL get()", r[i].name);
				sprintf(tmp, "  %s%c[%p] = '%s'\n",
					r[i].name, save_flag, r[i].data, "(null)");
				strcat(result, tmp);
			} else {
				sprintf(tmp, "  %s%c[%p] = '%s'\n",
					r[i].name, save_flag, r[i].data, r[i].get());
				strcat(result, tmp);
			}
			sprintf(tmp, "    - %s\n", r[i].desc);
			strcat(result, tmp);
		}
	}
	if ( printed == 0 )
	{
		strcat(result, "  (no such module exists)\n");
	}
}

int save_registry_file(char *file)
{
	FILE *out;
	int r;
	time_t now;
    char backup[MAX_PATH_LEN];

	strncpy(backup, file, MAX_PATH_LEN);
	strcat(backup, ".bak");
	sb_rename(file, backup);

	out = sb_fopen(file, "w");
	if ( out == NULL ) {
		error("cannot open file[%s] to write: %s", file, strerror(errno));
		return FAIL;
	}

	time(&now);
	fprintf(out, "# %s Registry File\n", PACKAGE_STRING);
	fprintf(out, "# This data has byte-order dependency.\n");
	fprintf(out, "# Platform: %s\n", CANONICAL_HOST);
	fprintf(out, "# Release Date: %s\n", RELEASE_DATE);
	fprintf(out, "# Date: %s", ctime(&now));

	r = save_registry(out, NULL);
	fclose(out);

	return r;
}

int restore_registry_file(char *file)
{
	FILE *in;
	int r;

	in = sb_fopen(file, "r");
	if ( in == NULL ) {
		info("cannot open file[%s] to write: %s", file, strerror(errno));
		return FAIL;
	}

	r = restore_registry(in, NULL);
	fclose(in);

	return r;
}

#define SAVE_REGISTRY_CHUNK (4)
int save_registry(FILE *out, char *module_name)
{
	char buf[SAVE_REGISTRY_CHUNK*2+1]; /* CHUNK * 2 + '\0' */
	int i, j, printed = 0, list_all = 0;
	module *m = NULL;
	registry_t *r = NULL;

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	for (m=first_module; m; m=m->next)
	{
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		if ( (r = m->registry) == NULL ) continue;
		printed = 1;

		/* XXX: line length should be shorter than SHORT_STRING_SIZE,
		 * so that we can parse easily. */
		for (i = 0; r[i].module != NULL; i++)
		{
			int len;

			if (r[i].flag == DONT_SAVE) continue;

			/* below is always not longer than SHORT_STRING_SIZE, 
			 * so there is no boundary check */
			fprintf(out, "%s %s %d \\\n", m->name, r[i].name, r[i].size);
#ifdef DEBUG_REGISTRY_MEMORY
			debug("registry:%s[%p]/data[%p]", r[i].name, &r[i], r[i].data);
#endif
			for (j = 0, len = 0;
				 j < r[i].size;
				 j+=SAVE_REGISTRY_CHUNK)
			{
				len+=SAVE_REGISTRY_CHUNK*2+1;
				if ( len > SHORT_STRING_SIZE - 5 ) {
					fprintf(out, " \\\n");
					len = SAVE_REGISTRY_CHUNK*2+1;
				}
				fprintf(out, " %s",
					bin2hex(buf, r[i].data + j,
								(SAVE_REGISTRY_CHUNK > r[i].size - j)
								 ? r[i].size - j : SAVE_REGISTRY_CHUNK
							)
					);

				/* boundary check.
				 * if line length is more than SHORT_STRING_SIZE,
				 * print "\\\n" */
			} /* for (j = 0, len = 0, ... */
			fprintf(out, "\n");
		} /* for (i = 0; ... */
	} /* for (m=first_module; ... */

	fflush(out);

	if ( printed == 0 ) return FAIL;
	else return SUCCESS;
}


int save_registry_str(char *result, char *module_name)
{
	char buf[SAVE_REGISTRY_CHUNK*2+1]; /* CHUNK * 2 + '\0' */
	int i, j, printed = 0, list_all = 0;
	module *m = NULL;
	registry_t *r = NULL;
	char tmp[1024];

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	for (m=first_module; m; m=m->next)
	{
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		if ( (r = m->registry) == NULL ) continue;
		printed = 1;

		/* XXX: line length should be shorter than SHORT_STRING_SIZE,
		 * so that we can parse easily. */
		for (i = 0; r[i].module != NULL; i++)
		{
			int len;

			if (r[i].flag == DONT_SAVE) continue;

			/* below is always not longer than SHORT_STRING_SIZE, 
			 * so there is no boundary check */
			sprintf(tmp, "%s %s %d \\\n", m->name, r[i].name, r[i].size);
			strcat(result, tmp);
			
#ifdef DEBUG_REGISTRY_MEMORY
			debug("registry:%s[%p]/data[%p]", r[i].name, &r[i], r[i].data);
#endif
			for (j = 0, len = 0;
				 j < r[i].size;
				 j+=SAVE_REGISTRY_CHUNK)
			{
				len+=SAVE_REGISTRY_CHUNK*2+1;
				if ( len > SHORT_STRING_SIZE - 5 ) {
					strcat(result, " \\\n");
					len = SAVE_REGISTRY_CHUNK*2+1;
				}
				sprintf(tmp, " %s",
					bin2hex(buf, r[i].data + j,
								(SAVE_REGISTRY_CHUNK > r[i].size - j)
								 ? r[i].size - j : SAVE_REGISTRY_CHUNK
							)
					);
				strcat(result, tmp);

				/* boundary check.
				 * if line length is more than SHORT_STRING_SIZE,
				 * print "\\\n" */
			} /* for (j = 0, len = 0, ... */
			strcat(result, "\n");
		} /* for (i = 0; ... */
	} /* for (m=first_module; ... */


	if ( printed == 0 ) return FAIL;
	else return SUCCESS;
}


int restore_registry(FILE *in, char *module_name)
{
	int j = 0, len, line_completed, restore_all = 0;
	registry_t *r = NULL;
	/* registry data file's line length is no more than SHORT_STRING_SIZE */
	char buf[SHORT_STRING_SIZE], chunk[SAVE_REGISTRY_CHUNK*2+1];
	char *module, *name; /* module name, registry name */
	char *size_str; /* size of registry data value */
	char *data;

	if ( module_name == NULL || strlen(module_name) == 0 )
		restore_all = 1;

	line_completed = 1;
	for ( ; ; ) {
		if ( fgets(buf, SHORT_STRING_SIZE, in) == NULL ) break;
		if ( buf[0] == '#' ) continue;
		if ( buf[0] != ' ' && line_completed == 0 ) {
			warn("data error");
			continue;
		}
		if ( line_completed == 1 ) goto read_header;
		else goto read_data;

	  read_header:
		module = strtok(buf, " \t");
		if ( module == NULL ) {
			warn("error with module name while parsing registry file");
			continue;
		}
		name = strtok(NULL, " \t");
		if ( name == NULL ) {
			warn("error with registry name while parsing registry file");
			continue;
		}

		r = registry_get(name);
		if ( r == NULL ) {
			warn("no such registry[%s %s]", module, name);
			continue;
		}
		else if ( r->flag == DONT_SAVE) {
			warn("registry[%s] of module[%s] should not be saved",
						r->module, r->name);
			continue;
		}

		size_str = strtok(NULL, " \t");
		if ( size_str == NULL ) {
			warn("error with registry size while parsing registry file");
			continue;
		}

		if ( r->size != atoi(size_str) ) {
			warn("registry size[%s] read is not equal to currnt registry size[%d]",
				  size_str, r->size);
			continue;
		}
#ifdef DEBUG_REGISTRY_MEMORY
		debug("registry:%s[%p]/data[%p]", r->name, r, r->data);
#endif
		line_completed = 0;
		j = 0; /* will be used at read_data for(;;) */
		continue;

	  read_data:
		if (strcmp(buf+strlen(buf)-2, "\\\n") == 0)
			buf[strlen(buf)-2] = '\0'; /* delete '\' */

		for ( data = strtok(buf, " \t");
			  j < r->size;
		      data = strtok(NULL, " \t"), j += len )
		{
			if ( data == NULL ) break; /* read new line again */
			strncpy(chunk, data, SAVE_REGISTRY_CHUNK*2);
			chunk[SAVE_REGISTRY_CHUNK*2] = '\0';
			len = (int)(strlen(chunk)/2);
			if ( j > r->size - len ) {
				warn("invalid data size for registry %s", r->name);
				line_completed = 1;
				break;
			}

			hex2bin(r->data + j, chunk, len);
		}

		if ( j == r->size ) {
			debug("registry[%s] size[%d] loaded", r->name, r->size);
			line_completed = 1;
		}
		continue;
	}

	return SUCCESS;
}

registry_t* registry_get(char *registry_name) {
	ENTRY e,*ep;
	registry_t *reg;

	e.key = registry_name;
	ep = hsearch(e,FIND);
	if (!ep) {
		error("failed getting [%s] from registry",registry_name);
		return NULL;
	}
	reg = (registry_t*)(ep->data);

	return reg;
}

/* bin2hex
 * s    : destination char buffer
 * bin  : binary data
 * size : size of bindary data
 */
static char *bin2hex(char *s, void *bin, int size)
{
	unsigned char *from, *to, *dest;
	static unsigned char hex[] = "0123456789ABCDEF";

	dest = s;
	from = to = bin; to += size;
	for ( ; from < to; from++) {
		*(dest++) = hex[ (*from) >> 4 ];
		*(dest++) = hex[ (*from) % 16 ];
	}
	*dest = '\0';

	return s;
}

/* hex2bin
 * bin  : destination binary data buffer
 * s    : hex string
 * size : size of binary data
 */
static void *hex2bin(void *bin, char *s, int size)
{
	unsigned char *from, *to, *dest;
	static unsigned char ascii[] = {
		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, /* '0' - '9' */
		 0, 0, 0, 0, 0, 0, 0,10,11,12, /* ':' - 'C' */
		13,14,15, 0, 0, 0, 0, 0, 0, 0, /* 'D' - 'M' */
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 'N' - 'W' */
		 0, 0, 0, 0, 0, 0, 0, 0, 0,10, /* 'X' - 'a' */
		11,12,13,14,15, 0, 0, 0, 0, 0, /* 'b' - 'k' */
	};

	dest = bin;
	from = to = s; to += size*2;
	for ( ; from < to; ) {
		if (*from < '0' || *from > 'f') *from = '0';
		if (*(from+1) < '0' || *(from+1) > 'f') *(from+1) = '0';

		*(dest) = (ascii[*(from)-'0'] << 4) | (ascii[*(from+1)-'0']);
		dest++; from += 2;
	}

	return bin;
}
