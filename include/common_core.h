/* $Id$ */
#ifndef COMMON_CORE_H
#define COMMON_CORE_H 1

/* pthread_rwlock�� ����ϱ� ���� �����Ѵ�. <features.h>�� ������ ��. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "auto_config.h"
#include "constants.h"

#define SB_DECLARE(type)	type
#define SB_DECLARE_DATA	

#include <stdio.h>      /* FILE* */
#include <sys/types.h>  /* pid_t */

/* �ֿ� �������� ���� incomplete type ���� */
typedef struct module_t module;       /* see modules.h */
typedef struct scoreboard_t scoreboard_t; /* see scoreboard.h */
typedef struct slot_t slot_t;         /* see scoreboard.h  */
typedef struct config_t config_t;     /* see config.h   */
typedef struct registry_t registry_t; /* see registry.h */

/* file�� open(2)�ϴ� ���� common_core.c�� gSoftBotRoot�� �����Ѵ�.
 * library�� ���������� �����ϱ� ����, file open/close�ϴ� path�� ���õ� wrapper
 * �Լ��� common_util.h �� �ƴ϶� common_core.h ���� declare�Ѵ�. */
SB_DECLARE(FILE*) sb_fopen(const char *path, const char *mode);
SB_DECLARE(FILE*) sb_freopen(const char *path, const char *mode, FILE *stream);
SB_DECLARE(int) sb_open(const char *path, int flags, ...);
SB_DECLARE(int) sb_unlink(const char *path);
SB_DECLARE(int) sb_rename(const char *oldpath, const char *newpath);

extern SB_DECLARE_DATA char gSoftBotRoot[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gErrorLogFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gQueryLogFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA char gRegistryFile[MAX_PATH_LEN];
extern SB_DECLARE_DATA pid_t gRootPid;

#ifndef CORE_PRIVATE
# include "modules.h"
# include "scoreboard.h"
# include "config.h"
# include "registry.h"
# include "log_error.h"
#else
#endif

#endif
