/* $Id$ */
#ifndef STR_MISC_H
#define STR_MISC_H 1

SB_DECLARE(char*) strtoupper(char *str);
SB_DECLARE(char*) strtolower(char *str);

SB_DECLARE(char*) strntoupper(char *str, int size);
SB_DECLARE(char*) strntolower(char *str, int size);

/*
 * NOTICE:
 * The comparison function must return an integer less than, equal to, or 
 * greater
 * than  zero  if  the first argument is considered to be respectively
 * less than, equal to, or greater than the second.  If two members compare
 * as equal,  their order in the sorted array is undefined.
 */
SB_DECLARE(int) hangul_strncmp(unsigned char *str1, unsigned char *str2, int size);

#endif
