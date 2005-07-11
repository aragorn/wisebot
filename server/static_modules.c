/* $Id$ */
#include "softbot.h"

extern module server_module;
extern module memory_module;
extern module error_log_module;
extern module mp_module;

module *server_static_modules[] = {
	&server_module,
	&error_log_module,
	&memory_module,
	&mp_module,
	NULL
};

