/* $Id$ */

#include "mod_api/rmas.h"
#include "mod_api/lexicon.h"
#include "mod_api/cdm.h"
#include "mod_api/docapi.h"
#include "mod_api/indexer.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/qpp.h"
#include "mod_api/did.h"

#include "mod_client.h"
#include "mod_indexer/hit.h"
#include "mod_site/mod_docattr_lgcaltex.h"

typedef struct {
	char *name;				/* user printable name of the function. */
	char *desc;				/* documentation for this function. */
} COMMAND;

#define BLANKLINE	"\tprint blank line"
#define HIDE		ON_BLACK BLACK

#define MAX_ENUM_NUM		1024
#define MAX_ENUM_LEN		SHORT_STRING_SIZE

int TCPSendLongData(int sockfd, long size, void *data, int server_side);

COMMAND commands[] = {
	{ "help", "display this text" },
	{ BLANKLINE, ""},

	{ "Misc",  ""},
	{ "rmas" ,  "rmas morphological analyzer without protocol4"},
	{ "indexwords",  "see how string is divided by index_word_extractor"},
	{ "tokenizer",  "result of tokenizing" },
	{ "qpp" ,  "result of preprocess" },
	{ BLANKLINE,  ""},

	{ "Canned Doc",  ""},
	{ "getsize","get size of document e.g) getsize 1" },
	{ "getabstract","get abstracted document " },
	{ "getfield","get field of document e.g) getfield 1 Body" },
	{ BLANKLINE,  ""},

	{ "undelete", "undelete document by did" },
	{ BLANKLINE, ""},

	{ "QueryProcessor",  ""},
	{ BLANKLINE,  ""},
	
	{ "Status",  ""},
	{ BLANKLINE,  ""},

	{ "Lexicon",  ""},
	{ "get_wordid", "get wordid for given word"},
	{ "get_new_wordid",  "get new wordid for given word"},
	{ "print_hash_bucket" ,  "print wordid"},
	{ BLANKLINE,  ""},

	{ "Docid",  ""},
	{ "getdocid",  "retreive document id"},
	{ BLANKLINE,  ""},
	
	{ NULL,		NULL }
};


COMMAND *find_command();

/* readline related stuff */

	
char *dupstr(char *s) {
	char *r;

	r = (char*)sb_malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}


COMMAND *find_command(char *name)
{
	register int i;

	for ( i = 0; commands[i].name; i++ )
		if (strcmp (name, commands[i].name) == 0)
			return (&commands[i]);

	return NULL;
}

/* interface to readline completion */

char *command_generator(const char *, int);

char *command_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	if (!state){
	   list_index = 0;
	   len = strlen (text);
	 }

	/* Return the next name which partially matches from the command list. */
	while ( (name = commands[list_index].name) ){
		list_index++;

		if (strncmp (name, text, len) == 0) {
			return (dupstr(name));
		}
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}
/* end readline related stuff */

int sb4_com_get_doc_size(int sockfd, char *arg)
{
	int result=0, len=0;
	char tmpbuf[1024];
	uint32_t docid = atol(arg);

	result = sb_run_server_canneddoc_get_size(docid);
	if (result < 0) {
		error("canneddoc_get error.\n");
		send_nak_str(sockfd,"canneddoc_get error.\n");
		return FAIL;
	}

	info("size of document[%u]: %d", docid, result);
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 6. send doc size */
	sprintf(tmpbuf, "%d", result);
	len = strlen(tmpbuf);

	if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
		error("cannot send cdm size");
		return FAIL;
	}

	return SUCCESS;
}
int sb4_com_get_abstracted_doc(int sockfd, char *arg)
{
	int result=0, offset, size, len=0, docsize=0;
	char field[MAX_FIELD_NAME_LEN];
	VariableBuffer var;
	RetrievedDoc rdoc;
	uint32_t docid = 0;
	char tmpbuf[STRING_SIZE];

#ifdef PROCESS_HANDLE
	static char *buf = NULL;
	if (buf == NULL) {
		buf = (char *)sb_malloc(DOCUMENT_SIZE);
		if (buf == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
#else
	char buf[DOCUMENT_SIZE];
#endif


	result = sscanf(arg, "%u %s %d %d", &docid, field, &offset, &size);
	if (result != 4) {
		printf("usage: getabstract [docid] [fieldno] [offset] [size]\n");
		send_nak_str(sockfd, "usage: getabstract [docid] [fieldno] [offset] [size]\n");
		return FAIL;
	}

	printf("you want document[%u].\n",docid);

	sb_run_buffer_initbuf(&var);

	rdoc.docId = docid;
	rdoc.rank = 0;
	rdoc.rsv = 0;
	rdoc.numAbstractInfo = 1;
	strncpy(rdoc.cdAbstractInfo[0].field, field, MAX_FIELD_NAME_LEN);
	rdoc.cdAbstractInfo[0].field[MAX_FIELD_NAME_LEN-1] = '\0';
#ifdef PARAGRAPH_POSITION
	rdoc.cdAbstractInfo[0].paragraph_position = 0;
#endif
	rdoc.cdAbstractInfo[0].position = offset;
	rdoc.cdAbstractInfo[0].size = size;

	result = sb_run_server_canneddoc_get_abstract(1, &rdoc, &var);
	if (result < 0) {
		printf("canneddoc_get_abstract error.\n");
		send_nak_str(sockfd, "canneddoc_get_abstract error.\n");
		sb_run_buffer_freebuf(&var);
		return FAIL;
	}

	docsize = sb_run_buffer_getsize(&var);
	sb_run_buffer_get(&var, 0, docsize, buf);
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 6. send field size */
	snprintf(tmpbuf, STRING_SIZE, "%u", (uint32_t)strlen(buf));
	tmpbuf[STRING_SIZE-1] = '\0';
	len = strlen(tmpbuf);

	if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
		error("cannot send field size");
		return FAIL;
	}

	/* 7. send field data */
	if ( TCPSendLongData(sockfd, strlen(buf), buf, FALSE) == FAIL ) {
		error("cannot send document");
		return FAIL;
	}
	
	sb_run_buffer_freebuf(&var);
	return SUCCESS;
}

int sb4_com_get_field(int sockfd, char *arg) 
{ 
	uint32_t docid; 
	char fieldname[256], *value; 
	int n, len=0; 
	DocObject *doc; 
	char tmpbuf[LONG_STRING_SIZE];
 
	// get argument 
	n = sscanf(arg, "%u %s", &docid, fieldname); 
	if (n != 2) { 
		printf("usage: getfield docid fieldname\n"); 
		send_nak_str(sockfd, "usage: getfield docid fieldname\n");
		return FAIL; 
	} 

	n = sb_run_doc_get(docid, &doc); 
	if (n < 0) { 
		sprintf(tmpbuf,"cannot get document object of document[%u]\n", docid); 
		send_nak_str(sockfd, tmpbuf); 
		return FAIL; 
	} 

	n = sb_run_doc_get_field(doc, NULL, fieldname, &value); 
	if (n < 0) { 
		sprintf(tmpbuf, "cannot get field[%s] from document object\n", fieldname); 
		send_nak_str(sockfd, tmpbuf);
		return FAIL; 
	} 

 	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		sb_free(value);
		return FAIL;
	}


	/* 6. send field size */
	snprintf(tmpbuf, STRING_SIZE, "%u", (uint32_t)strlen(value));
	
	tmpbuf[STRING_SIZE-1] = '\0';
	len = strlen(tmpbuf);

	if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
		error("cannot send field size");
		sb_free(value);
		return FAIL;
	}

	/* 7. send field data */
	if ( TCPSendLongData(sockfd, strlen(value), value, FALSE) == FAIL ) {
		error("cannot send document");
		sb_free(value);
		return FAIL;
	}
	
	sb_free(value); 
	sb_run_doc_free(doc); 
	
	return SUCCESS; 
} 

int sb4_com_get_oid_field(int sockfd, char *arg, void* did_db) 
{ 
	uint32_t docid; 
	char oid[256]; 
	char fieldname[256], *value; 
	int n, ret; 
	DocObject *doc; 
	char tmpbuf[LONG_STRING_SIZE];
 
	// get argument 
	n = sscanf(arg, "%s %s", oid, fieldname); 
	if (n != 2) { 
		printf("usage: getfield oid fieldname\n"); 
		send_nak_str(sockfd, "usage: getfield oid fieldname\n");
		return FAIL; 
	} 
	/* get docid */
	ret = sb_run_get_docid(did_db, oid, &docid);
	if (ret < 0) {
		sprintf(tmpbuf,"cannot get document id: error[%d]\n", ret);
		send_nak_str(sockfd, tmpbuf);
		return FAIL;
	}
//info("did:%d", docid);
	n = sb_run_doc_get(docid, &doc); 
	if (n < 0) { 
		sprintf(tmpbuf,"cannot get document object of document[%u]\n", docid); 
		send_nak_str(sockfd, tmpbuf); 
		return FAIL; 
	} 

	n = sb_run_doc_get_field(doc, NULL, fieldname, &value); 
	if (n < 0) { 
		sprintf(tmpbuf, "cannot get field[%s] from document object\n", fieldname); 
		send_nak_str(sockfd, tmpbuf);
		return FAIL; 
	} 
//info("field:%s", fieldname);
	value[SB4_MAX_SEND_SIZE-1] = '\0';
	if ( strlen(value) == 0 )
	{
		info("size zero");
          	sprintf(tmpbuf, "cannot get field[%s] from document object-size zero!!\n", fieldname);
		send_nak_str(sockfd, tmpbuf);
	}
	else
	{
	//	info("ACK");
  	        /* 4. send ACK */
	        if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
	                error("cannot send ACK");
			sb_run_doc_free(doc);
	                sb_free(value);
	                return FAIL;
	        }

	//	info("value:%s", value);
	        /* 5. send field data */
		if ( TCPSendData(sockfd, value, strlen(value), FALSE) == FAIL ) {
			error("cannot send field size");
			sb_free(value);
			sb_run_doc_free(doc);
			return FAIL;
		}
	}

	sb_free(value); 
	sb_run_doc_free(doc); 
	
	return SUCCESS; 
} 

int sb4_com_tokenizer (int sockfd, char *arg)
{
	tokenizer_t *tokenizer=NULL;
	token_t tokens[10];
	int i=0,j=0,r=0, len=0;
	char tmpbuf[LONG_STRING_SIZE];

	tokenizer = sb_run_new_tokenizer();

	if (strlen(arg) == 0) {
		send_nak_str(sockfd, "Usage: tokenizer strings\n       max number of token:100\n");
		return FAIL;
	}

	sb_run_tokenizer_set_text(tokenizer,arg);
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	i=0;
	while (1) {
		r=sb_run_get_tokens(tokenizer, tokens, 10);

		/* 6. send data exist */
		if ( TCPSendData(sockfd, "EXIST", 5, FALSE) == FAIL ) {
			error("cannot send exist");
			return FAIL;
		}	
		
		/* 7. send word cnt */
		sprintf(tmpbuf, "%d", r);
		len = strlen(tmpbuf);

		if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
			error("cannot send word cnt");
			return FAIL;
		}	
		
		memset(tmpbuf, 0x00, LONG_STRING_SIZE);	
		
		for (j=0; j<r; j++) {
			
			/* 8. send word info */
			sprintf(tmpbuf, "[%d]'th get_toknes call: %s[len:%d]\n", i, tokens[j].string, tokens[j].len);
			len = strlen(tmpbuf);

			if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
				error("cannot send word info");
				return FAIL;
			}
	
			if (tokens[j].type == TOKEN_END_OF_DOCUMENT)
				goto END;
		}
		i++;
	}
END:
	/* 9. send data exist */
		if ( TCPSendData(sockfd, "NOTEXIST", 8, FALSE) == FAIL ) {
			error("cannot send exist");
			return FAIL;
		}
		
	return 0;
}


int sb4_com_index_word_extractor (int sockfd, char *arg)
{
	char *text=NULL,*idstr=NULL;
	index_word_t indexwords[100];
	index_word_extractor_t *extractor=NULL;
	int i=0, n=0, id=0, len = 0;
	char tmpbuf[LONG_STRING_SIZE];

	if (strlen(arg) == 0) {
		send_nak_str(sockfd,"Non Format");
		return FAIL;
	}

	idstr = arg;
	text = strchr(arg,' ');
	if (text == NULL)  {
		send_nak_str(sockfd,"Non Format");
		return FAIL;
	}
	*text = '\0';
	text++;

	id = atoi(idstr);

	warn("id:%d, text:%s \n", id, text);

	extractor = sb_run_new_index_word_extractor(id);
	if ( extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE ) return FAIL;
	sb_run_index_word_extractor_set_text(extractor, text);
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	while ( (n=sb_run_get_index_words(extractor, indexwords, 100)) > 0) {
		debug("n:%d", n);
		
		/* 6. send data exist */
		if ( TCPSendData(sockfd, "EXIST", 5, FALSE) == FAIL ) {
			error("cannot send exist");
			return FAIL;
		}	
		
		/* 7. send word cnt */
		sprintf(tmpbuf, "%d", n);
		len = strlen(tmpbuf);

		if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
			error("cannot send word cnt");
			return FAIL;
		}	
		
		memset(tmpbuf, 0x00, LONG_STRING_SIZE);	
		for (i=0; i<n; i++) {
			
			/* 8. send word info */
			sprintf(tmpbuf, "[%s] \t (pos:%3d, field:%d)\n", indexwords[i].word, indexwords[i].pos, indexwords[i].field);
			len = strlen(tmpbuf);
			if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
				error("cannot send word info");
				return FAIL;
			}
		}
	}

	/* 9. send data no exist */
	if ( TCPSendData(sockfd, "NOTEXIST", 8, FALSE) == FAIL ) {
		error("cannot send no exist");
		return FAIL;
	}	
	
	sb_run_delete_index_word_extractor(extractor);

	return 0;
}

int sb4_com_index_word_extractor2 (int sockfd, char *arg)
{
        char *text=NULL,*idstr=NULL;
        index_word_t indexwords[100];
        index_word_extractor_t *extractor=NULL;
        int i=0, n=0, id=0, len = 0;
        char tmpbuf[LONG_STRING_SIZE];

        if (strlen(arg) == 0) {
                send_nak_str(sockfd,"Non Format");
                return FAIL;
        }

        idstr = arg;
        text = strchr(arg,' ');
        if (text == NULL)  {
                send_nak_str(sockfd,"Non Format");
                return FAIL;
        }
        *text = '\0';
        text++;

        id = atoi(idstr);

        info("id:%d, text:%s \n", id, text);

        extractor = sb_run_new_index_word_extractor(id);
        if ( extractor == NULL || extractor == (index_word_extractor_t*)MINUS_DECLINE ) return FAIL;
        sb_run_index_word_extractor_set_text(extractor, text);

        n=sb_run_get_index_words(extractor, indexwords, 100);
        info("n:%d", n);
	

        /* 5. send word cnt */
/*        sprintf(tmpbuf, "%d", n);
        len = strlen(tmpbuf);

        if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
               error("cannot send word cnt");
               return FAIL;
        }
*/
        memset(tmpbuf, 0x00, LONG_STRING_SIZE);
        for (i=0; i<n; i++) {
                strcat(tmpbuf, indexwords[i].word);
		strcat(tmpbuf, "^");
        }
	/* 6. send word info */
	len = strlen(tmpbuf);
	if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
		error("cannot send word info");
		return FAIL;
	}

        sb_run_delete_index_word_extractor(extractor);

        return 0;
}

int sb4_com_qpp (int sockfd, char *arg, void* word_db)
{
	QueryNode qnodes[64];
	int numofnodes=0, len = 0;
	char result[20000];
	
	if (strlen(arg) == 0) {
		send_nak_str(sockfd,"Usage: qpp query e.g)qpp ¿À·»Áö & »ç°ú\n");
		return FAIL;
	}
	
	memset(result, 0x00, 20000);
	
	numofnodes = sb_run_preprocess(word_db, arg,1024, qnodes, 64);
	sb_run_buffer_querynode_LG(result,qnodes,numofnodes);
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	/* 6. send qpp */
	len = strlen(result);

	if ( TCPSendData(sockfd, result, len, FALSE) == FAIL ) {
		error("cannot send qppt");
		return FAIL;
	}
	
	return SUCCESS;
}

int sb4_com_rmas_run(int sockfd, char *arg)
{
	int field=0, morpid=0, len = 0, wordcnt = 0;
	char *usage="Usage: rmas fieldid morpid text";
	char *fieldstr=NULL, *morpidstr=NULL, *text=NULL;
	index_word_t *indexword=NULL;
	int num_of_indexwords=0, i=0;
	int rv=0;
	char tmpbuf[LONG_STRING_SIZE];

	fieldstr = arg;
	morpidstr = strchr(arg,' ');
	if (morpidstr == NULL)  {
		send_nak_str(sockfd, usage);
		return FAIL;
	}
	*morpidstr = '\0';
	morpidstr++;

	text = strchr(morpidstr,' ');
	if (text == NULL) {
		send_nak_str(sockfd, usage);
		return FAIL;
	}
	*text = '\0';
	text++;

	field = atoi(fieldstr);
	morpid = atoi(morpidstr);


	warn("field:%d, morpid:%d", field, morpid);
	rv = sb_run_rmas_morphological_analyzer(field, text, (void *)&indexword,  &num_of_indexwords, morpid);
	if (rv != SUCCESS) {
		error("rmas_morphological_analyzer failed. return value[%d]", rv);
		return FAIL;
	}

	wordcnt = num_of_indexwords/ sizeof(index_word_t);
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 6. send word cnt */

	sprintf(tmpbuf, "%d", wordcnt);
	len = strlen(tmpbuf);

	if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
		error("cannot send word cnt");
		return FAIL;
	}
	
	memset(tmpbuf, 0x00, LONG_STRING_SIZE);
	for (i=0; i<wordcnt; i++) {
		/* 7. send word info */
		sprintf(tmpbuf, "%s\t(pos:%u, field:%u\n", indexword[i].word, indexword[i].pos, indexword[i].field);
		len = strlen(tmpbuf);
		if ( TCPSendData(sockfd, tmpbuf, len, FALSE) == FAIL ) {
			error("cannot send word info");
			return FAIL;
		}
	}
	return SUCCESS;
	
}

int sb4_com_undelete (int sockfd, char *arg)
{
	int i, j;
	uint32_t docid[1024], start=0, finish=0;
	char *comma=arg;

	if (!strlen(arg)) {
		send_nak_str(sockfd, "Non format!!\n");
		return FAIL;
	}

	for (i=0, j=0; arg[i]!='\0'; ) {
		if (arg[i] == ',') {
			if (start) {
				finish = atol(comma);
				for (;start<=finish;start++, j++) {
					docid[j] = start;
				}
				start = 0;
				finish = 0;
			}
			else {
				docid[j++] = atol(comma);
			}
			comma = arg + ++i;
		}
		else if (arg[i] == '-') {
			start = atol(comma);
			comma = arg + ++i;
		}
		else {
			i++;
		}
	}

	if (start) {
		finish = atol(comma);
		for (;start<=finish;start++, j++) {
			docid[j] = start;
		}
		start = 0;
		finish = 0;
	}
	else {
		docid[j++] = atol(comma);
	}

	debug("you can undelete upto %d document.\n",1024);

	debug("you want to undelete document");
	debug("%u", docid[0]);
	for (i=1; i<j; i++) {
		debug(", %u", docid[i]);
	}

	{
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		if ( sb_run_docattr_set_docmask_function(&docmask, "Undelete", "1") == FAIL )
			sb_run_docattr_set_docmask_function(&docmask, "Delete", "0");
		sb_run_docattr_set_array(docid, j, SC_MASK, &docmask);
	}
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	return SUCCESS;
}
/*int com_delete (char *arg)
{
	int i, j;
	uint32_t docid[1024], start=0, finish=0;
	char *comma=arg;

	if (!strlen(arg)) {
		send_nak_str(sockfd, "Non format!!\n");
		return FAIL;
	}

	for (i=0, j=0; arg[i]!='\0'; ) {
		if (arg[i] == ',') {
			if (start) {
				finish = atol(comma);
				for (;start<=finish;start++, j++) {
					docid[j] = start;
				}
				start = 0;
				finish = 0;
			}
			else {
				docid[j++] = atol(comma);
			}
			comma = arg + ++i;
		}
		else if (arg[i] == '-') {
			start = atol(comma);
			comma = arg + ++i;
		}
		else {
			i++;
		}
	}

	if (start) {
		finish = atol(comma);
		for (;start<=finish;start++, j++) {
			docid[j] = start;
		}
		start = 0;
		finish = 0;
	}
	else {
		docid[j++] = atol(comma);
	}

	debug("you can delete upto %d document.\n",1024);

	debug("you want to delete document");
	debug("%ld", docid[0]);
	for (i=1; i<j; i++) {
		debug(", %ld", docid[i]);
	}

	{
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
		sb_run_docattr_set_array(docid, j, SC_MASK, &docmask);
	}
	return SUCCESS;
}

*/
int sb4_com_get_new_wordid(int sockfd, char *arg, void* word_db)
{
	int ret;
	char *token=NULL;
	char orig_arg[SHORT_STRING_SIZE]="";
	char *tmp=arg;
	int count=0;

	char *word;
	uint32_t docid;
	word_t lexicon;	
	char tmpbuf[LONG_STRING_SIZE];

	if ( word_db == NULL ) {
		warn("lexicon module is not loaded");
		return FAIL;
	}

	// strtok modifies 1st argument, so we save it here
	strncpy(orig_arg,arg,SHORT_STRING_SIZE);
	orig_arg[SHORT_STRING_SIZE-1] = '\0';

	// count tokens
	while(1) {
		token = strtok(tmp, " \t");
		tmp = NULL;
		if (token != NULL)
			count++;
		else
			break;
	}

	if (count == 2) {
		token = strtok(orig_arg," \t");
		word = token;

		token = strtok(NULL," \t");
		docid = atoi(token);
	} else if (count == 1) {
		word = orig_arg;
		docid = 0; // set default docid
	} else {
		info("usage : get_new_wordid word [docid]");
		send_nak_str(sockfd, "usage : get_new_wordid word [docid]");
		return FAIL;
	}
	
	strncpy(lexicon.string, arg , MAX_WORD_LEN);
	
	ret = sb_run_get_new_wordid(word_db, &lexicon);
	if (ret < 0) {
		sprintf(tmpbuf, "error while get new wordid for word[%s]", lexicon.string);
		error("%s", tmpbuf);
		send_nak_str(sockfd, tmpbuf);
		return FAIL;
	}

	sprintf(tmpbuf, "ret[%d], word[%s] wordid[%u]\n",ret, lexicon.string, lexicon.id);

	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	/* 6. send wordid */
	if ( TCPSendData(sockfd, tmpbuf, strlen(tmpbuf), FALSE) == FAIL ) {
		error("cannot send wordid");
		return FAIL;
	}
	
	return SUCCESS;
}

int sb4_com_get_wordid (int sockfd, char *arg, void* word_db)
{
	int ret;
	word_t lexicon;
	char tmp[LONG_STRING_SIZE];
	
	if ( word_db == NULL ) {
		warn("lexicon module is not loaded");
		return FAIL;
	}

	debug("argument:[%s]",arg);
	
	strncpy(lexicon.string, arg , MAX_WORD_LEN);
	ret = sb_run_get_word(word_db, &lexicon);

	if (ret < 0) {
		sprintf(tmp, "ret[%d] : no such word[%s]",ret ,arg);
		error("%s", tmp);
		send_nak_str(sockfd, tmp);
		return FAIL;
	}

	sprintf(tmp, "ret[%d], word[%s]'s wordid[%d]\n", ret ,lexicon.string ,lexicon.id );
	
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 6. send wordid  */
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send wordid ");
		return FAIL;
	}

	return SUCCESS;
}

int sb4_com_get_docid(int sockfd, char *arg, void* did_db)
{
	int ret;
	uint32_t docid;
	char tmpbuf[LONG_STRING_SIZE];

	ret = sb_run_get_docid(did_db, arg, &docid);
	if (ret < 0) {
		sprintf(tmpbuf,"cannot get document id: error[%d]\n", ret);
		send_nak_str(sockfd, tmpbuf);
		return FAIL;
	}
	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 6. send docid  */
	sprintf(tmpbuf, "%u", docid);
	
	if ( TCPSendData(sockfd, tmpbuf, strlen(tmpbuf), FALSE) == FAIL ) {
		error("cannot send docid[%u]", docid);
		return FAIL;
	}
	
	return SUCCESS;
}


char *__trim(char *str, int *len)
{   
    char *tmp, *start;

    if (*len == 0) return str;
    
    start = str;
    for (tmp=str; tmp<str+*len; tmp++) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            start = tmp;
            break;
        }       
    }
    if (tmp == str + *len) {
        *len = 0;
        return str;
    }
    for (tmp=str+*len-1; tmp>=start; tmp--) {
        if (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\r') {
            break;
        }
    }
    *len = tmp - start + 1; 
    return start;           
}   


/* add khyang */
int sb4_com_help (char *result)
{
	register int i;
	int printed = 0; 
	char tmp[1024];
	
	for ( i = 0; commands[i].name; i++ ) {
		
		if (strcmp(commands[i].name,BLANKLINE) == 0) 
			strcat(result, "\n");
		else if (strcmp(commands[i].desc, "") == 0 )
		{
			sprintf(tmp, "%-10s\n", commands[i].name);
			strcat(result, tmp);
		}
		else
		{
			sprintf(tmp, "%-10s :  %s\n", commands[i].name, commands[i].desc);
			strcat(result, tmp);
		}
		printed++;
		
	}

	if (printed == 0) {
		
		for ( i = 0; commands[i].name; i++ ) {
			/* print in six columns **/
			if ( printed == 6 ) {
				printed = 0;
				strcat(result, "\n");
			}

			sprintf(tmp, "%s\t", commands[i].name);
			strcat(result, tmp);
			printed++;
		}

		if (printed) {
			strcat(result, "\n");
		}
		
		
	}

	return SUCCESS;
}

int sb4_com_index_list(int sockfd, char *arg, char *field, int count)
{

	int total_doc_num=0, list_tot_cnt=0;
	int i, nRet, result=0, lc=0, pg=0, del=0, k, j, lSrt, lEnd, nRecvCnt, inc, nCnt=0, sys;
	int dt1, dt2;
	docattr_t attr;
	char buf[SHORT_STRING_SIZE];
	char *tmpstr=NULL;
	char szResult[LONG_LONG_STRING_SIZE];
	RetrievedDoc rdoc;
	int sizeleft;
	DocObject    *doc;
	char **list;
	long *doclist;
	char tmp[STRING_SIZE];
	char szDel[STRING_SIZE];
	char szDID[STRING_SIZE];
	char tmp1[STRING_SIZE];
	
	info("arg:%s", arg);	
	info("field:%s", field);
	info("count:%d", count);


	if( get_str_item(tmp, arg, "SYS=", '^', STRING_SIZE) == FAIL)
	{
		error("SYS get error!!");
		send_nak_str(sockfd, "SYS No get error!! \n");
		return FAIL;
	}
	sys = atoi(tmp);
	
	if( get_str_item(tmp, arg, "LC=", '^', STRING_SIZE) == FAIL)
	{
		error("LC get error!!");
		send_nak_str(sockfd, "LC get error!! \n");
		return FAIL;
	}
	lc = atoi(tmp);
	
	if( get_str_item(tmp, arg, "PG=", '^', STRING_SIZE) == FAIL)
	{
		error("PG get error!!");
		send_nak_str(sockfd, "PG get error!! \n");
		return FAIL;
	}
	pg = atoi(tmp);
	
	if( get_str_item(tmp, arg, "DEL=", '^', STRING_SIZE) == FAIL)
	{
		error("DEL get error!!");
		send_nak_str(sockfd, "DEL get error!! \n");
		return FAIL;
	}
	if (strlen(tmp) != 0)	
		del = atoi(tmp);
	else
		del = -1;
	
	if( get_str_item(tmp, arg, "DT1=", '^', STRING_SIZE) == FAIL)
	{
		error("DT1 get error!!");
		send_nak_str(sockfd, "DT1 get error!! \n");
		return FAIL;
	}
	if (strlen(tmp) != 0)	
		dt1 = atoi(tmp);
	else
		dt1 = -1;	
	
	if( get_str_item(tmp, arg, "DT2=", '^', STRING_SIZE) == FAIL)
	{
		error("DT2 get error!!");
		send_nak_str(sockfd, "DT2 get error!! \n");
		return FAIL;
	}
	if (strlen(tmp) != 0)	
		dt2 = atoi(tmp);
	else
		dt2 = -1;	
	
	total_doc_num = sb_run_last_indexed_did();
	if ( total_doc_num == 0 )
	{
		error("Index Doc Count Zero!! \n");
		send_nak_str(sockfd, "Index Doc Count Zero!! \n");
		return FAIL;
	}
	

	doclist = (long*)sb_calloc(total_doc_num, sizeof(long));
	/** Tot Cnt **/
	for(i=1; i <= total_doc_num; i++)
	{		
		if (sb_run_docattr_get(i, &attr) == -1) 
		{
			error("cannot get docattr");
			send_nak_str(sockfd, "cannot get docattr \n");
			sb_free(doclist);
			return FAIL;
		}		
		
		sb_run_docattr_get_docattr_function(&attr, "SystemName", buf, SHORT_STRING_SIZE);
		if (sys !=  atoi(buf) )
			continue;
	
		sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE);
		if ( del != -1 )
		{
			if (del !=  atoi(buf) )
				continue;
		}
		doclist[list_tot_cnt] = i;
		list_tot_cnt++;
	}	

	lSrt = lc * pg;
	lEnd = lSrt + lc;
			
	if ( lSrt >= list_tot_cnt )
	{
		lSrt = list_tot_cnt - lc;
		if ( lSrt < 0 )
			lSrt = 0;
		lEnd = lSrt + lc;
	}

	if ( lEnd > list_tot_cnt )
		lEnd = list_tot_cnt;

	nRecvCnt = lEnd - lSrt;

	list = (char**)sb_calloc(nRecvCnt, sizeof(char*));
	for(j=0; j< nRecvCnt; j++)
	{
		list[j] = (char*)sb_calloc(LONG_LONG_STRING_SIZE, sizeof(char));
	}
	
	for(i=list_tot_cnt - lSrt - 1, inc=0; i >= 0; i--)
	{	
	
		if (sb_run_docattr_get(doclist[i], &attr) == -1) 
		{
			error("cannot get docattr");
			send_nak_str(sockfd, "cannot get docattr \n");
			return FAIL;
		}		

		sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE);
		if ( del != -1 )
		{
			if (del !=  atoi(buf) )
				continue;
		}
		sprintf(szDel, "%s", buf);
	
		rdoc.docId = doclist[i];
		rdoc.rank = 0;
		rdoc.rsv = 0;
		rdoc.numAbstractInfo = count;
		
		sprintf(szDID, "%ld", doclist[i]);
		
		for (k=0; k<count; k ++) 
		{	
		
			sprintf(tmp1, "INFO%d=", k);
			if( get_str_item(tmp, field, tmp1, '^', STRING_SIZE) == FAIL)
			{
				error("Field Info Err");
				send_nak_str(sockfd, "Field Info get error!! \n");
				//free?
				return FAIL;
			}
			strncpy(rdoc.cdAbstractInfo[k].field, tmp, STRING_SIZE);		
		}

		
#ifdef PARAGRAPH_POSITION
		rdoc.cdAbstractInfo[0].paragraph_position = 0;
#endif
		rdoc.cdAbstractInfo[0].position = 0;
		rdoc.cdAbstractInfo[0].size = 0;
		
		result = sb_run_doc_get_abstract(1, &rdoc, &doc);
		if (result < 0) {
			printf("canneddoc_get_abstract error.\n");
			send_nak_str(sockfd, "canneddoc_get_abstract error.\n");
			//free?
			return FAIL;
		}

		sizeleft = sizeof(szResult)-1;
		memset(szResult, 0x00, sizeof(szResult));
		
		strncat(szResult,"DEL",sizeleft);
		sizeleft -= strlen("DEL");
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		strncat(szResult,":",sizeleft);
		sizeleft -= 1;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		strncat(szResult,szDel,sizeleft);
		sizeleft -= strlen(szDel);
		sizeleft = (sizeleft < 0) ? 0:sizeleft;
		
		strncat(szResult,";;",sizeleft);
		sizeleft -= 2;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;
			
		strncat(szResult,"DID",sizeleft);
		sizeleft -= strlen("DID");
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		strncat(szResult,":",sizeleft);
		sizeleft -= 1;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;

		strncat(szResult,szDID,sizeleft);
		sizeleft -= strlen(szDID);
		sizeleft = (sizeleft < 0) ? 0:sizeleft;
		
		strncat(szResult,";;",sizeleft);
		sizeleft -= 2;
		sizeleft = (sizeleft < 0) ? 0:sizeleft;					
		
		for (k=0; k<count; k++) {
			tmpstr = NULL;
			
			sprintf(tmp1, "INFO%d=", k);
			if( get_str_item(tmp, field, tmp1, '^', STRING_SIZE) == FAIL)
			{
				error("Field Info Err");
				send_nak_str(sockfd, "Field Info get error!! \n");
				//free?
				return FAIL;
			}		
		
			nRet = sb_run_doc_get_field(doc, NULL, tmp, &tmpstr);
			if (nRet < 0) {
				error("doc_get_field error for doc[%ld], field[%s]", doclist[i], tmp);
				continue;
			}		
			strncat(szResult,tmp,sizeleft);
			sizeleft -= strlen(tmp);
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			strncat(szResult,":",sizeleft);
			sizeleft -= 1;
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			strncat(szResult,tmpstr,sizeleft);
			sizeleft -= strlen(tmpstr);
			sizeleft = (sizeleft < 0) ? 0:sizeleft;
			sb_free(tmpstr);

			strncat(szResult,";;",sizeleft);
			sizeleft -= 2;
			sizeleft = (sizeleft < 0) ? 0:sizeleft;

			if (sizeleft <= 0) {
				error("req->comments size lack while pushing comment(field:%s)", tmp);
				break;
			}
		}
		
				
		strcpy(list[inc], szResult);
		list[strlen(list[inc])] = 0x00;
		nCnt++;
		inc++;
		
		if ( nCnt == nRecvCnt)
			break;
	}
	
	/* Send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		//free?
		return FAIL;
	}
	
	/* Send Tot Cnt */
	sprintf(tmp, "%d", list_tot_cnt);
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send Tot Cnt ");
		//free?
		return FAIL;
	}
		
	/* Send Recv Cnt */
	sprintf(tmp, "%d", nRecvCnt);
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send recv Cnt ");
		//free?
		return FAIL;
	}
	
	/* Send List */
	for(i=0; i <nRecvCnt; i++)
	{
		if ( TCPSendData(sockfd, list[i], strlen(list[i]), FALSE) == FAIL ) {
			error("cannot send recv data ");
			//free?
			return FAIL;
		}
	}
	
	sb_free(doclist);
	for(j=0; j< nRecvCnt; j++)
	{
		sb_free(list[j]);
	}
	sb_free(list);
	
	return SUCCESS;
}



int sb4_com_doc_count(int sockfd)
{
	int last_registered_docid;
	int i, system_name[50], system_name_del[50], system_name_app[50];
	char szResult[LONG_LONG_STRING_SIZE];
	lgcaltex_attr_t attr;
	int nRecvCnt=50;
	char tmp[STRING_SIZE];
	
	for(i=0; i < 50; i++) {
		system_name[i]=0;
		system_name_app[i]=0;
		system_name_del[i]=0;
	}
	
	last_registered_docid = sb_run_last_indexed_did();
	
	info("last doc id [%d] from registry", last_registered_docid);
	
	for(i=1; i<=last_registered_docid; i++) {
	
	if (sb_run_docattr_get(i, &attr) == -1)
	{
	    error("cannot get docattr");
	    return FAIL;
	}
	
	
	if(attr.is_deleted) {
		system_name_del[attr.SystemName]++;
	} else {
		if(attr.AppFlag==1)
			system_name_app[attr.SystemName]++;
		else
			system_name[attr.SystemName]++;
		}
	}

	/* Send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* Send Recv Cnt */
	sprintf(tmp, "%d", nRecvCnt);
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send recv Cnt ");
		return FAIL;
	}
	
	
	/* Send doc count */
	for(i=0; i <50; i++)
	{
		memset(szResult, 0x00, sizeof(szResult));
		sprintf(szResult, "NO=%d^DEL=%d^INDEX=%d^APP=%d^", i, system_name_del[i], system_name[i], system_name_app[i]);
		if ( TCPSendData(sockfd, szResult, strlen(szResult), FALSE) == FAIL ) {
			error("cannot send recv data ");
			return FAIL;
		}
	}
    

    return SUCCESS;
}

int sb4_com_get_word_by_wordid (int sockfd, char *arg, void* word_db)
{
	int ret, i, lc=0, pg=0, word_tot_num=0;
	int lSrt, lEnd, nRecvCnt, j;
	char **list;
	word_t lexicon;
	char szResult[LONG_LONG_STRING_SIZE];
	char tmp[STRING_SIZE];

	if ( word_db == NULL ) {
		warn("lexicon module is not loaded");
		return FAIL;
	}

	printf("arg:%s\n", arg);	

	if( get_str_item(tmp, arg, "LC=", '^', STRING_SIZE) == FAIL)
	{
		error("LC get error!!");
		send_nak_str(sockfd, "LC get error!! \n");
		return FAIL;
	}
	lc = atoi(tmp);
	
	if( get_str_item(tmp, arg, "PG=", '^', STRING_SIZE) == FAIL)
	{
		error("PG get error!!");
		send_nak_str(sockfd, "PG get error!! \n");
		return FAIL;
	}
	pg = atoi(tmp);

	if ( sb_run_get_num_of_word( word_db, &word_tot_num ) != SUCCESS ) {
		error("get_num_of_word failed");
		send_nak_str(sockfd, "get_num_of_word failed \n");
		return FAIL;
	}
	
	lSrt = lc * pg;
	lEnd = lSrt + lc;
			
	nRecvCnt = lEnd - lSrt;

	list = (char**)sb_calloc(nRecvCnt, sizeof(char*));
	for(j=0; j<lc; j++)
	{
		list[j] = (char*)sb_calloc(LONG_LONG_STRING_SIZE, sizeof(char));
	}
	
	for(i=lSrt, nRecvCnt=0; i < lEnd; i++)
	{
		lexicon.id = (uint32_t)i+1;
		
		ret = sb_run_get_word_by_wordid(word_db, &lexicon);
		if ( ret == WORD_NOT_REGISTERED ) break;
		else if ( ret != WORD_OLD_REGISTERED ) goto error;
		
		memset(szResult, 0x00, LONG_LONG_STRING_SIZE);
		
		sprintf(szResult, "WID=%u^WORD=%s^", lexicon.id, lexicon.string);

		strcpy(list[nRecvCnt], szResult);
		list[strlen(list[nRecvCnt])] = 0x00;
		nRecvCnt++;
	}

	/* Send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		goto error;
	}

	/* Send Tot Cnt */
	sprintf(tmp, "%d", word_tot_num);
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send Tot Cnt ");
		goto error;
	}
	
	/* Send Recv Cnt */
	sprintf(tmp, "%d", nRecvCnt);
	if ( TCPSendData(sockfd, tmp, strlen(tmp), FALSE) == FAIL ) {
		error("cannot send recv Cnt ");
		goto error;
	}
	
	/* Send List */
	for(i=0; i <nRecvCnt; i++)
	{
		if ( TCPSendData(sockfd, list[i], strlen(list[i]), FALSE) == FAIL ) {
			error("cannot send recv data ");
			goto error;
		}
	}
	
	return SUCCESS;

error:
	for(i=0; i<lc; i++) sb_free(list[i]);
	sb_free(list);

	return FAIL;
}

int sb4_com_del_system_doc (int sockfd, char *arg)
{
	int total_doc_num=0;
	int i;
	docattr_t attr;
	char buf[SHORT_STRING_SIZE];
	int system_no;
	
	system_no = atoi(arg);
	info("system_no:%d", system_no);
	
	total_doc_num = sb_run_last_indexed_did();
	if ( total_doc_num == 0 )
	{
		error("Index Doc Count Zero!!");
		send_nak_str(sockfd, "Index Doc Count Zero!!");
		return FAIL;
	}
	
	
	for(i=1; i <= total_doc_num; i++)
	{		
		if (sb_run_docattr_get(i, &attr) == -1) 
		{
			error("cannot get docattr");
			send_nak_str(sockfd, "cannot get docattr");
			return FAIL;
		}		
		
	
		sb_run_docattr_get_docattr_function(&attr, "SystemName", buf, SHORT_STRING_SIZE);
		if ( system_no == atoi(buf))
		{
			sb_run_docattr_get_docattr_function(&attr, "Delete", buf, SHORT_STRING_SIZE);
			if (atoi(buf) == 1 )
				debug("already deleted docid: %d", i);
			else
			{
				/* delete mark to docattr */
				{
					docattr_mask_t docmask;
					uint32_t docid[1] = { i };
			
					DOCMASK_SET_ZERO(&docmask);
					sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
					sb_run_docattr_set_array(docid, 1, SC_MASK, &docmask);
				}
			}
		}
	}	

	/* send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	

	return SUCCESS;	
}


static int get_str_item(char *dest, char *dit, char *key, char delimiter, int len)
{
	char *start, *end, restore;

	start = strstr(dit, key);

	if ( start == NULL ) {
		dest[0] = '\0';
		return FAIL;
	}
	start += strlen(key);

	end = strchr(start, delimiter);
	if ( end == NULL ) {
		dest[0] = '\0';
		return FAIL;
	}

	restore = *end;
	*end = '\0';

	strncpy(dest, start, len);
	dest[len-1] = '\0';
	*end = restore;

	return SUCCESS;
}

/* getopt(3) related stuff ***************************************************/
extern char *optarg;
extern int optind,opterr,optopt;

#ifndef HAVE_GETOPT_LONG
struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
#endif

#define OPTION_COMMANDS		"hp:g:kltc:v"



/* end getopt related stuff */



/****************************************************************************/


module client_module = {
	CORE_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	NULL,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	NULL					/* register hook api */
};
