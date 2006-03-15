/* $Id$ */

#include "common_core.h"
#include "modules.h"

extern module server_module;

module *server_static_modules[] = {
	&server_module,
	NULL
};

