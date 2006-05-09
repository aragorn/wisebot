#ifndef _MOD_CDM2_DOCATTR_H_
#define _MOD_CDM2_DOCATTR_H_

int is_deleted(uint32_t docid, int* deleted);
int set_delete(uint32_t docid);
int set_docattr_mask(docattr_mask_t* mask,
		char* field_name, char* field_value, int value_length, const char* oid);

#endif // _MOD_CDM2_DOCATTR_H_

