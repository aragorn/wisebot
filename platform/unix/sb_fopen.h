/* $Id$ */
#ifndef SB_FOPEN_H
#define SB_FOPEN_H 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "softbot.h"

SB_DECLARE(FILE*) sb_fopen(const char *path, const char *mode);
SB_DECLARE(FILE*) sb_freopen(const char *path, const char *mode, FILE *stream);

#endif
