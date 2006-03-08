/**
 * $Id$
 * Created by nominam, 2002. 6. 25.
 */
#ifndef _SB_TSTAT_H_
#define _SB_TSTAT_H_ 1

#include "softbot.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

typedef struct {
	clock_t cs, cf;
	struct tms tms_s, tms_f;
	struct timeval tv_s, tv_f;
} tstat_t;

SB_DECLARE(int) sb_tstat_start(tstat_t *tstat);
SB_DECLARE(int) sb_tstat_finish(tstat_t *tstat);
SB_DECLARE(void) sb_tstat_print(tstat_t *tstat);
SB_DECLARE(void) sb_tstat_log(FILE *, char *);

#endif

