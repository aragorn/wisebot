/* $Id$ */
#ifdef STANDALONE_DEBUG
#   undef emerg
#   undef alert
#   undef crit
#   undef error
#   undef warn
#   undef notice
#   undef info
#   undef debug
#   undef EMERG
#   undef ALERT
#   undef CRIT
#   undef ERROR
#   undef WARN
#   undef NOTICE
#   undef INFO
#   undef DEBUG

#   define emerg(format, ...) \
		printf("[_] [emerg] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define alert(format, ...) \
		printf("[_] [alert] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define crit(format, ...) \
		printf("[_] [crit] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define error(format, ...) \
		printf("[_] [error] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define warn(format, ...) \
		printf("[_] [warn] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define notice(format, ...) \
		printf("[_] [notice] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define info(format, ...) \
		printf("[_] [info] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define debug(format, ...) \
		printf("[_] [debug] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")

#   define EMERG(format, ...) \
		printf("[_] [emerg] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define ALERT(format, ...) \
		printf("[_] [alert] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define CRIT(format, ...) \
		printf("[_] [crit] " __FILE__ " " __FUNCTION__"() " format , ##__VA_ARGS__), printf("\n")
#   define ERROR(format, ...) \
		printf("[_] [error] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define WARN(format, ...) \
		printf("[_] [warn] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define NOTICE(format, ...) \
		printf("[_] [notice] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define INFO(format, ...) \
		printf("[_] [info] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")
#   define DEBUG(format, ...) \
		printf("[_] [debug] " __FILE__ " " __FUNCTION__"() " format, ##__VA_ARGS__), printf("\n")

#   undef sb_malloc
#   undef sb_calloc
#   undef sb_free
#   undef sb_realloc
#   define sb_malloc(size) malloc(size)
#   define sb_realloc(ptr, size) realloc(ptr, size)
#   define sb_calloc(nelm, size) calloc(nelm, size)
#   define sb_free(ptr) free(ptr)

#   undef sb_open
#   undef sb_fopen
#   undef sb_unlink
#   define sb_open(path, flag, mode) open(path, flag, mode)
#   define sb_fopen(path, mode) fopen(path, mode)
#   define sb_unlink(path) unlink(path)

#	undef SB_ABORT
#	define SB_ABORT() abort()

#endif
