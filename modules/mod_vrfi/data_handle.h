/* $Id$ */
#ifndef DATA_HANDLE_H
#define DATA_HANDLE_H 1

#include "softbot.h"

typedef struct data_handle_t data_handle_t;

typedef struct {
	uint32_t startbyte;
	uint32_t size;
	uint16_t fileno;
} location_t;

extern char* get_data_info(data_handle_t *this);
extern data_handle_t *new_data_file_handle();
extern int data_open(data_handle_t *this, char filepath[]);
extern int data_close(data_handle_t *this);
extern int data_write(data_handle_t *this);
extern int append_data(data_handle_t *this,
			uint32_t size, void *data, location_t locations[]);

extern int get_data(data_handle_t *this,
			location_t locations[], int nlocations, void *data);
#define DATA_TAG ".data"
#define DATAFILE_SIZE	1000000000

#endif
