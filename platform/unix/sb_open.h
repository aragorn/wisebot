/*
 * $Id$
 */
#ifndef _SB_OPEN__H_
#define _SB_OPEN__H_ 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#include "softbot.h"

SB_DECLARE(int) sb_open(const char *path, int flags, ...);
SB_DECLARE(int) sb_unlink(const char *path);
SB_DECLARE(int) sb_rename(const char *oldpath, const char *newpath);

#endif
