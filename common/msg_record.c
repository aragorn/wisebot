#include "msg_record.h"
#include <string.h>

void msg_record_init(msg_record_t *rec){
	rec->used_bytes = 0;
	rec->read_bytes = 0;
	memset(rec->record, 0, sizeof(rec->record));
}

void msg_record_free(msg_record_t *rec){
	msg_record_init(rec);
	return;
}

int msg_record_insert(msg_record_t *rec_in, char *format, ...){
	msg_record_t *rec;
	int msglen, allowed_len;
	va_list args;

	rec = rec_in;
	if ( rec == NULL ) {
		DEBUG("null msg_record_t : do not record");
		return FAIL;
	}

	if ( !format ) {
		error("null input error");
		return FAIL;
	}

	if ( rec->used_bytes >= sizeof(rec->record) ){
		DEBUG("no more space to record");
		return SUCCESS;
	}

	allowed_len = ( (sizeof(rec->record)-rec->used_bytes) > MAX_RECORDED_MSG_LEN ) ?
				MAX_RECORDED_MSG_LEN : (sizeof(rec->record)-rec->used_bytes);
	
	va_start(args, format);
	msglen = vsnprintf(rec->record + rec->used_bytes, allowed_len, format, args );
	va_end(args);
	if ( msglen	< 0 ){
		error("error_msg too long");
		return FAIL;
	}else {
		if ( allowed_len <= msglen ) msglen = allowed_len-1;
	}
	rec->record[rec->used_bytes+msglen]= '\0';
	rec->used_bytes += msglen+1;

	if (rec->used_bytes == sizeof(rec->record) ){
		DEBUG("no more space to record, so the end of message was truncated");
		rec->record[sizeof(rec->record)-3]= '.';
		rec->record[sizeof(rec->record)-2]= '.';
		rec->record[sizeof(rec->record)-1]= '\0';
	}
	
	return SUCCESS;
}

void msg_record_rewind(msg_record_t *rec){
	rec->read_bytes = 0;
}


int msg_record_read(msg_record_t *rec, char *buffer){
	char *ptr = rec->record + rec->read_bytes;
	if ( rec->read_bytes >= rec->used_bytes ) {
		DEBUG("no more msg_record");
		return FAIL;
	}

	strcpy(buffer, ptr);
	rec->read_bytes += strlen(ptr)+1;
	
	return SUCCESS;
}

