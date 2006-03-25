/* $Id$ */
#include "mod_httpd.h"
#include "request.h"
#include "http_config.h"
#include "http_core.h"
#include "http_util.h"
#include "apr_strings.h"

/*
HOOK_STRUCT(
	HOOK_LINK(header_parser)
	HOOK_LINK(open_logs)
	HOOK_LINK(handler)
	HOOK_LINK(quick_handler)
)

SB_IMPLEMENT_HOOK_RUN_ALL(int, header_parser,
	(request_rec *r), (r), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_ALL(int, open_logs,
	(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s),
	(pconf, plog, ptemp, s), SUCCESS, DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, handler,
	(request_rec *r), (r), DECLINE)
SB_IMPLEMENT_HOOK_RUN_FIRST(int, quick_handler,
	(request_rec *r, int lookup), (r, lookup), DECLINE)
*/

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

	debug("ended");
    return result == DECLINE ? HTTP_INTERNAL_SERVER_ERROR : result;
}




