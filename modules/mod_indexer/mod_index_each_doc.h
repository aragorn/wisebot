/* $Id$ */
#ifndef MOD_INDEX_EACH_DOC_H
#define MOD_INDEX_EACH_DOC_H 1

#include "auto_config.h"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

/* XXX: obsolete but used by print forward index, and some functions.. */
typedef struct{
	uint16_t para_idx	:3; /* max 2^3-1=7 */
	uint16_t position	:7; /* max 2^7-1=127 */
	uint8_t type		:1; /* 1 */
	uint8_t field		:5; /* max 2^3-1=7 */
}__attribute__((packed)) paragraph_hit_t; /* see hit_t, standard_hit_t */

#define MAX_WITHIN_PARAGRAPH_POSITION 127
#define MAX_PARAGRAPH_IDX 7

extern int get_para_position(hit_t *hit);
extern uint32_t get_position(hit_t *hit);
extern int cmp_field(hit_t *u, hit_t *v);
extern int get_field(hit_t *hit);

#endif
