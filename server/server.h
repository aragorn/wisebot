/* $Id$ */
/* main.h */

#include "softbot.h"
#include "mod_mp/mod_mp.h"

/* refer to static_modules.c */
extern module *server_static_modules[];

/* waitpid stuff - XXX autoconf */
/* this stuff is moved to mod_mp. to be deleted soon.
 * --aragorn, 06/04 */
#include <sys/types.h>
#include <sys/wait.h>
