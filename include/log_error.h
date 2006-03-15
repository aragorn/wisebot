/* $Id$ */
#ifndef LOG_ERROR_H
#define LOG_ERROR_H 1

#ifndef COMMON_CORE_H
# error You should include "common_core.h" first.
#endif

#include <stdarg.h>

#ifdef DEBUG_SOFTBOT
#  define DEBUG_LOG_ERROR
#  define EMERG(format, ...)   emerg(format, ##__VA_ARGS__) 
#  define ALERT(format, ...)   alert(format, ##__VA_ARGS__) 
#  define CRIT(format, ...)    crit(format, ##__VA_ARGS__)
#  define ERROR(format, ...)   error(format, ##__VA_ARGS__) 
#  define WARN(format, ...)    warn(format, ##__VA_ARGS__)
#  define NOTICE(format, ...)  notice(format, ##__VA_ARGS__)
#  define INFO(format, ...)    info(format, ##__VA_ARGS__)  
#  define DEBUG(format, ...)   debug(format, ##__VA_ARGS__)
#  define debug(format, ...)  \
	log_error(LEVEL_DEBUG, __FILE__ , __FUNCTION__ , format , ##__VA_ARGS__)
#else
#  undef  DEBUG_LOG_ERROR
#  define EMERG(format, ...) 
#  define ALERT(format, ...) 
#  define CRIT(format, ...)  
#  define ERROR(format, ...) 
#  define WARN(format, ...)  
#  define NOTICE(format, ...)
#  define INFO(format, ...)  
#  define DEBUG(format, ...) 
#  define debug(format, ...)
#endif

/**
 * Redefine assert() to something more useful for SoftBot.
 *
 * Use sb_assert() if the condition should always be checked.
 * Use SB_DEBUG_ASSERT() if the condition should only be checked
 * when DEBUG_SOFTBOT is defined.
 */

#ifdef DEBUG_SOFTBOT
#define SB_DEBUG_ASSERT(exp) sb_assert(exp)
#else
#define SB_DEBUG_ASSERT(exp) ((void)0)
#endif

#ifdef DEBUG_SOFTBOT
#define SB_ABORT() _sb_abort(__FILE__, __FUNCTION__)
#else
#define SB_ABORT() ((void)0)
#endif

#define DEFAULT_LOGLEVEL LEVEL_WARN

enum log_levels {
	LEVEL_EMERG,
	LEVEL_ALERT,
	LEVEL_CRIT,
	LEVEL_ERROR,
	LEVEL_WARN,
	LEVEL_NOTICE,
	LEVEL_INFO,
	LEVEL_DEBUG,
	MAX_LOG_LEVEL
};

#define emerg(format, ...)  log_error(LEVEL_EMERG, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define alert(format, ...)  log_error(LEVEL_ALERT, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define crit(format, ...)   log_error(LEVEL_CRIT , __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define error(format, ...)  log_error(LEVEL_ERROR, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define warn(format, ...)   log_error(LEVEL_WARN,  __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define notice(format, ...) log_error(LEVEL_NOTICE, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define info(format, ...)   log_error(LEVEL_INFO,  __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define sb_assert(exp) \
	((exp) ? (void)0 : log_assert(#exp,__FILE__,__LINE__,__FUNCTION__))

extern char *gLogLevelStr[];
extern int  gLogLevel;

SB_DECLARE(int) set_debug_module_policy(const char *policy);
SB_DECLARE(int) add_debug_module(const char *name);

SB_DECLARE(void) open_error_log(const char *error_log, const char *query_log);
SB_DECLARE(void) reopen_error_log(const char *error_log, const char *query_log);
SB_DECLARE(void) close_error_log();
SB_DECLARE(void) save_pid(const char *file);

SB_DECLARE(void) log_error(int level, const char *aModule, \
						   const char *aCaller,const char *format, ...) \
						   __attribute__((format(printf,4,5)));
SB_DECLARE(void) log_error_core(int level, const char *aModule, const char *aCaller, \
								const char *format, va_list args);
SB_DECLARE(void) log_query(const char *query);
SB_DECLARE(void) log_assert(const char *exp,const char *file,int line,const char *func);
SB_DECLARE(void) _sb_abort(const char *file,const char *function);
SB_DECLARE(void) set_screen_log();
SB_DECLARE(double) timediff(struct timeval *first, struct timeval *later);
SB_DECLARE(int) log_setlevelstr(const char* levelstr);

#endif
