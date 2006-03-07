/* $Id$ */
#include <stdio.h>
#include "softbot.h"
#include "protocol.h"
#include "mod_httpd.h"
#include "core.h"

char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
module *static_modules[1];
extern module httpd_module;
extern module http_core_module;
extern module httpd_connection_module;

static int my_handler(request_rec *r);

int main(int argc, char *argv[], char *envp[])
{
	slot_t slot;

	init_log_error();
	init_set_proc_title(argc, argv, envp);

	printf("hello, world\n");

	httpd_module.init();
	httpd_module.register_hooks();

	http_core_module.init();
	http_core_module.register_hooks();

	/* before the core_http_module hooks in the default... */
	sb_hook_handler(my_handler,NULL,NULL,HOOK_REALLY_FIRST);

	core_httpd_module.init();
	core_httpd_module.register_hooks();

	httpd_connection_module.register_hooks();

	httpd_module.main(&slot);

	printf("%s\n", core_httpd_module.name);

	return 0;
}

static int my_handler(request_rec *r)
{
	printf("r->handler = '%s'\n", r->handler);
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
