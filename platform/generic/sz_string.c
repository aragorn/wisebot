/* $Id$ */
#include "softbot.h"
#include "sz_string.h"

char *sz_strncat(char *dest, const char *src, size_t n)
{
	char *buf=NULL;

	if (n < 0) {
		warn("n is %d. setting it 0", (int)n);
		n = 0;
	}

	if (strlen(src) >= n) {
		int orig_len = strlen(dest);
		int n2=0;

		n2 = n-1;
		buf = strncat(dest, src, n2);
		dest[orig_len+n-1] = '\0';
	}
	else {
		buf = strncat(dest, src, n);
	}


	return buf;
}

char *sz_strncpy(char *dest, const char *src, size_t n)
{
	char *buf=NULL;

	if (n < 0) {
		warn("n is %d. setting it 0", (int)n);
		n = 0;
	}

	buf = strncpy(dest, src, n);
	dest[n-1] = '\0';

	return buf;
}

int sz_snprintf(char *str, size_t size, const char *format, ...)
{
	int rv=0;
	va_list args;

	if (size < 0) {
		warn("size is %d. setting it 0", (int)size);
		size = 0;
	}

	va_start(args, format);
	rv = vsnprintf(str, size, format, args);
	va_end(args);

	str[size-1] = '\0';

	return rv;
}
