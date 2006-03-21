/* $Id$ */
#include "common_core.h"
#include "charconv.h"

HOOK_STRUCT(
	HOOK_LINK(charset_conv)
)

SB_IMPLEMENT_HOOK_RUN_FIRST(char *,charset_conv, \
		(char *str, const char *to, const char *from), \
		(str, to, from),0)
