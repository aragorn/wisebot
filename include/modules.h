/* $Id$ */
#ifndef MODULES_H
#define MODULES_H

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#ifdef MAX_CONFIG_MOD_NAME
#	define MAX_MODULE_NAME		MAX_CONFIG_MOD_NAME
#else
#	define MAX_MODULE_NAME		40
#endif

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


#define DEFAULT_SERVER_PORT		8605
#define DEFAULT_SERVER_PORTSTR	"8605"
extern SB_DECLARE_DATA int gSoftBotListenPort;

SB_DECLARE(int) assignSoftBotPort(const char *modname, char module_portid);
SB_DECLARE(void) show_portinfo();

/* XXX this must move to server.h */
SB_DECLARE(void) do_unittest();

#endif
