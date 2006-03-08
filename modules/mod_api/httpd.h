#ifndef _HTTPD_H_
#define _HTTPD_H_

#include "softbot.h"
#include "hook.h"


SB_DECLARE_HOOK( int, httpd_ipc_handler, \
	(int id, int input_len, void *input, int *output_len, void **output))

#endif // _HTTPD_H_
