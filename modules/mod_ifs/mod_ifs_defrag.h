#ifndef __DEFRAGMENT_H__
#define __DEFRAGMENT_H__

#include "mod_ifs.h"

typedef enum {
	DEFRAG_MODE_BUBBLE,
	DEFRAG_MODE_COPY
} defrag_mode_t;

extern int defrag_group_size;
extern defrag_mode_t defrag_mode;

int ifs_defrag(ifs_t* ifs, scoreboard_t* scoreboard);

#endif

