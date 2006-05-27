/* $Id */
#include <string.h> /* strncpy */
#include <stdlib.h> /* atoi */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "common_core.h"
#include "common_util.h"
#include "memory.h"
#include "ipc.h"
#include "memfile.h"

#include "mod_api/cdm2.h"
#include "mod_api/did.h"
#include "mod_api/docattr.h"
#include "mod_api/xmlparser.h"

#include "mod_cdm2.h"
#include "mod_cdm2_lock.h"
#include "mod_cdm2_file.h"
#include "mod_cdm2_docattr.h"
#include "mod_cdm2_util.h"
#include "cannedDocServer.h"

/* cdm 문서 header */
#define HEADER_SHORTFIELD_SIZE 11
#define HEADER_LONGFIELD_HEADERS_SIZE 11
#define HEADER_SIZE (HEADER_SHORTFIELD_SIZE+1+\
                     HEADER_LONGFIELD_HEADERS_SIZE+1)

static intptr_t aligned_offset = -1;

static cdm_db_set_t* cdm_set = NULL;
static int current_cdm_set = -1;

// open을 singleton으로 구현하기 위한 것
static cdm_db_t* singleton_cdm_db[MAX_CDM_SET];
static int singleton_cdm_db_ref[MAX_CDM_SET];

/////////////////////////////////////////////////////////////
// for config Field, DocAttrField, FieldRootName
struct cdmfield_t fields[MAX_FIELD_NUM];
int field_count = 0;

char docattr_fields[MAX_FIELD_NUM][SHORT_STRING_SIZE];
int docattr_field_count = 0;

char field_root_name[SHORT_STRING_SIZE] = "Document";
/////////////////////////////////////////////////////////////

static uint32_t cdm_last_docid(cdm_db_t* cdm_db);
static field_t* get_xml_field(parser_t* p, const char* field_name, const char* oid);

static struct cdmfield_t* find_cdmfield(const char* name);
static int find_field_from_doc(cdm_doc_custom_t* doc_custom, const char* name);

static int alloc_shared_cdm_db(cdm_db_custom_t* cdm_db, char* shared_file, int *mmap_attr)
{
	ipc_t mmap;

	mmap.type = IPC_TYPE_MMAP;
	mmap.pathname = shared_file;
	mmap.size = sizeof(cdm_db_shared_t);

	if ( alloc_mmap(&mmap, 0) != SUCCESS ) {
		crit("error while allocating mmap for cdm_db");
		return FAIL;
	}

	if ( mmap.attr == MMAP_CREATED ) memset( mmap.addr, 0, mmap.size );

	cdm_db->shared = mmap.addr;
	*mmap_attr = mmap.attr;

	debug("cdm_db->shared [%p]", cdm_db->shared);

	return SUCCESS;
}

static int free_shared_cdm_db(cdm_db_custom_t* cdm_db)
{
	int ret;

	ret = free_mmap(cdm_db->shared, sizeof(cdm_db_shared_t));
	if ( ret == SUCCESS ) cdm_db->shared = NULL;

	return ret;
}

static int cdm_open(cdm_db_t** cdm_db, int opt)
{
	int mmap_attr;
	cdm_db_custom_t* db = NULL;

	if ( cdm_set == NULL ) {
		warn("cdm_set is NULL. you must set CdmSet in config file");
		return DECLINE;
	}

	if ( opt >= MAX_CDM_SET || opt < 0 ) {
		error("opt[%d] is invalid. MAX_CDM_SET[%d]", opt, MAX_CDM_SET);
		return FAIL;
	}

	if ( !cdm_set[opt].set ) {
		warn("CdmSet[opt:%d] is not defined", opt);
		return DECLINE;
	}

	// 다른 module에서 이미 열었던 건데.. 그것을 return한다.
	if ( singleton_cdm_db[opt] != NULL ) {
		*cdm_db = singleton_cdm_db[opt];
		singleton_cdm_db_ref[opt]++;

		info("reopened cdm db[set:%d, ref:%d]", opt, singleton_cdm_db_ref[opt]);
		return SUCCESS;
	}
/*
	if ( !cdm_set[opt].set_did_set ) {
		error("DidSet is not set [CdmSet:%d]. see config", opt);
		return FAIL;
	}
*/
	*cdm_db = (cdm_db_t*) sb_calloc(1, sizeof(cdm_db_t));
	if ( *cdm_db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}

	db = (cdm_db_custom_t*) sb_calloc(1, sizeof(cdm_db_custom_t));
	if ( db == NULL ) {
		error("sb_calloc failed: %s", strerror(errno));
		goto error;
	}
/*
	if ( sb_run_open_did_db( &db->did_db, cdm_set[opt].did_set ) != SUCCESS ) {
		error("did db open failed: %s", strerror(errno));
		goto error;
	}
*/
	if ( alloc_shared_cdm_db( db, cdm_set[opt].shared_file, &mmap_attr ) != SUCCESS ) {
		error("alloc_shared_cdm_db failed");
		goto error;
	}

	copy_string( db->cdm_path, cdm_set[opt].cdm_path, MAX_PATH_LEN );
	copy_string( db->shared_file, cdm_set[opt].shared_file, MAX_PATH_LEN );
	db->cdm_file_size = cdm_set[opt].cdm_file_size;
	db->max_doc_num = cdm_set[opt].max_doc_num;
	db->locked = 0;

	if ( open_index_file( db ) != SUCCESS ) {
		error("open cdm index file failed: %s", strerror(errno));
		goto error;
	}
	if ( open_cdm_file( db, db->shared->db_no ) != SUCCESS ) {
		error("open last cdm db file failed: %s", strerror(errno));
		goto error;
	}

	(*cdm_db)->set = opt;
	(*cdm_db)->db = (void*) db;

	singleton_cdm_db[opt] = *cdm_db;
	singleton_cdm_db_ref[opt] = 1;

	return SUCCESS;

error:
	if ( db ) {
		if ( db->shared ) free_shared_cdm_db( db );
		//if ( db->did_db ) sb_run_close_did_db( db->did_db );

		sb_free( db );
	}
	if ( *cdm_db ) {
		sb_free( *cdm_db );
		*cdm_db = NULL;
	}

	return FAIL;
}

static int cdm_close(cdm_db_t* cdm_db)
{
	cdm_db_custom_t* db;
	int i, set;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;
	set = cdm_db->set;

	// 아직 reference count가 남아있으면 close하지 말아야 한다.
	singleton_cdm_db_ref[set]--;
	if ( singleton_cdm_db_ref[set] ) {
		info("cdm db[set:%d, ref:%d] is not closing now",
				set, singleton_cdm_db_ref[set]);
		return SUCCESS;
	}

	if ( free_shared_cdm_db( db ) != SUCCESS ) {
		warn("free shared cdm db failed");
	}
	/*if ( sb_run_close_did_db( db->did_db ) != SUCCESS ) {
		warn("close did db failed");
	}*/

	for ( i = 0; i < MAX_CDM_FILE_COUNT; i++ ) {
		close_cdm_file(db, i);
	}
	close_index_file(db);

	sb_free( cdm_db->db );
	sb_free( cdm_db );

	singleton_cdm_db[set] = NULL;

	return SUCCESS;
}

static int cdm_get_doc(cdm_db_t* cdm_db, uint32_t docid, cdm_doc_t** doc)
{
	cdm_db_custom_t* db;
	cdm_doc_custom_t* doc_custom;

	int deleted, ret;
	char _buf[HEADER_SIZE+1];
	char* buf = NULL;
	int data_size, shortfields_length, longfield_headers_length;
	int current_pos, current_field;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;

	if ( docid > db->shared->last_docid ) return CDM2_GET_INVALID_DOCID;

	/****************************************/
	/* docattr 들여다보고 삭제문서인지 조사 */
	if ( is_deleted(docid, &deleted) != SUCCESS ) return FAIL;
	/****************************************/

	// doc, doc_custom 을 한꺼번에 alloc
	*doc = (cdm_doc_t*) big_calloc(aligned_offset+sizeof(cdm_doc_custom_t));
	(*doc)->cdm_db = cdm_db;
	(*doc)->docid = docid;
	(*doc)->deleted = deleted;
	(*doc)->data = (void*) ((intptr_t)*doc + aligned_offset);

	doc_custom = (cdm_doc_custom_t*) (*doc)->data;

	/******************************************/
	/* index file 들여다 보고 cdm내 위치 조사 */
	/* doc_custom->dwDBNo,offset,length       */
	if ( read_index(db, doc_custom, docid) != SUCCESS ) goto error;
	/******************************************/

	/********************************/
	/* cdm 파일에서 header내용 읽음 */

	if ( open_cdm_file(db, doc_custom->dwDBNo) != SUCCESS ) goto error;

	/*
	 * see wiki [mod_cdm2]
	 * |-------- 11 --------|--|------------ 11 ----------|--| header
	 * | shortfield length  |\t| longfield_headers length |\n|
	 * +-----------------------------------------------------+
	 * | ...                                            |
	 */
	INDEX_RD_LOCK(db, goto error);
	ret = read_cdm(db, doc_custom, 0, _buf, HEADER_SIZE);

	if ( ret != SUCCESS ) {
		error("invalid document size [under header size]");
		INDEX_UN_LOCK(db, );
		goto error;
	}

	shortfields_length = atoi(_buf);
	longfield_headers_length = atoi(_buf+HEADER_SHORTFIELD_SIZE);
	/********************************/

	/***************************************/
	/* data부분 추가로 읽자                */
	data_size = shortfields_length+longfield_headers_length;

	buf = big_calloc(data_size);
	ret = read_cdm(db, doc_custom, HEADER_SIZE, buf, data_size);
	INDEX_UN_LOCK(db, );

	if ( ret != SUCCESS ) {
		error("cannot read cdm[%"PRIu32"]: %s", docid, strerror(errno));
		goto error;
	}

	doc_custom->longfield_bodies_start_offset = HEADER_SIZE+data_size;
	doc_custom->data = buf;
	/***************************************/

	/************************************/
	/* shortfield 읽기                  */
	/*                                  */
	/* Title \t 제목이다 \0\0\0         */
	/* Date \t 20060201 \0              */
	/*                                  */
	current_pos = 0;
	current_field = 0;
	while ( current_pos < shortfields_length ) {
		int start_pos;

		doc_custom->shortfield_names[current_field] = buf;

		buf = pass_til(buf, '\t', &current_pos, shortfields_length);
		*buf = '\0';
		buf++; current_pos++;
		doc_custom->shortfield_values[current_field] = buf;

		start_pos = current_pos;

		buf = pass_til(buf, '\0', &current_pos, shortfields_length);
		buf = pass_not(buf, '\0', &current_pos, shortfields_length);
		doc_custom->shortfield_size[current_field] = current_pos - start_pos - 1;

		current_field++;
	}
	doc_custom->shortfield_count = current_field;
	/************************************/

	/******************************/
	/* longfield header 읽기      */
	/*                            */
	/* Body \t 32 \t 155 \0       */
	/* Contents \t 183 \t 232 \0  */
	/*                            */
	current_pos = 0;
	current_field = 0;
	while ( current_pos < longfield_headers_length ) {
		doc_custom->longfield_names[current_field] = buf;

		buf = pass_til(buf, '\t', &current_pos, longfield_headers_length);
		*buf = '\0';
		buf++; current_pos++;
		doc_custom->longfield_textpositions[current_field] = atoi(buf);

		buf = pass_til(buf, '\t', &current_pos, longfield_headers_length);
		buf++; current_pos++;
		doc_custom->longfield_nextpositions[current_field] = atoi(buf);

		buf = pass_til(buf, '\0', &current_pos, longfield_headers_length);
		buf++; current_pos++;

		current_field++;
	}
	doc_custom->longfield_count = current_field;
	/******************************/

	return SUCCESS;

error:
	if ( buf ) sb_free(buf);
	sb_free(*doc);
	*doc = NULL;

	return FAIL;
}

static int cdmdoc_get_field_count(cdm_doc_t* doc)
{
	cdm_doc_custom_t* doc_custom;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return MINUS_DECLINE;
	doc_custom = (cdm_doc_custom_t*) doc->data;

	return doc_custom->shortfield_count + doc_custom->longfield_count;
}

/*
 * 다음과 같이 buf, fieldnames의 내용을 채운다
 *               buf  +----------+---------+---------+-----------+--------+
 *                    | Title \0 | Body \0 | Date \0 | Author \0 | ...    |
 * fieldnames         +----------+---------+---------+-----------+--------+
 * +---------------+  ^          ^         ^         ^
 * | fieldnames[0] | -'          |         |         |
 * +---------------+             |         |         |
 * | fieldnames[1] | ------------'         |         |
 * +---------------+                       |         |
 * | fieldnames[2] | ----------------------'         |
 * +---------------+                                 |
 * | fieldnames[3] | --------------------------------'
 * +---------------+             
 * | fieldnames[4] | -------> NULL
 * +---------------+
 * | ...           |
 * +---------------+
 */
static int cdmdoc_get_field_names(
		cdm_doc_t* doc, char** fieldnames, int max_field_count, char* buf, size_t size)
{
	cdm_doc_custom_t* doc_custom;
	int i = 0;
	int doc_field_count;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return MINUS_DECLINE;
	doc_custom = (cdm_doc_custom_t*) doc->data;

	if ( size <= 0 ) {
		warn("invalid buffer size: %Zu", size);
		return CDM2_NOT_ENOUGH_BUFFER;
	}

	doc_field_count = doc_custom->shortfield_count + doc_custom->longfield_count;
	buf[0] = '\0';

	for ( i = 0; i < doc_field_count && i < max_field_count; i++ ) {
		char* field_name;
		size_t append_size;

		if ( i < doc_custom->shortfield_count )
			field_name = doc_custom->shortfield_names[i];
		else field_name = doc_custom->longfield_names[i - doc_custom->shortfield_count];

		append_size = copy_string( buf, field_name, size );
		fieldnames[i] = buf;
		
		if ( append_size != CDM2_NOT_ENOUGH_BUFFER ) {
			// \0 까지 고려해서...
			size -= append_size+1;
			buf += append_size+1;
		}
		else {
			return CDM2_NOT_ENOUGH_BUFFER;
		}
	}

	if ( i < doc_field_count ) fieldnames[i] = NULL;
	return SUCCESS;
}

static int cdmdoc_get_field_by_bytepos(cdm_doc_t* doc, const char* fieldname, int bytepos, char* buf, size_t size)
{
	cdm_doc_custom_t* doc_custom;
	int field_index;
	size_t required_length, copy_length;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return MINUS_DECLINE;
	doc_custom = (cdm_doc_custom_t*) doc->data;

	if ( size <= 0 ) {
		warn("invalid buffer size: %Zu", size);
		return CDM2_NOT_ENOUGH_BUFFER;
	}

	field_index = find_field_from_doc(doc_custom, fieldname);
	if ( field_index < 0 ) {
		warn("field[%s] is not exists in doc[%"PRIu32"]", fieldname, doc->docid);
		buf[0] = '\0';
		return CDM2_FIELD_NOT_EXISTS;
	}
	else if ( field_index < doc_custom->shortfield_count ) { // shortfield
		required_length = strlen(doc_custom->shortfield_values[field_index]) - bytepos;
		if ( required_length >= size ) {
			warn("not enough buffer size(%Zu) for field[%s] required (%Zu)",
					size, fieldname, required_length+1);
			copy_length = size-1;
		}
		else copy_length = required_length;

		memcpy(buf, doc_custom->shortfield_values[field_index] + bytepos, copy_length);
		buf[copy_length] = '\0';

		if ( copy_length == required_length ) return copy_length;
		else return CDM2_NOT_ENOUGH_BUFFER;
	}
	else { // longfield
		off_t offset;
		size_t buf_left, body_left;
		size_t ret;

		field_index -= doc_custom->shortfield_count;

		buf_left = size;
		offset = doc_custom->longfield_bodies_start_offset +
			doc_custom->longfield_textpositions[field_index] + bytepos;
		body_left = doc_custom->longfield_nextpositions[field_index] -
			doc_custom->longfield_textpositions[field_index] - bytepos;
		ret = body_left-1; // \0 제외

		INDEX_RD_LOCK((cdm_db_custom_t*) doc->cdm_db->db, return FAIL);
		{
			size_t read_size = body_left;
			if ( read_size > buf_left ) read_size = buf_left;

			if ( read_cdm((cdm_db_custom_t*)doc->cdm_db->db,
					doc_custom, offset, buf, read_size) != SUCCESS ) {
				error("cannot read cdm file: %s", strerror(errno));
				INDEX_UN_LOCK((cdm_db_custom_t*) doc->cdm_db->db, );
				return FAIL;
			}

			buf += read_size;
		}
		INDEX_UN_LOCK((cdm_db_custom_t*) doc->cdm_db->db, );

		if ( buf_left <= body_left ) {
			*(buf-1) = '\0';
			return CDM2_NOT_ENOUGH_BUFFER;
		}
		else {
			*buf = '\0';
			return (int) ret;
		}
	}
}

static int cdmdoc_get_field_by_wordpos(cdm_doc_t* doc, const char* fieldname, int wordpos, char* buf, size_t size)
{
	return MINUS_DECLINE;
}

static int cdmdoc_get_field(cdm_doc_t* doc, const char* fieldname, char* buf, size_t size)
{
	return cdmdoc_get_field_by_bytepos(doc, fieldname, 0, buf, size);
}

static int cdmdoc_update_field(cdm_doc_t* doc, char* fieldname, char* buf, size_t size)
{
	cdm_doc_custom_t* doc_custom;
	cdm_db_custom_t* db;
	struct cdmfield_t* field;
	int idx, ret;
	off_t offset;
	char* _buf; // buf를 shortfield_size까지 확장해야 한다.
	            // cdm2 format에 shortfield는 안쓰는 공간을 \0 으로 채운다고 되어 있다.
	size_t shortfield_size;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return DECLINE;
	doc_custom = (cdm_doc_custom_t*) doc->data;
	db = (cdm_db_custom_t*) doc->cdm_db->db;

	field = find_cdmfield(fieldname);
	if ( field != NULL && field->is_index ) { // 색인필드면 update금지
		warn("index field[%s] is not updatable", fieldname);
		return CDM2_UPDATE_NOT_AVAILABLE;
	}

	idx = find_field_from_doc(doc_custom, fieldname);
	if ( idx < 0 ) {
		warn("cannot update field[%s]: field not found in document[%"PRIu32"]",
				fieldname, doc->docid);
		return CDM2_FIELD_NOT_EXISTS;
	}
	else if ( idx >= doc_custom->shortfield_count ) {
		warn("long field[%s] is not updatable", fieldname);
		return CDM2_UPDATE_NOT_AVAILABLE;
	}
	else if ( size > doc_custom->shortfield_size[idx] ) {
		char tmp[20] = { 0, };
		strncat(tmp, buf, 19);
		warn("not enough field size[%d] for field_value[%s...]",
				doc_custom->shortfield_size[idx], tmp);
		return CDM2_UPDATE_NOT_AVAILABLE;
	}

	shortfield_size = doc_custom->shortfield_size[idx];
	_buf = (char*) sb_malloc(shortfield_size+1);
	strncpy(_buf, buf, shortfield_size); // man page보면 0으로 채운다고 한다.
	_buf[shortfield_size] = '\0';

	offset = HEADER_SIZE + // 요거는 cdm_get_doc()을 보고 잘 맞춰야 한다.
		((intptr_t)doc_custom->shortfield_values[idx] - (intptr_t)doc_custom->data);

	INDEX_WR_LOCK(db, return FAIL);
	// shortfield_size+1은 이미 \0이므로 굳이 기록하지 않아도 된다.
	ret = write_cdm(db, doc_custom, offset, _buf, shortfield_size);
	INDEX_UN_LOCK(db, );
	sb_free(_buf);

	if ( ret != SUCCESS ) {
		error("cdm update failed");
		return FAIL;
	}

	// docattr field update
	if ( field != NULL && field->is_docattr ) {
		docattr_mask_t docmask;
		char docid_str[20];
		sprintf(docid_str, "%"PRIu32, doc->docid);

		DOCMASK_SET_ZERO(&docmask);
		// fieldname은 실제로 수정하지 않으니까 괜찮을까?
		set_docattr_mask( &docmask, fieldname, buf, size, docid_str );
		if ( sb_run_docattr_set_array(&doc->docid, 1, SC_MASK, &docmask) != SUCCESS ) {
			warn("cannot save docattr db of document[%"PRIu32"]", doc->docid);
		}
	}

	return SUCCESS;
}

static int cdmdoc_destroy(cdm_doc_t* doc)
{
	cdm_doc_custom_t* doc_custom;

	if ( cdm_set == NULL || !cdm_set[doc->cdm_db->set].set )
		return MINUS_DECLINE;
	doc_custom = (cdm_doc_custom_t*) doc->data;

	sb_free(doc_custom->data);
	sb_free(doc);

	return SUCCESS;
}

static int cdm_put_xmldoc(cdm_db_t* cdm_db, did_db_t* did_db, char* oid,
		const char* xmldoc, size_t size, uint32_t* newdocid, uint32_t* olddocid)
{
	cdm_db_custom_t* db;
	parser_t* p = NULL;
	field_t *f;
	char header[HEADER_SIZE+1];
	memfile *shortfields = NULL, *longfield_headers = NULL, *longfield_bodies = NULL;
	int i, shortfields_length = 0, longfield_headers_length = 0;
	docattr_mask_t docmask;
	int ret, oid_duplicated = 0;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;

	p = sb_run_xmlparser_parselen("CP949", xmldoc, size);
	if ( p == NULL ) {
		error("cannot parse document[%s]", oid);
		return CDM2_PUT_NOT_WELL_FORMED_DOC;
	}

	DOCMASK_SET_ZERO(&docmask);

	shortfields = memfile_new();
	longfield_headers = memfile_new();
	longfield_bodies = memfile_new();
	if ( !shortfields || !longfield_headers || !longfield_bodies ) {
		crit("not enough memory for allocation memfile!");
		goto error;
	}

	// shortfield 처리
	for ( i = 0; fields[i].type == SHORT && i < field_count; i++ ) {
		f = get_xml_field(p, fields[i].name, oid);
		if ( f == NULL ) continue;

		if ( write_shortfield_to_buf(
					shortfields, &fields[i], f->value, f->size, oid) != SUCCESS ) {
			goto error;
		}

		// set docattr
		if ( fields[i].is_docattr ) {
			set_docattr_mask( &docmask, fields[i].name, f->value, f->size, oid );
		}
	}
	shortfields_length = memfile_getSize(shortfields);

	// longfield 처리
	for ( ; i < field_count; i++ ) {
		f = get_xml_field(p, fields[i].name, oid);
		if ( f == NULL ) continue;

		if ( write_longfield_to_buf(longfield_headers, longfield_bodies,
					&fields[i], f->value, f->size, oid) != SUCCESS ) {
			goto error;
		}

		// set docattr
		if ( fields[i].is_docattr ) {
			set_docattr_mask( &docmask, fields[i].name, f->value, f->size, oid );
		}
	}
	longfield_headers_length = memfile_getSize(longfield_headers);

	INDEX_WR_LOCK(db, return FAIL);
	{
		int db_no, cdm_fd;
		unsigned long offset;
		IndexFileElement idxEle;

		// 기록할 위치 정하기
		db_no = db->shared->db_no;
		offset = db->shared->offset;

		// size 넘었으면 다음 파일로..
		if ( offset >= db->cdm_file_size ) {
			db_no++; offset = 0;

			if ( open_cdm_file( db, db_no ) != SUCCESS )
				goto error_unlock;
		}
		cdm_fd = db->fdCdmFiles[db_no];

		/////////////////////////////////////////
		// did 생성 가져오기
		//ret = sb_run_get_new_docid(db->did_db, oid, newdocid, olddocid);
		ret = sb_run_get_new_docid(did_db, oid, newdocid, olddocid);
		if ( ret < 0 ) {
			error("cannot get new document id of oid[%s]:error(%s)",
					oid, strerror(errno));
			goto error;
		}
		else if ( ret == DOCID_OLD_REGISTERED ) {
			info("old docid[%u] of OID[%s] is deleted. new docid is %u",
					*olddocid, oid, *newdocid);
			set_delete(*olddocid);
			oid_duplicated = 1;
		}

		if ( *newdocid != db->shared->last_docid+1 ) {
			warn("newdocid[%"PRIu32"] != last_docid of cdm[%"PRIu32"]+1",
					*newdocid, db->shared->last_docid);
		}
		/////////////////////////////////////////

		/* mask에 넣었던 docattr실제로 저장 (need newdocid) */
		if ( sb_run_docattr_set_array(newdocid, 1, SC_MASK, &docmask) != SUCCESS ) {
			warn("cannot save docattr db of document[%s]", oid);
		}

		/*******************
		 * index file 기록 * 
		 *******************/
		idxEle.dwDBNo = db_no;
		idxEle.offset = offset;
		idxEle.length = HEADER_SIZE + shortfields_length +
		                longfield_headers_length + memfile_getSize(longfield_bodies);
		idxEle.docId  = *newdocid;
		ret = InsertIndexElement(db->fdIndexFile, &idxEle);
		if ( ret < 0 ) {
			error("cdm document[%s] insert failed", oid);
			goto error_unlock;
		}

		/*******************
		 * cdm 파일에 기록 *
		 *******************/
#define CHECK_ERROR(func) \
		if ( func < 0 ) { \
			error("cdm document[%s] write failed", oid); \
			goto error_unlock; \
		}

		if ( lseek(cdm_fd, offset, SEEK_SET) != offset ) {
			error("lseek failed[offset: %"PRIu64"] - %s", (uint64_t)offset, strerror(errno));
			goto error_unlock;
		}

		snprintf(header, sizeof(header), "%-11d\t%-11d\n",
				shortfields_length, longfield_headers_length);
		write_buffer_init();
		CHECK_ERROR( write_buffer_from_data(cdm_fd, header, HEADER_SIZE) );
		CHECK_ERROR( write_buffer_from_memfile(cdm_fd, shortfields) );
		CHECK_ERROR( write_buffer_from_memfile(cdm_fd, longfield_headers) );
		CHECK_ERROR( write_buffer_from_memfile(cdm_fd, longfield_bodies) );
		CHECK_ERROR( write_buffer_flush(cdm_fd) );
#undef CHECK_ERROR

		/************************
		 * shared memory update *
		 ************************/
		db->shared->last_docid = *newdocid;
		db->shared->db_no = db_no;
		db->shared->offset = offset + idxEle.length;
	}
	INDEX_UN_LOCK(db, );

	memfile_free(longfield_bodies);
	memfile_free(longfield_headers);
	memfile_free(shortfields);
	sb_run_xmlparser_free_parser(p);

	if ( oid_duplicated ) return CDM2_PUT_OID_DUPLICATED;
	else return SUCCESS;

error_unlock:
	INDEX_UN_LOCK(db, );
error:
	if ( longfield_bodies ) memfile_free(longfield_bodies);
	if ( longfield_headers ) memfile_free(longfield_headers);
	if ( shortfields ) memfile_free(shortfields);
	sb_run_xmlparser_free_parser(p);
	return FAIL;
}

#define COPY_STRING(text) \
	append_size = copy_string(buf, text, size); \
	if ( append_size == CDM2_NOT_ENOUGH_BUFFER ) { \
		cdmdoc_destroy(doc); \
		return CDM2_NOT_ENOUGH_BUFFER; \
	} \
	buf += append_size; \
	buf_left -= append_size;
static int cdm_get_xmldoc(cdm_db_t* cdm_db, uint32_t docid, char* buf, size_t size)
{
	cdm_db_custom_t* db;
	cdm_doc_t* doc = NULL;
	cdm_doc_custom_t* doc_custom;
	int i, ret;
	char* fieldname;
	size_t append_size, buf_left;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return MINUS_DECLINE;
	db = (cdm_db_custom_t*) cdm_db->db;

	ret = cdm_get_doc(cdm_db, docid, &doc);
	if ( ret != SUCCESS ) return ret;
	else doc_custom = (cdm_doc_custom_t*) doc->data;

	buf[0] = '\0';
	buf_left = size;

	COPY_STRING("<");
	COPY_STRING(field_root_name);
	COPY_STRING(">\n");

	for ( i = 0; i < doc_custom->shortfield_count; i++ ) {
		fieldname = doc_custom->shortfield_names[i];

		COPY_STRING("<");
		COPY_STRING(fieldname);
		COPY_STRING("><![CDATA[");

		ret = cdmdoc_get_field(doc, fieldname, buf, buf_left);
		if ( ret < 0 ) { // maybe CDM2_NOT_ENOUGH_BUFFER
			cdmdoc_destroy(doc);
			return ret;
		}
		buf += ret;
		buf_left -= ret;

		COPY_STRING("]]></");
		COPY_STRING(fieldname);
		COPY_STRING(">\n");
	}

	for ( i = 0; i < doc_custom->longfield_count; i++ ) {
		fieldname = doc_custom->longfield_names[i];

		COPY_STRING("<");
		COPY_STRING(fieldname);
		COPY_STRING("><![CDATA[");

		ret = cdmdoc_get_field(doc, fieldname, buf, buf_left);
		if ( ret < 0 ) { // maybe CDM2_NOT_ENOUGH_BUFFER
			cdmdoc_destroy(doc);
			return ret;
		}
		buf += ret;
		buf_left -= ret;

		COPY_STRING("]]></");
		COPY_STRING(fieldname);
		COPY_STRING(">\n");
	}

	COPY_STRING("</");
	COPY_STRING(field_root_name);
	COPY_STRING(">\n");

	cdmdoc_destroy(doc);
	return (int) (size - buf_left);
}

static uint32_t cdm_last_docid(cdm_db_t* cdm_db)
{
	cdm_db_custom_t* db;

	if ( cdm_set == NULL || !cdm_set[cdm_db->set].set )
		return (uint32_t)-1;
	db = (cdm_db_custom_t*) cdm_db->db;

	return db->shared->last_docid;
	//return sb_run_get_last_docid(db->did_db);
}


/******************* not api from here *****************/

/* oid는 그냥 메시지 출력용 */
static field_t* get_xml_field(parser_t* p, const char* field_name, const char* oid)
{
	field_t* f;
	char path[STRING_SIZE];
	snprintf(path, STRING_SIZE, "/%s/%s", field_root_name, field_name);

	f = sb_run_xmlparser_retrieve_field(p, path);
	if ( f == NULL ) {
		warn("cannot get field[%s] of document [%s]", path, oid);
	}

	return f;
}

// 없으면 NULL
static struct cdmfield_t* find_cdmfield(const char* name)
{
	int i;

	for ( i = 0; i < field_count; i++ ) {
		if ( strcmp(fields[i].name, name) == 0 )
			return &fields[i];
	}

	return NULL;
}

/*
 * doc object에서 해당 field의 index를 찾는다.
 * (shortfield_xxx[], longfield_xxx[] 에 대응한다)
 *
 * 리턴값은 array index 값으로,
 * 해당 field가 shortfield이면 "< doc->shortfield_count" 이고
 * longfield이면 ">= doc->shortfield_count" 이다.
 * 그러므로 longfield index는 실제로 사용하려면 shortfield_count를 빼고 써야한다
 *
 * 못찾으면 -1 이다.
 */
static int find_field_from_doc(cdm_doc_custom_t* doc_custom, const char* name)
{
	int i, limit;

	limit = doc_custom->shortfield_count;
	for ( i = 0; i < limit; i++ ) {
		if ( strcmp(name, doc_custom->shortfield_names[i]) == 0 )
			return i;
	}

	limit = doc_custom->longfield_count;
	for ( i = 0; i < limit; i++ ) {
		if ( strcmp(name, doc_custom->longfield_names[i]) == 0 )
			return doc_custom->shortfield_count+i;
	}

	return -1;
}

static char* get_field_type_name(enum field_type_t type)
{
	switch ( type ) {
		case SHORT: return "SHORT";
		case LONG: return "LONG";
		default: return "UNKNOWN";
	}
}

/*********** module stuff ***********/

#define DEFAULT_CDM_PATH "dat/cdm"
#define DEFAULT_CDM_FILE_SIZE (1024*1024*1024) // 1024MB
#define DEFAULT_MAX_DOC_NUM 1000000

static int init()
{
	ipc_t lock;
	int i, last_mark;

	if ( field_count < 0 ) {
		error("config error");
		return FAIL;
	}

	// doc, doc_custom 은 memory를 한꺼번에 할당하므로
	// 둘로 잘 쪼개려면 aligned offset을 알아야 한다.
	aligned_offset = (sizeof(cdm_doc_t)+sizeof(long)-1)/sizeof(long)*sizeof(long);

	// set is_docattr in fields[]
	for ( i = 0; i < docattr_field_count; i++ ) {
		struct cdmfield_t* field = find_cdmfield(docattr_fields[i]);
		if ( field == NULL ) {
			warn("no Field[%s] matching DocAttrField. maybe error?", docattr_fields[i]);
			continue;
		}

		field->is_docattr = 1;
	}

	// sort field with type
	for ( last_mark = field_count-1;
			fields[last_mark].type == LONG && last_mark >= 0; last_mark-- );
	for ( i = 0; i < last_mark; i++ ) {
		struct cdmfield_t field;

		if ( fields[i].type == LONG ) {
			field = fields[i];
			fields[i] = fields[last_mark];
			fields[last_mark] = field;

			last_mark--;
		}
	}

	for ( i = 0; i < field_count; i++ ) {
		info("CDM field: %s (%s), size(%d), is_index(%d), is_docattr(%d)",
				fields[i].name, get_field_type_name(fields[i].type),
				fields[i].size, fields[i].is_index, fields[i].is_docattr);
	}

	if ( cdm_set == NULL ) return SUCCESS;

	lock.type = IPC_TYPE_SEM;
	lock.pid = SYS5_DOCID;

	for ( i = 0; i < MAX_CDM_SET; i++ ) {
		singleton_cdm_db[i] = NULL;

		if ( !cdm_set[i].set ) continue;

		if ( cdm_set[i].cdm_path[0] == '\0' ) {
			copy_string( cdm_set[i].cdm_path, DEFAULT_CDM_PATH, MAX_PATH_LEN );
		}

		if ( cdm_set[i].cdm_file_size <= 0 ) {
			cdm_set[i].cdm_file_size = DEFAULT_CDM_FILE_SIZE;
		}

		if ( cdm_set[i].max_doc_num == 0 ) {
			cdm_set[i].max_doc_num = DEFAULT_MAX_DOC_NUM;
		}

		snprintf( cdm_set[i].shared_file, MAX_PATH_LEN,"%s/cdm2.shared", cdm_set[i].cdm_path );
	}
	
	return SUCCESS;
}

/*****************************************************
 *                   config stuff
 *****************************************************/

static void get_field(configValue v)
{
	if ( field_count >= MAX_FIELD_NUM ) {
		error("too many field for CDM. max is %d", MAX_FIELD_NUM);
		error("increase MAX_FIELD_NUM and recompile");
		field_count = -1; // init() would fail
		return;
	}
	else if ( field_count < 0 ) return; // skip for error in init()

	copy_string(fields[field_count].name, v.argument[1], SHORT_STRING_SIZE);
	fields[field_count].is_index = ( strcmp(v.argument[2], "yes")==0 );

	// type, size
	if ( v.argNum >= 8 ) {
		if ( strncmp(v.argument[7], "SHORT", 5) == 0 ) {
			fields[field_count].type = SHORT;
			if ( v.argument[7][5] == '(' )
				fields[field_count].size = atoi(&v.argument[7][6]);
			else fields[field_count].size = 0;
		}
		else if ( strcmp(v.argument[7], "LONG") == 0 ) {
			fields[field_count].type = LONG;
			fields[field_count].size = 0;
		}
		else field_count = -1;
	}
	else {
		fields[field_count].type = SHORT;
		fields[field_count].size = 0;
	}

	field_count++;
}

// for fields[].is_docattr
static void get_docattrfield(configValue v)
{
	copy_string(docattr_fields[docattr_field_count], v.argument[0], SHORT_STRING_SIZE);
	docattr_field_count++;
}

static void get_field_root_name(configValue v)
{
	copy_string(field_root_name, v.argument[0], SHORT_STRING_SIZE);
}

static void get_cdm_set(configValue v)
{
	static cdm_db_set_t local_cdm_set[MAX_CDM_SET];
	int value = atoi( v.argument[0] );

	if ( value < 0 || value >= MAX_CDM_SET ) {
		error("Invalid CdmSet value[%s], MAX_CDM_SET[%d]",
				v.argument[0], MAX_CDM_SET);
		return;
	}

	if ( cdm_set == NULL ) {
		memset( local_cdm_set, 0, sizeof(local_cdm_set) );
		cdm_set = local_cdm_set;
	}

	current_cdm_set = value;
	cdm_set[value].set = 1;
}
/*
static void get_did_set(configValue v)
{
	if ( cdm_set == NULL || current_cdm_set < 0 ) {
		error("first, set CdmSet");
		return;
	}

	cdm_set[current_cdm_set].did_set = atoi( v.argument[0] );
	cdm_set[current_cdm_set].set_did_set = 1;
}
*/
static void get_cdm_path(configValue v)
{
	char* cdm_path;
	size_t path_len;

	if ( cdm_set == NULL || current_cdm_set < 0 ) {
		error("first, set CdmSet");
		return;
	}

	cdm_path = cdm_set[current_cdm_set].cdm_path;

	copy_string( cdm_path, v.argument[0], MAX_PATH_LEN-1 );
	path_len = strlen( cdm_path );
	if ( cdm_path[path_len-1] == '/' ) cdm_path[path_len-1] = '\0';
}

static void get_cdm_file_size(configValue v)
{
	char* ptr;
	size_t tmp;

	if ( cdm_set == NULL || current_cdm_set < 0 ) {
		error("first, set CdmSet");
		return;
	}

	tmp = strtol( v.argument[0], &ptr, 0 );
	if ( *ptr == 'K' ) tmp *= 1024;
	else if ( *ptr == 'M' ) tmp *= (1024*1024);
	else if ( *ptr == 'G' ) tmp *= (1024*1024*1024);

	cdm_set[current_cdm_set].cdm_file_size = tmp;
}

static void get_max_doc_num(configValue v)
{
	cdm_set[current_cdm_set].max_doc_num = atoi(v.argument[0]);
}

static config_t config[] = {
	CONFIG_GET("Field", get_field, VAR_ARG, "..."),
	CONFIG_GET("DocAttrField", get_docattrfield, VAR_ARG, "..."),
	CONFIG_GET("FieldRootName", get_field_root_name, 1, \
			"canned document root element name"),

	CONFIG_GET("CdmSet", get_cdm_set, 1, "CdmSet {number}"),
	//CONFIG_GET("DidSet", get_did_set, 1, "DidSet {number}"),
	CONFIG_GET("CdmPath", get_cdm_path, 1, "cdm directory path (relative, no trailing /)"),
	CONFIG_GET("CdmFileSize", get_cdm_file_size, 1, "maximum file size (ex> 1G 512M 1024K 100000000)"),
	CONFIG_GET("MaxDocNum", get_max_doc_num, 1, "maximum document count"),
	{NULL}
};

static void register_hooks(void)
{
	sb_hook_cdm_open( cdm_open, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_close( cdm_close, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_get_doc( cdm_get_doc, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field_count( cdmdoc_get_field_count, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field_names( cdmdoc_get_field_names, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field( cdmdoc_get_field, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field_by_bytepos( cdmdoc_get_field_by_bytepos, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_get_field_by_wordpos( cdmdoc_get_field_by_wordpos, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_update_field( cdmdoc_update_field, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdmdoc_destroy( cdmdoc_destroy, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_put_xmldoc( cdm_put_xmldoc, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_get_xmldoc( cdm_get_xmldoc, NULL, NULL, HOOK_MIDDLE );
	sb_hook_cdm_last_docid( cdm_last_docid, NULL, NULL, HOOK_MIDDLE );
}

module cdm2_module =
{
	STANDARD_MODULE_STUFF,
	config,                /* config */
	NULL,                  /* registry */
	init,                  /* initialize function */
	NULL,                  /* child_main */
	NULL,                  /* scoreboard */
	register_hooks         /* register hook api */
};
