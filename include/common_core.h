/* $Id$ */
#ifndef COMMON_CORE_H
#define COMMON_CORE_H 1


#ifdef WIN32
# include "win32_config.h"
#else
# include "auto_config.h"
#endif /* WIN32 */
#include "constants.h"

#define DEBUG_SOFTBOT  1

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


#ifndef COMMON_CORE_PRIVATE

#endif

#endif
