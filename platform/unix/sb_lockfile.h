#ifndef _SB_LOCKFILE_H_
#define _SB_LOCKFILE_H_

#include "softbot.h"

SB_DECLARE(int) sb_lockfile(const char *path);
SB_DECLARE(int) sb_unlockfile(int fd);

#endif
