/* $Id$ */
#include "mod_sample.h"

static int first_api1(int count)
{
	info("first api1(%d)", count);
	info("i will return FAIL");
	return FAIL;
}

static int first_api2(int count, char *mesg)
{
	info("first api2(%d, \"%s\")", count, mesg);
	return SUCCESS;
}

static void register_hooks(void)
{
	sb_hook_sample_api1(first_api1,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sample_api2(first_api2,NULL,NULL,HOOK_MIDDLE);
}

module sample1_module = {
	STANDARD_MODULE_STUFF,
	NULL,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks,		/* register hook api */
};
