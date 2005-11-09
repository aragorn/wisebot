/* $Id$ */
#ifndef HITS_H
#define HITS_H 1

#include "softbot.h"

#define MAX_STD_FIELD	32
#define MAX_EXT_FIELD	64 /* XXX: obsolete? no, it's used in mod_qpp/tokenizer.c FIXME*/
#define MAX_FIELD_STRING SHORT_STRING_SIZE

/* typedef of hit_t */
#define MAX_STD_POSITION 262143 /* 2*18-1 should go with standard_hit_t->position */
typedef struct {
	uint32_t position	:18; /* max 2^18-1=262143 */
	uint8_t type		:1; /* 0 */
	uint8_t field		:5; /* max 2^5-1=31 */
	uint8_t dummy;
}__attribute__((packed)) standard_hit_t;

typedef struct {
	uint16_t dummy;
} dummy_hit_t;

typedef union {
	dummy_hit_t ext_hit;
	standard_hit_t std_hit;
} hit_t; /* 2 bytes */

/* typedef of doc_hit_t */
#define STD_HITS_LEN		(HIT_SIZE)

#define MAX_NHITS 255
typedef struct _doc_hit_t{
	uint32_t	id;
	uint32_t	field;  /* bitmask for occurence of each field */
	uint32_t 	hitratio; /* °ü·Ã¼º khyang*/
	hit_t		hits[STD_HITS_LEN];
	uint8_t	nhits;
	uint8_t dummy[3]; // for address align
}__attribute__((packed)) doc_hit_t;

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
typedef struct _doc_hit_t forward_hit_t;
#endif

struct word_hit_t{
	uint32_t wordid; 	// 4bytes 
	hit_t hit;		// 2bytes
};	// 6bytes 
#ifndef WORD_HIT_T_DEFINED
	typedef struct word_hit_t word_hit_t;
	#define WORD_HIT_T_DEFINED
#endif

#endif
