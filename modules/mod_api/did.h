/* $Id$ */
#ifndef DID_H
#define DID_H 1

#include "softbot.h"
#include "hook.h"

#define FILE_CREAT_FLAG (O_RDWR|O_CREAT)
#define FILE_CREAT_MODE (0600)

#define DOCID_OVERFLOW          (-2)/* Cannot make new document id */

#define DOCID_NOT_REGISTERED       (-1)/* Document is not registered */
#define DOCID_OLD_REGISTERED       (1) /* Document is already registered */
#define DOCID_NEW_REGISTERED       (2) /* Document is newly registered */

#define BLOCK_DATA_SIZE             16000000 /* 16MB */
#define DID_MAX_BLOCK_NUM           256

typedef struct _did_db_t did_db_t;

extern did_db_t *wrapper_diddb;
extern char wrapper_diddb_path[STRING_SIZE];

SB_DECLARE_HOOK(did_db_t*,open_did_db,(char *path))
SB_DECLARE_HOOK(int,sync_did_db,(did_db_t *did_db))
SB_DECLARE_HOOK(int,close_did_db,(did_db_t *did_db))

SB_DECLARE_HOOK(int,get_new_docid, \
		(did_db_t *did_db, char *pKey, uint32_t *docid, uint32_t *olddid))
SB_DECLARE_HOOK(int,get_docid, \
		(did_db_t *did_db, char *pKey, uint32_t *docid))

//extern did_db_t gDidDB;
	
#endif
