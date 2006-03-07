/* $Id$ */
#ifndef __HTTP_CONFIG_H__
#define __HTTP_CONFIG_H__ 1

#include "hook.h"
#include "apr_pools.h"

/* Module-method dispatchers, also for http_request.c */
/**
 * Run the handler phase of each module until a module accepts the
 * responsibility of serving the request
 * @param r The current request
 * @return The status of the current request
 */
AP_CORE_DECLARE(int) ap_invoke_handler(request_rec *r);


#endif /* !__HTTP_CONFIG_H__ */

