#ifndef _ERRORLOG_H
#define _ERRORLOG_H

extern int screenlog;

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#define SRC_MARK        __FILE__,__LINE__
#define ERROR_LOG       LEVEL_ERROR,__FILE__,__LINE__
#define WARN_LOG        LEVEL_WARN,__FILE__,__LINE__
#define INFO_LOG        LEVEL_INFO,__FILE__,__LINE__
#define DEBUG_LOG       LEVEL_DEBUG,__FILE__,__LINE__
#include <stdarg.h> 

#define RESET       "\e[0m" 
#define BOLD        "\e[1m" 
#define ITALICS     "\e[3m" 
#define UNDERLINE   "\e[4m" 
#define INVERSE     "\e[7m" 
#define BOLDOF      "\e[22m" 
#define ITALICSOFF  "\e[23m" 
//#define UNDERLINE   "\e[24m" 
#define INVERSEOFF  "\e[27m" 
// foreground color// 
#define BLACK		"\e[30m" 
#define RED			"\e[31m" 
#define GREEN		"\e[32m" 
#define YELLOW		"\e[33m" 
#define BLUE		"\e[34m" 
#define MAGENTA		"\e[35m" 
#define CYAN		"\e[36m" 
#define WHITE		"\e[37m" 
#define DEFAULT		"\e[39m" 
// background color// 
#define _BLACK  	"\e[40m" 
#define _RED		"\e[41m" 
#define _GREEN 		"\e[42m" 
#define _YELLOW 	"\e[43m" 
#define _BLUE		"\e[44m" 
#define _MAGENTA	"\e[45m" 
#define _CYAN		"\e[46m" 
#define _WHITE		"\e[47m" 
#define _DEFAULT	"\e[49m" 

#ifndef WIN32
#	define debug(format, ...) \
	errorlog(LEVEL_DEBUG,__FILE__,__LINE__,format,##__VA_ARGS__)
#	define info(format, ...) \
	errorlog(LEVEL_INFO ,__FILE__,__LINE__,format,##__VA_ARGS__)
#	define warn(format, ...) \
	errorlog(LEVEL_WARN,__FILE__,__LINE__,format,##__VA_ARGS__)
#	define error(format, ...) \
	errorlog(LEVEL_ERROR,__FILE__,__LINE__,format,##__VA_ARGS__)
#else
	void debug(char *format, ...);
	void info(char *format, ...);
	void warn(char *format, ...);
	void error(char *format, ...);
#endif

enum loglevels {
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARN,
	LEVEL_ERROR
};

void open_errorlog(const char *file);
void errorlog(int level, const char *file, int line, const char *format, ...);
void log_assert(const char *file, int line, int statm, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

