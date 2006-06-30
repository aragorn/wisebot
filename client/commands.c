/* $Id$ */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "common_core.h"
#include "common_util.h"
#include "mod_api/tcp.h"
#include "mod_api/protocol4.h"
#include "mod_api/cdm.h"
#include "mod_api/cdm2.h"
#include "mod_api/docapi.h"
#include "mod_api/tokenizer.h"
#include "mod_api/qp.h"
#include "mod_api/qpp.h"
#include "mod_api/index_word_extractor.h"
#include "mod_api/morpheme.h"
#include "mod_api/rmas.h"
#include "mod_api/xmlparser.h"
#include "mod_api/did.h"

#include "client.h"
#include "mod_api/indexer.h"
#include "benchmark.h"

#define TOKEN_MAX_LEN 128
#define ARG_MAX_CNT 1024

char mRmacServerAddr[SHORT_STRING_SIZE] = "192.168.10.10";
char mRmacServerPort[SHORT_STRING_SIZE] = "9000";

char *docattrFields[MAX_FIELD_NUM] = { NULL };

static char sortingcriterion[STRING_SIZE] = { '\0' };
static int listcount=15;
static int first_page=0;

/* ut_stest.c stuff ***********************************************************/
/* siouk 2004/02/01 */
static void skip_white_space(char** p)
{
	while(1) {
		if(**p == '\0') break;

		if(**p == ' ' || **p == '\t' || **p == '\n')
			(*p)++;
		else
		    break;
	}
}

static void find_white_space(char** p)
{
	while(1) {
		if(**p == '\0') break;

		if(**p == ' ' || **p == '\t' || **p == '\n')
			break;
		else
		    (*p)++;
	}
}

static void find_quote(char** p, char quote)
{
	while(1) {
		if(**p == '\0') break;

		if(**p == quote) 
			break;
		else
			(*p)++;
	}
}

int process_normal(char** ptr, char** data)
{
	char*	p;
	
	p = *ptr;

	p++;
	find_white_space(&p);

	(*data) = (char*)malloc(sizeof(char)*(p - (*ptr+1)));
	if((*data) == NULL) {
		error("fail malloc");
		return FAIL;
	}

	strncpy((*data), *ptr, p - (*ptr));
	(*data)[p - (*ptr)] = '\0';

    (*ptr) = p;
	
	return SUCCESS;
}

int process_quote(char** ptr, char** data)
{
	char*	p;
	
	p = *ptr;

	p++;
	find_quote(&p, **ptr);

	(*data) = (char*)malloc(sizeof(char)*(p - (*ptr)));
	if((*data) == NULL) {
		error("fail malloc");
		return FAIL;
	}

    strncpy((*data), (*ptr)+1, p - (*ptr) - 1);
    (*data)[p - (*ptr)-1] = '\0';

	p++;
	(*ptr) = p;
		
	return SUCCESS;
}

int status_arg(const char **argv, int argc)
{
    int i;

	info("argc    : [%d]", argc);

	for(i = 0; i < argc; i++) {
		info("argv[%d] : [%s]", i, argv[i]);
	}

    return SUCCESS; 
}

int	make_arg(char*** argv, int* argc, char* param, const char* cmd)
{
	char* arg[ARG_MAX_CNT];
	char* p;
	int cnt=0,j;

	memset(arg, 0x00, sizeof(char*)*ARG_MAX_CNT);

	for(p = param, cnt = 1; p != NULL;) {
		if(*p == '\0') break;

		skip_white_space(&p);
		if(*p == '\0') break;

		if(*p == '\'' || *p == '\"') {
			if(process_quote(&p, &arg[cnt]) == SUCCESS) {
				cnt++;
				continue;
			} else {
				error("option invalid");
				return FAIL;
			}
		} else {
			if(process_normal(&p, &arg[cnt]) == SUCCESS) {
				cnt++;
				continue;
			} else {
				error("invalid option");
				return FAIL;
			}
		}
	}

	(*argv) = (char**)malloc(sizeof(char**)*cnt);
	if((*argv) == NULL) {
        error("fail malloc");
        return FAIL;
    }

	(*argv)[0] = (char*)malloc(sizeof(char)*strlen(cmd)+1);
	if((*argv)[0] == NULL) {
        error("fail malloc");
        return FAIL;
    }

	strcpy((*argv)[0], cmd);
	
	for(j = 1; j < cnt; j++) {
    	(*argv)[j] = arg[j];
    }

	(*argc) = cnt;

    return SUCCESS;
}

int destroy_arg(char** argv, int argc)
{
    int i;
    
    if(argv == NULL) {
    	error("argv null ptr");
    	return FAIL;
    }
    
    for(i=0; i<argc; i++) {
    	if(argv[i] == 0x00) {
    		error("argv[%d] null ptr", i);
    		return FAIL;
    	}
    	free(argv[i]);
    }
    
    free(argv);
    return SUCCESS;
}

/* ut_stest.c stuff end *******************************************************/

int com_repeat(char *arg)
{
	char orig_arg[STRING_SIZE]=""; // strtok modifies 1st arg. see strtok(3)
	char orig_command[STRING_SIZE]="";
	char *command="";
	int32_t repeat=-1L,i=0L;
	char *s=NULL,*endptr=NULL;

	strncpy(orig_arg,arg,STRING_SIZE);
	debug("orig_arg:%s",orig_arg);

	repeat = strtol(arg,&endptr,10);
	if (arg==endptr) {
		info("usage: repeat num commands");
		return FAIL;
	}

	command = strstr(orig_arg," ");
	
	info("repeating [%s] for %d times",command,repeat);

	s = stripwhite(command);

	strncpy(orig_command, s ,STRING_SIZE);
	orig_command[STRING_SIZE-1] = '\0';
	
	if (*s) {
		struct timeval ts, tf;
		double diff=0;

		gettimeofday(&ts, NULL);
		for (i=1; i<=repeat; i++) {
			execute_line(s);
					
			strcpy(s , orig_command);
			
			if ( (i*100/repeat%10) == 0) {
				emerg("%d%% done",i*100/repeat);
			}
			printf("\n");
		}
		gettimeofday(&tf, NULL);

		emerg("repeating \"%s\" for %d times done",command,repeat);
		info("command took %2.2f sec, %2.2f operation/sec.\n",diff,repeat/diff);
	}
	
	
	return SUCCESS;
}

int com_xrepeat(char *arg)
{
	char orig_arg[STRING_SIZE]=""; // strtok modifies 1st arg. see strtok(3)
	char buf[STRING_SIZE]="";
	char *command="",*tmp=NULL;
	int32_t repeat=-1L,i=0L;
	char *s=NULL,*startptr=arg,*endptr=NULL;
	char *example="example: xrepeat 10 1(start) 1(increase) register %%d testcd";
	int start=0,inc=0;

	strncpy(orig_arg,arg,STRING_SIZE);

	repeat = strtol(startptr,&endptr,10);
	if (arg==endptr) {
		info(example);
		return FAIL;
	}

	startptr = endptr;
	start = strtol(startptr,&endptr,10);
	if (startptr==endptr) {
		info(example);
		return FAIL;
	}

	startptr = endptr;
	inc = strtol(startptr,&endptr,10);
	if (startptr==endptr) {
		info(example);
		return FAIL;
	}

	command = strstr(endptr," ");
	if (command == NULL) {
		info(example);
		return FAIL;
	}

	printf("command:");
	puts(command);
	fflush(stdout);

	s = stripwhite(command);

	if (*s) {
		int num = start;
		struct timeval ts, tf;
		double diff=0;

		tmp = strstr(s,"%d");
		if (tmp == NULL) {
			info(example);
			return FAIL;
		}

		info("repeating command for %d times, start=%d,inc=%d",repeat,start,inc);

		gettimeofday(&ts, NULL);
		for (i=1; i<=repeat; i++) {
			snprintf(buf,STRING_SIZE,s,num);
			num += inc;
			execute_line(buf);
/*			if ( (i*100/repeat%10) == 0) {*/
/*				emerg("%d%% done",i*100/repeat);*/
/*			}*/
/*			printf("\n");*/
		}
		gettimeofday(&tf, NULL);

		diff = timediff(&tf, &ts);
		info("command took %2.2f sec, %2.2f operation/sec.\n",diff, repeat/diff);
	}

	return SUCCESS;
}
int com_log_level(char *arg)
{
	char *usage="usage: log [debug|info|warn|error|...] \n";
	fprintf(stderr,"arg:%s\n",arg);
	if (!strlen(arg)) {
		fprintf(stderr,usage);
		return FAIL;
	}
	log_setlevelstr(arg);
	return SUCCESS;
}

int com_connect(char *arg)
{
	char *token=NULL;
	char orig_arg[STRING_SIZE]="";
	char *tmp=arg;
	int count=0;

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
		strncpy(mServerAddr,token,SHORT_STRING_SIZE);
		mServerAddr[SHORT_STRING_SIZE-1] = '\0';

		token = strtok(NULL," \t");
		strncpy(mServerPort, token, SHORT_STRING_SIZE);
		mServerPort[SHORT_STRING_SIZE-1] = '\0';
	}
	else if (count == 1) {
		token = strtok(orig_arg," \t");
		strncpy(mServerPort, token, SHORT_STRING_SIZE);
		mServerPort[SHORT_STRING_SIZE-1] = '\0';
	}

	printf("connect to %s:%s\n", mServerAddr, mServerPort);

	gSoftBotListenPort = atoi(mServerPort);
	return SUCCESS;
}

#if 0
#define MAX_CANNED_DOC 400000
int com_register_doc(char *arg)
{
	int filedes,result,n;
	uint32_t docid = 0;
	VariableBuffer var;
	char canneddoc[MAX_CANNED_DOC], filename[256];

	n = sscanf(arg, "%ld %s", &docid, filename);
	if (n != 2) {
		printf("usage: register [document id] [canned document file]\n");
		return FAIL;
	}

	filedes = open(filename,O_RDONLY);
	if (filedes == -1) {
		printf("file open error.\n");
		return FAIL;
	}

	n = read(filedes,canneddoc,MAX_CANNED_DOC);
	if (n == -1) {
		printf("file read error.\n");
		return FAIL;
	}

	sb_run_buffer_initbuf(&var);

	result = sb_run_buffer_append(&var, n, canneddoc);
	if (result < 0) {
		printf("buffer append error.\n");
		return FAIL;
	}

	result = sb_run_client_canneddoc_put(docid, &var);
	if (result < 0) {
		printf("canneddoc put error(%d)\n", result);
		return FAIL;
	}

	printf("success.\n");
	return SUCCESS;
}
int com_register_doc_i(char *arg)
{
	int filedes,result,n,i;
	static uint32_t docid = 0;
	VariableBuffer var;
	char canneddoc[MAX_CANNED_DOC];
	char filename[256];

	double diff;
	struct timeval ts, tf;

	sscanf(arg, "%s %d", filename, &i);
	filedes = open(filename,O_RDONLY);
	if (filedes == -1) {
		printf("file open error.\n");
		return FAIL;
	}

	n = read(filedes,canneddoc,MAX_CANNED_DOC);
	if (n == -1) {
		printf("file read error.\n");
		return FAIL;
	}

	sb_run_buffer_initbuf(&var);

	result = sb_run_buffer_append(&var, n, canneddoc);
	if (result < 0) {
		printf("buffer append error.\n");
		return FAIL;
	}

	gettimeofday(&ts, NULL);
	for (;i>0;i--) {
		docid++;
		result = sb_run_client_canneddoc_put(docid, &var);
		if (result < 0) {
			printf("canneddoc put error(%d)\n", result);
			return FAIL;
		}
	}
	gettimeofday(&tf, NULL);

	diff = timediff(&tf, &ts);
	printf("sb_run_canneddoc_put() took %2.2f sec, %2.2f queries/sec.\n",
			diff, 1/diff);
	return SUCCESS;
}
#endif
//#define GET_FROM_REMOTE
//#undef GET_FROM_REMOTE
int com_get_doc(char *arg)
{
	int result=0;
	uint32_t docid = atol(arg);

#ifdef GET_FROM_REMOTE
	int sockfd=0;
	char *buf;
	buf = (char *)malloc(DOCUMENT_SIZE);
	if (buf == NULL) {
		crit("out of memory: %s", strerror(errno));
		return FAIL;
	}

	info("you want document[%u]",docid);
	info("connect to %s:%s...",mServerAddr,mServerPort);

	result = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( result == FAIL ) {
		error("tcp_connect failed.");
		free(buf);
		return FAIL;
	}

	result = sb_run_sb4c_get_doc(sockfd, (uint32_t)docid, 
			buf, DOCUMENT_SIZE);
	sb_run_tcp_close(sockfd);
	if (result != SUCCESS) {
		error("cannot get document from server[%s:%s]",
				mServerAddr, mServerPort);
		free(buf);
		return FAIL;
	}

	printf("%s\n", buf);
	free(buf);
#else
	if ( find_module("mod_cdm.c") != NULL ) {
		VariableBuffer var;

		sb_run_buffer_initbuf(&var);

		result = sb_run_server_canneddoc_get(docid,&var);
		if (result < 0) {
			printf("canneddoc_get error.\n");
			return FAIL;
		}

		sb_run_buffer_print(stdout, &var);
		printf("\n");
	}
	else { // use mod_cdm2
		char* buf;
		size_t size = 10 * 1024 * 1024; // 10MB

		if ( mCdmDb == NULL ) {
			error("cdmdb not opened");
			return FAIL;
		}

		buf = (char*) malloc(size);

		result = sb_run_cdm_get_xmldoc(mCdmDb, docid, buf, size);
		if ( result == CDM2_GET_INVALID_DOCID ) {
			error("invalid docid");
			return FAIL;
		}
		else if ( result < 0 ) {
			error("cannot get document: %d", result);
			free(buf);
			return FAIL;
		}

		fwrite( buf, result, 1, stdout );
		fputc( '\n', stdout );
		free(buf);
	}
#endif

	return SUCCESS;
}
int com_get_doc_size(char *arg)
{
	int result=0;
	uint32_t docid = atol(arg);

	result = sb_run_server_canneddoc_get_size(docid);
	if (result < 0) {
		error("canneddoc_get error.\n");
		return FAIL;
	}

	info("size of document[%u]: %d", docid, result);

	return SUCCESS;
}
int com_get_abstracted_doc(char *arg)
{
	int result=0, offset, size;
	char field[MAX_FIELD_NAME_LEN];
	VariableBuffer var;
	RetrievedDoc rdoc;
	uint32_t docid = 0;

	result = sscanf(arg, "%u %s %d %d", &docid, field, &offset, &size);
	if (result != 4) {
		printf("usage: getabstract [docid] [fieldno] [offset] [size]\n");
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

/*	result = CDM_getAbstract(1, &rdoc, &var);*/
/*	info("sizeof rdoc = %d", sizeof(rdoc));*/
	result = sb_run_server_canneddoc_get_abstract(1, &rdoc, &var);
/*	result = sb_run_client_canneddoc_get_abstract(1, &rdoc, &var);*/
	if (result < 0) {
		printf("canneddoc_get_abstract error.\n");
		sb_run_buffer_freebuf(&var);
		return FAIL;
	}

	sb_run_buffer_print(stdout, &var);
	printf("\n");

	sb_run_buffer_freebuf(&var);
	return SUCCESS;
}
int com_last_regi(char *arg)
{
	int sockfd;
	uint32_t result = 0;

	if (sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort) == FAIL) {
		printf("tcp_connect failed.\n");
		return FAIL;
	}

	if (sb_run_sb4c_last_docid(sockfd, &result) < 0) {
		printf("cannot get last registered document id\n");
		return FAIL;
	}

	sb_run_tcp_close(sockfd);

	printf("the last registered document id is [%u]\n", result);
	return SUCCESS;
}
int com_get_field(char *arg) 
{ 
	uint32_t docid; 
	char fieldname[256], filename[256]={0x00,}, buf[STRING_SIZE]; 
	int n=0, fd=0, iswrite_file=0; 
 
	// get argument 
	n = sscanf(arg, "%u %s %s", &docid, fieldname, filename); 
	if (n < 2) { 
		printf("usage: getfield docid fieldname [filename]\n"); 
		return FAIL; 
	} 

	if(filename[0] != 0x00) {
		fd = sb_open(filename, O_RDWR|O_CREAT|O_APPEND, S_IREAD|S_IWRITE);
		if(fd == -1) {
            error("can't open file[%s] : %s", filename, strerror(errno));
			return FAIL;
	    }

		iswrite_file = 1;
	}
 
	if ( find_module("mod_cdm.c") != NULL ) { // use mod_cdm
		DocObject *doc; 
		char* value;

		n = sb_run_doc_get(docid, &doc); 
		if (n < 0) { 
			sprintf(buf, "cannot get document object of document[%u]\n", docid);

			if(iswrite_file) {
				write(fd, buf, strlen(buf));
			} else {
				printf("%s", buf); 
			}

			close(fd);
			return FAIL; 
		} 
	 
		n = sb_run_doc_get_field(doc, NULL, fieldname, &value); 
		if (n < 0) { 
			sprintf(buf, "cannot get field[%s] from document object\n", fieldname);

			if(iswrite_file) {
				write(fd, buf, strlen(buf));
			} else {
				printf("%s", buf); 
			}

			close(fd);
			return FAIL; 
		} 
	 
		if(iswrite_file) {
			sprintf(buf, "Document[%u]['%s']:", docid, fieldname);	
			write(fd, buf, strlen(buf));
			write(fd, value, strlen(value));
			write(fd, "\n", 1);
			close(fd);
		} else {
			printf("Document[%u]['%s']: %s\n", docid, fieldname, value); 
		}

		free(value); 
		sb_run_doc_free(doc); 
	}
	else { // use mod_cdm2
		cdm_doc_t* doc;
		char* buf;
		size_t size = 1024 * 1024; // 1MB

		if ( mCdmDb == NULL ) {
			error("cdmdb not opened");
			return FAIL;
		}

		n = sb_run_cdm_get_doc(mCdmDb, docid, &doc);
		if ( n != SUCCESS ) {
			error("cannot get document");
			return FAIL;
		}

		buf = (char*) malloc(size);
		n = sb_run_cdmdoc_get_field(doc, fieldname, buf, size);
		if ( n == CDM2_FIELD_NOT_EXISTS ) {
			warn("field is not exists");
			free(buf);
			sb_run_cdmdoc_destroy(doc);
			return SUCCESS;
		}
		else if ( n == CDM2_NOT_ENOUGH_BUFFER ) {
			warn("not enough buffer");
		}
		else if ( n < 0 ) {
			warn("cannot get doc field");
			free(buf);
			sb_run_cdmdoc_destroy(doc);
			return FAIL;
		}

		fwrite(buf, n, 1, stdout);
		fputc( '\n', stdout );
		free(buf);
		sb_run_cdmdoc_destroy(doc);
	}

	return SUCCESS; 
} 
int com_get_abstracted_field(char *arg)
{
	int result=0, offset, size;
	char field[MAX_FIELD_NAME_LEN], *value;
	DocObject *doc;
	RetrievedDoc rdoc;
	uint32_t docid = 0;

	result = sscanf(arg, "%u %s %d %d", &docid, field, &offset, &size);
	if (result != 4) {
		printf("usage: getabstractfield [docid] [fieldname] [offset] [size]\n");
		return FAIL;
	}

	printf("you want document[%u].\n",docid);

	rdoc.docId = docid;
	rdoc.rank = 0;
	rdoc.rsv = 0;
	rdoc.numAbstractInfo = 1;
	strncpy(rdoc.cdAbstractInfo[0].field, field, MAX_FIELD_NAME_LEN);
	rdoc.cdAbstractInfo[0].position = offset;
	rdoc.cdAbstractInfo[0].size = size;

	result = sb_run_doc_get_abstract(1, &rdoc, &doc);
	if (result < 0) {
		printf("canneddoc_get error.\n");
		return FAIL;
	}
 
	result = sb_run_doc_get_field(doc, NULL, field, &value); 
	sb_run_doc_free(doc); 
	if (result < 0) { 
		printf("cannot get field[%s] from document object\n", field); 
		free(value); 
		return FAIL; 
	} 
 
	printf("Document[%u]['%s']: %s\n", docid, field, value); 
 
	free(value); 
	return SUCCESS; 
}

int com_update_field(char *arg)
{
	uint32_t docid;
	char fieldname[MAX_FIELD_NAME_LEN], fieldvalue[LONG_STRING_SIZE];
	int n, ret;
	n = sscanf(arg, "%u %s %s", &docid, fieldname, fieldvalue); 

	if ( find_module("mod_cdm.c") == NULL ) { // use cdm2 api
		cdm_doc_t* doc;

		if ( mCdmDb == NULL ) {
			error("cdmdb not opened");
			return FAIL;
		}

		n = sb_run_cdm_get_doc(mCdmDb, docid, &doc);
		if ( n != SUCCESS ) {
			error("cannot get document");
			return FAIL;
		}

		ret = sb_run_cdmdoc_update_field(doc, fieldname, fieldvalue, strlen(fieldvalue));
		sb_run_cdmdoc_destroy(doc);

		return ret;
	}
	else {
		error("not updatable (use mod_cdm2.c)");
		return FAIL;
	}
}

int com_register4_doc(char *arg)
{
	int sockfd, n;
	int filedes;
	char filedata[409600], *dit, *body;

	printf("connect to %s:%s...\n",mServerAddr,mServerPort);
	
	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		printf("tcp_connect failed.\n");
		return FAIL;
	}

	filedes = open(arg,O_RDONLY);
	if (filedes == -1) {
		printf("file open error.\n");
		return FAIL;
	}

	// read file
	bzero(filedata, 409600);
	n = read(filedes, filedata, 409600);
	if (n == -1) {
		printf("cannot read file:%s\n", arg);
		return FAIL;
	}
	filedata[409500] = '\0';

	// find body string
	body = strchr(filedata, '\n');
	*body = '\0';
	body++;

	// set dit string
	dit = filedata;

/*	printf("dit = %s\n\n", dit);*/
/*	printf("body = %s\n\n", body);*/
	// send dit, body to server
	n = sb_run_sb4c_register_doc(sockfd, dit, body, strlen(body));
	if ( n < 0 ) {
		printf("sb4c_register4_doc failed:error[%d].\n", n);
		return FAIL;
	}

	sb_run_tcp_close(sockfd);

	return SUCCESS;
}

int com_test_tokenizer(char *arg)
{
	tokenizer_t *tokenizer=NULL;
	token_t tokens[10];
	int r=0,i=0;

	tokenizer=sb_run_new_tokenizer();

	sb_run_tokenizer_set_text(tokenizer,"ABC 가나다 ^_^;");
	
	while (1) {
		r=sb_run_get_tokens(tokenizer, tokens, 10);

		for (i=0; i<r; i++) {
			printf("%s[len:%d]\n", tokens[i].string, tokens[i].len);
			if (tokens[i].type == TOKEN_END_OF_DOCUMENT)
				goto END;
		}
		printf("\n");
	}
END:
	return SUCCESS;
}

int com_strcmp(char *arg)
{
	char *usage = "Usage: strcmp str1 str2";
	char *space;

	space = strstr( arg, " " );

	if ( strlen( arg ) == 0 || space == NULL ) {
		warn( usage );
		return FAIL;
	}

	*space = '\0';
	space++;
	while ( isspace((int) *space) && *space != '\0' ) space++;

	info("hangul_strncmp result: %d", hangul_strncmp( arg, space, STRING_SIZE ));
	return SUCCESS;
}

int com_client_memstat (char *arg)
{
	registry_t *reg=NULL;
	char *mstat=NULL;
	reg = registry_get("MemoryStat");
	mstat = reg->get();

	printf("%s\n",mstat);
	return SUCCESS;
}

int com_tokenizer (char *arg)
{
	tokenizer_t *tokenizer=NULL;
	token_t tokens[10];
	int i=0,j=0,r=0;

	tokenizer = sb_run_new_tokenizer();

	if (strlen(arg) == 0) {
		printf("Usage: tokenizer strings\n");
		printf("       max number of token:100\n");
		return FAIL;
	}

	printf("tokenizing:%s\n",arg);

	sb_run_tokenizer_set_text(tokenizer,arg);
	
	i=0;
	while (1) {
		r=sb_run_get_tokens(tokenizer, tokens, 10);

		for (j=0; j<r; j++) {
			printf("[%d]'th get_toknes call: %s[len:%d]\n", 
					i, tokens[j].string, tokens[j].len);
			if (tokens[j].type == TOKEN_END_OF_DOCUMENT)
				goto END;
		}
		printf("\n");
		i++;
	}
END:
	return SUCCESS;
}

int com_index_word_extractor (char *arg)
{
	char *text=NULL,*idstr=NULL;
	char *usage="Usage: indexwords id strings";
	index_word_t indexwords[100];
	index_word_extractor_t *extractor=NULL;
	int i=0, n=0, id=0;

	if (strlen(arg) == 0) {
		warn(usage);
		return FAIL;
	}

	idstr = arg;
	text = strchr(arg,' ');
	if (text == NULL)  {
		warn(usage);
		return FAIL;
	}
	*text = '\0';
	text++;

	id = atoi(idstr);

	warn("id:%d, text:%s \n", id, text);

	extractor = sb_run_new_index_word_extractor(id);
	sb_run_index_word_extractor_set_text(extractor, text);
	while ( (n=sb_run_get_index_words(extractor, indexwords, 100)) > 0) {
		debug("n:%d", n);
		for (i=0; i<n; i++) {
			printf("[%s] \t (pos:%3d, field:%d)\n",
					indexwords[i].word, indexwords[i].pos, indexwords[i].field);
		}
	}

	sb_run_delete_index_word_extractor(extractor);

	return SUCCESS;
}
int com_morpheme (char *arg)
{
	Morpheme morp;
	WordList wordlist;
	char *text=NULL, *morpidstr=NULL;
	int morpid=1,i=0;

	if (strlen(arg) == 0) {
		printf("Usage:morpheme strings;[morpid(default=1)]"
				"e.g)morpheme 형태소분석할 문장\n");
		return FAIL;
	}

	text = arg;
	morpidstr = strchr(arg,';');
	if (morpidstr) {
		*morpidstr = '\0';
		morpidstr++;
		morpid = atoi(morpidstr);
	}
	warn("morpheme id:%d",morpid);
	warn("string:%s",text);

	sb_run_morp_set_text(&morp,arg,morpid);

	while (sb_run_morp_get_wordlist(&morp,&wordlist,morpid) != FAIL) {
		for (i=0; i<wordlist.num_of_words; i++) {
			printf("word[%d]:|%s|\n",
					wordlist.words[i].position,wordlist.words[i].word);
		}
		printf("\n");
	}

	return SUCCESS;
}

int com_qpp (char *arg)
{
	QueryNode qnodes[64];
	int numofnodes=0;
	if (strlen(arg) == 0) {
		printf("Usage: qpp query e.g)qpp 오렌지 & 사과\n");
		return FAIL;
	}

	numofnodes = sb_run_preprocess(mWordDb, arg,1024, qnodes, 64);
	sb_run_print_querynode(qnodes,numofnodes);
	return SUCCESS;
}

int search (char *arg, int silence, int* total_cnt)
{
#if 1
	int sockfd, i, n;
	sb4_search_result_t *result;
	char *docattr = NULL, *sc = NULL;

	if (!strlen(arg)) {
		printf("usage: search query/attrquery\n");
		return FAIL;
	}

	n = sb_run_sb4c_init_search(&result, COMMENT_LIST_SIZE/*result list size*/);
	if ( n == FAIL ) {
		error("sb4c_init_search failed.");
		return FAIL;
	}

	if(!silence)
	    printf("connect to %s:%s...\n",mServerAddr,mServerPort);

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect failed.");
		return FAIL;
	}

	docattr = strchr(arg, ';');
	if (docattr) {
		*docattr = '\0';
		docattr++;
	}

	if (sortingcriterion[0]) {
		sc = sortingcriterion;
	}

	n = sb_run_sb4c_search_doc(sockfd, arg, docattr, listcount, 
			first_page, sc, result);
	if ( n == FAIL ) {
		error("sb4c_search_doc failed.");
		return FAIL;
	}

	if(!silence) {
    	printf("word list: %s\n", result->word_list);
    	printf("total count: %d\n", result->total_count);
    	printf("received count: %d\n", result->received_count);
    	for ( i = 0; i < result->received_count; i++ ) {
    		printf("  [%d] %s\n",i, result->results[i]);
    	}
	}

	if(total_cnt != NULL)
	    *total_cnt = result->total_count;

	n = sb_run_sb4c_free_search(&result);
	if ( n == FAIL ) {
		error("sb4c_free_search failed.");
		return FAIL;
	}
	close(sockfd);

#endif
	return SUCCESS;
}

int com_search (char *arg)
{
	return search(arg, 0, NULL);
}

int com_search_setting (char *arg)
{
    int  count=0;

	count = sscanf(arg, "%d %d %s", &first_page, &listcount, sortingcriterion);
	if (count != 2 && count != 3) {
		error("search_setting [PG(page)] [LC(list count)] [SC]");	
		return FAIL;
	}

	if (count == 2) {
		sortingcriterion[0] = '\0';
	}

	info("first_page [%d] listcount [%d] SC [%s]", first_page, listcount,
			sortingcriterion);
	return SUCCESS;
}

int com_query_test(char *arg)
{
	char path[MAX_PATH_LEN];
	char buf[STRING_SIZE];
	char *usage="usage: query_test file_path\n";
	int i=0;
	FILE *fp;
	
	if (!strlen(arg)) {
		printf(usage);
		strcpy(path,"dat/test_query_set.txt");
	} else {
		strncpy(path, arg, strlen(arg));
		path[strlen(arg)]='\0';
	}

	if ((fp=sb_fopen(path,"r"))==NULL) {
		info("can't open test query file [%s]",path);
		return FAIL;
	}
	
	while(1)
	{
		if (fgets(buf, STRING_SIZE, fp) == NULL && feof(fp))
			break;
		buf[strlen(buf)-1]='\0';
		
		if (buf[0] == '#') {
			info("\n%s\n",buf);	
		} else {
			info("\nquery [%s]",buf);
			com_search(buf);
			i++;
		}
		fgets(buf, STRING_SIZE, stdin);
		
		if(strlen(buf)!=1) 
			break;
	}
	
	info("query test iterate %d",i);
	return SUCCESS;
}

int com_docattr_query_test(char *arg)
{
	docattr_cond_t cond;

	int ret = sb_run_qp_docattr_query_process(&cond, arg);
	return ret;
}

int com_rmas_run(char *arg)
{
	int field=0, morpid=0;
	char *usage="Usage: rmas fieldid morpid text";
	char *fieldstr=NULL, *morpidstr=NULL, *text=NULL;
	index_word_t *indexword=NULL;
	int num_of_indexwords=0, i=0;
	int rv=0;

	fieldstr = arg;
	morpidstr = strchr(arg,' ');
	if (morpidstr == NULL)  {
		warn(usage);
		return FAIL;
	}
	*morpidstr = '\0';
	morpidstr++;

	text = strchr(morpidstr,' ');
	if (text == NULL) {
		warn(usage);
		return FAIL;
	}
	*text = '\0';
	text++;

	field = atoi(fieldstr);
	morpid = atoi(morpidstr);


	warn("field:%d, morpid:%d", field, morpid);
	rv = sb_run_rmas_morphological_analyzer(field, text, (void*)&indexword,  &num_of_indexwords, morpid);
	if (rv != SUCCESS) {
		error("rmas_morphological_analyzer failed. return value[%d]", rv);
		return FAIL;
	}

	for (i=0; i<num_of_indexwords; i++) {
		printf("%s\t(pos:%u, field:%u\n", indexword[i].word, indexword[i].pos, indexword[i].field);
	}
	
	return SUCCESS;
}

int com_rmac_run(char *arg)
{
	int sockfd, i, n;
	void *result = NULL;
	char metadata[LONG_LONG_STRING_SIZE], filename[SHORT_STRING_SIZE];
	char *buf;
	int buflen, count;
	long resultlen;
	FILE *fp;

	if (sscanf(arg, "%s %s", metadata, filename) != 2) {
		error("usage: rmac metadata filename(cd format)");
		return FAIL;
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		error("cannot open file[%s]", filename);
		return FAIL;
	}

	buf = (char *)malloc(DOCUMENT_SIZE);
	if (buf == NULL) {
		crit("out of memory: %s", strerror(errno));
		return FAIL;
	}

	buflen = fread(buf, sizeof(char), DOCUMENT_SIZE, fp);
	fclose(fp);

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect failed.");
		free(buf);
		return FAIL;
	}

	n = sb_run_sb4c_remote_morphological_analyze_doc(sockfd, metadata, 
			buf, (long)buflen, &result, &resultlen);
	if ( n == FAIL ) {
		error("sb4c_search_doc failed.");
		goto FINISH;
	}

	if (resultlen % sizeof(index_word_t) != 0) {
		error("length of returned index word list is wrong");
		goto FINISH;
	}

	count = resultlen / sizeof(index_word_t);
	for (i=0; i<count; i++) {
		debug("%s:%d:%d", ((index_word_t*)result)[i].word,
							 ((index_word_t*)result)[i].pos,
							 ((index_word_t*)result)[i].field);
	}
	info("%d index words received\n", count);

FINISH:
	free(buf);
	close(sockfd);
	free(result);

	return SUCCESS;
}

int com_forward_index(char *arg)
{
#if FORWARD_INDEX==1
	uint32_t docid=0;
	int ret=0;
	char bprinthits=1;
	char *usage="usage: forward_index docid field [bprinthits(0/1)]\n";
	char field[STRING_SIZE];
	int fixedsize = sizeof(forwardidx_header_t);
	int default_var_size = sizeof(forward_hit_t);

	ret = sscanf(arg, "%ld %s %c",&docid, field, &bprinthits);
	if (ret < 2) {
		fprintf(stderr,usage);
		return FAIL;
	}
	warn("did[%ld] field[%s] , flag[%d]", docid, field, bprinthits);

	if (mForwardVRFI == NULL) {
		sb_run_vrfi_alloc(&mForwardVRFI);
		ret=sb_run_vrfi_open(mForwardVRFI,
						"dat/forward_index/forward_idx",
						fixedsize, default_var_size);
		if (ret < 0) {
			error("error opening forward index");
			error("path:dat/forward_index, file:fw_index)");
			return ret;
		}
	}
	ret = sb_run_print_forwardidx(mForwardVRFI,docid,field,bprinthits,stderr);
	if (ret < 0) {
		warn("sb_run_print_forwardidx returned ret:%d",ret);
	}
	
	return ret;
#else
	return SUCCESS;
#endif
}

int com_get_docattr(char *arg)
{
    docattr_t attr;
    uint32_t docid;
    char field[256];
    char buf[STRING_SIZE];

    if (sscanf(arg, "%u %s", &docid, field) != 2) {
        error("usage: get_docattr docid fieldname");
        return -1;
    }

    if (sb_run_docattr_get(docid, &attr) == -1) {
        error("cannot get docattr");
        return -1;
    }

    if (sb_run_docattr_get_docattr_function(&attr, field, buf, STRING_SIZE) 
			== -1) {
        error("cannot get value");
        return -1;
    }

    debug("docattr[%s]: %s", field, buf);
	return 1;
}

int com_set_docattr(char *arg)
{
    uint32_t docid;
    char field[SHORT_STRING_SIZE], value[STRING_SIZE];

    if (sscanf(arg, "%u %s %s", &docid, field, value) != 3) {
        error("usage: set_docattr docid fieldname value");
        return -1;
    }

	{
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, field, value);
		sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
	}

    debug("docattr[%s]: %s", field, value);
	return 1;
}

int com_set_docattr_by_oid(char *arg)
{
    char oid[STRING_SIZE], buf[STRING_SIZE], attrquery[STRING_SIZE];
	int result, sockfd=0;

    if (sscanf(arg, "%s %s", oid, attrquery) != 2) {
        error("usage: get_docattr docid attrquery(e.g field1:2&field2:3)");
        return -1;
    }

	snprintf(buf, STRING_SIZE, "OID=%s^AT=%s^", oid, attrquery);
	attrquery[STRING_SIZE-1] = '\0';

	result = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( result == FAIL ) {
		error("tcp_connect failed.");
		return FAIL;
	}

	if (sb_run_sb4c_set_docattr(sockfd, buf) == FAIL) {
		error("cannot set docattr");
		return FAIL;
	}

	sb_run_tcp_close(sockfd);

	printf("done: %s\n", buf);
	return 1;
}

int com_undelete (char *arg)
{
	int i, j;
	uint32_t docid[1024], start=0, finish=0;
	char *comma=arg;

	if (!strlen(arg)) {
		printf("usage: undelete [docid]; docid ::= NUMBER, docid; "
				"NUMBER ::= [0-9]* | [0-9]* - [0-9];\n");
		printf("\te.g) delete 1, 2, 5-13, 15, 20-30, 35, 56\n");
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
	return SUCCESS;
}
int com_delete (char *arg)
{
	int i, j;
	uint32_t docid[1024], start=0, finish=0;
	char *comma=arg;

	if (!strlen(arg)) {
		printf("usage: delete [docid]; docid ::= NUMBER, docid; "
				"NUMBER ::= [0-9]* | [0-9]* - [0-9];\n");
		printf("\te.g) delete 1, 2, 5-13, 15, 20-30, 35, 56\n");
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

	debug("you can delete upto %d document.",1024);

	debug("you want to delete document");
	debug("%u", docid[0]);
	for (i=1; i<j; i++) {
		debug(", %u", docid[i]);
	}

	{
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
		sb_run_docattr_set_array(docid, j, SC_MASK, &docmask);
	}
	return SUCCESS;
}

int com_delete_doc (char *arg)
{
	uint32_t docid;
	int sockfd, n;

	docid = (uint32_t)atol(arg);

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect failed.");
		return FAIL;
	}

	n = sb_run_sb4c_delete_doc(sockfd, docid);
	if ( n == FAIL ) {
		error("sb4c_delete_doc failed.");
		return FAIL;
	}

	return SUCCESS;
}

int com_get_new_wordid(char *arg)
{
	int ret;
	char *token=NULL;
	char orig_arg[SHORT_STRING_SIZE]="";
	char *tmp=arg;
	int count=0;

	char *word;
	word_t lexicon;	

	if ( mWordDb == NULL ) {
		warn("load lexicon module to use this command");
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

	if (count == 1) {
		word = orig_arg;
	} else {
		info("usage : get_new_wordid word");
		return FAIL;
	}
	
	strncpy(lexicon.string, arg , MAX_WORD_LEN);
	ret = sb_run_get_new_wordid(mWordDb, &lexicon);
	if (ret < 0) {
		error("error while get new wordid for word[%s]", lexicon.string);
		return FAIL;
	}

	printf("ret[%d], word[%s] wordid[%u]\n",ret, lexicon.string, lexicon.id);
	return SUCCESS;
}

int com_get_wordid (char *arg)
{
	int ret;
/*	LWord retWord;*/
	word_t lexicon;
	
	if ( mWordDb == NULL ) {
		warn("load lexicon module to use this command");
		return FAIL;
	}

	debug("argument:[%s]",arg);
	
	strncpy(lexicon.string, arg , MAX_WORD_LEN);
	ret = sb_run_get_word(mWordDb, &lexicon);

	if ( ret == WORD_NOT_REGISTERED ) {
		error("ret[%d] : no such word[%s]",ret ,arg);
		return FAIL;
	}
	else if ( ret < 0 ) {
		error("ret[%d] : error", ret);
		return FAIL;
	}

	printf("ret[%d], word[%s]'s wordid[%d]\n",
			ret ,lexicon.string ,lexicon.id );

	return SUCCESS;
}

int com_get_word_by_wordid (char *arg)
{
	int ret;
	word_t lexicon;

	if ( mWordDb == NULL ) {
		warn("load lexicon module to use this command");
		return FAIL;
	}

	if (!strlen(arg)) {
		printf("usage: get_word_by_wordid wordid\n");
		return FAIL;
	}

	memset( &lexicon, 0, sizeof(lexicon) );

	lexicon.id = (uint32_t)atol(arg);
	ret = sb_run_get_word_by_wordid(mWordDb, &lexicon);
	if ( ret == WORD_NOT_REGISTERED ) {
		info("word is not registered");
	}
	else if ( ret < 0 ) {
		error("error ret:%d", ret);
	}
	else info("id:(%u) word[%s] ", lexicon.id, lexicon.string);

	return SUCCESS;
}

int com_sync_word_db (char *arg)
{
	int ret;
	
	if ( mWordDb == NULL ) {
		warn("load lexicon module to use this command");
		return FAIL;
	}

	info("synchronize word db");
			
	ret = sb_run_sync_word_db(mWordDb);

	info("synchronize word db ret :%d",ret);
	
	return SUCCESS;
}

int com_get_num_of_wordid (char *arg)
{
	int ret;
	uint32_t wordid;
	
	if ( mWordDb == NULL ) {
		warn("load lexicon module to use this command");
		return FAIL;
	}

	info("get num of wordid");
	ret = sb_run_get_num_of_word(mWordDb, &wordid);
	
	info("ret[%d], last wordid is %u",ret , wordid);	
	return SUCCESS;
}

int com_get_docid(char *arg)
{
	int ret;
	uint32_t docid;

	ret = sb_run_get_docid(mDidDb, arg, &docid);
	if (ret < 0) {
		printf("cannot get document id:error[%d]\n", ret);
		return FAIL;
	}

	printf("result:%u\n", docid);
	return SUCCESS;
}

int com_get_new_docid(char *arg)
{
	int ret;
	uint32_t docid, olddocid;

	ret = sb_run_get_new_docid(mDidDb, arg, &docid, &olddocid);
	if (ret < 0) {
		printf("cannot get document id:error[%d]\n", ret);
		return FAIL;
	}

	printf("result: %u\n", docid);
	if ( ret == DOCID_OLD_REGISTERED )
		printf("olddocid: %u\n", olddocid);

	return SUCCESS;
}
int com_registry (char *arg)
{
	int n, sockfd;
	static char buf[STRING_SIZE] = "registry";

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect(%d,%s,%s) failed.", sockfd, mServerAddr, mServerPort);
		return FAIL;
	}

	if (strlen(arg) != 0)
		snprintf(buf, STRING_SIZE, "registry %s", arg);

	sb_run_sb4c_status(sockfd, buf, stdout);
	close(sockfd);

	return SUCCESS;
}

int com_config (char *arg)
{
	int n, sockfd;
	static char buf[STRING_SIZE] = "config";

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect(%d,%s,%s) failed.", sockfd, mServerAddr, mServerPort);
		return FAIL;
	}

	if (strlen(arg) != 0)
		snprintf(buf, STRING_SIZE, "config %s", arg);

	sb_run_sb4c_status(sockfd, buf, stdout);
	close(sockfd);

	return SUCCESS;
}

int com_status (char *arg)
{
	int n, sockfd;
	static char buf[] = "help";

	n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
	if ( n == FAIL ) {
		error("tcp_connect(%d,%s,%s) failed.", sockfd, mServerAddr, mServerPort);
		return FAIL;
	}

	if (strlen(arg) == 0)
		sb_run_sb4c_status(sockfd, buf, stdout);
	else
		sb_run_sb4c_status(sockfd, arg, stdout);
	close(sockfd);

	return SUCCESS;
}

int com_status_test (char *arg)
{
	int i, n, sockfd, iteration = 10000;
	static char buf[] = "help";
	FILE *nullout;
	struct timeval tv1, tv2;
	double diff;

	nullout = fopen("/dev/null", "a");
	gettimeofday(&tv1, NULL);
	for ( i = 0; i < iteration; i++ ) {
		if ( i % 10 == 9 ) fprintf(stderr, ".");

		n = sb_run_tcp_connect(&sockfd, mServerAddr, mServerPort);
		if ( n == FAIL ) {
			error("tcp_connect(%d,%s,%s) failed.", sockfd, mServerAddr, mServerPort);
			return FAIL;
		}

		if (strlen(arg) == 0)
			sb_run_sb4c_status(sockfd, buf, nullout);
		else
			sb_run_sb4c_status(sockfd, arg, nullout);
		sb_run_tcp_close(sockfd);
	}
	gettimeofday(&tv2, NULL);
	fclose(nullout);
	diff = timediff(&tv2, &tv1);
	fprintf(stdout, "\ncompleted %d times. total %2.2f sec, %2.2f queries/sec.\n",
			iteration, diff, iteration/diff);

	return SUCCESS;
}

int com_connectdb(char *arg)
{
#if 0
	char dbname[MAX_DBNAME_LEN], _dbpath[MAX_DBPATH_LEN], 
		 dbpath[MAX_DBPATH_LEN];
	if (sscanf(arg, "%s %s", dbname, _dbpath) != 2) {
		error("usage: connectdb [cdm db name] [cdm db path]");
		return FAIL;
	}
	sprintf(dbpath, "%s/%s", gSoftBotRoot, _dbpath);
	if ((clientcdmdb = sb_run_cdm_db_open(dbname, dbpath, CDM_SHARED)) 
			== NULL) {
		error("cannot connect db [%s, %s]", dbname, dbpath);
		return FAIL;
	}
	info("connected db [%s, %s] successfully", dbname, dbpath);
	return SUCCESS;
#endif
	warn("temporarily unavailable");
	return FAIL;
}

int com_selectdoc(char *arg)
{
#if 0
	char fieldtext[DOCUMENT_SIZE];
	char fieldname[STRING_SIZE], path[STRING_SIZE];
	uint32_t docid;
	void *parser;
	field_t *field;
	pdom_t *xmldom;
	int len=0;

	if (sscanf(arg, "%s from %d", fieldname, &docid) != 2) {
		error("usage: selectdoc [field name] from [docid]");
		return FAIL;
	}

	if (clientcdmdb == NULL) {
		error("connect db with 'connectdb' command first");
		return FAIL;
	}

	if ((xmldom = sb_run_cdm_retrieve_internal(clientcdmdb, docid)) == NULL) {
		error("error while doc_get document of internal key[%u]",docid);
		return FAIL;
	}
	parser = sb_run_cdm_get_parser(xmldom);

	sprintf(path, "/Document/%s", fieldname);
	field = sb_run_xmlparser_retrieve_field(parser, path);
	if (field == NULL) {
		error("cannot get doc of internal key[%u], field[%s]", 
				docid, path);
		sb_run_xmlparser_free_parser(parser);
		return FAIL;
	}

	len = field->size > DOCUMENT_SIZE ? DOCUMENT_SIZE : field->size;
	strncpy(fieldtext, field->value, len);
	fieldtext[len] = '\0';

	printf("%s\n", fieldtext);
	return SUCCESS;
#endif
	warn("temporarily unavailable");
	return FAIL;
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

int com_rebuild_docattr(char *arg)
{
    int i, start, finish, iSize, result;
    VariableBuffer var;
    static char *aCannedDoc = NULL;
    void *p;

	if (aCannedDoc == NULL) {
		aCannedDoc = (char *)malloc(DOCUMENT_SIZE);
		if (aCannedDoc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}

    if (sscanf(arg, "%d %d", &start, &finish) != 2) {
        error("usage: rebuild_docattr start finish");
        return -1;
    }

    /* parse cdm */
    for (i=start; i<=finish; i++) {
    if (i % 1000 == 0) crit("%d... rewrite docattr", i);

    sb_run_buffer_initbuf(&var);
    result = sb_run_server_canneddoc_get(i,&var);
    if (result < 0) {
        error("canneddoc_get error.\n");
        sb_run_buffer_freebuf(&var);
        continue;
    }
//  sb_run_buffer_freebuf(&var);
    iSize = sb_run_buffer_getsize(&var);
    if (iSize > DOCUMENT_SIZE) {
        sb_run_buffer_freebuf(&var);
        continue;
    }
//  bzero(aCannedDoc, DOCUMENT_SIZE);
    sb_run_buffer_get(&var, 0, iSize, aCannedDoc);
    sb_run_buffer_freebuf(&var);
    aCannedDoc[iSize] = '\0';

    p = sb_run_xmlparser_parselen("EUC-KR", aCannedDoc, iSize);
    if (p == NULL) {
        error("cannot parse document[%d]", i);
        continue;
    }

    { /* insert some field into docattr db */
        docattr_mask_t docmask;
        int len, j;
        char *val, value[STRING_SIZE], path[STRING_SIZE];
		char* field_value; int field_length;
        uint32_t docid;

        docid = i;
        /* setting docmask data */
        DOCMASK_SET_ZERO(&docmask);
        for (j=0; j<MAX_FIELD_NUM && docattrFields[j]; j++) {
            strcpy(path, "/Document/");
            strcat(path, docattrFields[j]);

            result = sb_run_xmlparser_retrieve_field(p, path, &field_value, &field_length);
            if (result != SUCCESS) {
                warn("cannot get field[/%s/%s] of ducument[%d] (path:%s)",
                        "/Document",
                        docattrFields[j], i, path);
                continue;
            }

            len = field_length;
            val = __trim(field_value, &len);
            len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
            strncpy(value, val, len);
            value[len] = '\0';

            if (len == 0) {
                continue;
            }

            if (sb_run_docattr_set_docmask_function(&docmask, docattrFields[j],
                        value) == -1) {
                warn("wrong type of value of field[/%s/%s] of ducument[%d]", 
                        "/Document",
                        docattrFields[j], i);
            }
        }

        if (sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask) == -1) {  
            warn("cannot insert field[/%s/%s] into docattr db of doc[%d]",   
                    "/Document",
                    docattrFields[j], i);
        }
    }
    sb_run_xmlparser_free_parser(p);

    }
    return 1;
}

int com_rebuild_rid(char *arg)
{
    int i, start, finish, iSize, result;
    VariableBuffer var;
    char aCannedDoc[DOCUMENT_SIZE];
    void *p;

    if (sscanf(arg, "%d %d", &start, &finish) != 2) {
        error("usage: rebuild_docattr start finish");
        return -1;
    }

    /* parse cdm */
    for (i=start; i<=finish; i++) {
    if (i % 1000 == 0) crit("%d... rewrite rid", i);

    sb_run_buffer_initbuf(&var);
    result = sb_run_server_canneddoc_get(i,&var);
    if (result < 0) {
        error("canneddoc_get error.\n");
        sb_run_buffer_freebuf(&var);
        continue;
    }
//  sb_run_buffer_freebuf(&var);
    iSize = sb_run_buffer_getsize(&var);
    if (iSize > DOCUMENT_SIZE) {
        sb_run_buffer_freebuf(&var);
        continue;
    }
//  bzero(aCannedDoc, DOCUMENT_SIZE);
    sb_run_buffer_get(&var, 0, iSize, aCannedDoc);
    sb_run_buffer_freebuf(&var);
    aCannedDoc[iSize] = '\0';

    p = sb_run_xmlparser_parselen("EUC-KR", aCannedDoc, iSize);
    if (p == NULL) {
        error("cannot parse document[%d]", i);
        continue;
    }

    { /* insert some field into docattr db */
        int len;
        char *val, value[STRING_SIZE], path[STRING_SIZE], rid[STRING_SIZE];  
		char* field_value; int field_length;
		uint32_t docid;

        /* TYPE2 Check */
        strcpy(path, "/Document/TYPE2");

        result = sb_run_xmlparser_retrieve_field(p, path, &field_value, &field_length);
        if (result != SUCCESS) {
            warn("cannot get field[%s] of ducument[%d]", path, i);
            goto done;
        }

        len = field_length;
        val = __trim(field_value, &len);
        len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
        strncpy(value, val, len);
        value[len] = '\0';

        if (len == 0) {
            warn("length of TYPE2 of docid %d is zero", i);
            goto done;
        }

        if (value[0] == '0') {
            docattr_mask_t docmask;
            docid = i;
            info("delete %d", i);

            DOCMASK_SET_ZERO(&docmask);
            sb_run_docattr_set_docmask_function(&docmask, "Delete", "1");
            sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);

            goto done;
        }
        else if (value[0] != '1') {
            error("invalid TYPE2 data of docid[%d]", i);
            goto done;
        }

        /* if TYPE2 == 1 */

        rid[0] = '\0';

        /* make rid */
        strcpy(path, "/Document/ctrlno");

        result = sb_run_xmlparser_retrieve_field(p, path, &field_value, &field_length);
        if (result != SUCCESS) {
            warn("cannot get field[%s] of ducument[%d]",path,i);
            goto done;
        }

        len = field_length;
        val = __trim(field_value, &len);
        len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
        strncpy(value, val, len);
        value[len] = '\0';

        if (len == 0) {
            warn("length of ctrl number of docid %d is zero", i);
            goto done;
        }

        strcat(rid, value);
        strcat(rid, "_");

        strcpy(path, "/Document/ctrltype");

        result = sb_run_xmlparser_retrieve_field(p, path, &field_value, &field_length);
        if (result != SUCCESS) {
            warn("cannot get field[%s] of ducument[%d]",path,i);
            goto done;
        }

        len = field_length;
        val = __trim(field_value, &len);
        len = (len>STRING_SIZE-1)?STRING_SIZE-1:len;
        strncpy(value, val, len);
        value[len] = '\0';

        if (len == 0) {
            warn("length of ctrl type of docid %d is zero", i);
            goto done;
        }

        strcat(rid, value);

        {   
            docattr_mask_t docmask;
            docid = i;

            DOCMASK_SET_ZERO(&docmask);
            CRIT("rid[%s] is created for docid[%d]", rid, i);
            sb_run_docattr_set_docmask_function(&docmask, "Rid", rid);
            sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
        }
    }
done:
    sb_run_xmlparser_free_parser(p);

    }
    return 1;
}

int com_benchmark(char *arg)
{
    int argc, status;
    char **argv;
	pid_t pid;

	info("arg : [%s]", arg);

	if(arg == NULL) {
		warn("arg is NULL");
	}

    if(make_arg(&argv, &argc, arg, "benchmark") != SUCCESS)
        return FAIL;

    status_arg((const char**)argv, argc);

	//XXX getopt not use twice in one prcess.
    pid = fork();
    if(pid == 0) { //child
        benchmark(argc, argv);
		exit(0);
	} else {
		waitpid(pid, &status, 0);
	}

    destroy_arg(argv, argc);

    return (status == 0) ? SUCCESS: FAIL;
}

int com_quit (char *arg)
{
//	exit(0);
	return DECLINE;
}

int com_help (char *arg)
{
	register int i;
	int printed = 0;

	for ( i = 0; commands[i].name; i++ ) {
		if ( !*arg || (strcmp(arg, commands[i].name) == 0) ) {
			if (strcmp(commands[i].name,BLANKLINE) == 0) 
				printf("\n");
			else 
				printf("%-10s :  %s\n", commands[i].name, commands[i].desc);
			printed++;
		}
	}

	if (printed == 0) {
		printf("No command match `%s'. Possibilities are:\n", arg);

		for ( i = 0; commands[i].name; i++ ) {
			/* print in six columns **/
			if ( printed == 6 ) {
				printed = 0;
				printf("\n");
			}

			printf("%s\t", commands[i].name);
			printed++;
		}

		if (printed) printf("\n");
	}

	return SUCCESS;
}

