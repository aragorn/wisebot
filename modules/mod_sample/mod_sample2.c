/* $Id$ */
#include "mod_sample.h"

static int second_api1(int count)
{
	info("second api1(%d)", count);
	return SUCCESS;
}

static int second_api2(int count, char *mesg)
{
	info("second api2(%d, \"%s\")", count, mesg);
	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_sample_api1(second_api1,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sample_api2(second_api2,NULL,NULL,HOOK_MIDDLE);
}

module sample2_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};
