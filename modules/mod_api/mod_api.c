/* $Id$ */
#include "common_core.h"

/* for dynamic loading */
module api_module = {
	CORE_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	NULL					/* register hook api */
};
