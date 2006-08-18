/* $Id$ */
#ifndef UTIL_H
#define UTIL_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

SB_DECLARE(const char *) sb_get_server_version(void);
SB_DECLARE(int) sb_server_root_relative(char *buf, const char *filename);
SB_DECLARE(char *) sb_strbin(uint32_t number, int size);
SB_DECLARE(char *) sb_trim(char* s);
SB_DECLARE(char *) sb_left_trim(char* s);
SB_DECLARE(char *) sb_right_trim(char* s);
SB_DECLARE(char *) replace(char *str, char s, char d);
SB_DECLARE(char *) get_time(const char *format);

#endif
