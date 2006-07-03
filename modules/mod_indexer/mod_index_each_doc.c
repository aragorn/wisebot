/* $Id$ */
#include <stdlib.h> /* abort(3) */
#include <string.h>
#include "common_core.h"
#include "mod_api/indexer.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/vrfi.h"
#include "mod_api/lexicon.h"
#include "mod_daemon_indexer.h"
#include "mod_index_each_doc.h"

static int mNumOfField = 0;
static char mFieldName[MAX_EXT_FIELD][MAX_FIELD_STRING];

//#define PARAHIT(hit)	EXTHIT(paragraph_hit_t,hit)

int get_fieldid_by_fieldname(char *field)
{
	int i=0;

	INFO("mNumOfField:%d", mNumOfField);
	for (i=0; i<mNumOfField; i++) {
		INFO("field[%d]:%s", i, mFieldName[i]);
	}

	for (i=0; i<mNumOfField; i++) {
		if (strncmp(mFieldName[i], field, MAX_FIELD_STRING)==0){
			return i;
		}
	}

	return -1;
}

#ifdef DEBUG_SOFTBOTD
	#define MAX_LACKING_WORDS 100000
	typedef struct {
		uint32_t wordid;
		uint32_t docid;
		uint32_t nshorts;
	} position_lacking_word_t;
	position_lacking_word_t words[MAX_LACKING_WORDS];
	int nlackingwords=0;
#endif

int fill_dochit
	(doc_hit_t *dochits, int max_dochits, uint32_t docid, word_hit_t *wordhits, uint32_t nhitelm)
{
	int dochit_idx=0, i=0, nshift=0;
	uint32_t _nhitelm = 0;
	uint32_t _saved_hit_elm = 0;
	uint32_t wordhit_idx=0;

	_nhitelm = nhitelm;
	for (dochit_idx=0; dochit_idx < max_dochits && wordhit_idx < nhitelm; dochit_idx++) {
		dochits[dochit_idx].id = docid;
		dochits[dochit_idx].nhits = (_nhitelm > STD_HITS_LEN) ? STD_HITS_LEN : _nhitelm;
		dochits[dochit_idx].field = 0;

		for (i=0; i < dochits[dochit_idx].nhits; i++) {
			nshift = wordhits[wordhit_idx].hit.std_hit.field;
			dochits[dochit_idx].field |= ( ((uint32_t)1) << nshift );
			dochits[dochit_idx].hits[i] = wordhits[wordhit_idx].hit;
			wordhit_idx++;
		}
		_nhitelm -= (dochits[dochit_idx].nhits);
		_saved_hit_elm += dochits[dochit_idx].nhits;
	}
#ifdef DEBUG_SOFTBOTD
	/*
	if (nlackingwords >= MAX_LACKING_WORDS) {
		error("nlackingwords[%d] >= MAX_LACKING_WORDS[%d]", nlackingwords, MAX_LACKING_WORDS);
	}
	words[nlackingwords].wordid = wordhits->wordid;
	nlackingwords++; */
#endif
	return dochit_idx;
}

/*
void fill_dochit
	(doc_hit_t *dochit, DocId did, uint32_t hitnum, word_hit_t *wordhit)
{
	int i=0,savehitnum=0,nshift=0;

	dochit->id = did;
	dochit->nhits = (hitnum > MAX_NHITS) ? MAX_NHITS:hitnum;
	dochit->field = 0;
	savehitnum = (hitnum > STD_HITS_LEN) ? STD_HITS_LEN:hitnum;
#ifdef DEBUG_SOFTBOTD
	if (dochit->nhits <= 5) {
		pos_stat[0]++;
	} 
	else if (dochit->nhits <= 10) {
		pos_stat[1]++;
	} 
	else if (dochit->nhits <= 20) {
		pos_stat[2]++;
	}
	else if (dochit->nhits <= 40) {
		pos_stat[3]++;
	}
	else { // > 40
		pos_stat[4]++;
		if (dochit->nhits > *max_pos)
			*max_pos = dochit->nhits;
	}
#endif

	for (i=0; i<hitnum; i++) {
		if (wordhit[i].hit.std_hit.type == 0) { // std_hit 
			nshift = wordhit[i].hit.std_hit.field;
		} else { // ext_hit (para_hit_t type) 
			nshift = PARAHIT(wordhit[i].hit)->field;
		}

		if (i<savehitnum) 
			dochit->hits[i] = wordhit[i].hit;
		
		dochit->field |= ( ((uint32_t)1) << nshift);
	}
}*/

#if 0
int cmp_field(hit_t *u,hit_t *v)
{
	if (u->std_hit.type != v->std_hit.type)
		return FALSE;
	
	if (u->std_hit.type == 0) { /* std_hit */
		if (u->std_hit.field == v->std_hit.field) { return TRUE; }
		else { return FALSE; }
	}
	else { /* ext_hit */
		if ( PARAHIT(*u)->field == PARAHIT(*v)->field ) { return TRUE; }
		else { return FALSE; }
	}

	return TRUE; /* never reaches here */
}

int get_field(hit_t *hit)
{
	if (hit->std_hit.type == 0) {
		return hit->std_hit.field;
	}
	else {
		return EXTHIT(paragraph_hit_t,*hit)->field;
	}
	return -1; /* never reaches here */
}

int get_para_position(hit_t *hit)
{
	if (hit->std_hit.type == 0) {
		return 0;
	}
	else {
		return EXTHIT(paragraph_hit_t,*hit)->para_idx;
	}
	return -1; /* never reaches here */
}

uint32_t get_position(hit_t *hit)
{
	if (hit->std_hit.type == 0) {
		return hit->std_hit.position;
	} else {
		return EXTHIT(paragraph_hit_t,*hit)->position;
	}

	return -1; /* never reaches here */
}
#endif

int index_each_doc(void* word_db, uint32_t docid, word_hit_t *wordhit,
							uint32_t wordhit_size, uint32_t *idx, void *data, int size)
{
	index_word_t *indexwords=NULL;
	int32_t num_of_indexwords=0;
	standard_hit_t *stdhit=NULL;
	int i = 0, rv=0;
	word_t lexicon;	

	indexwords = (index_word_t *)data;
	num_of_indexwords = size / sizeof(index_word_t);

	(*idx) = 0;
	for (i=0; i<num_of_indexwords; i++) {
		if ( (*idx) >= wordhit_size ) {
			warn("document[%u] has too many words ( > wordhit_size[%u])",
							docid, wordhit_size);
			break;
		}

		// XXX: assuming every hit is standard_hit_t (not ext_hit)
		stdhit=&(wordhit[*idx].hit.std_hit);
		stdhit->field = indexwords[i].field;
		stdhit->position = (indexwords[i].pos < MAX_STD_POSITION) ?
								indexwords[i].pos : MAX_STD_POSITION;
		
		if (indexwords[i].word[0] == '\0') {
			warn("index word is null");
			continue;
		}

		strncpy(lexicon.string, indexwords[i].word, MAX_WORD_LEN);
		lexicon.string[MAX_WORD_LEN-1]='\0';
		rv = sb_run_get_new_wordid(word_db, &lexicon );
		if (rv < 0) {
			error("cannot make new wordid with word[%s], ret[%d], docid[%u]",
											lexicon.string, rv, docid);
			INFO("indexwords[%d].word:%s", *idx, indexwords[i].word);
			continue;
		}
		wordhit[*idx].wordid = lexicon.id;

		(*idx)++;
	}

	return SUCCESS;
}

int _cmp_wordhits(const void *a,const void *b)
{
	word_hit_t v1 = *(word_hit_t*)a;
	word_hit_t v2 = *(word_hit_t*)b;

	if ( v1.hit.std_hit.field > v2.hit.std_hit.field )
		return 1;
	else if ( v1.hit.std_hit.field < v2.hit.std_hit.field )
		return -1;

	/* XXX: paragraph search is not used now.. 
	if ( get_para_position(&(v1.hit)) > get_para_position(&(v2.hit)) )
		return 1;
	else if ( get_para_position(&(v1.hit)) < get_para_position(&(v2.hit)) )
		return -1;
	*/

	if ( v1.hit.std_hit.position > v2.hit.std_hit.position )
		return 1;
	else if ( v1.hit.std_hit.position < v2.hit.std_hit.position )
		return -1;
	
	return 0;
}
#if FORWARD_INDEX==1
static int print_forwardidx (VariableRecordFile *this, 
	 	DocId did, char *field, char bprinthits, FILE* stream)
{
#define MAX_WORDS_PER_DOC 1000000 // XXX:refer to mod_indexer.c:MAX_WORDS_PER_DOC
	int i=0,j=0,ret=0;
	char *word=NULL;
	uint8_t nhits=0;
	uint32_t hitsize=0,idx=0;
	word_t lword;
   	WordId wordid=0;
	int fieldid=0, nwords=0;
         
	forward_hit_t *forwardhits = NULL;
	word_hit_t *wordhits = NULL;

	fprintf(stream, "did:%u, field:%s, bhits:%d.\n",
					(uint32_t)did, field, bprinthits);

	fieldid = get_fieldid_by_fieldname(field);
	if (fieldid < 0) {
		fprintf(stream, "Field[%s] does not exist.\n",field);
		return FAIL;
	}

	forwardhits = sb_malloc(MAX_WORDS_PER_DOC * sizeof(forward_hit_t));
	wordhits = sb_malloc(MAX_WORDS_PER_DOC*STD_HITS_LEN * sizeof(word_hit_t));

	nwords=sb_run_vrfi_get_variable(this, (uint32_t)did, 0, MAX_WORDS_PER_DOC, forwardhits);
	if (nwords < 0) { 
		sb_free(forwardhits); sb_free(wordhits);
		error("vrfi_get_variable error"); 
		return FAIL; 
	}

	INFO("nwords:%d", nwords);

	for (i=0,idx=0; i<nwords; i++) {
		nhits = forwardhits[i].nhits;
		if (nhits > STD_HITS_LEN) nhits = STD_HITS_LEN;

		for (j=0; j<nhits; j++) {
			wordhits[idx].wordid = forwardhits[i].id;
			wordhits[idx].hit = forwardhits[i].hits[j];
		/*	DEBUG("forwardhits[%d].hits[%d].std_hit.field:%d",
					i, j, forwardhits[i].hits[j].std_hit.field);*/
			idx++;
		}
	}
	hitsize=idx;

	mergesort(wordhits,hitsize,sizeof(word_hit_t),_cmp_wordhits);

	fprintf(stream,"Forward index for doc[%u] field(%s:%d)(ntotal:%u)\n",
						(uint32_t)did, field, fieldid, hitsize);

	for (i=0; i<hitsize; i++) {
		hit_t *hit=NULL;
		hit = &(wordhits[i].hit);
		wordid=wordhits[i].wordid;

		if (hit->std_hit.type == 0) { // std_hit
/*			DEBUG("hit->std_hit.field:%d",hit->std_hit.field);*/
			if ( hit->std_hit.field != fieldid )
				continue;
		} else { // ext_hit
			crit("hit type 1 is not supported yet");
			continue;
		}

		lword.id = wordid;
		ret = sb_run_get_word_by_wordid(&gWordDB, &lword); 
		if (ret < 0) { 
			error("wordid:%u doesn't exist",(uint32_t)lword.id); 
			continue;
		}

		word = lword.string;
		fputs(word, stream);
		
		if (bprinthits==1) {
			fputs(":(",stream);
			if (hit->std_hit.type == 0) {// std_hit
				fprintf(stream,"pos:%d",hit->std_hit.position);
			}
			else {
				crit("hit type 1 is not supported yet");
			}
			fputs(")",stream);
		}
		fputs(",\t",stream);
		if (i%4 == 0) fputs("\n",stream);
	}
	fputc('\n',stream);

	fprintf(stream, "did:%u, field:%s, bhits:%d.\n",
					(uint32_t)did, field, bprinthits);

	sb_free(forwardhits);
	sb_free(wordhits);
	return SUCCESS;
#undef MAX_WORDS_PER_DOC
}
#endif

static void register_hooks(void)
{
	static const char *post[]={"mod_indexer.c",NULL};
	/*XXX: HOOK_FIRST, "mod_indexer.c" does not work */
	sb_hook_index_each_doc(index_each_doc, NULL, post, HOOK_MIDDLE);
#if FORWARD_INDEX==1
	sb_hook_print_forwardidx(print_forwardidx, NULL, NULL, HOOK_MIDDLE);
#endif

//	sb_hook_get_para_position(get_para_position, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_get_position(get_position, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_cmp_field(cmp_field, NULL, NULL, HOOK_MIDDLE);
//	sb_hook_get_field(get_field, NULL, NULL, HOOK_MIDDLE);
}

static void set_fieldname(configValue v)
{
	int fieldid=0;

	if (v.argNum < 6) {
		error("Field directive must have 6 args at least.");
		error("\t e.g) Field 1 Court yes(index) 0(morp_id) 0(morp_id) yes(paragraph search)");
		error("\t but argNum:%d for Field %s",v.argNum,v.argument[0]);
		return;
	}

	if (strcasecmp("yes",v.argument[2]) != 0) {
		DEBUG("Field %s does not need indexing",v.argument[1]);
		return;
	}

	fieldid = atoi(v.argument[0]);
	if (fieldid >= MAX_EXT_FIELD) {
		error("fieldid(%d) for Field:%s >= MAX_EXT_FIELD(%d)",
					fieldid, v.argument[1], MAX_EXT_FIELD);
		return;
	}

	strncpy(mFieldName[fieldid],v.argument[1],MAX_FIELD_STRING);
	mFieldName[fieldid][MAX_FIELD_STRING-1] = '\0';

	if (mNumOfField != fieldid ) 
		warn("Field should be sorted by fieldid (ascending)");

	mNumOfField = (fieldid+1 >= mNumOfField)? fieldid+1 : mNumOfField;

	return;
}

static config_t config[] = {
	CONFIG_GET("Field",set_fieldname,VAR_ARG,\
			"field name used by indexer (e.g: Title,Author)"),
	{NULL}
};

module index_each_doc_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,    				/* registry */
	NULL,					/* initialize function of module */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};

