/* $Id$ */
/* 
 * XXX configure에 의해 저절로 생기게 해야 할까?
 */
#include "common_core.h"

extern module client_module;

module *client_static_modules[] = {
	&client_module,
	NULL
};
