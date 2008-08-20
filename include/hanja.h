/* $Id$ */
#ifndef HANJA_H
#define HANJA_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

SB_DECLARE(char*) sb_hanja2hangul(char* dest, const char* src, int len, const char* charset);

#endif
