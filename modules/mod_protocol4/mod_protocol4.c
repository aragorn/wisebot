/* $Id$ */
#include <time.h>
#include <sys/wait.h>

#include "mod_api/mod_api.h"
#include "mod_vbm/mod_vbm.h"
#include "mod_cdm/mod_cdm.h"
#include "mod_qp/mod_qp.h"
#include "mod_api/rmas.h"
#include "mod_client.h"
//#include "sn2.h"

#define PROCESS_HANDLE 1
//#undef PROCESS_HANDLE
#define BUF_SIZE 100


/*****************************************************************************/
static void make_querystr(char *formatted, int size, char *query, char *docAttr, int listcount, int page, char *sh);
static int parse_dit(sb4_dit_t *sb4_dit, char *dit);
static int get_str_item(char *dest, char *dit, char *key, char delimiter, int len);
static int get_int_item (char *dit, char *key, char delimiter);
static int get_long_item (char *dit, char *key, char delimiter);
static int send_nak(int sockfd, int error_code);
static char *get_field_and_ma_id_from_meta_data(char *sptr , char* field_name,  int *id);

int TCPSendLongData(int sockfd, long size, void *data, int server_side); // this function is used by RMAS and RMAC
int TCPRecvLongData(int sockfd, long size , void *data, int server_side); // this function is used by RMAS and RMAC

static char regifail_file_path[STRING_SIZE] = "dat/cdm/register.fail";
static int regifailfd = 0;
static int regifailnum = 0;

int sb4c_remote_morphological_analyze_doc(int sockfd, char *meta_data , void *send_data , 
		long send_data_size ,  void **receive_data , long *receive_data_size)
{
	char buf[SB4_MAX_SEND_SIZE+1];
	int len = 0x00;

	if (send_data_size <= 0) {
		error("wrong send_data_size : %ld", send_data_size);
		return FAIL;
	}

	/* 1. send OP code */
	if ( TCPSendData(sockfd, SB4_OP_RMA_DOC, 3, FALSE) == FAIL ) {
		error("cannot send OP_RMA_DOC(%s)", SB4_OP_RMA_DOC);
		return FAIL;
	}

	/* 2. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_RMA_CODE"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_RMA_CODE"); return FAIL; }

	/* 3. send META data */
//	INFO("metadata[%d]: %s", strlen(meta_data), meta_data);
	if ( TCPSendData(sockfd, meta_data, strlen(meta_data), FALSE) == FAIL ) {
		error("cannot send RMA_META_DATA");
		return FAIL;
	}
	/* 4. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for send meta_data"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for meta_data"); return FAIL; }

	/* 5. send data_size */
	sprintf(buf, "%ld", send_data_size);
	len = strlen(buf);

	if ( TCPSendData(sockfd, buf, len, FALSE) == FAIL ) {
		error("cannot send RMA_SRC_DATA_SIZE");
		return FAIL;
	}

	/* 6. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for RMA_SRC_DATA"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for RMA_SRC_DATA"); return FAIL; }


	/* 7. send data */
	if ( TCPSendLongData(sockfd, send_data_size, send_data, FALSE) == FAIL ) {
		error("cannot send RMA_SRC_DATA");
		return FAIL;
	}

	/* 8. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for RMA_SRC_DATA"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for RMA_SRC_DATA"); return FAIL; }


	/* 9. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 10. recv recive_data_size */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv RECV_DATA_SIZE"); return FAIL; }

	buf[len] = '\0';
	*receive_data_size = atol(buf);

	/* 11. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	// allocate receive_data_buffer
	*receive_data = sb_calloc(*receive_data_size, 1);
	if (*receive_data == 0x00) {
		error("allocation error : receive data ");
		return FAIL;
	}

	/* 12. receive index_word_array from server */
	if ( TCPRecvLongData(sockfd, *receive_data_size, *receive_data, FALSE) == FAIL ) {
		error("cannot recv word_list");
		return FAIL;
	}

	/* 13. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK for RMA");
		return FAIL;
	}

	return SUCCESS;
}

int sb4s_remote_morphological_analyze_doc(int sockfd)
{

	char buf[SB4_MAX_SEND_SIZE+1];
	char field_name[MAX_FIELD_NAME_LEN] , *pbuf=NULL;
	char meta_data[SB4_MAX_SEND_SIZE+1];
	int data_size =0, nret;
	int recv_data_size = 0, ma_id = 0 , len = 0, field_id;
	void *recv_data = NULL,  *tmp_data = NULL;
	parser_t *parser = NULL;
	field_t *field;
	sb4_merge_buffer_t merge_buffer;
	char *buffer = NULL;
	uint32_t buffer_size=0;

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive meta_data */
	if ( TCPRecvData(sockfd, buf,&len, TRUE) == FAIL ) {
		error("cannot recv meta_data");
		return FAIL;
	}
	buf[len] = '\0';
	strncpy(meta_data, buf, SB4_MAX_SEND_SIZE);
	meta_data[len] = '\0';
//	INFO("metadata: %s", meta_data);

	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 5. receive recv_data_size */
	if ( TCPRecvData(sockfd, buf, &len, TRUE) == FAIL ) {
		error("cannot recv recv_data_size");
		return FAIL;
	}
	buf[len] = '\0';
//	INFO("data size is %s", buf);

	recv_data_size = atol(buf);
	recv_data = sb_calloc(1 , recv_data_size);

	/* 6. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		sb_free(recv_data);
		return FAIL;
	}

	/* 7. receive recv_data */
//	INFO("start recv data which size is %d", recv_data_size);
	if ( TCPRecvLongData(sockfd, recv_data_size, recv_data, TRUE) != SUCCESS ) {
		error("cannot recv data");
		return FAIL;
	}

	/* 8. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		sb_free(recv_data);
		return FAIL;
	}

	/* 9. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, TRUE) == FAIL ) { 
		error("cannot recv OP_ACK or OP_NAK"); 
		sb_free(recv_data);
		return FAIL; 
	}
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 ) { 
		error("received OP_NAK");
		sb_free(recv_data); 
		return FAIL; 
	}


//	INFO("starting parsing ducument");
	parser = sb_run_xmlparser_parselen("CP949" , (char *)recv_data, recv_data_size);
	if (parser == NULL) { 
		error("cannot parse document");
		return FAIL;
	}
//	INFO("parsing document successfully");

	sb_free(recv_data);

	pbuf = meta_data;
	merge_buffer.data = NULL;
	merge_buffer.data_size = 0;
	merge_buffer.allocated_size = 0;

	field_id = 0;

	do {
		char path[STRING_SIZE];
		char *field_id_ptr=0x00;
		char fid[3];
		int  field_id_is_given = 0;

		pbuf = get_field_and_ma_id_from_meta_data(pbuf , field_name,  &ma_id);
//              INFO("fieldname: %s", field_name);

		field_id_ptr = strchr(field_name, '#');
		if (field_id_ptr != NULL) field_id_is_given = 1;
		if (field_id_is_given == 1)
		{
			strcpy(fid, field_id_ptr+1);
			*field_id_ptr = '\0';
			field_id = atol(fid);
		}
		INFO("fieldname[%s] id[%d]", field_name, field_id);


		sprintf(path, "/Document/%s", field_name);
		path[STRING_SIZE-1] = '\0';
		field = sb_run_xmlparser_retrieve_field(parser , path);

		if (field_id_is_given)
		{
			if (field == NULL) {
				warn("cannot retrieve field[%s]", path);
				continue;
			}

			if (field->size == 0) {
				continue;
			}
		}
		else
		{
			if (field == NULL) {
				warn("cannot retrieve field[%s]", path);
				field_id++;
				continue;
			}

			if (field->size == 0) {
				field_id++;
				continue;
			}
		}

		if ( field->size+1 > buffer_size ) {

			sb_free(buffer);

			buffer = sb_malloc(sizeof(char) * (field->size+1));
			if(buffer==NULL) {
				error("cannot allocate buffer");
				sb_run_xmlparser_free_parser(parser);
				sb_free(buffer);
				sb_free(merge_buffer.data);
				return FAIL;
			}
			buffer_size = field->size + 1;
		}

		memcpy(buffer, field->value, field->size);
		buffer[field->size] = '\0';

	//	INFO("starting ma [%s:%d] field->value:[%s]", field_name, field_id, buffer);

		tmp_data = NULL;
		nret = sb_run_rmas_morphological_analyzer(field_id, buffer, &tmp_data, 
				&data_size, ma_id);
		//INFO("finish ma: data_size: %d", data_size);
		if (nret == FAIL) {
			warn("failed to do morp.. analysis for doc(while analyzing)");
			sb_free(buffer);
			sb_free(merge_buffer.data);
			sb_run_xmlparser_free_parser(parser);
			return FAIL;
		}

		/* FIXME tmp_data can be NULL when there is a empty field. */
		/*
		if (tmp_data == NULL) {
			warn("tmp_data == NULL");
			sb_free(tmp_data);
			sb_free(buffer);
			sb_free(merge_buffer.data);
			sb_run_xmlparser_free_parser(parser);
			return FAIL;
		}
		*/

		nret = sb_run_rmas_merge_index_word_array( &merge_buffer , tmp_data , data_size);
		if (nret == FAIL) {
			warn("failed to do morp.. analysis for doc(while merging)");
			sb_free(tmp_data);
			sb_free(buffer);
			sb_free(merge_buffer.data);
			sb_run_xmlparser_free_parser(parser);
			return FAIL;
		}
		sb_free(tmp_data);

		field_id++;

	} while (*pbuf);

	sb_run_xmlparser_free_parser(parser);
	parser = NULL;
	sb_free(buffer);
	buffer = NULL;

	/* 10. send data size */
	snprintf(buf, SB4_MAX_SEND_SIZE, "%d" , merge_buffer.data_size);
	buf[SB4_MAX_SEND_SIZE] = '\0';
	len = strlen(buf);
	//INFO("length: %s", buf);

/*	if (merge_buffer.data == NULL) {
		error("allocation error : send data");
		return FAIL;
	} */

	if ( TCPSendData(sockfd, buf, len, TRUE) == FAIL ) {
		error("cannot send RMA_DATA_SIZE");
		sb_free(merge_buffer.data);
		return FAIL;
	}

	/* 11. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, TRUE) == FAIL ) { 
		error("cannot recv OP_ACK or OP_NAK for RMA_DATA_SIZE"); 
		sb_free(merge_buffer.data);
		return FAIL; 
	}
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 ) {
		error("received OP_NAK for RMA_DATA_SIZE"); 
		sb_free(merge_buffer.data);
		return FAIL;
	}
	
	//check data to send. 
/*	{*/
/*		int i;*/
/*		int k = merge_buffer.data_size / sizeof(index_word_t);*/
/*		int r = 3 < k ? 3 : k;*/

/*	 	index_word_t *idx = NULL;*/

/*		for(i=0;i<r;i++) {*/
/*			idx = (index_word_t*)merge_buffer.data + i;*/
/*			INFO("RMAS Pos:%d Word:%s", idx->pos , idx->word);*/
/*		}*/

/*		INFO("************************************************");*/
/*			for(i=k-r;i<k;i++) {*/
/*			idx = (index_word_t*)merge_buffer.data + i;*/
/*			INFO("RMAS Pos:%d Word:%s", idx->pos , idx->word);*/
/*		}*/
/*		INFO("************************************************");*/
/*	}*/
		

	//INFO("sending index word list: %d", merge_buffer.data_size);
	/* 12. send data */
	if ( TCPSendLongData(sockfd, merge_buffer.data_size, merge_buffer.data, TRUE) == FAIL ) {
		error("cannot send RMA_DATA");
		sb_free(merge_buffer.data);
		return FAIL;
	}
	sb_free(merge_buffer.data);
	//INFO("finish sending index word list");
	
	/* 13. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, TRUE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for RMA_DATA"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for RMA_DATA"); return FAIL; }


	return SUCCESS;
}

static char *get_field_and_ma_id_from_meta_data(char *sptr , char* field_name,  int *id)
{
	int i;
	char ids[4];
	
	for(;*sptr && *sptr != ':' ; sptr++, field_name++)
	{
		*field_name = *sptr;
	}
	*field_name = '\0';
	sptr++;

	for(i=0;*sptr && *sptr != '^' && i < MAX_FIELD_NAME_LEN ; sptr++, i++)
	{
		ids[i] = *sptr;
	}
	ids[i] = '\0';
	sptr++;

	*id = atol(ids);
//	INFO("morp-id: %d", *id);

	return sptr;

}

static int sb4c_get_doc(int sockfd, uint32_t docid, char *buf, int bufsize) 
{
	int len, docsize;
	char tmpbuf[STRING_SIZE];

	/* 1. send OP code */
	if ( TCPSendData(sockfd, SB4_OP_GET_DOC, 3, FALSE) == FAIL ) {
		error("cannot send SB4_OP_GET_DOC(%s)", SB4_OP_GET_DOC);
		return FAIL;
	}

	/* 2. recv ACK or NAK */
	if ( TCPRecvData(sockfd, tmpbuf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_CODE"); return FAIL; }
	if ( strncmp(tmpbuf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_CODE"); return FAIL; }

	/* 3. send docid */
	snprintf(tmpbuf, STRING_SIZE, "%u", docid);
	tmpbuf[STRING_SIZE-1] = '\0';
	if ( TCPSendData(sockfd, tmpbuf, strlen(tmpbuf), FALSE) == FAIL ) {
		error("cannot send docid");
		return FAIL;
	}

	/* 4. recv ACK or NAK */
	if ( TCPRecvData(sockfd, tmpbuf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for docid"); return FAIL; }
	if ( strncmp(tmpbuf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for docid"); return FAIL; }

	/* 5. recv document size */
	if ( TCPRecvData(sockfd, tmpbuf, &len, FALSE) == FAIL ) {
		error("cannot recv document size");
		return FAIL;
	}
	tmpbuf[len] = '\0';
	docsize = atol(tmpbuf);
//	INFO("docsize: %s, %d", tmpbuf, docsize);

	/* 6. recv document */
	if (docsize > bufsize-1) {
		error("document size[%d] is bigger than buffer size[%d]", docsize, bufsize);
		return FAIL;
	}
	if ( TCPRecvLongData(sockfd, docsize, buf, FALSE) == FAIL ) {
		error("cannot recv document");
		return FAIL;
	}
	buf[docsize] = '\0';

	return SUCCESS;
}
			
int sb4s_get_doc(int sockfd)
{
	uint32_t docid;
	int len, rv, docsize;
	char tmpbuf[STRING_SIZE];
	VariableBuffer varbuf;

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

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive docid */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv docid");
		return FAIL;
	}
	tmpbuf[len] = '\0';
	docid = (uint32_t)atol(tmpbuf);

	/* get document */
	sb_run_buffer_initbuf(&varbuf);
	rv = sb_run_server_canneddoc_get((DocId)docid, &varbuf);
	if (rv < 0) {
		error("cannot get document[%u]", docid);
		send_nak(sockfd, SB4_ERD_DENY_DEL);
		return FAIL;
	}
	docsize = sb_run_buffer_getsize(&varbuf);
	if (docsize > DOCUMENT_SIZE) {
		error("docsize[%d] is bigger than system limit:DOCUMENT_SIZE[%d]",
				docsize, DOCUMENT_SIZE);
		send_nak(sockfd, SB4_ERD_TOO_BIG);
		return FAIL;
	}
	sb_run_buffer_get(&varbuf, 0, docsize, buf);

	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 5. send document size */
	snprintf(tmpbuf, STRING_SIZE, "%d", docsize);
	tmpbuf[STRING_SIZE-1] = '\0';
	len = strlen(tmpbuf);
//	INFO("document size is %s, %d", tmpbuf, docsize);
	if ( TCPSendData(sockfd, tmpbuf, len, TRUE) == FAIL ) {
		error("cannot send document size");
		return FAIL;
	}

	/* 6. send data */
	if ( TCPSendLongData(sockfd, docsize, buf, TRUE) == FAIL ) {
		error("cannot send document");
		return FAIL;
	}
	buf[docsize] = '\0';
//	INFO("document[%u]: %s", docid, buf);

	return SUCCESS;
}

// #define SB4_OP_SET_DOCATTR			"110"
static int sb4c_set_docattr(int sockfd, char *dit)
{
	char buf[STRING_SIZE];
	int len = 0;
	/* 1. send OP code */
	if ( TCPSendData(sockfd, SB4_OP_SET_DOCATTR, 3, FALSE) == FAIL ) {
		error("cannot send OP_REGISTER_DOC(%s)", SB4_OP_REGISTER_DOC);
		return FAIL;
	}

	/* 2. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_CODE"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_CODE"); return FAIL; }

	/* 3. send dit */
	if ( TCPSendData(sockfd, dit, strlen(dit), FALSE) == FAIL ) {
		error("cannot send DIT");
		return FAIL;
	}

	/* 4. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_CODE"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_CODE"); return FAIL; }

	return SUCCESS;
}

static char *get_docattr_field_and_value(char *str, char *field, char *value)
{
	char *ptr, *tmpptr, *ret;

	if (*str == '\0') return NULL;
	ptr = strchr(str, ':');

	if (ptr == NULL) return NULL;
	*ptr = '\0';
	strncpy(field, str, SHORT_STRING_SIZE);
	field[SHORT_STRING_SIZE-1] = '\0';
	ptr++;

	if (*ptr == '\0') return NULL;

	tmpptr = strchr(ptr, '&');
	ret = tmpptr + 1;
	if (tmpptr == NULL) {
		tmpptr = ptr + strlen(ptr);
		ret = tmpptr;
	}
	*tmpptr = '\0';

	strncpy(value, ptr, STRING_SIZE);
	value[STRING_SIZE-1] = '\0';

	return ret;
}

static int sb4s_set_docattr(int sockfd)
{
	char buf[SB4_MAX_SEND_SIZE], oid[STRING_SIZE];
	char attr[STRING_SIZE], field[SHORT_STRING_SIZE], value[STRING_SIZE];
	char *next;
	int n, len;
	DocId docid;
	docattr_mask_t docmask;
	/* 1. recv OP code */
	/* done */

	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. recv DIT */
	if (TCPRecvData(sockfd, buf, &len, TRUE) != SUCCESS) {
		error("cannot recv DIT");
		return FAIL;
	}
	buf[len] = '\0';

	/* parse DIT */
	if (get_str_item(oid, buf, "OID=", '^', STRING_SIZE) == FAIL) {
		goto FAILURE;
	}
	if (oid[0] == '\0') {
		goto FAILURE;
	}

	if (get_str_item(attr, buf, "AT=", '^', STRING_SIZE) == FAIL) {
		goto FAILURE;
	}
	if (attr[0] == '\0') {
		goto FAILURE;
	}

	/* get docid */
	if ((n = sb_run_client_get_docid(oid, &docid)) < 0 &&
			n == DI_NOT_REGISTERED) {
		error("cannot get new docid");
		goto FAILURE;
	}

	/* set docattr db */
	next = attr;
	DOCMASK_SET_ZERO(&docmask);
	while ((next = get_docattr_field_and_value(next, field, value)) != NULL) {
		sb_run_docattr_set_docmask_function(&docmask, field, value);
	}

	/* mask docattr of old ruleno with history bit on */
	sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);

	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	return SUCCESS;

FAILURE:
	send_nak(sockfd, SB4_ERT_RECV_DATA); //XXX: which error code?
	return FAIL;
}

static int sb4c_register_doc(int sockfd, char *dit, char *body, int body_size) 
{
	int i, len, count, left;
	char buf[SB4_MAX_SEND_SIZE+1];
	header_t header;

	if ( strlen(dit) > SB4_MAX_SEND_SIZE ) {
		error("DIT length too long: %d", (int)strlen(dit));
		return FAIL;
	}

	/* 1. send OP code */
	if ( TCPSendData(sockfd, SB4_OP_REGISTER_DOC, 3, FALSE) == FAIL ) {
		error("cannot send OP_REGISTER_DOC(%s)", SB4_OP_REGISTER_DOC);
		return FAIL;
	}

	/* 2. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_CODE"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_CODE"); return FAIL; }

	/* 3. send dit */
	if ( TCPSendData(sockfd, dit, strlen(dit), FALSE) == FAIL ) {
		error("cannot send DIT");
		return FAIL;
	}

	/* 4. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for DIT"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for DIT"); return FAIL; }

	/* 5. send body */
	count = body_size / SB4_MAX_SEND_SIZE;
	left  = body_size % SB4_MAX_SEND_SIZE;
	if ( (left == 0) && (body_size > 0) ) {
		count--;
		left = SB4_MAX_SEND_SIZE;
	}
	
	header.tag = SB4_TAG_CONT;
	sprintf(header.size, "%d", SB4_MAX_SEND_SIZE);
	for ( i = 0; i < count; i++ ) {
		if ( TCPSend(sockfd, &header, body + i*SB4_MAX_SEND_SIZE, FALSE) == FAIL ) {
			error("cannot send %dth body packet", i);
			return FAIL;
		}
	}

	header.tag = SB4_TAG_END;
	sprintf(header.size, "%d", left);
	if ( TCPSend(sockfd, &header, body + body_size - left, FALSE) == FAIL ) {
		error("cannot send last body packet");
		return FAIL;
	}

	/* 6. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for BODY"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for BODY"); return FAIL; }

	/* 7. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, FALSE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}


	return SUCCESS;
}

static void regi_fail_log(char *doc, int doclen)
{
	static char split[STRING_SIZE] = "\n===========================================================\n";
	static char buf[STRING_SIZE];

	sprintf(buf, "%d th fail document\n\n", regifailnum);

	write(regifailfd, split, strlen(split));
	write(regifailfd, buf, strlen(buf));
	write(regifailfd, doc, doclen);
}

void print_docid_oid_log(DocId docid, char *oid)
{

}


/*#define DEBUG_REGISTER_DOC 1*/
static int sb4s_register_doc(int sockfd)
{
	int n=0, len=0, body_size=0, meta_size=0;
	VariableBuffer var_buf, meta_buf;
	char buf[SB4_MAX_SEND_SIZE+1];
	sb4_dit_t sb4_dit;
	header_t header;
	DocId docid = 0;

	// 첨부파일을 필터에 넣기 위해 잠시 사용하는 파일 이름들...
	int fd_filter_in, fd_filter_out;
	int working_len, worked_len, finished_len; // 작업할 양, 작업한 양, 전체 끝난 양
	char* canned_doc_pos;
	int sn2f_result;

#ifdef PROCESS_HANDLE
	static char *canned_doc = NULL;
	static char *org_doc = NULL;
	static char *meta_doc = NULL;
	
	if (canned_doc == NULL) {
		canned_doc = (char *)sb_malloc(DOCUMENT_SIZE);
		if (canned_doc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
	
	
	if (org_doc == NULL) {
		org_doc = (char *)sb_malloc(BIN_DOCUMENT_SIZE);
		if (org_doc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
	
	
	if (meta_doc == NULL) {
		meta_doc = (char *)sb_malloc(DOCUMENT_SIZE);
		if (meta_doc == NULL) {
			crit("out of memory: %s", strerror(errno));
			return FAIL;
		}
	}
#else
	char canned_doc[DOCUMENT_SIZE];
	char org_doc[BIN_DOCUMENT_SIZE];
	char meta_doc[DOCUMENT_SIZE];
#endif

#ifdef DEBUG_REGISTER_DOC
	struct timeval tv1, tv2;
	double diff;

	gettimeofday(&tv1, NULL);
#endif

	setproctitle("softbotd: mod_softbot4.c(%s: registering started)",__FILE__);

	/* send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	n = sb_run_buffer_initbuf(&var_buf); 
	if ( n != SUCCESS ) {
		error("error in buffer_initbuf()");
		return FAIL;
	}

	/* recv DIT */
	n = TCPRecvData(sockfd, buf, &len, TRUE);
	if ( n != SUCCESS ) {
		error("cannot recv DIT");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}
	buf[len] = '\0';

	/* parse DIT */
	memset(&sb4_dit, 0, sizeof(sb4_dit_t));
	n = parse_dit(&sb4_dit, buf);
	if ( n != SUCCESS ) {
		error("cannot parse DIT");
		send_nak(sockfd, SB4_ERD_DIT_FAULT);
		return FAIL;
	}

	if (sb4_dit.OID[0] == '\0') {
		send_nak(sockfd, SB4_ERD_OID_NONE);
		return FAIL;
	}

	/* send OP_ACK for successful receiving of DIT */
	n = TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_ACK);
		return FAIL;
	}
#ifdef DEBUG_REGISTER_DOC
	gettimeofday(&tv2, NULL);
	diff = timediff(&tv2, &tv1);
	debug("[%2.2fsec] received DIT and sent ACK", diff);
#endif

	if ( sb4_dit.PT[0] != '\0' && strcmp(sb4_dit.PT, "BIN") == 0) //첨부 문서 등록시 Metainfo정보 recv
	{
		n = sb_run_buffer_initbuf(&meta_buf); 
		if ( n != SUCCESS ) {
			error("error in buffer_initbuf()");
			return FAIL;
		}
	
		do {
			if ( TCPRecv(sockfd, &header, buf, TRUE) != SUCCESS ) {
				error("error occur while receiving meta_buf");
				send_nak(sockfd, SB4_ERT_RECV_DATA);
				return FAIL;
			}
	
			len = atoi(header.size);

			n = sb_run_buffer_append(&meta_buf, len, buf); 
			if ( n < 0 ) {
				error("out of memory during receiving body[error:%d]", n);
				send_nak(sockfd, SB4_ERS_MEM_LACK);
				sb_run_buffer_freebuf(&meta_buf); 
				return FAIL;
			}
		} while (header.tag == SB4_TAG_CONT);
		
		/* send OP_ACK for successful receiving of metainfo */
		n = TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE);
		if ( n != SUCCESS ) {
			send_nak(sockfd, SB4_ERT_SEND_ACK);
			sb_run_buffer_freebuf(&meta_buf); 
			return FAIL;
		}
	
	}

	/* recv BODY */
	do {
		if ( TCPRecv(sockfd, &header, buf, TRUE) != SUCCESS ) {
			error("error occur while receiving var_buf");
			send_nak(sockfd, SB4_ERT_RECV_DATA);
			return FAIL;
		}

		len = atoi(header.size);

#ifdef DEBUG_REGISTER_DOC
		gettimeofday(&tv2, NULL);
		diff = timediff(&tv2, &tv1);
		debug("[%2.2fsec] receiving body %d bytes", diff, len);
#endif

		n = sb_run_buffer_append(&var_buf, len, buf); 
		if ( n < 0 ) {
			error("out of memory during receiving body[error:%d]", n);
			send_nak(sockfd, SB4_ERS_MEM_LACK);
			sb_run_buffer_freebuf(&var_buf); 
			return FAIL;
		}
		memset(buf, 0, (SB4_MAX_SEND_SIZE+1));
	} while (header.tag == SB4_TAG_CONT);


	/* send OP_ACK for successful receiving of BODY */
	n = TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_ACK);
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}
	
	/* finally send OP_ACK */
	n = TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE);
	if ( n != SUCCESS ) {
		error("cannot send OP_ACK");
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}

	/* below codes is not allowed to send nak op_code,
	 * inspite of error occur
	 */

	/* if protocol is cdm, just get body and insert into cdm directly */
	if (sb4_dit.PT[0] != '\0' && strcmp(sb4_dit.PT, "CDM") == 0) {
		body_size = sb_run_buffer_getsize(&var_buf); 
		if (body_size > DOCUMENT_SIZE-1) {
			error("too big document");
#ifdef DEBUG_SOFTBOTD
			{
				char *tmpbuf = NULL;
				tmpbuf = (char *)sb_malloc(body_size);
				if (tmpbuf != NULL) {
					sb_run_buffer_get(&var_buf, 0, body_size, tmpbuf);
					regi_fail_log(tmpbuf, body_size);
				}
				sb_free(tmpbuf);
			}
#endif
			sb_run_buffer_freebuf(&var_buf);
			return FAIL;
		}
		n = sb_run_buffer_get(&var_buf, 0, body_size, canned_doc); 
		if ( n < 0 ) {
			error("cannot get var_buf from variable buffer");
			sb_run_buffer_freebuf(&var_buf);
			return FAIL;
		}
		canned_doc[body_size] = '\0';
		goto protocol_cdm;
	}

#ifdef DEBUG_REGISTER_DOC
	gettimeofday(&tv2, NULL);
	diff = timediff(&tv2, &tv1);
	debug("[%2.2fsec] received body ", diff);
#endif

	/* 첨부 filter */
	meta_size = sb_run_buffer_getsize(&meta_buf);
	memset(meta_doc, 0x00, strlen(meta_doc));
	n = sb_run_buffer_get(&meta_buf, 0, meta_size, meta_doc); 
	if ( n < 0 ) {
		error("cannot get meta_buf from variable buffer");
		sb_run_buffer_freebuf(&meta_buf);
		return FAIL;
	}
	
	meta_doc[meta_size] = '\0';
	strcpy(canned_doc, "<Document>\n");
	strcat(canned_doc, meta_doc);

	body_size = sb_run_buffer_getsize(&var_buf);
	
	n = sb_run_buffer_get(&var_buf, 0, body_size, org_doc); 
	if ( n < 0 ) {
		error("cannot get var_buf from variable buffer");
		sb_run_buffer_freebuf(&var_buf);
		return FAIL;
	}
		
	strcat(canned_doc, "<Body><![CDATA[");

	if (body_size > 0) {
#define SN2_FILTER_EXEC "bin/sn2f"
		char sn2f_command[MAX_PATH_LEN];
		char filter_path[MAX_PATH_LEN];
		char filter_in_path[MAX_PATH_LEN];
		char filter_out_path[MAX_PATH_LEN];
		char filter_err_path[MAX_PATH_LEN];

		snprintf(filter_path    , MAX_PATH_LEN, "%s/dat/filter"   , gSoftBotRoot);
		snprintf(filter_in_path , MAX_PATH_LEN, "%s/filter.in.%d" , filter_path, getpid());
		snprintf(filter_out_path, MAX_PATH_LEN, "%s/filter.out.%d", filter_path, getpid());
		snprintf(filter_err_path, MAX_PATH_LEN, "%s/filter.err.%d", filter_path, getpid());
		snprintf(sn2f_command   , MAX_PATH_LEN, "%s/%s %s %s",
				 gSoftBotRoot, SN2_FILTER_EXEC, filter_in_path, filter_out_path);
		
		// TODO: dat/filter 디렉토리가 있는지 조사
		mkdir(filter_path, 0755);

		/////////////////////////////////////////////////
		// prepare input file
		fd_filter_in = open(filter_in_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (fd_filter_in < 0) {
			// org_doc은 어디서 free하는 지 모르겠다.
			error("open failed:%s [%s]", filter_in_path, strerror(errno));
			sb_run_buffer_freebuf(&var_buf);
			return FAIL;
		}

		finished_len = 0;
		do {
			working_len = body_size-finished_len;
			if (working_len > 1024) working_len = 1024;

			worked_len = write(fd_filter_in, org_doc+finished_len, working_len);
			if (worked_len < 0) {
				error("write failed:%s [%s]", filter_in_path, strerror(errno));
				sb_run_buffer_freebuf(&var_buf);
				close(fd_filter_in);
				remove(filter_in_path);
				return FAIL;
			}
			finished_len += worked_len;
		} while(finished_len < body_size);
		close(fd_filter_in);

		/////////////////////////////////////////////////
		// run filter
		sn2f_result = system(sn2f_command);
		remove(filter_in_path);
		if (sn2f_result < 0 || !WIFEXITED(sn2f_result) || WEXITSTATUS(sn2f_result) != 0) {
			error("sn2f filter returned %d, exit status %d", sn2f_result, WEXITSTATUS(sn2f_result));
			error("filter failed: %s", sn2f_command);
			sb_run_buffer_freebuf(&var_buf);
			rename(filter_out_path, filter_err_path);
			return FAIL;
		}

		/////////////////////////////////////////////////
		// get output file
		fd_filter_out = open(filter_out_path, O_RDONLY);
		if (fd_filter_out < 0) {
			error("cannot open filter output file[%s]: %s", filter_out_path, strerror(errno));
			sb_run_buffer_freebuf(&var_buf);
			return FAIL;
		}

		finished_len = 0;
		canned_doc_pos = canned_doc+strlen(canned_doc);
		do {
			if ((canned_doc_pos-canned_doc)+1024 > DOCUMENT_SIZE-1)
				working_len = DOCUMENT_SIZE-1-(canned_doc_pos-canned_doc);
			else working_len = 1024;

			if (working_len <= 0) {
				error("attached file is too long: %s", sb4_dit.OID);
				sb_run_buffer_freebuf(&var_buf);
				close(fd_filter_out);
				remove(filter_out_path);
				return FAIL;
			}

			worked_len = read(fd_filter_out, canned_doc_pos, working_len);
			if (worked_len == 0) break; // end of file
			else if (worked_len < 0) {
				error("read failed:%s [%s]", filter_out_path, strerror(errno));
				sb_run_buffer_freebuf(&var_buf);
				close(fd_filter_out);
				remove(filter_out_path);
				return FAIL;
			}

			finished_len += worked_len;
			canned_doc_pos += worked_len;
		} while (1);

		*canned_doc_pos = '\0';
		close(fd_filter_out);
		remove(filter_out_path);
	} // if (body_size > 0)

	strcat(canned_doc, "]]></Body>\n");
	strcat(canned_doc, "</Document>");
	
	len = strlen(canned_doc);
	if (len > DOCUMENT_SIZE-1) {
		error("too big document");
		sb_run_buffer_freebuf(&var_buf);
		return FAIL;
	}
#if 0
	/* make canned document from DIT, BODY */
	sprintf(buf,    "<DocumentName>%s</DocumentName>"
					"<OtherId>%s</OtherId>"
					"<Author>%s</Author>"
					"<Title><![CDATA[%s]]></Title>"
					"<Keyword>%s</Keyword>"
					"<Field1>%s</Field1>"
					"<Field2>%s</Field2>"
					"<Field3>%s</Field3>"
					"<Field4>%s</Field4>"
					"<Field5>%s</Field5>"
					"<Field6>%s</Field6>"
					"<Field7>%s</Field7>"
					"<Field8>%s</Field8>",

					(sb4_dit.DN[0] == '\0')?"":sb4_dit.DN,
					(sb4_dit.OID[0] == '\0')?"":sb4_dit.OID,
					(sb4_dit.AT[0] == '\0')? "":sb4_dit.AT,
					(sb4_dit.TT[0] == '\0')? "":sb4_dit.TT,
					(sb4_dit.KW[0] == '\0')? "":sb4_dit.KW,
					(sb4_dit.FD1[0] == '\0')? "":sb4_dit.FD1,
					(sb4_dit.FD2[0] == '\0')? "":sb4_dit.FD2,
					(sb4_dit.FD3[0] == '\0')? "":sb4_dit.FD3,
					(sb4_dit.FD4[0] == '\0')? "":sb4_dit.FD4,
					(sb4_dit.FD5[0] == '\0')? "":sb4_dit.FD5,
					(sb4_dit.FD6[0] == '\0')? "":sb4_dit.FD6,
					(sb4_dit.FD7[0] == '\0')? "":sb4_dit.FD7,
					(sb4_dit.FD8[0] == '\0')? "":sb4_dit.FD8
	);

	/* get size of BODY */
	body_size = sb_run_buffer_getsize(&var_buf); 
	if (body_size < 0) {
		error("error occur while getting size of var_buf from variable buffer");
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}
	/* memory allocation for canned document */
	len = strlen("<Document>") + strlen(buf) 
			+ strlen("<Body><![CDATA[") + body_size + strlen("]]></Body>")
			+ strlen("</Document>") ;

	if (len > DOCUMENT_SIZE-1) {
		error("too big document");
		sb_run_buffer_freebuf(&var_buf);
		return FAIL;
	}
	if (canned_doc == NULL || len > alloclen) {
		canned_doc = (char *)sb_realloc(canned_doc, len);
		if (canned_doc == NULL) {
			send_nak(sockfd, SB4_ERS_MEM_LACK);
			sb_run_buffer_freebuf(&var_buf); 
			return FAIL;
		}
		alloclen = len;
	}
	/* copy "Document" tag, dit informations */
	strcpy(canned_doc, "<Document>");
	strcat(canned_doc, buf);

	/* copy var_buf */
	strcat(canned_doc, "<Body><![CDATA[");
	len = strlen(canned_doc);

	n = sb_run_buffer_get(&var_buf, 0, body_size, canned_doc + len); 
	if ( n < 0 ) {
		error("cannot get var_buf from variable buffer");
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}
	canned_doc[len + body_size] = '\0';
	strcat(canned_doc, "]]></Body>");

	/* copy "Document" end-tag */
	strcat(canned_doc, "</Document>");
#endif

protocol_cdm:
	
	sb_run_buffer_freebuf(&var_buf); 

	//debug("transformed canned doc(%d):%s", strlen(canned_doc), canned_doc);
	n = sb_run_buffer_append(&var_buf, strlen(canned_doc), canned_doc); 
	if ( n < 0 ) {
		error("out of memory during regiter transformed canned document");
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}
#ifdef DEBUG_REGISTER_DOC
	gettimeofday(&tv2, NULL);
	diff = timediff(&tv2, &tv1);
	debug("[%2.2fsec] canned doc is ready", diff);
#endif

	if (!sb4_dit.OID[0]) {
		warn("empty other id(key of docid)");
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}

#if 0 
	/* the point of creating docid is inserted into cdm */

	/* get document id */
	n = sb_run_client_get_new_docid(sb4_dit.OID, &docid, &olddocid); 
	if ( n < 0 ) {
		error ("cannot get new document id:error(%d)", n);
		sb_run_buffer_freebuf(&var_buf); 
		return FAIL;
	}
	rv_of_get_newid = n;

	/* call canneddoc_put */
	n = sb_run_server_canneddoc_put(docid, &var_buf); 
	sb_run_buffer_freebuf(&var_buf); 
	if ( n < 0 ) {
		error("cannot register canned document[%ld] because of error(%d)", docid, n);

		//XXX: 어째튼 옛날 아이디의 문서는 지워야 한다.
		if (rv_of_get_newid == DI_OLD_REGISTERED) {
			docattr_mask_t docmask;

			DOCMASK_SET_ZERO(&docmask);
			sb_run_docattr_set_docmask_function(&docmask, "Delete", NULL);
			sb_run_docattr_set_array(&olddocid, 1, SC_MASK, &docmask);
		}
		return FAIL;
	}

	print_docid_oid_log(docid, sb4_dit.OID);

#ifdef _NOT_
	if (sb4_dit.RID[0]) {
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		//INFO("RIO: %s", sb4_dit.RID);
		sb_run_docattr_set_docmask_function(&docmask, "Rid", sb4_dit.RID);
		sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
	}
#endif	

	if (rv_of_get_newid == DI_OLD_REGISTERED) {
		docattr_mask_t docmask;

		info("old docid[%ld] of OID[%s] is deleted. new docid is %ld", olddocid,
				sb4_dit.OID, docid);
		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, "Delete", NULL);
		sb_run_docattr_set_array(&olddocid, 1, SC_MASK, &docmask);
	}
#endif

	/************** fixed part *************************/
	
	n = sb_run_server_canneddoc_put_with_oid(sb4_dit.OID, &docid, &var_buf); 
	sb_run_buffer_freebuf(&var_buf); 
	if ( n < 0 ) {
		error("cannot register canned document[%ld] because of error(%d)",
				docid, n);
		return FAIL;
	}
	
	if (sb4_dit.RID[0]) {
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		//INFO("RIO: %s", sb4_dit.RID);
		sb_run_docattr_set_docmask_function(&docmask, "Rid", sb4_dit.RID);
		sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
	}
	/***************************************************/

	if (docid % 1000 == 0) {
		info("%ld documents is registered.", docid);
	}

#ifdef DEBUG_REGISTER_DOC
	gettimeofday(&tv2, NULL);
	diff = timediff(&tv2, &tv1);
	debug("[%2.2fsec] client_canneddoc_put ok", diff);
#endif

#ifdef DEBUG_REGISTER_DOC
	gettimeofday(&tv2, NULL);
	diff = timediff(&tv2, &tv1);
	debug("[%2.2fsec] sent ACK", diff);
#endif
	setproctitle("softbotd: mod_softbot4.c(%s:ended)",__FILE__);

	return SUCCESS;
}

int sb4c_last_docid(int sockfd, DocId *docid)
{
	char buf[STRING_SIZE];
	int len;

	/* 1. Send OP CODE */
	if ( TCPSendData(sockfd, SB4_OP_LAST_DOCID, 3, FALSE) == FAIL ) {
		error("cannot send SB4_OP_LAST_DOCID(%s)", SB4_OP_LAST_DOCID);
		return FAIL;
	}

	/* 2. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for OP_CODE");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0 ) {
		error("received OP_NAK for OP_CODE");
		return FAIL;
	}
	
	/* 3. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for get docid");
		return FAIL;
	}
	
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0 ) {
		error("received OP_NAK for get docid");
		return FAIL;
	}
	

	/* 4. Recv result (docid) */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for last docid");
		return FAIL;
	}
	buf[len] = '\0';
	*docid = atol(buf);

	return SUCCESS;
}

int sb4s_last_docid(int sockfd)
{
	char buf[STRING_SIZE];
	int len, n;
	DocId docid;

	/* 1. receive OP_CODE */
	/* 2. Send ACK NAK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	/* get document id */
	if ((docid = sb_run_server_canneddoc_last_registered_id()) < 0) {
		error("cannot get new docid");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}

	sprintf(buf, "%ld", docid);
	len = strlen(buf);

	/* 3. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 4. Send docid */
	n = TCPSendData(sockfd, buf, len, TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_ACK);
		return FAIL;
	}

	return SUCCESS;
}

int sb4c_delete_oid(int sockfd, char *oid)
{
	char buf[STRING_SIZE];
	int len;

	/* 1. Send OP CODE */
	if ( TCPSendData(sockfd, SB4_OP_DELETE_OID, 3, FALSE) == FAIL ) {
		error("cannot send SB4_OP_DELETE_OID(%s)", SB4_OP_DELETE_OID);
		return FAIL;
	}

	/* 2. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for OP_CODE");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0 ) {
		error("received OP_NAK for OP_CODE");
		return FAIL;
	}

	/* 3. Send Data (oid)	*/
	strncpy(buf, oid, STRING_SIZE-1);
	buf[STRING_SIZE-1] = '\0';
	if ( TCPSendData(sockfd, buf, strlen(buf), FALSE) == FAIL ) {
		error("cannot send OID");
		return FAIL;
	}

	/* 4. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for OID");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0) {
		error("received OP_NAK for OID");
		return FAIL;
	}
	
	/* 5. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for OID to delete");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0) {
		error("received OP_NAK for OID to delete");
		return FAIL;
	}

	return SUCCESS;
}

#define MAX_DOC_SPLIT			64
int sb4s_delete_oid(int sockfd)
{
	char buf[STRING_SIZE];
	char tmp[STRING_SIZE];
	int i, len, n, failed;
	DocId docid;

	/* 1. Recv OP_CODE */
	/* 2. Send ACK NAK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	/* 3. Recv OID */
	n = TCPRecvData(sockfd, buf, &len, TRUE);
	if ( n != SUCCESS ) {
		error("cannot recv OID");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}
	buf[len] = '\0';

	/* get document id */
	warn("del oid = %s", buf);
	if ((n = sb_run_client_get_docid(buf, &docid)) < 0 &&
			n != DI_NOT_REGISTERED) {
		error("cannot get_docid from oid[%s]", buf);
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}

	/* 4. Send OP_ACK for successful receiving of docid */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	/* delte doc of cdm */
#if 0
	n = sb_run_server_canneddoc_delete(docid);
	if ( n < 0 ) {
		return FAIL;
	}
#endif

	/* if no docid */
	if (n == DI_NOT_REGISTERED) {
		failed = 0;
		for (i=1; i<=MAX_DOC_SPLIT; i++) {
			if (failed == 3) break;

			/* generate new docid (oid if document is splitted) */
			sprintf(tmp, "%s-%d", buf, i);

			/* get document id */
			if ((n = sb_run_client_get_docid(tmp, &docid)) > 0) {
				docattr_mask_t docmask;

				DOCMASK_SET_ZERO(&docmask);
				sb_run_docattr_set_docmask_function(&docmask, NULL, NULL);
				sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);

				continue;
			}
			else if (n == DI_NOT_REGISTERED) {
				failed++;
			}
			else {
				error("cannot get new docid");
				return FAIL;
			}
		}
	}
	/* delete mark to docattr */
	else {
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, "Delete", NULL);
		sb_run_docattr_set_array(&docid, 1, SC_MASK, &docmask);
	}

	/* 5. Send ACK NAK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}
	
	return SUCCESS;
}

int sb4c_delete_doc(int sockfd, uint32_t docid)
{
	char buf[SHORT_STRING_SIZE];
	int len;

	/* 1. Send OP CODE */
	if ( TCPSendData(sockfd, SB4_OP_DELETE_DOC, 3, FALSE) == FAIL ) {
		error("cannot send OP_DELETE_DOC(%s)", SB4_OP_DELETE_DOC);
		return FAIL;
	}

	/* 2. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for OP_CODE");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0 ) {
		error("received OP_NAK for OP_CODE");
		return FAIL;
	}

	/* 3. Send Data (docid) */
	sprintf(buf, "DID=%d^", docid);
	if ( TCPSendData(sockfd, buf, strlen(buf), FALSE) == FAIL ) {
		error("cannot send DID to delete");
		return FAIL;
	}

	/* 4. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for Docid");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0) {
		error("received OP_NAK for Docid");
		return FAIL;
	}
	
	/* 5. Recv ACK NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv OP_ACK or OP_NAK for DID to delete");
		return FAIL;
	}
	if ( strncmp(buf, SB4_OP_NAK, 3) == 0) {
		error("received OP_NAK for DID to delete");
		return FAIL;
	}

	return SUCCESS;
}

#if 0
int sb4s_delete_doc(int sockfd)
{
	char buf[SHORT_STRING_SIZE];
	int len, n;
/*	static int docattr_is_opened_already = 0;*/
	uint32_t docid;
/*	doc_mask3_t docattr_mask;*/

	/* 1. Send OP_CODE*/
	/* 2. Send ACK NAK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	/* 3. Recv DID */
	n = TCPRecvData(sockfd, buf, &len);
	if ( n != SUCCESS ) {
		error("cannot recv DID");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}
	buf[len] = '\0';
	docid = (uint32_t)get_int_item(buf, "DID=", '^');

	/* 4. Send OP_ACK for successful receiving of Docid */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}


	/* delte doc of cdm */
#if 0
	n = sb_run_server_canneddoc_delete(docid);
	if ( n < 0 ) {
		return FAIL;
	}
#endif

	/* delete mark to docattr */
	{
		docattr_mask_t docmask;

		DOCMASK_SET_ZERO(&docmask);
		sb_run_docattr_set_docmask_function(&docmask, "Delete", NULL);
		sb_run_docattr_set_array((DocId*)&docid, 1, SC_MASK, &docmask);
	}

	/* 5. Send OP_ACK for successful delete of Docid */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}
	return SUCCESS;
}
#endif

int sb4s_delete_doc(int sockfd)
{
	char buf[LONG_STRING_SIZE];
	int len, n;
	//uint32_t docid;
	int i, j;
	DocId docid[1024], start=0, finish=0, last;
	char *comma, *arg;
	
	/* 1. Send OP_CODE*/
	/* 2. Send ACK NAK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	/* 3. Recv DID */
	n = TCPRecvData(sockfd, buf, &len, TRUE);
	if ( n != SUCCESS ) {
		error("cannot recv DID");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}

	/* 4. Send OP_ACK for successful receiving of Docid */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}

	buf[len] = '\0';
	comma = buf;
	arg = buf;

	last = sb_run_server_canneddoc_last_registered_id();
	
	for (i=0, j=0; arg[i]!='\0'; ) {
		if (arg[i] == ',') {
			if (start) {
				finish = atol(comma);
				for (;start<=finish;start++, j++) {
					if (start > last)
					{
						send_nak_str(sockfd, "not exist docid");
						return FAIL;		
					}
					else
						docid[j] = start;
				}
				start = 0;
				finish = 0;
			}
			else {
				if (atol(comma) > last)
				{
					send_nak_str(sockfd, "not exist docid");
					return FAIL;		
				}	
				else
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
			if (start > last)
			{
				send_nak_str(sockfd, "not exist docid");
				return FAIL;		
			}
			else
				docid[j] = start;
		}
		start = 0;
		finish = 0;
	}
	else {
		if (atol(comma) > last)
		{
			send_nak_str(sockfd, "not exist docid");
			return FAIL;		
		}	
		else
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
		sb_run_docattr_set_docmask_function(&docmask, "Delete", NULL);
		sb_run_docattr_set_array(docid, j, SC_MASK, &docmask);
		//info("end sb_run_docattr_set_array:%d",j);		
	}
	

	/* 5. Send OP_ACK for successful delete of Docid */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) == FAIL ) {
		error("cannot send OP_ACK");
		return FAIL;
	}
	return SUCCESS;
}

static int sb4c_init_search(sb4_search_result_t **result, int list_size)
{
	int i;

	*result = (sb4_search_result_t *)sb_calloc(1, sizeof(sb4_search_result_t));
	if ( *result == NULL ) {
		error("cannot calloc sb4_search_result_t");
		return FAIL;
	}

	(*result)->word_list = (char *)sb_calloc(1, SB4_MAX_SEND_SIZE);
	if ( (*result)->word_list == NULL ) {
		error("cannot calloc char *word_list");
		return FAIL;
	}

	(*result)->results = (char **)sb_calloc(list_size, sizeof(char *));
	if ( (*result)->results == NULL ) {
		error("cannot calloc char **results");
		return FAIL;
	}

	(*result)->allocated = list_size;
	for ( i = 0; i < list_size; i++ ) {
		(*result)->results[i] = (char *)sb_calloc(SB4_MAX_SEND_SIZE, 1);
		if ( (*result)->results[i] == NULL ) {
			error("cannot calloc char *results[%d]", i);
			return FAIL;
		}
	}

	return SUCCESS;
}

static int sb4c_free_search(sb4_search_result_t **result)
{
	int i;

	if ( *result == NULL ) return SUCCESS;

	for ( i = 0; i < (*result)->allocated; i++ ) 
		sb_free((*result)->results[i]);
	sb_free((*result)->results);
	sb_free((*result)->word_list);
	sb_free(*result);

	*result = NULL;

	return SUCCESS;
}

static int sb4c_search_doc (int sockfd, char *query, 
		char *docAttr, int listcount, int page, char *sh,
		sb4_search_result_t *result) 
{
	int i, len;
	char buf[SB4_MAX_SEND_SIZE+1];
	char formatted_query[LONG_STRING_SIZE];

	if ( strlen(query) > SB4_MAX_SEND_SIZE ) {
		error("query string lengh too long: %d", (int)strlen(query));
		return FAIL;
	}

	/* 1. send OP code */
	if ( TCPSendData(sockfd, SB4_OP_SEARCH_DOC, 3, FALSE) == FAIL ) {
		error("cannot send OP_SEARCH_DOC(%s)", SB4_OP_SEARCH_DOC);
		return FAIL;
	}

	/* 2. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for OP_CODE"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for OP_CODE"); return FAIL; }

	/* 3. send query string */
	// XXX: list count & page is temporary
	make_querystr(formatted_query, LONG_STRING_SIZE, 
			query, docAttr, listcount, page, sh);
	if ( TCPSendData(sockfd, formatted_query, strlen(formatted_query), FALSE) 
			== FAIL ) {
		error("cannot send query string");
		return FAIL;
	}
	
	DEBUG("send query [%s]",formatted_query);
	
	/* 4. recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ error("cannot recv OP_ACK or OP_NAK for query string"); return FAIL; }
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ error("received OP_NAK for query string"); return FAIL; }

	/* 5. receive word_list from server */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv word_list");
		return FAIL;
	}
	buf[len] = '\0';
	strncpy(result->word_list, buf, SB4_MAX_SEND_SIZE);
	result->word_list[SB4_MAX_SEND_SIZE-1] = '\0';

	/* 6. receive total list count from server */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv total list count");
		return FAIL;
	}
	buf[len] = '\0';
	result->total_count = atol(buf);

	/* 7. receive list count from server */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
		error("cannot recv list count");
		return FAIL;
	}
	buf[len] = '\0';
	result->received_count = atol(buf);

	DEBUG("received_count:%d",result->received_count);

	/* 8. receive each result from server */
	for ( i = 0; i < result->received_count; i++ ) {
		if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL ) {
			error("cannot recv results[%d]", i);
			return FAIL;
		}
		buf[len] = '\0';
		strncpy(result->results[i], buf, SB4_MAX_SEND_SIZE);
		result->results[i][SB4_MAX_SEND_SIZE-1] = '\0';
	}

	return SUCCESS;
}

static char *replace_newline_to_space(char *str) {
	char *ch;
	ch = str;
	while ( (ch = strchr(ch, '\n')) != NULL ) {
		*ch = ' ';
	}
	return str;
}
#define QUERY_LOG		"logs/query_log"
static int query_log_fd = -1;
static void open_query_log()
{
	char path[STRING_SIZE];
	sb_server_root_relative(path, QUERY_LOG);
	query_log_fd = open(path, O_APPEND | O_CREAT, 0666);
}
static void write_query_log(char *query, char *subquery)
{
	char buf[LONG_STRING_SIZE];
	wr_lock(query_log_fd, SEEK_SET, 0, 0);
	sprintf(buf, "%s\t%s\n", query, subquery);
	write(query_log_fd, buf, strlen(buf));
	un_lock(query_log_fd, SEEK_SET, 0, 0);
}
static int sb4s_search_doc(int sockfd) 
{
	int i, len, error_status, n;
	char buf[SB4_MAX_SEND_SIZE+1];
	request_t req;
	int searched_list_size=0,tmp=0;

	error_status = 0;
	/* 1. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 2. recv query string */
	n = TCPRecvData(sockfd, buf, &len, TRUE);
	if ( n != SUCCESS ) {
		error("cannot recv query string");
		send_nak(sockfd, SB4_ERT_RECV_DATA);
		return FAIL;
	}

    buf[len] = '\0';
    strcat(buf, "^");

    info("request:|%s|",buf);

    req.list_size = get_int_item(buf, "LC=", '^');
    /*if ( req.list_size < 1 || req.list_size > 100 )
        req.list_size = 20;*/
	if ( req.list_size < 1 )
		req.list_size = 20;
    req.first_result = get_long_item(buf, "PG=", '^') * req.list_size;
    if ( req.first_result < 0 )
        req.first_result = 0;
    get_str_item(req.query_string, buf, "QU=", '^', MAX_QUERY_STRING_SIZE);
    get_str_item(req.attr_string, buf, "AT=", '^', MAX_ATTR_STRING_SIZE);
    get_str_item(req.sort_string, buf, "SH=", '^', MAX_SORT_STRING_SIZE);
    req.filtering_id = get_int_item(buf, "FT=", '^');

	DEBUG("req buf size :%d",len);
	DEBUG("req.query_string:[%s]",req.query_string);
	DEBUG("req.attr_string:[%s]",req.attr_string);
	DEBUG("req.sort_string:[%s]",req.sort_string);
	DEBUG("req.list_size:%d",req.list_size);
	DEBUG("req.first_result:%d",req.first_result);
	DEBUG("req.filtering_id:%d", req.filtering_id);

	if (query_log_fd == -1)
		open_query_log();

	if (query_log_fd != -1) {
		write_query_log(req.query_string, req.attr_string);
	}

/*	req.type = LIGHT_SEARCH;*/
	req.type = FULL_SEARCH;
	
/*	sb_run_qp_light_search(&req);*/
	n = sb_run_qp_full_search(&req);
	if (n != SUCCESS) {
		error("full search failed. ret.sb4error:%d", req.sb4error);
		send_nak(sockfd, req.sb4error);
		return FAIL;
	}

	/* 3. send OP_ACK */
	n = TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_ACK);
		sb_run_qp_finalize_search(&req);
		return FAIL;
	}

	/* 4. send word_list to client */
	n = TCPSendData(sockfd, req.word_list, strlen(req.word_list), TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_DATA);
		sb_run_qp_finalize_search(&req);
		return FAIL;
	}

	/* if req.result list is NULL, */
	if (req.result_list == NULL) {
		/* send total list count(0) to client */
		DEBUG("result is NULL");

		sprintf(buf, "0");

		n = TCPSendData(sockfd, buf, strlen(buf), TRUE);
		if ( n != SUCCESS ) {
			send_nak(sockfd, SB4_ERT_SEND_DATA);
			return FAIL;
		}

		/* send list count(0) to client */
		n = TCPSendData(sockfd, buf, strlen(buf), TRUE);
		if ( n != SUCCESS ) {
			send_nak(sockfd, SB4_ERT_SEND_DATA);
			return FAIL;
		}
		
		return SUCCESS;
	}

	/* 5. send total list count to client */
	sprintf(buf, "%d", req.result_list->ndochits);

	n = TCPSendData(sockfd, buf, strlen(buf), TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_DATA);
		sb_run_qp_finalize_search(&req);
		return FAIL;
	}

	/* 6. send list count to client */
	if (req.result_list->ndochits < req.first_result) {
		searched_list_size = 0;
	}
	else {
		tmp = req.result_list->ndochits - req.first_result;

		if (tmp > req.list_size) {
			searched_list_size = req.list_size;
		}
		else {
			searched_list_size = tmp;
		}

		if ( searched_list_size > COMMENT_LIST_SIZE ) {
			warn("searched_list_size[%d] > COMMENT_LIST_SIZE[%d]",
					searched_list_size, COMMENT_LIST_SIZE);
			searched_list_size = COMMENT_LIST_SIZE;
		}
	}

	DEBUG("searched_list_size[%d] req.result_list->ndochits [%d] req.first_result[%d]"
			,searched_list_size , req.result_list->ndochits , req.first_result);
	
	sprintf(buf,"%d",searched_list_size);
	n = TCPSendData(sockfd, buf, strlen(buf), TRUE);
	if ( n != SUCCESS ) {
		send_nak(sockfd, SB4_ERT_SEND_DATA);
		sb_run_qp_finalize_search(&req);
		return FAIL;
	}

	/* 7. send each result to client */
	for (i = 0; i < searched_list_size; i++) {
		/*
		sprintf(buf, "DID=%d^HIT=%d^TT=%s^CMT=%s^OID=%s^",
					req.result_list->doc_hits[i].docid,
					req.result_list->relevancy[i],
					req.titles[i],
					replace_newline_to_space(req.comments[i]),
					req.otherId[i]	
				); */
		sprintf(buf, "DID=%d^HIT=%d^WH=%d^CMT=%s^",
					req.result_list->doc_hits[i].id,
					req.result_list->doc_hits[i].hitratio,
					//req.result_list->relevancy[i],
					req.result_list->doc_hits[i].nhits,
					replace_newline_to_space(req.comments[i])
				); 			
		DEBUG("result[%d]:%s",i,buf);
		
		n = TCPSendData(sockfd, buf, strlen(buf), TRUE);
		if ( n != SUCCESS ) {
			send_nak(sockfd, SB4_ERT_SEND_DATA);
			sb_run_qp_finalize_search(&req);
			return FAIL;
		}
	}

	sb_run_qp_finalize_search(&req);
	return SUCCESS;
}

static int sb4s_dispatch(int sockfd)
{
	int len, n;
	char buf[SB4_MAX_SEND_SIZE+1];

	n =  TCPRecvData(sockfd, buf, &len, TRUE);
	if ( n != SUCCESS ) {
		error("cannot recv opcode");
		return FAIL;
	}

	if ( strncmp(buf, SB4_OP_REGISTER_DOC, 3) == 0 )
		return sb_run_sb4s_register_doc(sockfd);
	else if ( strncmp(buf, SB4_OP_GET_DOC, 3) == 0 )
		return sb_run_sb4s_get_doc(sockfd);
	else if ( strncmp(buf, SB4_OP_SET_DOCATTR, 3) == 0 )
		return sb_run_sb4s_set_docattr(sockfd);
	else if ( strncmp(buf, SB4_OP_DELETE_DOC, 3) == 0 )
		return sb_run_sb4s_delete_doc(sockfd);
	else if ( strncmp(buf, SB4_OP_DELETE_OID, 3) == 0 )
		return sb_run_sb4s_delete_oid(sockfd);
/*	else if ( strncmp(buf, SB4_OP_REGISTER_DOCS, 3) == 0 )*/
/*		return sb_run_sb4s_register_docs(sockfd); */
	else if ( strncmp(buf, SB4_OP_SEARCH_DOC, 3) == 0 )
		return sb_run_sb4s_search_doc(sockfd);
	else if ( strncmp(buf, SB4_OP_STATUS, 3) == 0 )
		return sb_run_sb4s_status(sockfd);
	else if ( strncmp(buf, SB4_OP_LAST_DOCID, 3) == 0 )
		return sb_run_sb4s_last_docid(sockfd);
	else if ( strncmp(buf, SB4_OP_RMA_DOC, 3) == 0 )
		return sb_run_sb4s_remote_morphological_analyze_doc(sockfd);
/* add khyang */
	else if ( strncmp(buf, SB4_OP_STATUS_STR, 3) == 0 )
		return sb_run_sb4s_status_str(sockfd);	
	else if ( strncmp(buf, SB4_OP_LOG, 3) == 0 )
		return sb_run_sb4s_set_log(sockfd);
	else if ( strncmp(buf, SB4_OP_HELP, 3) == 0 )
		return sb_run_sb4s_help(sockfd);
	else if ( strncmp(buf, SB4_OP_RMAS, 3) == 0 )
		return sb_run_sb4s_rmas(sockfd);
	else if ( strncmp(buf, SB4_OP_INDEXWORDS, 3) == 0 )
		return sb_run_sb4s_indexwords(sockfd);
	else if ( strncmp(buf, SB4_OP_TOKENIZER, 3) == 0 )
		return sb_run_sb4s_tokenizer(sockfd);	
	else if ( strncmp(buf, SB4_OP_QPP, 3) == 0 )
		return sb_run_sb4s_qpp(sockfd);
	else if ( strncmp(buf, SB4_OP_GET_CDM_SIZE, 3) == 0 )
		return sb_run_sb4s_get_cdm_size(sockfd);	
	else if ( strncmp(buf, SB4_OP_GET_ABSTRACT, 3) == 0 )
		return sb_run_sb4s_get_abstract(sockfd);
	else if ( strncmp(buf, SB4_OP_GET_FIELD, 3) == 0 )
		return sb_run_sb4s_get_field(sockfd);	
	else if ( strncmp(buf, SB4_OP_UNDEL_DOC, 3) == 0 )
		return sb_run_sb4s_undel_doc(sockfd);
	else if ( strncmp(buf, SB4_OP_GET_WORDID, 3) == 0 )
		return sb_run_sb4s_get_wordid(sockfd);	
	else if ( strncmp(buf, SB4_OP_GET_NEW_WORDID, 3) == 0 )
		return sb_run_sb4s_get_new_wordid(sockfd);	
	else if ( strncmp(buf, SB4_OP_PRINT_HASH_BUCKET, 3) == 0 )
		return sb_run_sb4s_print_hash_bucket(sockfd);	
	else if ( strncmp(buf, SB4_OP_GET_DOCID, 3) == 0 )
		return sb_run_sb4s_get_docid(sockfd);														
	else if ( strncmp(buf, SB4_OP_INDEX_LIST, 3) == 0 )
		return sb_run_sb4s_index_list(sockfd);
	else if ( strncmp(buf, SB4_OP_WORD_LIST, 3) == 0 )
		return sb_run_sb4s_word_list(sockfd);	
	else if ( strncmp(buf, SB4_OP_DELETE_SYSTEM_DOC, 3) == 0 )
		return sb_run_sb4s_del_system_doc(sockfd);	
	else if ( strncmp(buf, SB4_OP_SYSTEMDOC_COUNT, 3) == 0 )
		return sb_run_sb4s_systemdoc_count(sockfd);
			
	else {
		warn("no handler for opcode[%s]", buf);
		if ( TCPSendData(sockfd, SB4_OP_NAK, 3, TRUE) == FAIL ) {
			error("cannot send OP_NAK(%s)", SB4_OP_NAK);
			return FAIL;
		}
	}

	return SUCCESS;
}

static int sb4c_status(int sockfd, char *cmd, FILE *output) 
{
	int len;
	FILE *stream;
	char buf[SB4_MAX_SEND_SIZE+1];

	/* 1. Send OP code */
	if ( TCPSendData(sockfd, SB4_OP_STATUS, 3, FALSE) == FAIL ) {
		error("cannot send OP_STATUS(%s)", SB4_OP_STATUS);
		return FAIL;
	}

	/* 2. Recv ACK or NAK */
	if ( TCPRecvData(sockfd, buf, &len, FALSE) == FAIL )
	{ 
		error("cannot recv OP_ACK or OP_NAK for OP_CODE"); 
		return FAIL; 
	}
	
	buf[len] = '\0';
	if ( strncmp(buf, SB4_OP_ACK, 3) != 0 )
	{ 
		error("received OP_NAK[%s] for OP_CODE", buf); 
		return FAIL; 
	}

	stream = fdopen(sockfd, "r+");
	if ( stream == NULL ) {
		error("fdopen(sockfd, \"r+\"): %s", strerror(errno));
		close(sockfd); fclose(stream);
		return FAIL;
	}
	setlinebuf(stream);

	snprintf(buf, STRING_SIZE, "%s\n", cmd);
	if ( fputs(buf, stream) == EOF ) {
		error("fputs(buf,stream): %s", strerror(errno));
		fclose(stream);
		return FAIL;
	}

	while ( fgets(buf, STRING_SIZE, stream) != NULL ) {
	//	if ( buf[0] == '\n' || buf[0] == '\r' ) break; // FIXME ?? do we need this?
		if ( fputs(buf, output) == EOF ) {
			error("fputs(buf,output): %s", strerror(errno));
			fclose(stream);
			return FAIL;
		}
	}
	
	return SUCCESS;
}


static int sb4s_status(int sockfd)
{
	FILE *stream;
	char *cmd, *arg, buf[SB4_MAX_SEND_SIZE+1];

	/* 1. Recv OP_CODE */
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	stream = fdopen(sockfd, "r+");
	if ( stream == NULL ) {
		error("fdopen: %s", strerror(errno));
		close(sockfd); fclose(stream);
		return FAIL;
	}
	setlinebuf(stream);

	/* read command */
	if ( fgets(buf, STRING_SIZE, stream) == NULL ) {
		error("fgets: %s", strerror(errno));
		fclose(stream);
		return FAIL;
	}
	
	cmd = strtok(buf, " \t\r\n");
	arg = strtok(NULL, " \t\r\n");

	if ( strncasecmp("modules", cmd, STRING_SIZE) == 0 ) {
		list_modules(stream);
	} else if ( strncasecmp("static_modules", cmd, STRING_SIZE) == 0 ) {
		list_static_modules(stream);
	} else if ( strncasecmp("config", cmd, STRING_SIZE) == 0 ) {
		list_config(stream, arg);
	} else if ( strncasecmp("registry", cmd, STRING_SIZE) == 0 ) {
		list_registry(stream, arg);
	} else if ( strncasecmp("save_registry", cmd, STRING_SIZE) == 0 ) {
		save_registry(stdout, arg);
	} else if ( strncasecmp("restore_registry", cmd, STRING_SIZE) == 0 ) {
		restore_registry_file(arg);
	} else if ( strncasecmp("scoreboard", cmd, STRING_SIZE) == 0 ) {
		list_scoreboard(stream, arg);
	} else {
		if ( strncasecmp("help", buf, STRING_SIZE) != 0 )
			fprintf(stream, "unknown option, [%s]\n", cmd);
		fprintf(stream, "usage: status <option>\n");
		fprintf(stream, " static_modules : show static modules\n");
		fprintf(stream, " modules    : show loaded modules\n");
		fprintf(stream, " config [module] : show config table of the module\n");
		fprintf(stream, " registry [module] : show registry table of the module\n");
		fprintf(stream, " scoreboard [module] : show scoreboard of the module\n");
		fprintf(stream, " save_registry [module] : save registry table of the module\n");
		fprintf(stream, " restore_registry [file] : restore registry from the file\n");
		fprintf(stream, " help       : show this message\n");
	}

	return SUCCESS;
}


static int sb4s_status_str(int sockfd)
{
	char *cmd, *arg, buf[SB4_MAX_SEND_SIZE+1], szSize[SB4_MAX_SEND_SIZE+1];
	int len, send_size;
	char send_data[20000];
	
	/* 1. Recv OP_CODE */
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive command */
	memset(buf, 0x00, SB4_MAX_SEND_SIZE+1);
	if ( TCPRecvData(sockfd, buf,&len, TRUE) == FAIL ) {
		error("cannot recv command");
		return FAIL;
	}
	
	
	cmd = strtok(buf, " \t\r\n");
	arg = strtok(NULL, " \t\r\n");

	if ( strncasecmp("modules", cmd, STRING_SIZE) == 0 ) {
		list_modules_str(send_data);
	} else if ( strncasecmp("static_modules", cmd, STRING_SIZE) == 0 ) {
		list_static_modules_str(send_data);
	} else if ( strncasecmp("config", cmd, STRING_SIZE) == 0 ) {
		list_config_str(send_data, arg);
	} else if ( strncasecmp("registry", cmd, STRING_SIZE) == 0 ) {
		list_registry_str(send_data, arg);
	} else if ( strncasecmp("save_registry", cmd, STRING_SIZE) == 0 ) {
		save_registry_str(send_data, arg);
	} else if ( strncasecmp("restore_registry", cmd, STRING_SIZE) == 0 ) {
		restore_registry_file(arg);
	} else if ( strncasecmp("scoreboard", cmd, STRING_SIZE) == 0 ) {
		list_scoreboard_str(send_data, arg);
	} else {
		
		if ( strncasecmp("help", buf, STRING_SIZE) != 0 )
			sprintf(send_data, "unknown option, [%s]\n", cmd);
		
		strcat(send_data, "usage: status <option>\n");
		
		strcat(send_data, " static_modules : show static modules\n");
		strcat(send_data, " modules    : show loaded modules\n");
		strcat(send_data, " config [module] : show config table of the module\n");
		strcat(send_data, " registry [module] : show registry table of the module\n");
		strcat(send_data, " scoreboard [module] : show scoreboard of the module\n");
		strcat(send_data, " save_registry [module] : save registry table of the module\n");
		strcat(send_data, " restore_registry [file] : restore registry from the file\n");
		strcat(send_data, " help       : show this message\n");
			
	}
	
	send_size = strlen(send_data);
	sprintf(szSize, "%d", send_size);
	

	/* 4. send data size*/
	if ( TCPSendData(sockfd, szSize, strlen(szSize), TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 5. send data */
	if ( TCPSendLongData(sockfd, send_size, send_data, TRUE) == FAIL ) {
		error("cannot send Status data");
		return FAIL;
	}

	/* 6. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	return SUCCESS;
}


int sb4s_set_log(int sockfd)
{
	int len;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive command */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv command");
		return FAIL;
	}
	tmpbuf[len] = '\0';
	
	
	/* set Log */
	log_setlevelstr(tmpbuf);

	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	return SUCCESS;
}

int sb4s_help(int sockfd)
{
	int send_size, nRet;
	char send_data[20000];
	char szSize[1024];

	/* 1. receive OP_CODE */	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* get help */
	memset(send_data, 0x00, sizeof(send_data));
	
	nRet = sb4_com_help(send_data);
		
	send_size = strlen(send_data);
	sprintf(szSize, "%d", send_size);
	

	/* 3. send data size*/
	if ( TCPSendData(sockfd, szSize, strlen(szSize), TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 4. send data */
	if ( TCPSendLongData(sockfd, send_size, send_data, TRUE) == FAIL ) {
		error("cannot send help data");
		return FAIL;
	}

	/* 5. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	return SUCCESS;
}

int sb4s_rmas(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive command (fieldid, morpid, string)*/
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv command");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get rmas */
	nRet = sb4_com_rmas_run(sockfd, tmpbuf);

	return SUCCESS;
}


int sb4s_indexwords(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive command(morpid, string) */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv command");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get indexwords */
	nRet = sb4_com_index_word_extractor(sockfd, tmpbuf);

	return SUCCESS;
	
}

int sb4s_tokenizer(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive command(string) */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv string");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get indexwords */
	nRet = sb4_com_tokenizer(sockfd, tmpbuf);

	return SUCCESS;	
	
}

int sb4s_qpp(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive Query */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv query");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get qpp */
	nRet = sb4_com_qpp(sockfd, tmpbuf);

	return SUCCESS;	
}


#if 0
int sb4s_show_spool(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive spool path, docid */
	if ( TCPRecvData(sockfd, tmpbuf, &len) == FAIL ) {
		error("cannot recv spool path, docid");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* show spool */
	nRet = sb4_com_showspool(sockfd, tmpbuf);

	return SUCCESS;	
}
#endif

int sb4s_get_cdm_size(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive docid */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv docid");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get cdm size */
	nRet = sb4_com_get_doc_size(sockfd, tmpbuf);

	return SUCCESS;	
}

int sb4s_get_abstract(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive docid, field명 */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv docid, field명");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get abstract */
	nRet = sb4_com_get_abstracted_doc(sockfd, tmpbuf);

	return SUCCESS;	
	
}

int sb4s_get_field(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive docid, field명 */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv docid, field명");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get field */
	nRet = sb4_com_get_field(sockfd, tmpbuf);

	return SUCCESS;		
}

int sb4s_undel_doc(int sockfd)
{
	int len, nRet;
	char tmpbuf[LONG_STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive docid */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv docid");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* undel doc */
	nRet = sb4_com_undelete(sockfd, tmpbuf);
	
	return SUCCESS;		
}

int sb4s_get_wordid(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive word */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv word");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get wordid */
	nRet = sb4_com_get_wordid(sockfd, tmpbuf);
	
	return SUCCESS;	
	
}

int sb4s_get_new_wordid(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive word, docid */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv word, docid");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get new wordid */
	nRet = sb4_com_get_new_wordid(sockfd, tmpbuf);
	
	return SUCCESS;	
}

int sb4s_print_hash_bucket(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive bucket_idx */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv bucket_idx");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get hash bucket */
	nRet = sb4_com_print_hash_bucket(sockfd, tmpbuf);
	
	return SUCCESS;	
}

int sb4s_get_docid(int sockfd)	
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive oid */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv oid");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* getdodid */
	nRet = sb4_com_get_docid(sockfd, tmpbuf);
	
	return SUCCESS;	
}

int sb4s_index_list(int sockfd)
{
	int len, nRet, nCnt;
	char tmpbuf[STRING_SIZE];
	char count[STRING_SIZE];
	char tmpbuf1[SB4_MAX_SEND_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive list Query(LC, PG) */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv list Query");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 5. receive Field count*/
	if ( TCPRecvData(sockfd, count, &len, TRUE) == FAIL ) {
		error("cannot recv list Field count");
		return FAIL;
	}
	count[len] = '\0';
	nCnt = atoi(count);
	
	/* 6. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* 7. receive Info Field */
	if ( TCPRecvData(sockfd, tmpbuf1, &len, TRUE) == FAIL ) {
		error("cannot recv list Info Field");
		return FAIL;
	}
	tmpbuf1[len] = '\0';
	
	/* 8. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get index list */
	nRet = sb4_com_index_list(sockfd, tmpbuf, tmpbuf1, nCnt);
	
	return SUCCESS;	
}



int sb4s_word_list(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive word list Query(LC, PG) */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv list Query");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get word list */
	nRet = sb4_com_get_word_by_wordid(sockfd, tmpbuf);
	
	return SUCCESS;	
}

int sb4s_systemdoc_count(int sockfd)
{
	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get doc count */
	return sb4_com_doc_count(sockfd);
}


int sb4s_del_system_doc(int sockfd)
{
	int len, nRet;
	char tmpbuf[STRING_SIZE];

	/* 1. receive OP_CODE */
	// done
	
	/* 2. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}

	/* 3. receive System NO */
	if ( TCPRecvData(sockfd, tmpbuf, &len, TRUE) == FAIL ) {
		error("cannot recv System No");
		return FAIL;
	}
	tmpbuf[len] = '\0';

	 
	/* 4. send ACK */
	if ( TCPSendData(sockfd, SB4_OP_ACK, 3, TRUE) != SUCCESS ) {
		error("cannot send ACK");
		return FAIL;
	}
	
	/* get word list */
	nRet = sb4_com_del_system_doc(sockfd, tmpbuf);
	
	return SUCCESS;	
}

/****************************************************************************/
static int send_nak(int sockfd, int error_code) // used only server side
{
	int ret;
	char buf[12];

	ret = TCPSendData(sockfd, SB4_OP_NAK, 3, TRUE);
	if( ret != SUCCESS ) return FAIL;

	sprintf(buf, "%d", error_code);

	ret = TCPSendData(sockfd, buf, strlen(buf), TRUE);
	if ( ret != SUCCESS ) return FAIL;

	return SUCCESS;
}

int send_nak_str(int sockfd, char *error_string) // used only server side
{
	int ret;

	ret = TCPSendData(sockfd, SB4_OP_NAK, 3, TRUE);
	if( ret != SUCCESS ) return FAIL;

	ret = TCPSendData(sockfd, error_string, strlen(error_string), TRUE);
	if ( ret != SUCCESS ) return FAIL;

	return SUCCESS;
}


// FIXME: only simplest implementation
//        need fixing!!
static void make_querystr (char *formatted, int formatted_size, 
		char *query, char *docAttr, int listcount, int page, char *sh)
{
	char buf[STRING_SIZE];
	int left;

	warn("formatting query string is temporarily implemented!!");
	warn("please fix me!");

	if (docAttr) {
		snprintf(formatted, formatted_size, "LC=%d^PG=%d^QU=%s^AT=%s^", 
											listcount, page, query, docAttr);
	}
	else {
		snprintf(formatted, formatted_size, "LC=%d^PG=%d^QU=%s^AT=^", 
											listcount, page, query);
	}

	if (sh) {
		snprintf(buf, STRING_SIZE, "SH=%s^", sh);
		left = formatted_size - strlen(formatted);
		strncat(formatted, buf, left > 0 ? left : 0);
	}

/*
	DEBUG("LC=%d^PG=%d^QU=%s^AT=%s^SH=%s^", 
			listcount , page , query, docAttr, sh);
	DEBUG("formatted:%s",formatted);
*/
}

static int parse_dit(sb4_dit_t *sb4_dit, char *dit)
{
	if ( dit == NULL ) return FAIL;
	if ( sb4_dit == NULL ) return FAIL;

/*	sb4_dit->HIT = get_int_item(dit, "HIT=", '^', 12);*/
/*	sb4_dit->DID = get_int_item(dit, "DID=", '^', 12);*/
/*	sb4_dit->DIC = get_int_item(dit, "HIC=", '^', 12);*/
/*	sb4_dit->DAC = get_int_item(dit, "DAC=", '^', 12);*/

	sb4_dit->HIT = get_int_item(dit, "HIT=", '^');
	sb4_dit->DID = get_int_item(dit, "DID=", '^');

	get_str_item(sb4_dit->PT, dit, "PT=", '^', SB4_MAX_PT_LEN);
	get_str_item(sb4_dit->CT, dit, "CT=", '^', SB4_MAX_CT_LEN);
	get_str_item(sb4_dit->DTR, dit, "DTR=", '^', SB4_MAX_CT_LEN);
	get_str_item(sb4_dit->DTC, dit, "DTC=", '^', SB4_MAX_DT_LEN);
	get_str_item(sb4_dit->DN, dit, "DN=", '^', SB4_MAX_DOC_NM);
	get_str_item(sb4_dit->OID, dit, "OID=", '^', SB4_MAX_OID);
	get_str_item(sb4_dit->RID, dit, "RID=", '^', SB4_MAX_OID);
	get_str_item(sb4_dit->DSC, dit, "DSC=", '^', SB4_MAX_DSC_LEN);
	get_str_item(sb4_dit->TT, dit, "TT=", '^', SB4_MAX_DOC_TT);
	get_str_item(sb4_dit->AT, dit, "AT=", '^', SB4_MAX_DOC_AT);
	get_str_item(sb4_dit->KW, dit, "KW=", '^', SB4_MAX_DOC_KW);
	get_str_item(sb4_dit->FD1, dit, "FD1=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD2, dit, "FD2=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD3, dit, "FD3=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD4, dit, "FD4=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD5, dit, "FD5=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD6, dit, "FD6=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD7, dit, "FD7=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->FD8, dit, "FD8=", '^', SB4_MAX_DOC_FD);
	get_str_item(sb4_dit->SUM, dit, "SUM=", '^', SB4_MAX_DOC_TT);

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

// XXX: no need to pass len ?? 05/22 --jiwon
/*static int get_int_item(char *dit, char *key, char delimiter, int len)*/
static int get_int_item(char *dit, char *key, char delimiter)
{
	char *start, *end, restore;
	int val=0;

/*	DEBUG("dit: [%s]",dit);*/
/*	DEBUG("key: [%s]",key);*/
/*	DEBUG("delim: [%c]",delimiter);*/

	start = strstr(dit, key);
	if ( start == NULL ) {
/*		DEBUG("start is NULL");*/
		return FAIL;
	}
	start += strlen(key);

	end = strchr(start, delimiter);
	if ( end == NULL ) {
/*		DEBUG("end is NULL");*/
		return FAIL;
	}

	restore = *end;
	*end = '\0';

	val = atoi(start);

	*end = restore;

	return val;
}

static int get_long_item(char *dit, char *key, char delimiter)
{
	char *start, *end, restore;
	long val=0L;

	start = strstr(dit, key);
	if ( start == NULL ) return FAIL;
	start += strlen(key);

	end = strchr(start, delimiter);
	if ( end == NULL ) return FAIL;

	restore = *end;
	*end = '\0';

	val = atol(start);

/*	DEBUG("long item:%ld",val);*/
	
	*end = restore;

	return val;
}
/****************************************************************************/
/* SoftBot4 lb_tcp.c */

int TCPSendData(int sockfd, void *data, int len, int server_side)
{
	header_t header;

	if ( len > SB4_MAX_SEND_SIZE ) return FAIL;

	header.tag = SB4_TAG_END;
	sprintf(header.size, "%d", len);

	return TCPSend(sockfd, &header, data, server_side);
}	

int TCPRecvData(int sockfd, void *data, int *len, int server_side)
{
	header_t header;

	if ( TCPRecv(sockfd, &header, data, server_side) == FAIL ) return FAIL;

	*len = atoi(header.size);
	return SUCCESS;
}

int TCPSend(int sockfd, header_t *header, void *data, int server_side)
{
	int timeout;

	if ( server_side == TRUE ) timeout = sb_run_tcp_server_timeout();
	else timeout = sb_run_tcp_client_timeout();

	if ( sb_run_tcp_send(sockfd, header, sizeof(header_t), timeout) != SUCCESS ) {
		warn("cannot send header");
		return FAIL;
	}
	if ( sb_run_tcp_send(sockfd, data, atoi(header->size), timeout) != SUCCESS ) {
		warn("cannot send header");
		return FAIL;
	}
	return SUCCESS;
}

int TCPRecv(int sockfd, header_t *header, void *data, int server_side)
{
	int timeout;

	if ( server_side == TRUE ) timeout = sb_run_tcp_server_timeout();
	else timeout = sb_run_tcp_client_timeout();

	if ( sb_run_tcp_recv(sockfd, header, sizeof(header_t), timeout) != SUCCESS ) {
		warn("cannot recv header");
		return FAIL;
	}
	if ( sb_run_tcp_recv(sockfd, data, atoi(header->size), timeout) != SUCCESS ) {
		warn("cannot recv content");
		return FAIL;
	}
	return SUCCESS;
}

int TCPSendLongData(int sockfd, long size, void *data, int server_side)
{
	int timeout;

	if ( server_side == TRUE ) timeout = sb_run_tcp_server_timeout();
	else timeout = sb_run_tcp_client_timeout();

	if ( sb_run_tcp_send(sockfd, data, size, timeout) != SUCCESS ) {
		warn("cannot send header : RECVLONGDATA");
		return FAIL;
	}
	return SUCCESS;
}

int TCPRecvLongData(int sockfd, long size , void *data, int server_side)
{
	int timeout;

	if ( server_side == TRUE ) timeout = sb_run_tcp_server_timeout();
	else timeout = sb_run_tcp_client_timeout();

	if ( sb_run_tcp_recv(sockfd, data, size, timeout) != SUCCESS ) {
		warn("cannot recv content : RECVLONGDATA");
		return FAIL;
	}
	return SUCCESS;
}


/*****************************************************************************/
static int init(void)
{
	assign_word_db();

	/* open file */
	regifailfd = sb_open(regifail_file_path, O_RDWR | O_CREAT | O_APPEND, 0666);
	if (regifailfd == -1) {
		error("cannot open file[%s]", regifail_file_path);
		return FAIL;
	}
	return SUCCESS;
}

static void register_hooks(void)
{
	// FIXME obsolete
/*	sb_hook_sb4c_connect(sb4c_connect,NULL,NULL,HOOK_MIDDLE);*/
/*	sb_hook_sb4c_close(sb4c_close,NULL,NULL,HOOK_MIDDLE);*/

	sb_hook_sb4c_register_doc(sb4c_register_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_get_doc(sb4c_get_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_set_docattr(sb4c_set_docattr,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_delete_doc(sb4c_delete_doc,NULL,NULL,HOOK_MIDDLE);

	sb_hook_sb4c_init_search(sb4c_init_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_free_search(sb4c_free_search,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_search_doc(sb4c_search_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4c_last_docid(sb4c_last_docid,NULL,NULL,HOOK_MIDDLE);

	sb_hook_sb4s_dispatch(sb4s_dispatch,NULL,NULL,HOOK_MIDDLE);

	sb_hook_sb4s_register_doc(sb4s_register_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_get_doc(sb4s_get_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_set_docattr(sb4s_set_docattr,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_delete_doc(sb4s_delete_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_delete_oid(sb4s_delete_oid,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_search_doc(sb4s_search_doc,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_last_docid(sb4s_last_docid,NULL,NULL,HOOK_MIDDLE);

	sb_hook_sb4c_status(sb4c_status,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_status(sb4s_status,NULL,NULL,HOOK_MIDDLE);

	sb_hook_sb4c_remote_morphological_analyze_doc(sb4c_remote_morphological_analyze_doc, NULL , NULL , HOOK_MIDDLE);
	sb_hook_sb4s_remote_morphological_analyze_doc(sb4s_remote_morphological_analyze_doc, NULL , NULL , HOOK_MIDDLE);
	
	/* add khyang */
	sb_hook_sb4s_status_str(sb4s_status_str,NULL,NULL,HOOK_MIDDLE);
	sb_hook_sb4s_set_log(sb4s_set_log, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_help(sb4s_help, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_rmas(sb4s_rmas, NULL, NULL, HOOK_MIDDLE);
	
	sb_hook_sb4s_indexwords(sb4s_indexwords, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_tokenizer(sb4s_tokenizer, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_qpp(sb4s_qpp, NULL, NULL, HOOK_MIDDLE);
	
	sb_hook_sb4s_get_cdm_size(sb4s_get_cdm_size, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_get_abstract(sb4s_get_abstract, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_get_field(sb4s_get_field, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_undel_doc(sb4s_undel_doc, NULL, NULL, HOOK_MIDDLE);
	//sb_hook_sb4s_config(sb4s_config, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_get_wordid(sb4s_get_wordid, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_get_new_wordid(sb4s_get_new_wordid, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_print_hash_bucket(sb4s_print_hash_bucket, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_get_docid(sb4s_get_docid, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_index_list(sb4s_index_list, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_word_list(sb4s_word_list, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_del_system_doc(sb4s_del_system_doc, NULL, NULL, HOOK_MIDDLE);
	sb_hook_sb4s_systemdoc_count(sb4s_systemdoc_count, NULL, NULL, HOOK_MIDDLE);	
	
}



module protocol4_module = {
	STANDARD_MODULE_STUFF,
	NULL,					/* config */
	NULL,					/* registry */
	init,					/* initialize */
	NULL,					/* child_main */
	NULL,					/* scoreboard */
	register_hooks			/* register hook api */
};
