/* $Id$ */
#ifndef _CHARCONV_H_
#define _CHARCONV_H_ 1

#include "softbot.h"

#define EUCKR_TO_UTF8(data) sb_run_charset_conv((data), "UTF-8", "CP949")
#define UTF8_TO_EUCKR(data) sb_run_charset_conv((data), "CP949", "UTF-8")

#if defined (__cplusplus)
extern "C" {
#endif

SB_DECLARE_HOOK(char *,charset_conv,(char *str, const char *to, const char *from))

#if defined (__cplusplus)
}
#endif

#endif

