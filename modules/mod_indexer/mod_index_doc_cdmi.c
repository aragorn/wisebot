/* $Id$ */
#include "softbot.h"
#include "mod_indexer.h"
#include "mod_index_doc.h"
#include "mod_api/mod_api.h"
#include "mod_api/register.h"

extern cdm_db_t *m_cdmdb;
/*static char cdmdbname[MAX_DBNAME_LEN];*/
/*static char cdmdbpath[MAX_DBPATH_LEN];*/

static int mNumOfField = 0;
static char rootelement[MAX_FIELD_STRING] = "Document";
static char mFieldName[MAX_EXT_FIELD][MAX_FIELD_STRING];

/* 0 for no morpheme analysis 1 for default morpheme analyzer */
static char mMorpAnalyzerId[MAX_EXT_FIELD]={
	0, 0, 0, 0, 0, 0, 0, 1, 1, 2
};
static char mParagraphSearch[MAX_EXT_FIELD]={
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1
};

#define PARAHIT(hit)	EXTHIT(paragraph_hit_t,hit)

/*static int init() {*/
/*	m_cdmdb = sb_run_cdm_db_open(cdmdbname, cdmdbpath, CDM_SHARED);*/
/*	if (m_cdmdb == NULL) {*/
/*		error("cannot open cdmdb[%s, %s]", cdmdbname, cdmdbpath);*/
/*		return FAIL;*/
/*	}*/

/*	return SUCCESS;*/
/*}*/

int get_fieldid_by_fieldname(char *field)
{
	int i=0;
	for (i=0; i<mNumOfField; i++) {
		if (strncmp(mFieldName[i], field, MAX_FIELD_STRING)==0){
			return i;
		}
	}

	return -1;
}

void fill_dochit
	(doc_hit_t *dochit, DocId did, uint32_t hitnum, word_hit_t *wordhit)
{
	int i=0,savehitnum=0,nshift=0;

	dochit->id = did;
	dochit->nhits = (hitnum > MAX_NHITS) ? MAX_NHITS:hitnum;
	dochit->field = 0;
	savehitnum = (hitnum > STD_HITS_LEN) ? STD_HITS_LEN:hitnum;

	for (i=0; i<hitnum; i++) {
		if (wordhit[i].hit.std_hit.type == 0) { /* std_hit */
			nshift = wordhit[i].hit.std_hit.field;
		} else { /* ext_hit (para_hit_t type) */
			nshift = PARAHIT(wordhit[i].hit)->field;
		}

		if (i<savehitnum) 
			dochit->hits[i] = wordhit[i].hit;
		
		dochit->field |= ( ((uint32_t)1) << nshift);
	}
}

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

int index_one_doc(uint32_t did,word_hit_t *wordhit,
					uint32_t wordhit_size,uint32_t *idx)
{
	char path[STRING_SIZE], fieldtext[DOCUMENT_SIZE];
	int len;
	parser_t *parser;
	field_t *field;
	pdom_t *xmldom;

	char *paragraph=NULL;
	Morpheme morp; int ret=0;
	uint16_t i=0,j=0;
	WordList wordlist;
	uint16_t paragraph_idx=0;
	Paragraph para;
/*	LWord lword;*/
	word_t lexicon;
	char id=0,flag=0;
	paragraph_hit_t *parahit=NULL;
	standard_hit_t *stdhit=NULL;
	Word *word=NULL;

	if ((xmldom = sb_run_cdm_retrieve_internal(m_cdmdb, did)) == NULL) {
		error("error while doc_get document of internal key[%u]",did);
		return FAIL;
	}
	parser = sb_run_cdm_get_parser(xmldom);

	*idx=0;

	for (i=0; i<mNumOfField; i++) {// for each field in document
		snprintf(path, STRING_SIZE, "/%s/%s", rootelement, mFieldName[i]);
		path[STRING_SIZE-1] = '\0';
		field = sb_run_xmlparser_retrieve_field(parser, path);
		if (field == NULL) {
			warn("doc_get_field error for doc of internal key[%u], field[%s]", 
					did, path);
			goto NEXT_FIELD;
		}
		len = field->size > DOCUMENT_SIZE ? DOCUMENT_SIZE : field->size;
		strncpy(fieldtext, field->value, len);
		fieldtext[len] = '\0';

		sb_run_morp_set_paragraphtext(&para,fieldtext);
		paragraph_idx = 0;

		flag=mParagraphSearch[i];
		while ( (paragraph=sb_run_morp_get_paragraph(&para,flag)) != NULL) {

			id=mMorpAnalyzerId[i];

			sb_run_morp_set_text(&morp,paragraph,id);

			while (sb_run_morp_get_wordlist(&morp,&wordlist,id) != FAIL) {
				for (j=0; j<wordlist.num_of_words; j++) {
					word=&(wordlist.words[j]);

					if (mParagraphSearch[i]==1) {
						parahit=EXTHIT(paragraph_hit_t,wordhit[*idx].hit);

						parahit->type = 1;
						parahit->field = i;

						parahit->para_idx = (paragraph_idx < MAX_PARAGRAPH_IDX) ?
												paragraph_idx : MAX_PARAGRAPH_IDX;

						parahit->position = 
							(word->position < MAX_WITHIN_PARAGRAPH_POSITION) ? 
								word->position : MAX_WITHIN_PARAGRAPH_POSITION;
					} else {
						stdhit=&(wordhit[*idx].hit.std_hit);

						stdhit->type = 0;
						stdhit->field = i;
						stdhit->position = (word->position < MAX_STD_POSITION) ?
											word->position : MAX_STD_POSITION;
						debug("word[%s] field:%d, position:%d",
							wordlist.words[j].word, stdhit->field, stdhit->position);
					}

					debug("trying to make new wordid for [%s]",
										wordlist.words[j].word);
					strncpy(lexicon.string, wordlist.words[j].word, MAX_WORD_LENGTH);
					ret = sb_run_get_new_word(&gWordDB, &lexicon, did);
					
					if (ret < 0){
						alert("lexicon returned [%d] for new word[%s]",ret,
													lexicon.string);
						continue;
					}
					wordhit[*idx].wordid = lexicon.id;
					(*idx)++;
					if (*idx >= wordhit_size) {
						warn("doc[%u] has too many words",did);
						warn("increase MAX_WORD_HIT_LEN(%d) and recompile.",wordhit_size);

						ret = SUCCESS;
						goto NEXT_DOC;
					}
				} // for each words made by morp_analyzer

			}	// for each string in the text
			paragraph_idx++;

		}/* for each paragraph in field text 
		    (or whole text in field if not paragraph search) */
	NEXT_FIELD:
	} // for each field in a document

	ret = SUCCESS;
NEXT_DOC:
	sb_run_cdm_release(xmldom);

	return ret;
}

int _cmp_wordhits(const void *a,const void *b)
{
	word_hit_t v1 = *(word_hit_t*)a;
	word_hit_t v2 = *(word_hit_t*)b;

	if ( v1.hit.std_hit.type > v2.hit.std_hit.type )
		return 1;
	else if ( v1.hit.std_hit.type < v2.hit.std_hit.type )
		return -1;

	if ( get_field(&(v1.hit)) > get_field(&(v2.hit)) )
		return 1;
	else if ( get_field(&(v1.hit)) < get_field(&(v2.hit)) )
		return -1;

	if ( get_para_position(&(v1.hit)) > get_para_position(&(v2.hit)) )
		return 1;
	else if ( get_para_position(&(v1.hit)) < get_para_position(&(v2.hit)) )
		return -1;

	if ( get_position(&(v1.hit)) > get_position(&(v2.hit)) )
		return 1;
	else if ( get_position(&(v1.hit)) < get_position(&(v2.hit)) )
		return -1;
	
	return 0;
}
#if FORWARD_INDEX==1
static int print_forwardidx
	(VariableRecord *this, DocId did, \
		char *field, uint32_t skip, uint32_t nelm, char bprinthits,
		FILE* stream)
{
#define MAX_WORDS_PER_DOC 10000 // XXX:refer to mod_indexer.c:MAX_WORDS_PER_DOC
	int i=0,j=0,ret=0;
	char *word=NULL;
	uint8_t nhits=0;
	uint32_t tmp=0,hitsize=0,idx=0;
/*	LWord lword;*/
	word_t lexicon;
   	WordId wordid=0;
	int fieldid=0;
         
	forwardidx_header_t forwardidx_header;
//	forward_hit_t forwardhits[MAX_WORDS_PER_DOC];
//	word_hit_t wordhits[MAX_WORDS_PER_DOC*STD_HITS_LEN];
	forward_hit_t *forwardhits = NULL;
	word_hit_t *wordhits = NULL;

	warn("did:%u, field:%s, skip:%u, nelm:%u, bhits:%d.",
						(uint32_t)did, field, skip, nelm, bprinthits);

	fieldid = get_fieldid_by_fieldname(field);
	if (fieldid < 0) {
		fprintf(stream, "Field[%s] does not exist.\n",field);
		return FAIL;
	}

	ret=sb_run_vrf_get_fixed(this, (uint32_t)did, &forwardidx_header);
	if (ret != SUCCESS) { error("vrf_get_fixed error");return FAIL; }

	tmp = forwardidx_header.nwords;
	tmp = (tmp > MAX_WORDS_PER_DOC) ? MAX_WORDS_PER_DOC:tmp;

	forwardhits = malloc(MAX_WORDS_PER_DOC * sizeof(forward_hit_t));
	wordhits = malloc(MAX_WORDS_PER_DOC*STD_HITS_LEN * sizeof(word_hit_t));

	ret=sb_run_vrf_get_variable(this, (uint32_t)did, 0, tmp, forwardhits);
	if (ret < 0) { 
		free(forwardhits); free(wordhits);
		error("vrf_get_variable error"); 
		return FAIL; 
	}

	for (i=0,idx=0; i<tmp; i++) {
		nhits = forwardhits[i].nhits;
		nhits = (nhits > STD_HITS_LEN) ? STD_HITS_LEN:nhits;
		for (j=0; j<nhits; j++) {
			wordhits[idx].wordid = forwardhits[i].id;
			wordhits[idx].hit = forwardhits[i].hits[j];
			debug("forwardhits[%d].hits[%d].std_hit.field:%d",
					i, j, forwardhits[i].hits[j].std_hit.field);
			idx++;
		}
	}
	hitsize=idx;

	mergesort(wordhits,hitsize,sizeof(word_hit_t),_cmp_wordhits);

	fprintf(stream,"Forward index for doc[%u] field(%s:%d)(nelm,ntotal:%u,%u)\n",
						(uint32_t)did, field, fieldid, nelm, hitsize);

	for (i=0; i<hitsize; i++) {
		hit_t *hit=NULL;
		hit = &(wordhits[i].hit);
		wordid=wordhits[i].wordid;

		if (hit->std_hit.type == 0) { // std_hit
			debug("hit->std_hit.field:%d",hit->std_hit.field);
			if ( hit->std_hit.field != fieldid )
				continue;
		} else { // ext_hit
			debug("hit->exthit.field:%d",EXTHIT(paragraph_hit_t,*hit)->field);
			if ( EXTHIT(paragraph_hit_t,*hit)->field != fieldid )
				continue;
		}

		lexicon.id = wordid;
		ret = sb_run_get_word_by_wordid(&gWordDB, &lexicon);

		if (ret < 0) { 
			error("wordid:%u doesn't exist",(uint32_t)wordid); 
			continue;
		}

		if (i < skip) continue;
		if (i > skip+nelm) break;

/*		word = lword.szWord;*/
		word = lexicon.string;
		fputs(word,stream); 



		if (bprinthits==1) {
			fputs(":(",stream);
			if (hit->std_hit.type == 0) {// std_hit
				fprintf(stream,"f:%d ",hit->std_hit.field);
				fprintf(stream,"pos:%d ",hit->std_hit.position);
			}
			else {
				fprintf(stream,"f:%d ",EXTHIT(paragraph_hit_t,*hit)->field);
				fprintf(stream,"para:%d ",EXTHIT(paragraph_hit_t,*hit)->para_idx);
				fprintf(stream,"pos:%d ",EXTHIT(paragraph_hit_t,*hit)->position);
			}
			fputs(")",stream);
		}
		fputs(", ",stream);
	}
	fputc('\n',stream);

	free(forwardhits);
	free(wordhits);
	return SUCCESS;
#undef MAX_WORDS_PER_DOC
}
#endif

/****************************************************************************/
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
		debug("Field %s does not need indexing",v.argument[1]);
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

	mMorpAnalyzerId[fieldid]=atoi(v.argument[3]);

	if (strcasecmp("YES",v.argument[5]) == 0) { mParagraphSearch[fieldid]=1; }
	else {mParagraphSearch[fieldid]=0;}

	if (mNumOfField != fieldid ) 
		warn("Field should be sorted by fieldid (ascending)");

	mNumOfField = (fieldid+1 >= mNumOfField)? fieldid+1 : mNumOfField;

	return;
}

/*static void set_cdmdb(configValue v)*/
/*{*/
/*	strncpy(cdmdbname, v.argument[0], MAX_DBNAME_LEN);*/
/*	cdmdbname[MAX_DBNAME_LEN-1] = '\0';*/

/*	snprintf(cdmdbpath, MAX_DBPATH_LEN, "%s/%s",*/
/*			gSoftBotRoot, v.argument[1]);*/
/*	cdmdbpath[MAX_DBPATH_LEN-1] = '\0';*/

/*	info("configure: cdmdb[%s, %s]", cdmdbname, cdmdbpath);*/

/*}*/

static config_t config[] = {
	CONFIG_GET("Field",set_fieldname,VAR_ARG,\
			"field name used by indexer (e.g: Title,Author)"),
/*	CONFIG_GET("CdmDatabase", set_cdmdb, 2,*/
/*			"cdm database db name, path: CdmDatabase [dbname dbpath]"),*/
	{NULL}
};

static void register_hooks(void)
{
	static const char *post[]={"mod_indexer.c",NULL};
	/*XXX: HOOK_FIRST, "mod_indexer.c" does not work */
	sb_hook_index_one_doc(index_one_doc, NULL, post, HOOK_MIDDLE);
#if FORWARD_INDEX==1
	sb_hook_print_forwardidx(print_forwardidx, NULL, NULL, HOOK_MIDDLE);
#endif
}

module index_doc_module = {
	STANDARD_MODULE_STUFF,
	config,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize function of module */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
