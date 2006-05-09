#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> // for strerror()

#include "common_core.h"
#include "common_util.h"
#include "memory.h"
#include "mod_cdm2.h"
#include "mod_cdm2_lock.h"
#include "mod_cdm2_file.h"
#include "cannedDocServer.h"

int open_cdm_file(cdm_db_custom_t* db, int num)
{
	int fd;
	char filename[MAX_PATH_LEN];

	if ( db->fdCdmFiles[num] != 0 ) return SUCCESS;

	snprintf(filename, sizeof(filename), CDM_FILE_FORMAT, db->cdm_path, num);
	fd = sb_open(filename, O_CREAT|O_RDWR|O_SYNC, 0600);
	if ( fd == -1 ) {
		error("open cdm file[%s] failed", filename);
		return FAIL;
	}

	db->fdCdmFiles[num] = fd;
	return SUCCESS;
}

int close_cdm_file(cdm_db_custom_t* db, int num)
{
	int ret;

	if ( db->fdCdmFiles[num] == 0 ) return SUCCESS;

	ret = close(db->fdCdmFiles[num]);
	db->fdCdmFiles[num] = 0;

	if ( ret == 0 ) return SUCCESS;

	error("close cdm fd[%d] failed", db->fdCdmFiles[num]);
	return FAIL;
}

int open_index_file(cdm_db_custom_t* db)
{
	int fd;
	char filename[MAX_PATH_LEN];
	char tmp[1];
	off_t offset;

	if ( db->fdIndexFile != 0 ) return SUCCESS;

	snprintf(filename, sizeof(filename), "%s/cdm2.index", db->cdm_path);
	fd = sb_open(filename, O_CREAT|O_RDWR|O_SYNC, 0600);
	if ( fd == -1 ) {
		error("open cdm index file[%s] failed", filename);
		return FAIL;
	}

	offset = lseek(fd, db->max_doc_num*sizeof(IndexFileElement), SEEK_SET);
	if ( offset == (off_t)-1 ) {
		error("disk size is too small to allocate index file (key.idx) whose size is %d.",
				db->max_doc_num*sizeof(IndexFileElement));
		return FAIL;
	}

	if ( write(fd, tmp, 1) < 0 ) {
		error("expand index file failed: %s", strerror(errno));
		return FAIL;
	}

	db->fdIndexFile = fd;
	return SUCCESS;
}

int close_index_file(cdm_db_custom_t* db)
{
	int ret;

	if ( db->fdIndexFile == 0 ) return SUCCESS;

	ret = close(db->fdIndexFile);
	db->fdIndexFile = 0;

	if ( ret == 0 ) return SUCCESS;

	error("close cdm index file fd[%d] failed", db->fdIndexFile);
	return FAIL;
}

int read_index(cdm_db_custom_t* db, cdm_doc_custom_t* doc_custom, uint32_t docid)
{
	IndexFileElement idxEle;
	int ret;

	INDEX_RD_LOCK(db, return FAIL);
	ret = SelectIndexElement(db->fdIndexFile, docid, &idxEle);
	INDEX_UN_LOCK(db, );

	if ( ret != SUCCESS ) {
		warn("no document retrieved by docid[%u]", docid);
		return FAIL;
	}

	doc_custom->dwDBNo = idxEle.dwDBNo;
	doc_custom->offset = idxEle.offset;
	doc_custom->length = idxEle.length;

	return SUCCESS;
}

/*
 * doc_custom->dwDBNo 에 해당하는 파일에서 size만큼 읽어온다
 * doc_custom의 dwDBNo, offset은 반드시 정상적인 값이어야 한다.
 * doc_custom의 나머지 값들은 아직 비정상일 수 있다.
 *
 * offset : 현재 cdm문서기준으로 relative offset
 *
 * 주의 : 반드시 INDEX_RD_LOCK으로 cdm file을 잠그고 와야 한다.
 */
int read_cdm(cdm_db_custom_t* db,
		cdm_doc_custom_t* doc_custom, off_t offset, char* buf, size_t size)
{
	off_t real_offset, lseek_result;
	ssize_t read_result;
	int fd = db->fdCdmFiles[doc_custom->dwDBNo];

	if ( !db->locked ) {
		error("cdm file is not locked. call INDEX_RD_LOCK()");
		return FAIL;
	}

	real_offset = doc_custom->offset + offset;

	lseek_result = lseek(fd, real_offset, SEEK_SET);
	if ( lseek_result != real_offset ) {
		error("leek failed[cdm offset: %"PRIu64" , real offset: %"PRIu64"] - %s",
				(uint64_t)offset, (uint64_t)real_offset, strerror(errno));
		return FAIL;
	}
	read_result = read(fd, buf, size);

	if ( read_result < 0 ) {
		error("read cdm file[%lu, fd:%d] failed: %s", doc_custom->dwDBNo, fd, strerror(errno));
		return FAIL;
	}
	else if ( read_result != (ssize_t) size ) {
		warn("read not enough data[length:%Zd] from cdm file[%lu, fd:%d]: %s",
				read_result, doc_custom->dwDBNo, fd, strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

/*
 * doc_custom->dwDBNo 에 해당하는 파일에서 size만큼 읽어온다
 * doc_custom의 dwDBNo, offset은 반드시 정상적인 값이어야 한다.
 * doc_custom의 나머지 값들은 아직 비정상일 수 있다.
 *
 * offset : 현재 cdm문서기준으로 relative offset
 *
 * 주의 : 반드시 INDEX_WR_LOCK으로 cdm file을 잠그고 와야 한다.
 */
int write_cdm(cdm_db_custom_t* db,
		cdm_doc_custom_t* doc_custom, off_t offset, const char* buf, size_t size)
{
	off_t real_offset, lseek_result;
	ssize_t write_result;
	int fd = db->fdCdmFiles[doc_custom->dwDBNo];

	if ( !db->locked ) {
		error("cdm file is not locked. call INDEX_RD_LOCK()");
		return FAIL;
	}

	real_offset = doc_custom->offset + offset;

	lseek_result = lseek(fd, real_offset, SEEK_SET);
	if ( lseek_result != real_offset ) {
		error("leek failed[cdm offset: %"PRIu64" , real offset: %"PRIu64"] - %s",
				(uint64_t)offset, (uint64_t)real_offset, strerror(errno));
		return FAIL;
	}
	write_result = write(fd, buf, size);

	if ( write_result < 0 ) {
		error("write cdm file[%lu, fd:%d] failed: %s", doc_custom->dwDBNo, fd, strerror(errno));
		return FAIL;
	}
	else if ( write_result != (ssize_t) size ) {
		warn("write not enough data[length:%Zd] to cdm file[%lu, fd:%d]: %s",
				write_result, doc_custom->dwDBNo, fd, strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

/*
 * shortfield format에 맞게 field name, field value 기록한다.
 * oid는 단순히 로그 메시지 출력용..
 *
 * SUCCESS/FAIL
 *
 * ex> Title \t 제목이다 \0
 *     Date \t 20040302 \0
 */
#define RETURN_ERROR(ret) \
	if ( ret == -1 ) { \
		error("memfile_append failed. doc[%s], field[%s]", oid, field->name); \
		return FAIL; \
	}
int write_shortfield_to_buf(memfile* buf,
		struct cdmfield_t* field, const char* field_value, int value_length, const char* oid)
{
	int zero_length;

	if ( field->size ) {
		if ( value_length > field->size ) {
			warn("not enough field size. doc[%s], field[%s]", oid, field->name);
			value_length = field->size;
			zero_length = 1;
		}
		else {
			zero_length = field->size - value_length + 1;
		}
	}
	else {
		zero_length = 1;
	}

	RETURN_ERROR( memfile_append(buf, field->name, strlen(field->name)) );
	RETURN_ERROR( memfile_append(buf, "\t", 1) );
	RETURN_ERROR( memfile_append(buf, field_value, value_length) );
	RETURN_ERROR( append_zero(buf, zero_length) );

	return SUCCESS;
}

// word position, byte position 기록은 나중에 하자
int write_longfield_to_buf(memfile* headers, memfile* bodies,
		struct cdmfield_t* field, const char* field_value, int value_length, const char* oid)
{
	int position_size = 0;
	int start_offset, text_position, next_position;

	start_offset = memfile_getSize(bodies);
	text_position = start_offset + position_size;
	next_position = text_position + value_length + 1;

	// fieldname, text_position, next_position
	RETURN_ERROR( memfile_appendF(headers, "%s\t%d\t%d",
				field->name, text_position, next_position) );
	RETURN_ERROR( append_zero(headers, 1) );
	RETURN_ERROR( memfile_append(bodies, field_value, value_length) );
	RETURN_ERROR( append_zero(bodies, 1) );

	return SUCCESS;
}
#undef RETURN_ERROR

int append_zero(memfile* buf, int count)
{
	char zero[32] = { 0, };
	int ret, left, zero_size;

	left = count;
	while ( left > 0 ) {
		if ( left > sizeof(zero) ) zero_size = (int) sizeof(zero);
		else zero_size = left;

		ret = memfile_append(buf, zero, left);
		if ( ret < 0 ) return ret;

		left -= zero_size;
	}

	return count;
}

#define UNIT 1024*1024  // 1MB
static char* write_buffer = NULL;
static int buffer_position = 0;
static int buffer_left = UNIT;

void write_buffer_init()
{
	if ( write_buffer == NULL ) {
		write_buffer = (char*) sb_malloc(UNIT);
	}

	if ( buffer_position > 0 ) {
		warn("buffer_position[%d] is not 0", buffer_position);
	}

	buffer_position = 0;
	buffer_left = UNIT;
}

int write_buffer_from_data(int fd, char* data, int data_size)
{
	int copy_size;

	while ( data_size > 0 ) {
		if ( buffer_left > data_size ) copy_size = data_size;
		else copy_size = buffer_left;

		memcpy(write_buffer, data, copy_size);
		
		buffer_position += copy_size;
		buffer_left -= copy_size;
		data_size -= copy_size;

		if ( buffer_left <= 0 ) {
			if ( write_buffer_flush(fd) != SUCCESS ) return FAIL;
		}
	}

	return SUCCESS;
}

int write_buffer_from_memfile(int fd, memfile* data)
{
	int data_size, copy_size;

	data_size = memfile_getSize(data);
	memfile_setOffset(data, 0);

	while ( data_size > 0 ) {
		copy_size = memfile_read(data, write_buffer+buffer_position, buffer_left);
		if ( copy_size < 0 ) {
			error("memfile_read() failed");
			return FAIL;
		}

		buffer_position += copy_size;
		buffer_left -= copy_size;
		data_size -= copy_size;

		if ( buffer_left <= 0 ) {
			if ( write_buffer_flush(fd) != SUCCESS ) return FAIL;
		}
	}

	return SUCCESS;
}

int write_buffer_flush(int fd)
{
	if ( buffer_position > 0 ) {
		if ( write(fd, write_buffer, buffer_position) < buffer_position ) {
			error("error while write cdm to file: %s", strerror(errno));
			return FAIL;
		}

		buffer_position = 0;
		buffer_left = UNIT;
	}

	return SUCCESS;
}
#undef UNIT

