/* $Id$ */
#include "softbot.h"

extern module server_module;
extern module memory_module;
extern module error_log_module;
extern module mp_module;

module *static_modules[] = {
	&server_module,
	&error_log_module,
	&memory_module,
	&mp_module,
	NULL
};

module *unittest_modules[] = {
/*	&lexicon_test_module,*/
/*	&registry_test_module,*/
//	&morpheme_test_module,
/*	&vrf_test_module,*/
/*	&docattr_test_module,*/
	NULL
};
