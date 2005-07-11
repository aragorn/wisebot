/* $Id$ */
/* 
 * XXX configure에 의해 저절로 생기게 해야 할까?
 */
#include "softbot.h"

extern module client_module;
extern module error_log_module;
extern module memory_module;
//extern module api_module;
//extern module spool_module;
//extern module lexicon_module;

module *client_static_modules[] = {
	&client_module,
	&error_log_module,
	&memory_module,
//	&api_module,
//	&spool_module,
//	&lexicon_module,
	NULL
};
