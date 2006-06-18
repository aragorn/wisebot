/* $Id$ */
#ifndef MOD_MP_H
#define MOD_MP_H

#define MONITORING_SCOREBOARD_PERIOD (3)

#define INIT_THREAD_SCOREBOARD(scoreboard,size) \
		sb_first_init_scoreboard(scoreboard,__FILE__,TYPE_THREAD,size)
#define INIT_PROCESS_SCOREBOARD(scoreboard,size) \
		sb_first_init_scoreboard(scoreboard,__FILE__,TYPE_PROCESS,size)

#endif /* MOD_MP_H */
