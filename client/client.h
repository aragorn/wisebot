/*
 * $Id$
 */
#ifndef _SOFTBOT_CLIENT_H_
#define _SOFTBOT_CLIENT_H_ 1

#include <readline/readline.h>
#include <readline/history.h>
#include "mod_api/lexicon.h"
#include "mod_api/did.h"
#include "mod_api/cdm.h"
#include "mod_api/cdm2.h"
#include "ansi_color.h"

char *stripwhite(char *line);
char *dupstr(char *s);
int execute_line(char *line);

extern module *client_static_modules[];
extern char mServerAddr[SHORT_STRING_SIZE];
extern char mServerPort[SHORT_STRING_SIZE];

typedef struct {
	char *name;				/* user printable name of the function. */
	rl_icpfunc_t *func;		/* function to call to do the job. */
	char *desc;				/* documentation for this function. */
} COMMAND;

#define BLANKLINE	"\tprint blank line"
#define HIDE		ON_BLACK BLACK

extern COMMAND commands[];

extern cdm_db_t* mCdmDb;
extern did_db_t* mDidDb;
extern word_db_t* mWordDb;

#endif
