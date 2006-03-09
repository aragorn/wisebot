/* $Id$ */
#include "util_string.h"

char *sb_escape_html(apr_pool_t *p, const char *s)
{
    int i, j;
    char *x;

    /* first, count the number of extra characters */
    for (i = 0, j = 0; s[i] != '\0'; i++)
	if (s[i] == '<' || s[i] == '>')
	    j += 3;
	else if (s[i] == '&')
	    j += 4;

    if (j == 0)
	return apr_pstrmemdup(p, s, i);

    x = apr_palloc(p, i + j + 1);
    for (i = 0, j = 0; s[i] != '\0'; i++, j++)
	if (s[i] == '<') {
	    memcpy(&x[j], "&lt;", 4);
	    j += 3;
	}
	else if (s[i] == '>') {
	    memcpy(&x[j], "&gt;", 4);
	    j += 3;
	}
	else if (s[i] == '&') {
	    memcpy(&x[j], "&amp;", 5);
	    j += 4;
	}
	else
	    x[j] = s[i];

    x[j] = '\0';
    return x;
}

/*
 * Check for an absoluteURI syntax (see section 3.2 in RFC2068).
 */
int sb_is_url(const char *u)
{
	register int x;

	for (x = 0; u[x] != ':'; x++) {
		if ((!u[x]) ||
			((!apr_isalpha(u[x])) && (!apr_isdigit(u[x])) &&
			 (u[x] != '+') && (u[x] != '-') && (u[x] != '.'))) {
			return 0;
		}
	}

	return (x ? 1 : 0);	/* If the first character is ':', it's broken, too */
}

void sb_str_tolower(char *str)
{
	while (*str) {
		*str = apr_tolower(*str);
		++str;
	}
}

