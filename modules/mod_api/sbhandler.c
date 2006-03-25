/* $id$ */
#include "common_core.h"
#include "sbhandler.h"

HOOK_STRUCT(
	HOOK_LINK(sbhandler_get_table)
	/* HOOK_LINK(sbhandler_make_xmlnode) */
	HOOK_LINK(sbhandler_make_memfile)
	HOOK_LINK(sbhandler_make_file)
	HOOK_LINK(sbhandler_get_nbyte)
	HOOK_LINK(sbhandler_get_file)
	HOOK_LINK(sbhandler_put_file)
	HOOK_LINK(sbhandler_append_msg_record)
	HOOK_LINK(sbhandler_append_file)
)

SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_get_table, \
		(char *name_space, void **tab), \
		(name_space, tab), DECLINE)
/*
SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_append_msg_record, \
		(request_rec *r, msg_record_t *msgs), (r, msgs), DECLINE)
*/

SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_append_file, \
		(request_rec *r, char *path), (r, path), DECLINE)
	
/*
SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_make_xmlnode, \
		(request_rec *r, xmlnode *out), (r, out), DECLINE)
*/

SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_make_memfile, \
		(request_rec *r, memfile **out), (r, out), DECLINE)

SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_make_file, \
		(request_rec *r, char *out_filename), (r, out_filename), DECLINE)
	
SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_get_nbyte, \
		(request_rec *r, void *buf, int nbyte, int *read, int *seeneos), \
		(r, buf, nbyte, read, seeneos), DECLINE)
	
SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_get_file, \
		(request_rec *r, char *out_filename), (r, out_filename), DECLINE)
	
SB_IMPLEMENT_HOOK_RUN_FIRST( int, sbhandler_put_file, \
		(request_rec *r, char *path), (r, path), DECLINE)
