/* $Id$ */
#include "sb_tstat.h"

int sb_tstat_start(tstat_t *tstat) {
	if ((tstat->cs = times(&(tstat->tms_s))) == (clock_t)-1) {
		error("%s", strerror(errno));
		return -1;
	}
	if (gettimeofday(&(tstat->tv_s), NULL) == -1) {
		error("%s", strerror(errno));
		return -1;
	}
	return 1;
}

int sb_tstat_finish(tstat_t *tstat) {
	if ((tstat->cf = times(&(tstat->tms_f))) == (clock_t)-1) {
		error("%s", strerror(errno));
		return -1;
	}
	if (gettimeofday(&(tstat->tv_f), NULL) == -1) {
		error("%s", strerror(errno));
		return -1;
	}
	return 1;
}

void sb_tstat_print(tstat_t *tstat) {
	clock_t clock_per_second;
/*	printf("gauged by gettimeofday(): Realtime:%d(ms)", */
/*			(tstat->tv_f.tv_sec - tstat->tv_s.tv_sec)*1000 +*/
/*			(tstat->tv_f.tv_usec - tstat->tv_s.tv_usec)/1000);*/
	info("gauged by gettimeofday(): Realtime:%d(s)", 
			(int)(tstat->tv_f.tv_sec - tstat->tv_s.tv_sec));

	clock_per_second = sysconf(_SC_CLK_TCK);
	info("gauged by times(): Realtime:%.2f(s), User Time %.2f(s), System Time %.2f(s)\n",
			(float)(tstat->cf - tstat->cs) / clock_per_second,
			(float)(tstat->tms_f.tms_utime - tstat->tms_s.tms_utime) / clock_per_second,
			(float)(tstat->tms_f.tms_stime - tstat->tms_s.tms_stime) / clock_per_second);
}

/* pid tag gettimeofday(ms) times(ms) usertime(ms) systemtime(ms) */
void sb_tstat_log(FILE *tlog, char *tag) {
	clock_t c;
	struct timeval tv;
	struct tms t;

	const float clock_per_millisecond = (float)sysconf(_SC_CLK_TCK) / 1000.;

	gettimeofday(&tv, NULL);
	c = times(&t);

	fprintf(tlog, "%d %s %.3f ",
			getpid(), tag, (float)tv.tv_sec * 1000. + (float)tv.tv_usec / 1000.);

	fprintf(tlog, "%.3f %.3f %.3f\n",
			(float)c           / clock_per_millisecond,
			(float)t.tms_utime / clock_per_millisecond,
			(float)t.tms_stime / clock_per_millisecond);
}
