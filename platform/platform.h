/*
 * $Id$
 */

#ifndef PLATFORM_H
#define PLATFORM_H 1

#warning ***********************************************************
#warning  Using platform.h is deprecated. See common/common_util.
#warning  -- aragorn, 2006/03/15
#warning ***********************************************************

#include "unix/setproctitle.h"
#include "unix/sb_fopen.h"
#include "unix/sb_open.h"
#include "unix/sb_lockfile.h"
#include "unix/sb_tstat.h"
#include "unix/flock.h"
#include "unix/hash.h"
#include "generic/msort.h"
#include "generic/queue.h"
#include "generic/qsort2.h"
#include "generic/connect_nonb.h"
#include "generic/sz_string.h"
#include "generic/str_misc.h"
#include "generic/md5_global.h"
#include "generic/md5.h"

#include "unix/os-aix-dso.h"

#endif
