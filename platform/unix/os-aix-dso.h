/* $Id$ */

#ifndef OS_AIX_DSO_H
#define OS_AIX_DSO_H

#include "auto_config.h"
#ifdef AIX5
#include "softbot.h"

SB_DECLARE(void *) dlopen(const char *path, int mode);
SB_DECLARE(void *) dlsym(void *handle, const char *symbol);
SB_DECLARE(const char *) dlerror(void);
SB_DECLARE(int) dlclose(void *handle);

#undef  RTLD_LAZY
#define RTLD_LAZY	1	/* lazy function call binding */
#undef  RTLD_NOW
#define RTLD_NOW	2	/* immediate function call binding */
#undef  RTLD_GLOBAL
#define RTLD_GLOBAL	0x100	/* allow symbols to be global */

#endif // AIX5

#endif // OS_AIX_DSO_H
