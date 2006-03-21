/* $Id$ */
#ifndef SBHANDLER_H
#define	SBHANDLER_H 1

#include "mod_httpd_handler/mod_standard_handler.h"

SB_DECLARE_HOOK( int, sbhandler_get_table, \
		(char *name_space, void **tab))

/*
SB_DECLARE_HOOK ( int, sbhandler_append_msg_record, \
		(request_rec *r, msg_record_t *msgs))
*/
SB_DECLARE_HOOK ( int, sbhandler_append_file, \
		(request_rec *r, char *path))
/*
SB_DECLARE_HOOK( int, sbhandler_make_xmlnode, \
		(request_rec *r, xmlnode *out))
SB_DECLARE_HOOK( int, sbhandler_make_memfile, \
		(request_rec *r, memfile **out))
SB_DECLARE_HOOK( int, sbhandler_make_file, \
		(request_rec *r, char *out_filename))

SB_DECLARE_HOOK( int, sbhandler_get_nbyte, \
		(request_rec *r, void *buf, int nbyte, int *read, int *seeneos))
	
SB_DECLARE_HOOK( int, sbhandler_get_file, \
		(request_rec *r, char *out_filename))
	
SB_DECLARE_HOOK ( int, sbhandler_put_file, \
		(request_rec *r, char *path))
*/	
#endif //_SBHANDLER_H_
