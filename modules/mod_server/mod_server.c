/* $Id$ */
#include "softbot.h"
#include "mod_metadummy.h"

HOOK_STRUCT(
	HOOK_LINK(dummy_api1)
	HOOK_LINK(dummy_api2)
	HOOK_LINK(noexist)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(int,dummy_api1,(int arg1),(arg1),0)
SB_IMPLEMENT_HOOK_RUN_FIRST(int,dummy_api2,(char *str),(str),1)
//SB_IMPLEMENT_HOOK_RUN_FIRST(int,noexist,(int x),(x),0)

static int my_dummy_api1(int arg1) {
	printf("in meta: dummy_api1: arg1 = %d\n",arg1);
	return 0;
}
static int my_dummy_api2(char *string) {
	printf("in meta: dummy_api2: string = %s\n",string);
	return 0;
}

static int message_processing(int sock) {
	Message *message_in, *message_out;
	Method *method;
	char *buffer;

	recv_message(sock, buffer);
	message_in = message_parsing(buffer);
	method = get_method(message);

	(method->callbackfunction)(message_in, message_out);

	send_response(message_out);

	message_free(message_in);
	message_free(message_out);
}

static int module_main (int argc, char *argv[])
{
	setproctitle("nextsoftbot %s",__FILE__);
	printf("hello, world! I'm %s\n",__FILE__);

	while (1) {
		char *tmp;

		lock_socket();
		new = accept(sock);
		unlock_socket();

		message_processing(new);

		close(new);

/*		int loglevel;*/
/*		registry_t *reg;*/

/*		sb_run_dummy_api1(1);*/
/*		sb_run_dummy_api2("|string args|");*/
/*		sb_run_noexist();*/

		sleep(3);
		fprintf(stderr,"mod_metadummy called\n");
		tmp = get_total_registry_info();
		fprintf(stderr,"list of registries:\n%s",tmp);

/*		sleep(3);*/
/*		reg = registry_get("LogLevelStat");*/
/*		tmp = reg->get(reg->data);*/
/*		fprintf(stderr,"## from registry, got LogLevelStat:|%s|\n",tmp);*/

/*		sleep(3);*/
/*		reg = registry_get("CurrentLogLevel");*/
/*		tmp = reg->get(reg->data);*/
/*		loglevel = atoi(tmp);*/
/*		fprintf(stderr,"## from registry, got CurrentLogLevel:|%d|\n",loglevel);*/

/*		fprintf(stderr,"## !!! loglevel status must change because"*/
/*					   "metadummy2 changes\n");*/
	}

	return 0;
}

static void register_hooks(void)
{
	sb_hook_dummy_api1(my_dummy_api1,NULL,NULL,HOOK_MIDDLE);
	sb_hook_dummy_api2(my_dummy_api2,NULL,NULL,HOOK_MIDDLE);
}

module metadummy_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	module_main,			/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
