#include "softbot.h"
#include "mod_api/did_client.h"
#include "mod_api/did.h"

static int DI_getId(char *pKey, DocId *docid)
{
	int rv;
	rv = sb_run_get_docid(wrapper_diddb, pKey, (uint32_t*)docid);
	if (rv == DOCID_OLD_REGISTERED)
		return SUCCESS;
	else if (rv == DOCID_NOT_REGISTERED)
		return DI_ID_NOT_EXIST;
	else
		return FAIL;
}

int DI_getNewId(char *pKey, DocId *docid, DocId *olddocid)
{
	int rv;
	rv = sb_run_get_new_docid(wrapper_diddb, pKey,
			(uint32_t*)docid, (uint32_t*)olddocid);
	if (rv == DOCID_NEW_REGISTERED)
		return DI_NEW_REGISTERED;
	if (rv == DOCID_OLD_REGISTERED)
		return DI_OLD_REGISTERED;
	else
		return FAIL;
}

static void register_hooks(void)
{
	sb_hook_client_get_docid(DI_getId,NULL,NULL,HOOK_MIDDLE);
	sb_hook_client_get_new_docid(DI_getNewId,NULL,NULL,HOOK_MIDDLE);
}

/*************************************************************************/
/* configuration stuff here */

static void get_did_slot_path(configValue v) {
	strncpy(wrapper_diddb_path, v.argument[0], 256);
}

/*****************************************************************************/
static config_t config[] = {
	CONFIG_GET("DidDbPath",get_did_slot_path, 1, "did slot db local path"),
	{NULL}
};

module did_client_module = {
	STANDARD_MODULE_STUFF,
	config,				/* config */
	NULL,				/* registry */
	NULL,               /* initialize function */
	NULL,				/* child_main */
	NULL,				/* scoreboard */
	register_hooks		/* register hook api */
};

