#ifndef _MSG_RECORD_H_
#define _MSG_RECORD_H_ 

//#include "common_core.h"

//------------------------------------------------------//
#define MAX_RECORDED_MSG_LEN	(128)

typedef struct msg_record_t msg_record_t;

struct msg_record_t {
	int used_bytes;
	int read_bytes;
	char record[LONG_STRING_SIZE];
};

#define MSG_RECORD(msg_record, level, format, ... ) \
		msg_record_insert(msg_record, format, ##__VA_ARGS__); \
		level(format, ##__VA_ARGS__);

//------------------------------------------------------//
void msg_record_init(msg_record_t *rec);
void msg_record_free(msg_record_t *rec);
void msg_record_rewind(msg_record_t *rec);
int msg_record_insert(msg_record_t *rec, char *format, ...);
int msg_record_read(msg_record_t *rec, char *buffer);
//------------------------------------------------------//

#endif // _MSG_RECORD_H_
