/* $Id$ */
#include "common_core.h"
//#include "mod_httpd.h"
#include "conf.h"
#include "protocol.h"
#include "mod_mp/mod_mp.h"
#include "mod_httpd_test_handler.h"

/*****************************************************************************/
static int test_handler(request_rec *r)
{
	info("r->handler = '%s'\n", r->handler);
	ap_set_content_type(r, "text/html; charset=iso-8859-1");
	r->server->timeout = 1; /* hack */
	ap_rvputs(r,
		  DOCTYPE_HTML_2_0
		  "<html>"
		    "<head>\n"
		      "<title>",
		        "hurrah!",
		      "</title>\n"
		    "</head>"
		    "<body>\n"
		      "<h1>",
		        "hurrah!<br>"
			"hurrah!",
		      "</h1>\n",
		    "</body>"
		  "</html>\n",
		  NULL);

	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_handler(test_handler,NULL,NULL,HOOK_REALLY_FIRST);
	return;
}

module httpd_test_handler_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks				/* register hook api */
};

