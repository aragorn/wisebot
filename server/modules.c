/* $Id$ */
#include "modules.h"

// aix dlopen cannot deal with shared object properly. use apache-hacked dlopen
#ifndef AIX5
#  include <dlfcn.h>
#endif


#include <sys/stat.h>
#include <sys/types.h>

#define MAX_PORTINFO	32
typedef struct {
	char modname[MAX_MODULE_NAME];
	int port;
} SoftBotPortInfo;

int gSoftBotListenPort = DEFAULT_SERVER_PORT;
int mSoftBotPortInfoNum = 0;
SoftBotPortInfo mSoftBotPortInfo[MAX_PORTINFO];

#define MAX_DYNAMIC_MODULES (64)

typedef struct moduleinfo {
	const char *name; /* module symbol name like "cdm_module in mod_cdm.c", 
					   * NOT module C file name */
	module *modp;
} moduleinfo;

/*
 * In the module linked list(which starts with first_module)
 * stay unregistered-hooked modules,
 * In the loaded_modules array
 * stay after-register-hooked modules.
 * Currently those twos differs in that ways,
 */

static int total_modules = 0;
static int dynamic_modules = 0;
module *first_module=NULL;
extern module *static_modules[];
static module **loaded_modules = NULL;
static moduleinfo loaded_dynamic_modules[MAX_DYNAMIC_MODULES+1];

static module* add_module(module *this, const char *mod_symbol_name);
static void register_hooks(module *m);
//static int unload_module(void *data);

#define DIR_SEP '/'	/* XXX: autoconf stuff. (in constants.h ??) */

module *find_module(const char *mod_name)
{
	module *m=NULL;
	char mod_short_name[MAX_MODULE_NAME]="";
	const char *p=NULL;

	/* strip off directory path, and extentions(like .c, .so, ) */
	p = strrchr(mod_name,DIR_SEP);
	if (p) p++; // get rid of '/'
	else p = mod_name;
	
	strcpy(mod_short_name,p);

	/* use find_module_by_symbol instead.
	 * --aragorn (2002/08/20) */
#if 0
	/* for dynamic module (mod_xxx.so) */
	p2 = strrchr(mod_short_name,'.');
	if (p2) {
		*p2 = '\0';
		//XXX: module name is "mod_xxx.c"
		strcat(mod_short_name, ".c");
	}
#endif

	debug("mod_short_name:[%s]",mod_short_name);

	for (m = first_module; m; m=m->next) {
		if (strncmp(m->name,mod_short_name,strlen(mod_short_name))==0) {
			return m;
		}
	}
	return NULL;
}

module *find_module_by_symbol(const char *mod_symbol_name)
{
	moduleinfo *mi=NULL;

	debug("mod_symbol_name:[%s]",mod_symbol_name);

	for (mi = loaded_dynamic_modules; mi->modp; mi++) {
		if (strncmp(mi->name,mod_symbol_name,strlen(mod_symbol_name))==0) {
			return mi->modp;
		}
	}
	return NULL;
}
/**
 * call register hook function and
 * register to loaded_modules array
 */
int load_modules(int type)
{
	module *m, **m2 = loaded_modules;

	while (*m2 != NULL) m2++;

	if (type != DYNAMIC_MODULE_TYPE && type != STATIC_MODULE_TYPE) {
		error("%d is not supported type", type);
		return FAIL;
	}

	for (m = first_module; m != NULL; m=m->next) {
		if (!(m->type & type))
			continue;
		register_hooks(m);	
		*m2++ = m;
	}

	return SUCCESS;
}

int load_static_modules()
{
	module **m;
	int ret=0;

	total_modules = 0;
	for (m = static_modules; *m != NULL; m++) {
		(*m)->type |= STATIC_MODULE_TYPE;
		total_modules++;
	}

	/* initialize list of loaded modules */
	loaded_modules = (module **)sb_calloc(
		total_modules+MAX_DYNAMIC_MODULES+1,sizeof(module *));
	if (loaded_modules == NULL) {
		error("out of memory!: %s", strerror(errno));
		exit(1);
	}

	/* duplication check */
	for (m = static_modules; *m != NULL; m++) {
		if (find_module((*m)->name) != NULL) {
			warn("static module %s already loaded",(*m)->name);
			continue;
		}
		if (add_module(*m, NULL) == NULL) error("add_module failed");
	}

	ret = load_modules(STATIC_MODULE_TYPE);

	return ret;
}



int load_dynamic_modules()
{
#if 0
	module *m, **m2 = loaded_modules;

	while (*m2 != NULL) m2++;

	for (m = first_module; m != NULL ; m=m->next) {
		if (!(m->type & DYNAMIC_MODULE_TYPE))
			continue;
		register_hooks(m);
		*m2++ = m;
	}
	return SUCCESS;
#endif
	return load_modules(DYNAMIC_MODULE_TYPE);
}

/* 
 * This is called for the directive LoadModule and actually loads
 * a shared object file into the address space of the server process.
 */
module* add_dynamic_module(const char *mod_symbol_name, const char *modulename, char registry_only)
{
	static void *selfhandle = NULL; // handle to executable file itself
	char modulefile[MAX_PATH_LEN];
	void *modhandle;
	void *modsym;
	const char *error;
	module *modp;

	/* we should not load a module that is loaded already.
	 * duplication check should be done by module's symbol name,
	 * because one mod_xxx.so may have more than one module symbol.
	 * this kind of mod_xxx.so should be loaded as many times as
	 * the number of module symbol that it contains.
	 * -- aragorn (2002/08/20)
	 */
	if (find_module_by_symbol(mod_symbol_name) != NULL) {
		warn("module %s already loaded", mod_symbol_name);
		return NULL;
	}

	/* load the file into the softbot address space */

	/* always load executable itself first so that
	 * subsequent loaded object can resolve symbols
	 * in executables */
#if 0 /* XXX: dlopening the executable is not needed, I think. --aragorn */
	if (selfhandle == NULL) {
		info("opening executable itself");
		selfhandle = dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
		if (selfhandle == NULL) {
			error("cannot load itself: %s",dlerror());
		}
	}
#endif

	sb_server_root_relative(modulefile, modulename);
	modhandle = dlopen(modulefile, RTLD_NOW|RTLD_GLOBAL);

	if (modhandle == NULL) {
		error("cannot load(dlopen) %s into server: %s", modulename, dlerror());
		return NULL;
	}
	debug("loaded module %s[%p]", mod_symbol_name, modhandle);

	/* retrive the pointer to the module structure symbol 
	 * through the module name */
	modsym = dlsym(modhandle, mod_symbol_name);
	if (modsym == NULL && (error = dlerror()) != NULL) {
		error("cannot locate(dlsym) API module structure symbol `%s' in file %s",
				mod_symbol_name, modulename);
		error("dlerror: %s", error);
		return NULL;
	}
	modp = (module*) modsym;

	/* make sure the found module structure is really a module structure */
	if (modp->magic != MODULE_MAGIC_COOKIE) {
		error("API module structure `%s' in file %s is garbled - "
			" perhaps this is not a SoftBot module DSO?",
			mod_symbol_name, modulename);
		return NULL;
	}

	/* add this module to the SoftBot core structures */
	modp->dynamic_load_handle = (void *)modhandle;
	modp->type |= DYNAMIC_MODULE_TYPE;

	if (registry_only) {
		// modp->registry is the only useful thing for registry_only mode --jiwon.
		modp->config = NULL;
		modp->init = NULL;
		modp->main = NULL;
		modp->scoreboard = NULL;
		modp->register_hooks = NULL;
	}

	INFO("calling add_module");

	return add_module(modp, mod_symbol_name);
}


int init_core_modules(module *start_module)
{
	module *mod;
	int ret;

	for (mod = start_module; mod; mod = mod->next) {
		if ( !(mod->type & CORE_MODULE_TYPE) ) continue;

		if (mod->init) {
			ret=mod->init();
			if (ret < 0) {
				error("Initialization %s failed",mod->name);
				return FAIL;
			}
		}
		else 
			debug("module %s has no init function",mod->name);
	}

	return SUCCESS;
}

int init_standard_modules(module *start_module)
{
	module *mod;
	int ret;

	for (mod = start_module; mod; mod = mod->next) {
		if ( !(mod->type & STANDARD_MODULE_TYPE) ) continue;

		if (mod->init) {
			debug("module %s ->init()",mod->name);
			ret=mod->init();
			if (ret < 0) {
				error("Initialization %s failed",mod->name);
				return FAIL;
			}
		}
		else 
			info("module %s has no init function",mod->name);
	}

	return SUCCESS;
}


void list_static_modules(FILE *out)
{
	int i;
	module *m;

	fprintf(out, "<?xml version=\"1.0\" ?>\n"
				"<xml>\n" );
	fprintf(out, "<!-- Compiled-in modules: -->\n");
	for(i = 0; static_modules[i]; i++) {
		m = static_modules[i];

		if (strrchr(m->name,'/'))
			m->name = 1+ strrchr(m->name,'/');
		if (strrchr(m->name,'\\'))
			m->name = 1+ strrchr(m->name,'\\');

		fprintf(out, "<module name=\"%s\" />\n", m->name);
	}
	fprintf(out, "</xml>");
	fflush(out);
}

void list_static_modules_str(char *result)
{
	int i;
	module *m;
	char tmp[1024];

	sprintf(result, "Compiled-in modules:\n");
	for(i = 0; static_modules[i]; i++) {
		m = static_modules[i];

		if (strrchr(m->name,'/'))
			m->name = 1+ strrchr(m->name,'/');
		if (strrchr(m->name,'\\'))
			m->name = 1+ strrchr(m->name,'\\');

		sprintf(tmp, "  %s\n",m->name);
		strcat(result, tmp);
	}
}

void list_modules(FILE *out)
{
	module *m = NULL;
	moduleinfo *mi = NULL;

	fprintf(out, "<?xml version=\"1.0\" ?>\n"
				"<xml>\n" );
	fprintf(out, "<!-- Loaded modules: -->\n");
	fprintf(out, "<Loaded_modules>\n");
	for (m=first_module; m; m=m->next) {
		fprintf(out, "<module name=\"%s\" />\n"	, m->name);
	}
	fprintf(out, "</Loaded_modules>\n");

	fprintf(out, "<!-- Loaded dynamic modules: -->\n");
	fprintf(out, "<Loaded_dynamic_modules>\n");
	for (mi=&(loaded_dynamic_modules[0]); mi->modp; mi++) {
		fprintf(out, "<module name=\"%s\" >\n"
				"<symbol_name>%s</symbol_name>\n"
				"</module>\n"	, mi->modp->name,mi->name);
	}
	fprintf(out, "</Loaded_dynamic_modules>\n"
				"</xml>\n");
	fflush(out);
}


void list_modules_str(char *result)
{
	module *m = NULL;
	moduleinfo *mi = NULL;
	char tmp[1024];

	strcpy(result, "Loaded modules:\n");
	
	for (m=first_module; m; m=m->next) {
		sprintf(tmp, "  %s\n",m->name);
		strcat(result, tmp);
	}
	
	strcat(result, "\nLoaded dynamic modules:\n");
	for (mi=&(loaded_dynamic_modules[0]); mi->modp; mi++) {
		sprintf(tmp, "  %s (%s)\n",mi->modp->name,mi->name);
		strcat(result, tmp);
	}
		
}


void list_config(FILE *out, char *module_name)
{
	int i, printed = 0, list_all = 0;
	module *m = NULL;
	config_t *c = NULL;

	fprintf(out, "<?xml version=\"1.0\" ?>\n"
				"<xml>\n" );
	
	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	fprintf(out, "<!-- Config of %s : -->\n",
				(list_all) ? "all modules" : module_name );

/*	if ( list_all )*/
/*		fprintf(out, "Config of all modules:\n");*/
/*	else*/
/*		fprintf(out, "Config of %s:\n", module_name);*/
	
	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		c = m->config;

/*		if ( list_all == 1 ) {*/
/*			if ( c == NULL ) continue;*/
/*			fprintf(out, "%s\n",m->name);*/
/*		} else if ( c == NULL ) {*/
/*			fprintf(out, "  (no config)\n");*/
/*			break;*/
/*		}*/
		fprintf(out, "<module name=\"%s\" >\n", m->name);
		if ( c ) {
			for (i = 0; c[i].name != NULL; i++) {
				fprintf(out, 
						"<config name=\"%s\" >\n"
						"<argument_num>%d</argument_num>\n"
						"<desc>%s</desc>\n"
						"</config>\n",
						c[i].name, c[i].argNum, c[i].desc);
			}
		}
		fprintf(out, "</module>\n");
		
	}
	if ( printed == 0 )
		fprintf(out, "<error>no such module exists</error>\n");
	fprintf(out, "</xml>");
	fflush(out);
}



void list_config_str(char *result, char *module_name)
{
	int i, printed = 0, list_all = 0;
	module *m = NULL;
	config_t *c = NULL;
	char tmp[1024];

	if ( module_name == NULL || strlen(module_name) == 0 )
		list_all = 1;

	if ( list_all )
		sprintf(result, "Config of all modules:\n");
	else
		sprintf(result, "Config of %s:\n", module_name);

	for (m=first_module; m; m=m->next) {
		if ( list_all == 0 && strcmp(m->name, module_name) != 0 )
			continue;
		printed = 1;
		c = m->config;

		if ( list_all == 1 ) {
			if ( c == NULL ) continue;
			sprintf(tmp, "%s\n",m->name);
			strcat(result, tmp);
		} else if ( c == NULL ) {
			strcat(result, "  (no config)\n");
			break;
		}

		for (i = 0; c[i].name != NULL; i++) {
			sprintf(tmp, "  %s (%d) %s\n", c[i].name, c[i].argNum, c[i].desc);
			strcat(result, tmp);
		}
	}
	if ( printed == 0 )
	{
		strcat(result, "  (no such module exists)\n");
	}
}
/***************************************************************************
 * private functions
 */

/* add the module at the end of linked list pointed by `first_module'.
 * if the module is dynamic one, add the module at the end of array,
 * `loaded_dynamic_modules' too. */
static module* add_module(module *this, const char *mod_symbol_name)
{
	module *m; //*mprev;
	char *ptr;

	if (this->version != MODULE_MAGIC_NUMBER_MAJOR) {
		error("module `%s' is not compatible with this "
			"version of SoftBot.", this->name);
		error("please contact the vendor for the correct version.");
		return NULL;
	}

    /* Some C compilers put a complete path into __FILE__, but we want
     * only the filename (e.g. mod_includes.c). So check for path
     * components (Unix and DOS), and remove them.
     */
	ptr = strrchr(this->name, '/');
	if (ptr)
		this->name = ptr+1;

	ptr = strrchr(this->name, '\\');
	if (ptr)
		this->name = ptr+1;

	if (this->type & DYNAMIC_MODULE_TYPE) {
		moduleinfo *mi;
		char *symbol_name;

		if (dynamic_modules == MAX_DYNAMIC_MODULES) {
			error("module '%s' could not be loaded.", this->name);
			error("because the dynamic module limit was reached.");
			error("please increase MAX_DYNAMIC_MODULES(%d) and recompile.",
					MAX_DYNAMIC_MODULES);
			return NULL;
		}

		/* add this module at the end of array, `loaded_dynamic_modules'. */
		mi = &(loaded_dynamic_modules[dynamic_modules]);
		mi->modp = this;
		symbol_name = (char *)sb_calloc(strlen(mod_symbol_name)+1,sizeof(char));
		if ( symbol_name == NULL ) {
			error("out of memory!: %s", strerror(errno));
			exit(1);
		}
		strncpy(symbol_name, mod_symbol_name, strlen(mod_symbol_name));
		symbol_name[strlen(mod_symbol_name)] = '\0';
		mi->name = symbol_name;

		total_modules++;
		dynamic_modules++;
	}

	if (!first_module)
		first_module = this;
	else {
		for (m=first_module; m->next!=NULL; m=m->next);

		m->next = this;
	}

	debug("add module %s", this->name);
	return this;
}

static void register_hooks(module *m)
{
	if (m->register_hooks) {
		if(debug_module_hooks)
		{
			printf("Registering hooks for %s\n",m->name);
		}
		/* char *current_hooking_module : see hook.c, hook.h */
		current_hooking_module = m->name;
		m->register_hooks();
	}
}

#if 0
static int unload_module(void *data)
{
	moduleinfo *modi = (moduleinfo*)data;

	/* only unload if module information is still existing */
	if (modi->modp == NULL)
		return SUCCESS;

	/* remove the module pointer from the core structure */
	//removed_loaded_module(modi->modp);

	/* destroy the module information */
	modi->modp = NULL;
	modi->name = NULL;
	return SUCCESS;
}
#endif

void do_unittest()
{
	module *m;
	char testdone=0;

	for (m=first_module; m!=NULL; m=m->next) {
		if (!(m->type & TEST_MODULE_TYPE))
			continue;
		if (m->main) {
			testdone=1;
			warn("testing for %s started",m->name);
			m->main(NULL);
			warn("testing for %s ended",m->name);
		}
		else {
			error("no main in unittest module[%s] !?",m->name);
		}
	}
	if (!testdone) {
		error("in configuration file, add test with TestModule directive");
		error("like TestModule module_struct path/modulename.so");
	}
}


/* gSoftBotListenPort 에서 portid만큼 더한 값을 return해 준다. */
int assignSoftBotPort(const char *modname, char module_portid)
{
	int i=0,idx=0;

	if (module_portid >= 'A') {
		module_portid = module_portid - 'A' + 1;
	}

	// check already assigned port info
	for (i=0; i<mSoftBotPortInfoNum; i++) {
		if (strncmp(modname,mSoftBotPortInfo[i].modname,MAX_MODULE_NAME)==0) {
			return mSoftBotPortInfo[i].port;
		}
	}

	if (mSoftBotPortInfoNum == MAX_PORTINFO) {
		crit("assigning port for %s failed",modname);
		crit("increase MAX_PORTINFO[%d] and recompile",MAX_PORTINFO);
		return FAIL;
	}

	idx=mSoftBotPortInfoNum;
	mSoftBotPortInfo[idx].port = gSoftBotListenPort + module_portid;
	strncpy(mSoftBotPortInfo[idx].modname,modname,MAX_MODULE_NAME);

	mSoftBotPortInfoNum++;
	return mSoftBotPortInfo[idx].port;
}

void show_portinfo()
{
	int i=0;
	SoftBotPortInfo *info=NULL;

	printf("\n");
	printf(ON_GREEN "Ports information that is used by SoftBot\n"RESET);
	for (i=0; i<mSoftBotPortInfoNum; i++) {
		info = mSoftBotPortInfo+i;
		printf(GREEN"%s: %d\n"RESET,info->modname, info->port);
	}
}
