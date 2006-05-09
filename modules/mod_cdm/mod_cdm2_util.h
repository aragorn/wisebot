#ifndef _MOD_CDM2_UTIL_H_
#define _MOD_CDM2_UTIL_H_

void* big_calloc(size_t size);
void* big_realloc(void* buf, size_t size);
char* pass_til(char* buf, char term, int* pos, int max_pos);
char* pass_not(char* buf, char term, int* pos, int max_pos);
size_t copy_string(char* buf, const char* text, size_t size);

#endif // _MOD_CDM2_UTIL_H_

