/* $Id$ */
#ifndef SZ_STRING_H
#define SZ_STRING_H 1

/* zero terminated string functions 
 * be careful when you use this,
 * because it's differnt from original function
 * in that it overwrite '\0' at the end
 */
SB_DECLARE(char*) sz_strncat(char *dest, const char *src, size_t n);
SB_DECLARE(char*) sz_strncpy(char *dest, const char *src, size_t n);
SB_DECLARE(int) sz_snprintf(char *str, size_t size, const char *format, ...);

#endif
