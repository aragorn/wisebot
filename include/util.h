/* $Id$ */
#ifndef UTIL_H
#define UTIL_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#include <stdint.h>

SB_DECLARE(const char *) sb_get_server_version(void);
SB_DECLARE(int) sb_server_root_relative(char *buf, const char *filename);
SB_DECLARE(char *) sb_strbin(uint32_t number, int size);

#endif
