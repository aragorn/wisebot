#include "softbot.h"
#include "httpd.h"

HOOK_STRUCT(
	HOOK_LINK(httpd_ipc_handler)
)

SB_IMPLEMENT_HOOK_RUN_FIRST( int, httpd_ipc_handler, \
	(int id, int input_len, void *input, int *output_len, void **output), \
	(id, input_len, input, output_len, output), DECLINE)
