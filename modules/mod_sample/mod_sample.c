/* $Id$ */
#include "mod_sample.h"
#include "mod_mp/mod_mp.h"

HOOK_STRUCT(
    HOOK_LINK(sample_api1)
    HOOK_LINK(sample_api2)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int, sample_api1, (int count),(count),DECLINE)
SB_IMPLEMENT_HOOK_RUN_ALL(int, sample_api2, \
  (int count, char *mesg),(count, mesg),SUCCESS,DECLINE)

static int term_signal = 0;

static RETSIGTYPE _shutdown(int sig)
{
  term_signal ++;
}

static int module_main (slot_t *slot)
{
	int n;

	sb_set_default_sighandlers(_shutdown, _shutdown);

	for (n=0; 1; n++) {
		if (term_signal) break;

		if ( sb_run_sample_api1(n)==SUCCESS ) {
			info("sample_api1 returned SUCCESS");
		} else {
			error("sample_api1 did NOT returned SUCCESS");
		}
		sleep(1);
		sb_run_sample_api2(n, "hello");
		sleep(1);
	}

	return 0;

}

static void dummy_conf_func(configValue a){
	info("config get test %s",a.argument[0]);
}


static config_t config[] = {
	CONFIG_GET("dummy1" , dummy_conf_func, 1 , "dummy config get test"),
	CONFIG_GET("dummy2" , dummy_conf_func, 1 , "dummy config get test"),
	{NULL}
};


module sample_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	NULL,				/* initialize */
	module_main,		/* child_main */
	NULL,				/* scoreboard */
	NULL,				/* register hook api */
};
