/* $Id$ */
#ifndef HEADER_HANDLE_H
#define HEADER_HANDLE_H 1

#include <stdint.h> /* uint32_t */

typedef struct header_handle_t header_handle_t;

typedef struct {
	uint32_t start;
	uint16_t nelm;
	uint8_t fileno;
} header_t;

typedef struct {
	uint32_t position;
	uint8_t fileno;
} header_pos_t;

typedef struct {
	uint32_t key;
	header_pos_t prev;
} header_metadata_t;

typedef struct {
	header_t header;
	header_metadata_t metadata;
} header_data_t;


extern header_handle_t *new_header_file_handle();
extern int header_open(header_handle_t *this, char filepath[]);
extern int header_close(header_handle_t *this);
extern int header_write(header_handle_t *this);
extern int append_headers(/*in*/header_handle_t *this, 
					/*in*/uint32_t key, /*in*/header_pos_t *lastpos, 
					/*in*/header_t headers[], /*in*/int nelm,
					/*out*/header_pos_t *new_last_pos); // XXX: key+lastpos -> metadata
extern int get_headers(/*in*/header_handle_t *this,
					/*in*/header_metadata_t *header_meta,
					/*in*/uint32_t skip, /*in*/uint32_t nelm, /*out*/uint32_t *r_nelm,
					/*out*/header_t headers[], /*in*/ int size);
					
extern char* get_header_info(header_handle_t *this);

#define HEADER_TAG ".header"
#define HEADERFILE_SIZE INT32_MAX

#endif
