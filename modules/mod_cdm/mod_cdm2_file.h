#ifndef _MOD_CDM2_FILE_H_
#define _MOD_CDM2_FILE_H_

#include <sys/types.h> // for off_t, size_t
#include "memfile.h"
#include "mod_cdm2.h"

#define CDM_FILE_FORMAT    "%s/cdm2_%03d.db"

int open_cdm_file(cdm_db_custom_t* db, int num);
int close_cdm_file(cdm_db_custom_t* db, int num);
int open_index_file(cdm_db_custom_t* db);
int close_index_file(cdm_db_custom_t* db);

int read_index(cdm_db_custom_t* db, cdm_doc_custom_t* doc_custom, uint32_t docid);
int read_cdm(cdm_db_custom_t* db,
		cdm_doc_custom_t* doc_custom, off_t offset, char* buf, size_t size);
int write_cdm(cdm_db_custom_t* db,
		cdm_doc_custom_t* doc_custom, off_t offset, const char* buf, size_t size);

int write_shortfield_to_buf(memfile* buf, struct cdmfield_t* field,
		const char* field_value, int value_length, const char* oid);
int write_longfield_to_buf(memfile* headers, memfile* bodies,
		struct cdmfield_t* field, const char* field_value, int value_length, const char* oid);
int append_zero(memfile* buf, int count);

// write() call을 한 번이라도 더 줄이기 위해서...
void write_buffer_init();
int write_buffer_from_data(int fd, char* data, int data_size);
int write_buffer_from_memfile(int fd, memfile* data);
int write_buffer_flush(int fd);

#endif // _MOD_CDM2_FILE_H_

