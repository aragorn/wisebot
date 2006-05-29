/* $Id$ */
#ifndef INDEXER_H
#define INDEXER_H 1

#define MAX_STD_FIELD	32
#define MAX_EXT_FIELD	64 /* XXX: obsolete? no, it's used in mod_qpp/tokenizer.c FIXME*/
#define MAX_FIELD_STRING SHORT_STRING_SIZE

#include "vrfi.h"

typedef union hit_t hit_t;
typedef struct doc_hit_t doc_hit_t;
typedef struct word_hit_t word_hit_t;

#define SKT_BMT
/*** hit_t *******************************************************************/

#define MAX_STD_POSITION 262143 /* 2*18-1 should go with standard_hit_t->position */

#ifdef SKT_BMT
typedef struct {
	uint32_t position	:12; /* max 2^12-1=4095 */
	uint8_t type		:1; /* 0 */
	uint8_t field		:3; /* max 2^3-1=7 */
//	uint8_t dummy;
}__attribute__((packed)) standard_hit_t;
#else
typedef struct {
	uint32_t position	:18; /* max 2^18-1=262143 */
	uint8_t type		:1; /* 0 */
	uint8_t field		:5; /* max 2^5-1=31 */
//	uint8_t dummy;
}__attribute__((packed)) standard_hit_t;
#endif

typedef struct {
	uint16_t dummy;
} dummy_hit_t;

union hit_t {
	dummy_hit_t ext_hit;
	standard_hit_t std_hit;
}; /* 2 bytes */

/*** doc_hit_t ***************************************************************/
#define STD_HITS_LEN		(HIT_SIZE)

#define MAX_NHITS 255
#ifdef SKT_BMT
struct doc_hit_t {
	uint32_t	id;
	uint8_t	field;  /* bitmask for occurence of each field */
//	uint32_t	field;  /* bitmask for occurence of each field */
	uint8_t 	hitratio; /* 包访己 khyang*/
	hit_t		hits[STD_HITS_LEN];
	uint8_t	nhits;
//	uint8_t dummy[3]; // for address align
}__attribute__((packed));
#else
struct doc_hit_t {
	uint32_t	id :24;
	uint32_t	field;  /* bitmask for occurence of each field */
	uint8_t 	hitratio; /* 包访己 khyang*/
	hit_t		hits[STD_HITS_LEN];
	uint8_t	nhits;
//	uint8_t dummy[3]; // for address align
}__attribute__((packed));
#endif

#define MAX_DOCHITS_PER_DOCUMENT 512

typedef struct {
	uint32_t	ndocs;
	uint32_t	ntotal_hits;
} inv_idx_header_t;

#if FORWARD_INDEX == 1
typedef struct {
	uint32_t nwords;
	uint32_t ntotal_hits;
} forwardidx_header_t;
typedef struct doc_hit_t forward_hit_t;
#endif

/*** word_hit_t **************************************************************/
struct word_hit_t {
	uint32_t wordid; 	// 4bytes 
	hit_t hit;		// 2bytes
}; // 6bytes


#define EXTHIT(type,hit)	((type*)& ((hit).ext_hit))

#define ERROR_DOCUMENT	 (-101)
#define INDEXER_WAIT	 (-102)
#define DELETED_DOCUMENT (-103)
	
SB_DECLARE_HOOK(int,index_each_doc, \
	(void* word_db, uint32_t did,word_hit_t *wordhit,uint32_t hitsize,
	 uint32_t *hitidx, void *data, int size) )

SB_DECLARE_HOOK(int,index_one_doc, \
	(uint32_t did,word_hit_t *wordhit,uint32_t hitsize,uint32_t *hitidx) )
SB_DECLARE_HOOK(uint32_t,last_indexed_did,(void))
SB_DECLARE_HOOK(int, print_forwardidx,\
  (VariableRecordFile *v, uint32_t did, \
   char *field, char bprinthits, FILE* stream))

SB_DECLARE_HOOK(int,get_para_position,(hit_t *hit))
SB_DECLARE_HOOK(uint32_t,get_position,(hit_t *hit))
SB_DECLARE_HOOK(int,cmp_field,(hit_t *u, hit_t *v))
SB_DECLARE_HOOK(int,get_field,(hit_t *hit))
SB_DECLARE_HOOK(const char*,get_indexer_socket_file,(void))

#endif
