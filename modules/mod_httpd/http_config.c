/* $Id$ */
#include "common_core.h"
#include "hook.h"
#include "mod_httpd.h"
#include "request.h" /* sb_run_insert_filter */
#include "http_config.h"
#include "http_core.h"
#include "http_util.h"
#include "log.h" /* ap_log_rerror */
#include "apr_strings.h"

//#include "apr_strings.h"
//#include "conf.h"
//#include "core.h"
//#include "http_util.h"
//#include "util_cfgtree.h"

HOOK_STRUCT(
	HOOK_LINK(header_parser)
	HOOK_LINK(pre_config)
	HOOK_LINK(post_config)
	HOOK_LINK(open_logs)
	HOOK_LINK(child_init)
	HOOK_LINK(handler)
	HOOK_LINK(handler_init)
	HOOK_LINK(handler_finalize)
	HOOK_LINK(quick_handler)
	HOOK_LINK(optional_fn_retrieve)
)

SB_IMPLEMENT_HOOK_RUN_ALL(int, header_parser,
                          (request_rec *r), (r), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int, pre_config,
                          (apr_pool_t *pconf, apr_pool_t *ptemp),
                          (pconf, ptemp), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int, post_config,
                          (apr_pool_t *pconf, apr_pool_t *ptemp, server_rec *s),
                          (pconf, ptemp, s), SUCCESS, DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, open_logs,
                          (apr_pool_t *pconf,
                           apr_pool_t *ptemp, server_rec *s),
                          (pconf, ptemp, s), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_VOID_ALL(child_init,
                       (apr_pool_t *pchild, server_rec *s),
                       (pchild, s))

SB_IMPLEMENT_HOOK_RUN_FIRST(int, handler, (request_rec *r),
                            (r), DECLINE)

/* for handler initialization (qp_init) -- woosong/fortuna */
SB_IMPLEMENT_HOOK_RUN_ALL(int, handler_init, (void), (), SUCCESS, DECLINE)

/* for handler initialization (sbdb_close) -- eerun/fortuna */
SB_IMPLEMENT_HOOK_RUN_VOID_ALL(handler_finalize, (void), ())
	
SB_IMPLEMENT_HOOK_RUN_FIRST(int, quick_handler, (request_rec *r, int lookup),
                            (r, lookup), DECLINE)

SB_IMPLEMENT_HOOK_RUN_VOID_ALL(optional_fn_retrieve, (void), ())

/*****************************************************************************/
/* total_modules is the number of modules that have been linked
 * into the server.
 * XXX meaningless variable. to be removed.
 */
void *dir_config_vector[];
void *server_config_vector[];

void *create_dir_configs[] = { create_core_dir_config, NULL, NULL, NULL };
void *merge_dir_configs[];
void *create_server_configs[] = { create_core_server_config, NULL, NULL, NULL };
void *merge_server_configs[];
const command_rec *commands[] = { core_cmds, NULL, NULL, NULL };


module *ap_top_module = NULL;
module **ap_loaded_modules=NULL;

typedef int (*handler_func)(request_rec *);
typedef void *(*dir_maker_func)(apr_pool_t *, char *);
typedef void *(*server_maker_func)(apr_pool_t *, server_rec *s);
typedef void *(*merger_func)(apr_pool_t *, void *, void *);
ap_directive_t *ap_conftree;

/* Dealing with config vectors.  These are associated with per-directory,
 * per-server, and per-request configuration, and have a void* pointer for
 * each modules.  The nature of the structure pointed to is private to the
 * module in question... the core doesn't (and can't) know.  However, there
 * are defined interfaces which allow it to create instances of its private
 * per-directory and per-server structures, and to merge the per-directory
 * structures of a directory and its subdirectory (producing a new one in
 * which the defaults applying to the base directory have been properly
 * overridden).
 */

static ap_conf_vector_t *create_empty_config(apr_pool_t *p)
{
	/*
    void *conf_vector = apr_pcalloc(p, sizeof(void *) *
                                    (total_modules + DYNAMIC_MODULE_LIMIT));
	*/
	void *conf_vector = apr_pcalloc(p, sizeof(void *) * CONF_VECTOR_SIZE);
    return conf_vector;
}

static ap_conf_vector_t *create_default_per_dir_config(apr_pool_t *p)
{
	/*
    void **conf_vector = apr_pcalloc(p, sizeof(void *) *
                                     (total_modules + DYNAMIC_MODULE_LIMIT));
    module *modp;

    for (modp = ap_top_module; modp; modp = modp->next) {
        dir_maker_func df = modp->create_dir_config;

        if (df)
            conf_vector[modp->module_index] = (*df)(p, NULL);
    }
	*/
	void **conf_vector = apr_pcalloc(p, sizeof(void *) * CONF_VECTOR_SIZE);
	int i;

	for (i = 0; i < CONF_VECTOR_SIZE; i++) {
		dir_maker_func df = create_dir_configs[i];
		if (df)
			conf_vector[i] = (*df)(p, NULL);
	}

    return (ap_conf_vector_t *)conf_vector;
}

AP_CORE_DECLARE(ap_conf_vector_t *) ap_merge_per_dir_configs(apr_pool_t *p,
                                           ap_conf_vector_t *base,
                                           ap_conf_vector_t *new_conf)
{
	/*
    void **conf_vector = apr_palloc(p, sizeof(void *) * total_modules);
    void **base_vector = (void **)base;
    void **new_vector = (void **)new_conf;
    module *modp;

    for (modp = ap_top_module; modp; modp = modp->next) {
        int i = modp->module_index;

        if (!new_vector[i]) {
            conf_vector[i] = base_vector[i];
        }
        else {
            merger_func df = modp->merge_dir_config;
            if (df && base_vector[i]) {
                conf_vector[i] = (*df)(p, base_vector[i], new_vector[i]);
            }
            else
                conf_vector[i] = new_vector[i];
        }
    }
	*/
    void **conf_vector = apr_pcalloc(p, sizeof(void *) * CONF_VECTOR_SIZE);
    void **base_vector = (void **)base;
    void **new_vector = (void **)new_conf;
	int  i;

    for (i = 0; i < CONF_VECTOR_SIZE; i++) {
        if (!new_vector[i]) {
            conf_vector[i] = base_vector[i];
        }
        else {
            merger_func df = merge_dir_configs[i];
            if (df && base_vector[i]) {
                conf_vector[i] = (*df)(p, base_vector[i], new_vector[i]);
            }
            else
                conf_vector[i] = new_vector[i];
        }
    }

    return (ap_conf_vector_t *)conf_vector;
}

static ap_conf_vector_t *create_server_config(apr_pool_t *p, server_rec *s)
{
    void **conf_vector = apr_pcalloc(p, sizeof(void *) * CONF_VECTOR_SIZE);
	int i;

	for (i = 0; i < CONF_VECTOR_SIZE; i++) {
		server_maker_func df = create_server_configs[i];
		if (df)
			conf_vector[i] = (*df)(p, s);
	}

    return (ap_conf_vector_t *)conf_vector;
}

static void merge_server_config(apr_pool_t *p, ap_conf_vector_t *base,
                                 ap_conf_vector_t *virt)
{
    /* Can reuse the 'virt' vector for the spine of it, since we don't
     * have to deal with the moral equivalent of .htaccess files here...
     */

    void **base_vector = (void **)base;
    void **virt_vector = (void **)virt;
	int i;

	for (i = 0; i < CONF_VECTOR_SIZE; i++) {
        merger_func df = merge_server_configs[i];

        if (!virt_vector[i])
            virt_vector[i] = base_vector[i];
        else if (df)
            virt_vector[i] = (*df)(p, base_vector[i], virt_vector[i]);
    }
}

AP_CORE_DECLARE(ap_conf_vector_t *) ap_create_request_config(apr_pool_t *p)
{
    return create_empty_config(p);
}

AP_CORE_DECLARE(ap_conf_vector_t *) ap_create_conn_config(apr_pool_t *p)
{
    return create_empty_config(p);
}

AP_CORE_DECLARE(ap_conf_vector_t *) ap_create_per_dir_config(apr_pool_t *p)
{
    return create_empty_config(p);
}

/*****************************************************************
 *
 * Resource, access, and .htaccess config files now parsed by a common
 * command loop.
 *
 * Let's begin with the basics; parsing the line and
 * invoking the function...
 */

static const char *invoke_cmd(const command_rec *cmd, cmd_parms *parms,
                              void *mconfig, const char *args)
{
    char *w, *w2, *w3;
    const char *errmsg;

    if ((parms->override & cmd->req_override) == 0)
        return apr_pstrcat(parms->pool, cmd->name, " not allowed here", NULL);

    parms->info = cmd->cmd_data;
    parms->cmd = cmd;

    switch (cmd->args_how) {
    case RAW_ARGS:
#ifdef RESOLVE_ENV_PER_TOKEN
        args = ap_resolve_env(parms->pool,args);
#endif
        return cmd->AP_RAW_ARGS(parms, mconfig, args);

    case NO_ARGS:
        if (*args != 0)
            return apr_pstrcat(parms->pool, cmd->name, " takes no arguments",
                               NULL);

        return cmd->AP_NO_ARGS(parms, mconfig);

    case TAKE1:
        w = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name, " takes one argument",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE1(parms, mconfig, w);

    case TAKE2:
        w = ap_getword_conf(parms->pool, &args);
        w2 = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || *w2 == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name, " takes two arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE2(parms, mconfig, w, w2);

    case TAKE12:
        w = ap_getword_conf(parms->pool, &args);
        w2 = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name, " takes 1-2 arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE2(parms, mconfig, w, *w2 ? w2 : NULL);

    case TAKE3:
        w = ap_getword_conf(parms->pool, &args);
        w2 = ap_getword_conf(parms->pool, &args);
        w3 = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || *w2 == '\0' || *w3 == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name, " takes three arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE3(parms, mconfig, w, w2, w3);

    case TAKE23:
        w = ap_getword_conf(parms->pool, &args);
        w2 = ap_getword_conf(parms->pool, &args);
        w3 = *args ? ap_getword_conf(parms->pool, &args) : NULL;

        if (*w == '\0' || *w2 == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name,
                               " takes two or three arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE3(parms, mconfig, w, w2, w3);

    case TAKE123:
        w = ap_getword_conf(parms->pool, &args);
        w2 = *args ? ap_getword_conf(parms->pool, &args) : NULL;
        w3 = *args ? ap_getword_conf(parms->pool, &args) : NULL;

        if (*w == '\0' || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name,
                               " takes one, two or three arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE3(parms, mconfig, w, w2, w3);

    case TAKE13:
        w = ap_getword_conf(parms->pool, &args);
        w2 = *args ? ap_getword_conf(parms->pool, &args) : NULL;
        w3 = *args ? ap_getword_conf(parms->pool, &args) : NULL;

        if (*w == '\0' || (w2 && *w2 && !w3) || *args != 0)
            return apr_pstrcat(parms->pool, cmd->name,
                               " takes one or three arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        return cmd->AP_TAKE3(parms, mconfig, w, w2, w3);

    case ITERATE:
        while (*(w = ap_getword_conf(parms->pool, &args)) != '\0') {
            if ((errmsg = cmd->AP_TAKE1(parms, mconfig, w)))
                return errmsg;
        }

        return NULL;

    case ITERATE2:
        w = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || *args == 0)
            return apr_pstrcat(parms->pool, cmd->name,
                               " requires at least two arguments",
                               cmd->errmsg ? ", " : NULL, cmd->errmsg, NULL);

        while (*(w2 = ap_getword_conf(parms->pool, &args)) != '\0') {
            if ((errmsg = cmd->AP_TAKE2(parms, mconfig, w, w2)))
                return errmsg;
        }

        return NULL;

    case FLAG:
        w = ap_getword_conf(parms->pool, &args);

        if (*w == '\0' || (strcasecmp(w, "on") && strcasecmp(w, "off")))
            return apr_pstrcat(parms->pool, cmd->name, " must be On or Off",
                               NULL);

        return cmd->AP_FLAG(parms, mconfig, strcasecmp(w, "off") != 0);

    default:
        return apr_pstrcat(parms->pool, cmd->name,
                           " is improperly configured internally (server bug)",
                           NULL);
    }
}

AP_CORE_DECLARE(const command_rec *) ap_find_command(const char *name,
                                                     const command_rec *cmds)
{
    while (cmds->name) {
        if (!strcasecmp(name, cmds->name))
            return cmds;

        ++cmds;
    }

    return NULL;
}

AP_CORE_DECLARE(const command_rec *) ap_find_command_in_modules(const char *cmd_name, module **mod)
{
    const command_rec *cmdp;
	int i;

	for (i = 0; i < CONF_VECTOR_SIZE; i++) {
		if (commands[i] && (cmdp = ap_find_command(cmd_name, commands[i]))) {
            *mod = NULL; /* FIXME this should be the pointer to this module. */
            return cmdp;
        }
    }

    return NULL;
}

#if 0
AP_CORE_DECLARE(void *) ap_set_config_vectors(server_rec *server,
                                              ap_conf_vector_t *section_vector,
                                              const char *section,
                                              module *mod, apr_pool_t *pconf)
{
    void *section_config = ap_get_module_config(section_vector, mod);
    void *server_config = ap_get_module_config(server->module_config, mod);

    if (!section_config && mod->create_dir_config) {
        /* ### need to fix the create_dir_config functions' prototype... */
        section_config = (*mod->create_dir_config)(pconf, (char *)section);
        ap_set_module_config(section_vector, mod, section_config);
    }

    if (!server_config && mod->create_server_config) {
        server_config = (*mod->create_server_config)(pconf, server);
        ap_set_module_config(server->module_config, mod, server_config);
    }

    return section_config;
}
#endif

static const char *execute_now(char *cmd_line, const char *args,
                               cmd_parms *parms,
                               apr_pool_t *p, apr_pool_t *ptemp,
                               ap_directive_t **sub_tree,
                               ap_directive_t *parent);

static const char *ap_build_config_sub(apr_pool_t *p, apr_pool_t *temp_pool,
                                       const char *l, cmd_parms *parms,
                                       ap_directive_t **current,
                                       ap_directive_t **curr_parent,
                                       ap_directive_t **conftree)
{
    const char *retval = NULL;
    const char *args;
    char *cmd_name;
    ap_directive_t *newdir;
    module *mod = ap_top_module;
    const command_rec *cmd;

    if (*l == '#' || *l == '\0')
        return NULL;

#if RESOLVE_ENV_PER_TOKEN
    args = l;
#else
    args = ap_resolve_env(temp_pool, l);
#endif

    cmd_name = ap_getword_conf(p, &args);

//    debug("cmd_name = %s\n", cmd_name);

    if (*cmd_name == '\0') {
        /* Note: this branch should not occur. An empty line should have
         * triggered the exit further above.
         */
        return NULL;
    }

    if (cmd_name[1] != '/') {
        char *lastc = cmd_name + strlen(cmd_name) - 1;
        if (*lastc == '>') {
            *lastc = '\0' ;
        }
    }

    newdir = apr_pcalloc(p, sizeof(ap_directive_t));
    newdir->filename = parms->config_file->name;
    newdir->line_num = parms->config_file->line_number;
    newdir->directive = cmd_name;
    newdir->args = apr_pstrdup(p, args);

    debug("looking for %s in modules\n", cmd_name);
    if ((cmd = ap_find_command_in_modules(cmd_name, &mod)) != NULL) {
        //debug("found %s in modules\n", cmd_name);
        if (cmd->req_override & EXEC_ON_READ) {
            ap_directive_t *sub_tree = NULL;

            //debug("1");
            parms->err_directive = newdir;
            retval = execute_now(cmd_name, args, parms, p, temp_pool,
                                 &sub_tree, *curr_parent);
            if (*current) {
                (*current)->next = sub_tree;
            }
            else {
            	//debug("2");
                *current = sub_tree;
                if (*curr_parent) {
                    (*curr_parent)->first_child = (*current);
                }
                if (*current) {
                    (*current)->parent = (*curr_parent);
                }
            }
            //debug("3");
            if (*current) {
            	//debug("4");
                if (!*conftree) {
                    /* Before walking *current to the end of the list,
                     * set the head to *current.
                     */
                    *conftree = *current;
                }
            	//debug("5");
                while ((*current)->next != NULL) {
            	    //debug("6");
                    (*current) = (*current)->next;
                    (*current)->parent = (*curr_parent);
                }
            	//debug("7");
            }
            return retval;
        }
    }

    //debug("8");
    if (cmd_name[0] == '<') {
        if (cmd_name[1] != '/') {
            (*current) = add_directive_node(curr_parent, *current, newdir, 1);
        }
        else if (*curr_parent == NULL) {
            parms->err_directive = newdir;
            return apr_pstrcat(p, cmd_name,
                               " without matching <", cmd_name + 2,
                               " section", NULL);
        }
        else {
            char *bracket = cmd_name + strlen(cmd_name) - 1;

            if (*bracket != '>') {
                parms->err_directive = newdir;
                return apr_pstrcat(p, cmd_name,
                                   "> directive missing closing '>'", NULL);
            }

            *bracket = '\0';

            if (strcasecmp(cmd_name + 2,
                           (*curr_parent)->directive + 1) != 0) {
                parms->err_directive = newdir;
                return apr_pstrcat(p, "Expected </",
                                   (*curr_parent)->directive + 1, "> but saw ",
                                   cmd_name, ">", NULL);
            }

            *bracket = '>';

            /* done with this section; move up a level */
            *current = *curr_parent;
            *curr_parent = (*current)->parent;
        }
    }
    else {
        *current = add_directive_node(curr_parent, *current, newdir, 0);
    }

    return retval;
}

AP_DECLARE(const char *) ap_build_cont_config(apr_pool_t *p,
                                              apr_pool_t *temp_pool,
                                              cmd_parms *parms,
                                              ap_directive_t **current,
                                              ap_directive_t **curr_parent,
                                              char *orig_directive)
{
    char l[MAX_STRING_LEN];
    char *bracket;
    const char *retval;
    ap_directive_t *sub_tree = NULL;

    bracket = apr_pstrcat(p, orig_directive + 1, ">", NULL);
    while (!(ap_cfg_getline(l, MAX_STRING_LEN, parms->config_file))) {
        if (!memcmp(l, "</", 2)
            && (strcasecmp(l + 2, bracket) == 0)
            && (*curr_parent == NULL)) {
            break;
        }
        retval = ap_build_config_sub(p, temp_pool, l, parms, current, curr_parent, &sub_tree);
        if (retval != NULL)
            return retval;

        if (sub_tree == NULL && curr_parent != NULL) {
            sub_tree = *curr_parent;
        }

        if (sub_tree == NULL && current != NULL) {
            sub_tree = *current;
        }
    }

    *current = sub_tree;
    return NULL;
}

AP_DECLARE(const char *) ap_build_config(cmd_parms *parms,
                                         apr_pool_t *p, apr_pool_t *temp_pool,
                                         ap_directive_t **conftree)
{
    ap_directive_t *current = *conftree;
    ap_directive_t *curr_parent = NULL;
    char l[MAX_STRING_LEN];
    const char *errmsg;

    if (current != NULL) {
        while (current->next) {
            current = current->next;
        }
    }

    while (!(ap_cfg_getline(l, MAX_STRING_LEN, parms->config_file))) {
        errmsg = ap_build_config_sub(p, temp_pool, l, parms,
                                     &current, &curr_parent, conftree);
        if (errmsg != NULL)
            return errmsg;

        if (*conftree == NULL && curr_parent != NULL) {
            *conftree = curr_parent;
        }

        if (*conftree == NULL && current != NULL) {
            *conftree = current;
        }
    }

    if (curr_parent != NULL) {
        errmsg = "";

        while (curr_parent != NULL) {
            errmsg = apr_psprintf(p, "%s%s%s:%u: %s> was not closed.",
                                  errmsg,
                                  *errmsg == '\0' ? "" : APR_EOL_STR,
                                  curr_parent->filename,
                                  curr_parent->line_num,
                                  curr_parent->directive);
                                  curr_parent = curr_parent->parent;
        }

        return errmsg;
    }

    return NULL;
}
/*****************************************************************
 *
 * Reading whole config files...
 */

static cmd_parms default_parms =
{NULL, 0, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


AP_DECLARE(char *) ap_server_root_relative(apr_pool_t *p, const char *file)
{
    char *newpath;
    if (apr_filepath_merge(&newpath, gSoftBotRoot, file,
                           APR_FILEPATH_TRUENAME, p) == APR_SUCCESS)
        return newpath;
    else
        return NULL;
}


static const char *execute_now(char *cmd_line, const char *args,
                               cmd_parms *parms,
                               apr_pool_t *p, apr_pool_t *ptemp,
                               ap_directive_t **sub_tree,
                               ap_directive_t *parent)
{
    module *mod = ap_top_module;
    const command_rec *cmd;

    if (!(cmd = ap_find_command_in_modules(cmd_line, &mod))) {
        return apr_pstrcat(parms->pool, "Invalid command '",
                           cmd_line,
                           "', perhaps mis-spelled or defined by a module "
                           "not included in the server configuration",
                           NULL);
    }
    else {
        return invoke_cmd(cmd, parms, sub_tree, args);
    }
}

/*****************************************************************************/

typedef struct {
    char *fname;
} fnames;

static int fname_alphasort(const void *fn1, const void *fn2)
{
    const fnames *f1 = fn1;
    const fnames *f2 = fn2;

    return strcmp(f1->fname,f2->fname);
}

AP_DECLARE(void) ap_process_resource_config(server_rec *s, const char *fname,
                                            ap_directive_t **conftree,
                                            apr_pool_t *p,
                                            apr_pool_t *ptemp)
{
    cmd_parms parms;
    //apr_finfo_t finfo;
    const char *errmsg;
    ap_configfile_t *cfp;

    /* don't require conf/httpd.conf if we have a -C or -c switch */
/* FIXME SERVER_CONFIG_FILE is not for this case.
    if ((ap_server_pre_read_config->nelts
        || ap_server_post_read_config->nelts)
        && !(strcmp(fname, ap_server_root_relative(p, SERVER_CONFIG_FILE)))) {
        if (apr_lstat(&finfo, fname, APR_FINFO_TYPE, p) != APR_SUCCESS)
            return;
    }
*/

    /*
     * here we want to check if the candidate file is really a
     * directory, and most definitely NOT a symlink (to prevent
     * horrible loops).  If so, let's recurse and toss it back
     * into the function.
     */
    if (ap_is_rdirectory(ptemp, fname)) {
        apr_dir_t *dirp;
        apr_finfo_t dirent;
        int current;
        apr_array_header_t *candidates = NULL;
        fnames *fnew;
        apr_status_t rv;
        char errmsg[120];

        /*
         * first course of business is to grok all the directory
         * entries here and store 'em away. Recall we need full pathnames
         * for this.
         */
        fprintf(stderr, "Processing config directory: %s\n", fname);
        rv = apr_dir_open(&dirp, fname, p);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "%s: could not open config directory %s: %s\n",
                    "softbotd", fname,
                    apr_strerror(rv, errmsg, sizeof errmsg));
            exit(1);
        }

        candidates = apr_array_make(p, 1, sizeof(fnames));
        while (apr_dir_read(&dirent, APR_FINFO_DIRENT, dirp) == APR_SUCCESS) {
            /* strip out '.' and '..' */
            if (strcmp(dirent.name, ".")
                && strcmp(dirent.name, "..")) {
                fnew = (fnames *) apr_array_push(candidates);
                fnew->fname = ap_make_full_path(p, fname, dirent.name);
            }
        }

        apr_dir_close(dirp);
        if (candidates->nelts != 0) {
            qsort((void *) candidates->elts, candidates->nelts,
                  sizeof(fnames), fname_alphasort);

            /*
             * Now recurse these... we handle errors and subdirectories
             * via the recursion, which is nice
             */
            for (current = 0; current < candidates->nelts; ++current) {
                fnew = &((fnames *) candidates->elts)[current];
                fprintf(stderr, " Processing config file: %s\n", fnew->fname);
                ap_process_resource_config(s, fnew->fname, conftree, p, ptemp);
            }
        }

        return;
    }

    /* GCC's initialization extensions are soooo nice here... */

    parms = default_parms;
    parms.pool = p;
    parms.temp_pool = ptemp;
    parms.server = s;
    parms.override = (RSRC_CONF | OR_ALL) & ~(OR_AUTHCFG | OR_LIMIT);

    if (ap_pcfg_openfile(&cfp, p, fname) != APR_SUCCESS) {
        error("could not open document config file %s", fname);
        exit(1);
    }

    parms.config_file = cfp;

    errmsg = ap_build_config(&parms, p, ptemp, conftree);

    if (errmsg != NULL) {
		error("Syntax error on line %d of %s: ",
               parms.err_directive->line_num,
               parms.err_directive->filename);
        error("%s", errmsg);
        exit(1);
    }

    ap_cfg_closefile(cfp);
}
/*****************************************************************
 *
 * Getting *everything* configured...
 */

static void init_config_globals(apr_pool_t *p)
{
    /* Global virtual host hash bucket pointers.  Init to null. */
	/* XXX vhost is not used now. */
    //ap_init_vhost_config(p);
}

static server_rec *init_server_config(process_rec *process, apr_pool_t *p)
{
    apr_status_t rv;
    server_rec *s = (server_rec *) apr_pcalloc(p, sizeof(server_rec));

    apr_file_open_stderr(&s->error_log, p);
    s->process = process;
    s->port = 0;
    s->server_admin = DEFAULT_ADMIN;
    s->server_hostname = NULL;
/*    s->error_fname = DEFAULT_ERRORLOG;*/
    s->loglevel = DEFAULT_LOGLEVEL;
    s->limit_req_line = DEFAULT_LIMIT_REQUEST_LINE;
    s->limit_req_fieldsize = DEFAULT_LIMIT_REQUEST_FIELDSIZE;
    s->limit_req_fields = DEFAULT_LIMIT_REQUEST_FIELDS;
    s->timeout = apr_time_from_sec(DEFAULT_TIMEOUT);
    s->keep_alive_timeout = apr_time_from_sec(DEFAULT_KEEPALIVE_TIMEOUT);
    s->keep_alive_max = DEFAULT_KEEPALIVE;
    s->keep_alive = 1;
    s->next = NULL;
    s->addrs = apr_pcalloc(p, sizeof(server_addr_rec));

    /* NOT virtual host; don't match any real network interface */
    rv = apr_sockaddr_info_get(&s->addrs->host_addr,
                               NULL, APR_INET, 0, 0, p);
    sb_assert(rv == APR_SUCCESS); /* otherwise: bug or no storage */

    s->addrs->host_port = 0; /* matches any port */
    s->addrs->virthost = ""; /* must be non-NULL */
    s->names = s->wild_names = NULL;

    s->module_config = create_server_config(p, s);
    s->lookup_defaults = create_default_per_dir_config(p);

    return s;
}

server_rec *ap_read_config(process_rec *process, apr_pool_t *ptemp,
                           const char *filename,
                           ap_directive_t **conftree)
{
    const char *confname;
    apr_pool_t *p = process->pconf;
    server_rec *s = init_server_config(process, p);

    init_config_globals(p);

    confname = ap_server_root_relative(p, filename);
    if (!confname) {
		crit("Invalid config file path %s", filename);
        exit(1);
    }
    ap_process_resource_config(s, confname, conftree, p, ptemp);

    return s;
}



AP_CORE_DECLARE(int) ap_invoke_handler(request_rec *r)
{
    const char *handler;
    const char *p;
    int result;
    const char *old_handler = r->handler;

	debug("started");
    /*
     * The new insert_filter stage makes the most sense here.  We only use
     * it when we are going to run the request, so we must insert filters
     * if any are available.  Since the goal of this phase is to allow all
     * modules to insert a filter if they want to, this filter returns
     * void.  I just can't see any way that this filter can reasonably
     * fail, either your modules inserts something or it doesn't.  rbb
     */
    sb_run_insert_filter(r);

    if (!r->handler) {
        handler = r->content_type ? r->content_type : ap_default_type(r);
        if ((p=ap_strchr_c(handler, ';')) != NULL) {
            char *new_handler = (char *)apr_pmemdup(r->pool, handler,
                                                    p - handler + 1);
            char *p2 = new_handler + (p - handler);
            handler = new_handler;

            /* MIME type arguments */
            while (p2 > handler && p2[-1] == ' ')
                --p2; /* strip trailing spaces */

            *p2='\0';
        }

        r->handler = handler;
    }

    result = sb_run_handler(r);

    r->handler = old_handler;

    if (result == DECLINE && r->handler && r->filename) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
            "handler \"%s\" not found for: %s", r->handler, r->filename);
    }

	debug("ended result[%d]", result);
    //return result == DECLINE ? HTTP_INTERNAL_SERVER_ERROR : result;
    return result;
}

