/* $Id$ */
#include <stdarg.h>
#include "mod_httpd.h"
#include "log.h"

void ap_log_rerror(const char *file, const char *caller, int line,
		int level, apr_status_t status, const request_rec *r,
		const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_error_core(level, file, caller, fmt, args);
	va_end(args);

	/*
	 * IF APLOG_TOCLIENT is set,
	 * AND the error level is 'warning' or more severe,
	 * AND there isn't already error text associated with this request,
	 * THEN make the message text available to ErrorDocument and
	 * other error processors.
	 */
	va_start(args,fmt);
	if ((level & APLOG_TOCLIENT)
		&& ((level & APLOG_LEVELMASK) <= APLOG_WARNING)
		&& (apr_table_get(r->notes, "error-notes") == NULL)) {
		apr_table_setn(r->notes, "error-notes",
					   sb_escape_html(r->pool, apr_pvsprintf(r->pool, fmt,
															 args)));
	}
	va_end(args);
}

