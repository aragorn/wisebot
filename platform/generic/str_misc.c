/* $Id$ */
#include "softbot.h"
#include <ctype.h>

char *strtoupper(char *str)
{
	char *tmp = str;
	for ( ; *tmp; tmp++) {
		*tmp = toupper(*tmp);
	}
	return str;
}

char *strtolower(char *str)
{
	char *tmp = str;
	for ( ; *tmp; tmp++) {
		*tmp = tolower(*tmp);
	}
	return str;
}

char *strntoupper(char *str, int size)
{
	char *tmp = str;
	int i=0;
	for (i=0; *tmp && i<size; tmp++, i++) {
		*tmp = toupper(*tmp);
	}
	return str;
}

char *strntolower(char *str, int size)
{
	char *tmp = str;
	int i=0;
	for (i=0; *tmp && i<size; tmp++, i++) {
		*tmp = tolower(*tmp);
	}
	return str;
}

/*
 * NOTICE:
 * The comparison function must return an integer less than, equal to, or 
 * greater
 * than  zero  if  the first argument is considered to be respectively
 * less than, equal to, or greater than the second.  If two members compare
 * as equal,  their order in the sorted array is undefined.
 */
#define _IS_KOREAN(c)	((c >= 0xb0) && (c <= 0xc8))
int hangul_strncmp(unsigned char *str1, unsigned char *str2, int size)
{
	int i, diff;
	int len1, len2;
	len1 = strlen(str1);
	len2 = strlen(str2);
	if (len1 == 0 && len2 != 0) {
		return 1;
	}
	else if (len1 != 0 && len2 == 0) {
		return -1;
	}
	else if (len1 == 0 && len2 == 0) {
		return 0;
	}

	for (i=0; i<size; ) {
		if (str1[i] && !str2[i])
			return 1;
		if (!str1[i] && str2[i])
			return -1;
		if (!str1[i] && !str2[i])
			return 0;

		if (_IS_KOREAN(str1[i]) && _IS_KOREAN(str2[i])) {
			if (i + 1 == size) {
				return str1[i] - str2[i];
			}
			diff = strncmp(str1 + i, str2 + i, 2);
			if (diff) return diff;
			i += 2;
		}
		else if (_IS_KOREAN(str1[i])) {
			return -1;
		}
		else if (_IS_KOREAN(str2[i])) {
			return 1;
		}
		else {
			diff = str1[i] - str2[i];
			if (diff) return diff;
			i += 1;
		}
	}
	return 0;
}

