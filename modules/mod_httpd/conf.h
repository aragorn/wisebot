/* $Id$ */
#ifndef __CONF_H__
#define __CONF_H__

#include "mod_httpd.h"
#include "util_cfgtree.h"

/**
 * @file _config.h
 * @brief mod_httpd Configuration
 */

/*
 * The central data structures around here...
 */

/* Command dispatch structures... */

/**
 * How the directives arguments should be parsed.
 * @remark Note that for all of these except RAW_ARGS, the config routine is
 *      passed a freshly allocated string which can be modified or stored
 *      or whatever...
 */
enum cmd_how {
    RAW_ARGS,			/**< cmd_func parses command line itself */
    TAKE1,			/**< one argument only */
    TAKE2,			/**< two arguments only */
    ITERATE,			/**< one argument, occuring multiple times
				 * (e.g., IndexIgnore)
				 */
    ITERATE2,			/**< two arguments, 2nd occurs multiple times
				 * (e.g., AddIcon)
				 */
    FLAG,			/**< One of 'On' or 'Off' */
    NO_ARGS,			/**< No args at all, e.g. </Directory> */
    TAKE12,			/**< one or two arguments */
    TAKE3,			/**< three arguments only */
    TAKE23,			/**< two or three arguments */
    TAKE123,			/**< one, two or three arguments */
    TAKE13			/**< one or three arguments */
};

/**
 * This structure is passed to a command which is being invoked,
 * to carry a large variety of miscellaneous data which is all of
 * use to *somebody*...
 */
typedef struct cmd_parms_struct cmd_parms;

#if defined(AP_HAVE_DESIGNATED_INITIALIZER) || defined(DOXYGEN)

/** 
 * All the types of functions that can be used in directives
 * @internal
 */
typedef union {
    /** function to call for a no-args */
    const char *(*no_args) (cmd_parms *parms, void *mconfig);
    /** function to call for a raw-args */
    const char *(*raw_args) (cmd_parms *parms, void *mconfig,
			     const char *args);
    /** function to call for a take1 */
    const char *(*take1) (cmd_parms *parms, void *mconfig, const char *w);
    /** function to call for a take2 */
    const char *(*take2) (cmd_parms *parms, void *mconfig, const char *w,
			  const char *w2);
    /** function to call for a take3 */
    const char *(*take3) (cmd_parms *parms, void *mconfig, const char *w,
			  const char *w2, const char *w3);
    /** function to call for a flag */
    const char *(*flag) (cmd_parms *parms, void *mconfig, int on);
} cmd_func;

/** This configuration directive does not take any arguments */
# define AP_NO_ARGS	func.no_args
/** This configuration directive will handle it's own parsing of arguments*/
# define AP_RAW_ARGS	func.raw_args
/** This configuration directive takes 1 argument*/
# define AP_TAKE1	func.take1
/** This configuration directive takes 2 arguments */
# define AP_TAKE2	func.take2
/** This configuration directive takes 3 arguments */
# define AP_TAKE3	func.take3
/** This configuration directive takes a flag (on/off) as a argument*/
# define AP_FLAG	func.flag

/** method of declaring a directive with no arguments */
# define AP_INIT_NO_ARGS(directive, func, mconfig, where, help) \
    { directive, { .no_args=func }, mconfig, where, RAW_ARGS, help }
/** method of declaring a directive with raw argument parsing */
# define AP_INIT_RAW_ARGS(directive, func, mconfig, where, help) \
    { directive, { .raw_args=func }, mconfig, where, RAW_ARGS, help }
/** method of declaring a directive which takes 1 argument */
# define AP_INIT_TAKE1(directive, func, mconfig, where, help) \
    { directive, { .take1=func }, mconfig, where, TAKE1, help }
/** method of declaring a directive which takes multiple arguments */
# define AP_INIT_ITERATE(directive, func, mconfig, where, help) \
    { directive, { .take1=func }, mconfig, where, ITERATE, help }
/** method of declaring a directive which takes 2 arguments */
# define AP_INIT_TAKE2(directive, func, mconfig, where, help) \
    { directive, { .take2=func }, mconfig, where, TAKE2, help }
/** method of declaring a directive which takes 1 or 2 arguments */
# define AP_INIT_TAKE12(directive, func, mconfig, where, help) \
    { directive, { .take2=func }, mconfig, where, TAKE12, help }
/** method of declaring a directive which takes multiple 2 arguments */
# define AP_INIT_ITERATE2(directive, func, mconfig, where, help) \
    { directive, { .take2=func }, mconfig, where, ITERATE2, help }
/** method of declaring a directive which takes 1 or 3 arguments */
# define AP_INIT_TAKE13(directive, func, mconfig, where, help) \
    { directive, { .take3=func }, mconfig, where, TAKE13, help }
/** method of declaring a directive which takes 2 or 3 arguments */
# define AP_INIT_TAKE23(directive, func, mconfig, where, help) \
    { directive, { .take3=func }, mconfig, where, TAKE23, help }
/** method of declaring a directive which takes 1 to 3 arguments */
# define AP_INIT_TAKE123(directive, func, mconfig, where, help) \
    { directive, { .take3=func }, mconfig, where, TAKE123, help }
/** method of declaring a directive which takes 3 arguments */
# define AP_INIT_TAKE3(directive, func, mconfig, where, help) \
    { directive, { .take3=func }, mconfig, where, TAKE3, help }
/** method of declaring a directive which takes a flag (on/off) as a argument*/
# define AP_INIT_FLAG(directive, func, mconfig, where, help) \
    { directive, { .flag=func }, mconfig, where, FLAG, help }

#else /* AP_HAVE_DESIGNATED_INITIALIZER */

typedef const char *(*cmd_func) ();

# define AP_NO_ARGS  func
# define AP_RAW_ARGS func
# define AP_TAKE1    func
# define AP_TAKE2    func
# define AP_TAKE3    func
# define AP_FLAG     func

# define AP_INIT_NO_ARGS(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, RAW_ARGS, help }
# define AP_INIT_RAW_ARGS(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, RAW_ARGS, help }
# define AP_INIT_TAKE1(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE1, help }
# define AP_INIT_ITERATE(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, ITERATE, help }
# define AP_INIT_TAKE2(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE2, help }
# define AP_INIT_TAKE12(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE12, help }
# define AP_INIT_ITERATE2(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, ITERATE2, help }
# define AP_INIT_TAKE13(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE13, help }
# define AP_INIT_TAKE23(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE23, help }
# define AP_INIT_TAKE123(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE123, help }
# define AP_INIT_TAKE3(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, TAKE3, help }
# define AP_INIT_FLAG(directive, func, mconfig, where, help) \
    { directive, func, mconfig, where, FLAG, help }

#endif /* AP_HAVE_DESIGNATED_INITIALIZER */

/**
 * The command record structure.  Each modules can define a table of these
 * to define the directives it will implement.
 */
typedef struct command_struct command_rec; 
struct command_struct {
    /** Name of this command */
    const char *name;
    /** The function to be called when this directive is parsed */
    cmd_func func;
    /** Extra data, for functions which implement multiple commands... */
    void *cmd_data;		
    /** What overrides need to be allowed to enable this command. */
    int req_override;
    /** What the command expects as arguments 
     *  @defvar cmd_how args_how*/
    enum cmd_how args_how;

    /** 'usage' message, in case of syntax errors */
    const char *errmsg;
};

/**
 * @defgroup ConfigDirectives Allowed locations for configuration directives.
 *
 * The allowed locations for a configuration directive are the union of
 * those indicated by each set bit in the req_override mask.
 *
 * @{
 */
#define OR_NONE 0             /**< *.conf is not available anywhere in this override */
#define OR_LIMIT 1	     /**< *.conf inside <Directory> or <Location>
				and .htaccess when AllowOverride Limit */
#define OR_OPTIONS 2         /**< *.conf anywhere
                                and .htaccess when AllowOverride Options */
#define OR_FILEINFO 4        /**< *.conf anywhere
				and .htaccess when AllowOverride FileInfo */
#define OR_AUTHCFG 8         /**< *.conf inside <Directory> or <Location>
				and .htaccess when AllowOverride AuthConfig */
#define OR_INDEXES 16        /**< *.conf anywhere
				and .htaccess when AllowOverride Indexes */
#define OR_UNSET 32          /**< unset a directive (in Allow) */
#define ACCESS_CONF 64       /**< *.conf inside <Directory> or <Location> */
#define RSRC_CONF 128	     /**< *.conf outside <Directory> or <Location> */
#define EXEC_ON_READ 256     /**< force directive to execute a command 
                which would modify the configuration (like including another
                file, or IFModule */
/** this directive can be placed anywhere */
#define OR_ALL (OR_LIMIT|OR_OPTIONS|OR_FILEINFO|OR_AUTHCFG|OR_INDEXES)

/** @} */

/**
 * This can be returned by a function if they don't wish to handle
 * a command. Make it something not likely someone will actually use
 * as an error code.
 */
#define DECLINE_CMD "\a\b"

/** Common structure for reading of config files / passwd files etc. */
typedef struct ap_configfile_t ap_configfile_t;
struct ap_configfile_t {
    int (*getch) (void *param);	    /**< a getc()-like function */
    void *(*getstr) (void *buf, size_t bufsiz, void *param);
				    /**< a fgets()-like function */
    int (*close) (void *param);	    /**< a close handler function */
    void *param;                    /**< the argument passed to getch/getstr/close */
    const char *name;               /**< the filename / description */
    unsigned line_number;           /**< current line number, starting at 1 */
};

/**
 * This structure is passed to a command which is being invoked,
 * to carry a large variety of miscellaneous data which is all of
 * use to *somebody*...
 */
struct cmd_parms_struct {
    /** Argument to command from cmd_table */
    void *info;
    /** Which allow-override bits are set */
    int override;
    /** Which methods are <Limit>ed */
    apr_int64_t limited;
    /** methods which are limited */
    apr_array_header_t *limited_xmethods;
    /** methods which are xlimited */
    method_list_t *xlimited;

    /** Config file structure. */
    ap_configfile_t *config_file;
    /** the directive specifying this command */
    ap_directive_t *directive;

    /** Pool to allocate new storage in */
    apr_pool_t *pool;
    /** Pool for scratch memory; persists during configuration, but 
     *  wiped before the first request is served...  */
    apr_pool_t *temp_pool;
    /** Server_rec being configured for */
    server_rec *server;
    /** If configuring for a directory, pathname of that directory.  
     *  NOPE!  That's what it meant previous to the existance of <Files>, 
     * <Location> and regex matching.  Now the only usefulness that can be 
     * derived from this field is whether a command is being called in a 
     * server context (path == NULL) or being called in a dir context 
     * (path != NULL).  */
    char *path;
    /** configuration command */
    const command_rec *cmd;

    /** per_dir_config vector passed to handle_command */
    struct ap_conf_vector_t *context;
    /** directive with syntax error */
    const ap_directive_t *err_directive;
};








/* CONFIGURATION VECTOR FUNCTIONS */

/** configuration vector structure */
typedef struct ap_conf_vector_t ap_conf_vector_t;

enum config_vector_index {
	CORE_HTTPD_MODULE = 0,
	CONF_VECTOR_SIZE,
};

#if defined(AP_DEBUG) || defined(DOXYGEN)
/**
 * Generic accessors for other modules to get at their own module-specific
 * data
 * @param conf_vector The vector in which the modules configuration is stored.
 *        usually r->per_dir_config or s->module_config
 * @param index The index of the array to get the data for.
 * @return The module-specific data
 */
AP_DECLARE(void *) ap_get_module_config(const ap_conf_vector_t *cv,
                                        enum config_vector_index index);

/**
 * Generic accessors for other modules to set at their own module-specific
 * data
 * @param conf_vector The vector in which the modules configuration is stored.
 *        usually r->per_dir_config or s->module_config
 * @param index The index of the array to set the data for.
 * @param val The module-specific data to set
 */
AP_DECLARE(void) ap_set_module_config(ap_conf_vector_t *cv,
                                      enum config_vector_index index,
                                      void *val);

#else /* AP_DEBUG */

#define ap_get_module_config(v,i)	\
    (((void **)(v))[i])
#define ap_set_module_config(v,i,val)	\
    ((((void **)(v))[i]) = (val))

#endif /* AP_DEBUG */

/**
 * Read all config files and setup the server
 * @param process The process running the server
 * @param temp_pool A pool to allocate temporary data from.
 * @param config_name The name of the config file
 * @param conftree Place to store the root of the config tree
 * @return The setup server_rec list.
 */
server_rec *ap_read_config(process_rec *process, 
                           apr_pool_t *temp_pool, 
                           const char *config_name, 
                           ap_directive_t **conftree);



/* For http_request.c... */

/**
 * Setup the config vector for a request_rec
 * @param p The pool to allocate the config vector from
 * @return The config vector
 */
AP_CORE_DECLARE(ap_conf_vector_t*) ap_create_request_config(apr_pool_t *p);

/**
 * Setup the config vector for per dir module configs
 * @param p The pool to allocate the config vector from
 * @return The config vector
 */
AP_CORE_DECLARE(ap_conf_vector_t *) ap_create_per_dir_config(apr_pool_t *p);

/**
 * Run all of the modules merge per dir config functions
 * @param p The pool to pass to the merge functions
 * @param base The base directory config structure
 * @param new_conf The new directory config structure
 */
AP_CORE_DECLARE(ap_conf_vector_t*) ap_merge_per_dir_configs(apr_pool_t *p,
                                           ap_conf_vector_t *base,
                                           ap_conf_vector_t *new_conf);

/* For http_connection.c... */
/**
 * Setup the config vector for a connection_rec
 * @param p The pool to allocate the config vector from
 * @return The config vector
 */
AP_CORE_DECLARE(ap_conf_vector_t*) ap_create_conn_config(apr_pool_t *p);





  /* Hooks */

/**
 * Run the header parser functions for each module
 * @param r The current request
 * @return OK or DECLINED
 */
SB_DECLARE_HOOK(int,header_parser,(request_rec *r))

/**
 * Run the pre_config function for each module
 * @param pconf The config pool
 * @param plog The logging streams pool
 * @param ptemp The temporary pool
 * @return SUCCESS or DECLINE on success anything else is a error
 */
SB_DECLARE_HOOK(int,pre_config,(apr_pool_t *pconf, apr_pool_t *ptemp))

/**
 * Run the post_config function for each module
 * @param pconf The config pool
 * @param plog The logging streams pool
 * @param ptemp The temporary pool
 * @param s The list of server_recs
 * @return OK or DECLINED on success anything else is a error
 */
SB_DECLARE_HOOK(int,post_config,(apr_pool_t *pconf,
                                 apr_pool_t *ptemp, server_rec *s))

/**
 * Run the open_logs functions for each module
 * @param pconf The config pool
 * @param plog The logging streams pool
 * @param ptemp The temporary pool
 * @param s The list of server_recs
 * @return OK or DECLINED on success anything else is a error
 */
SB_DECLARE_HOOK(int,open_logs,(apr_pool_t *pconf,apr_pool_t *ptemp,
								server_rec *s))

/**
 * Run the child_init functions for each module
 * @param pchild The child pool
 * @param s The list of server_recs in this server 
 */
SB_DECLARE_HOOK(void,child_init,(apr_pool_t *pchild, server_rec *s))

/**
 * Run the handler functions for each module
 * @param r The request_rec
 * @remark non-wildcard handlers should HOOK_MIDDLE, wildcard HOOK_LAST
 */
SB_DECLARE_HOOK(int,handler,(request_rec *r))

/**
 * Run handler initialization functions for each module (before spawning threads)
 * ADDED for softbot handler(qp_init specifically) -- woosong/fortuna
 */
SB_DECLARE_HOOK(int ,handler_init,(void))

/**
 * Run handler finalization functions for each module (after destroy threads)
 * ADDED for softbot handler(sbdb_close specifically ) -- eerun/fortuna
 */
SB_DECLARE_HOOK(void,handler_finalize,(void))

/**
 * Run the quick handler functions for each module. The quick_handler
 * is run before any other requests hooks are called (location_walk,
 * directory_walk, access checking, et. al.). This hook was added
 * to provide a quick way to serve content from a URI keyed cache.
 * 
 * @param r The request_rec
 * @param lookup_uri Controls whether the caller actually wants content or not.
 * lookup is set when the quick_handler is called out of 
 * ap_sub_req_lookup_uri()
 */
SB_DECLARE_HOOK(int,quick_handler,(request_rec *r, int lookup_uri))

/**
 * Retrieve the optional functions for each module.
 * This is run immediately before the server starts. Optional functions should
 * be registered during the hook registration phase.
 */
SB_DECLARE_HOOK(void,optional_fn_retrieve,(void))

/**
 * The root of the configuration tree
 * @defvar ap_directive_t *conftree
 */
extern ap_directive_t *ap_conftree;

#endif

