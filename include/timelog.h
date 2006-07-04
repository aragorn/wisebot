#ifndef TIMELOG_H
#define TIMELOG_H 1

#if USE_TIMELOG
#	define timelog(tag)			sb_tstat_log(tag)
#else
#	define timelog(tag)
#endif

#endif
