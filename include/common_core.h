/* $Id$ */
#ifndef COMMON_CORE_H
#define COMMON_CORE_H 1

/* pthread_rwlock을 사용하기 위해 선언한다. <features.h>를 참조할 것. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "auto_config.h"
#include "constants.h"

#define SB_DECLARE(type)	type
#define SB_DECLARE_DATA	

#include <stdio.h>      /* FILE* */
#include <sys/types.h>  /* pid_t */

/* 주요 변수형에 대한 incomplete type 선언 */
typedef struct module_t module;       /* see modules.h */
typedef struct scoreboard_t scoreboard_t; /* see scoreboard.h */
typedef struct slot_t slot_t;         /* see scoreboard.h  */
typedef struct config_t config_t;     /* see config.h   */
typedef struct registry_t registry_t; /* see registry.h */

/* file을 open(2)하는 경우는 common_core.c의 gSoftBotRoot를 참조한다.
 * library를 독립적으로 구성하기 위해, file open/close하는 path와 관련된 wrapper
 * 함수는 common_util.h 가 아니라 common_core.h 에서 declare한다. */
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
