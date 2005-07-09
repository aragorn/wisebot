/* $Id$ */
#ifndef DID2_H
#define DID2_H 1

#include "softbot.h"
#include "hook.h"

#define DOCID_OVERFLOW             (-11)/* Cannot make new document id */

#define DOCID_NOT_REGISTERED       (-12)/* Document is not registered */
#define DOCID_OLD_REGISTERED       (11) /* Document is already registered */
#define DOCID_NEW_REGISTERED       (12) /* Document is newly registered */

#define MAX_DOCID_KEY_LEN          (1024)

typedef struct _did_db_t {
	int set;
	void* db;
} did_db_t;

#define MAX_DID_SET (10)

SB_DECLARE_HOOK(int, open_did_db, (did_db_t** did_db, int opt))
SB_DECLARE_HOOK(int, sync_did_db, (did_db_t* did_db))
SB_DECLARE_HOOK(int, close_did_db, (did_db_t* did_db))

SB_DECLARE_HOOK(int,get_new_docid, \
		(did_db_t* did_db, char *pKey, uint32_t *docid, uint32_t *olddid))
SB_DECLARE_HOOK(int,get_docid, \
		(did_db_t* did_db, char *pKey, uint32_t *docid))

#endif
