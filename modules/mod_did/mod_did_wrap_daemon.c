#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/did.h"

static int init()
{
    warn("this is an obsoleted module. please remove this module in etc/softbot.conf file.");

	return SUCCESS;
}

module did_daemon_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	init,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	NULL				/* register hook api */
};

