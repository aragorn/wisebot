/* $Id$ */
#ifndef _DID_DAEMON_H_
#define _DID_DAEMON_H_ 1

#include "softbot.h"

/* XXX: this file includes api interfaces. need to get rid of followings? */
// following three error_definitions are returned by getId and getNewId
#define DI_NOT_REGISTERED		(-1)/* No id exist with given key */
#define DI_ID_OVERFLOW			(-2)/* Cannot make new document id */
#define DI_ID_NOT_EXIST			(-3)/* Non existing id */
#define DI_UNREACHABLE_SERVER	(-4)/* Cannot connect DS server */

#define DI_OLD_REGISTERED		(1) /* Document is already registered */
#define DI_NEW_REGISTERED		(2) /* Document is newly registered */

/* change above to followings? 
 * define DOCID_NOT_REGISTERED
 * define DOCID_OVERFLOW
 * define DOCID_NOT_EXIST
 */
SB_DECLARE_HOOK(int,load_docid_db,())
SB_DECLARE_HOOK(void,unload_docid_db,())

SB_DECLARE_HOOK(int,local_get_docid,(char *pKey, DocId *pDocId))
SB_DECLARE_HOOK(int,local_get_new_docid, \
	(char *pKey, DocId *pDocId, DocId *olddid))

#endif
