/* $Id$ */
#include "softbot.h"
#include "util.h"

#if USE_APR==1
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

/*****************************************************************************/
char *ap_getword_white(apr_pool_t *p, const char **line)
{
    const char *pos = *line;
    int len;
    char *res;

    while (!apr_isspace(*pos) && *pos) {
        ++pos;
    }

    len = pos - *line;
    res = (char *)apr_palloc(p, len + 1);
    memcpy(res, *line, len);
    res[len] = 0;

    while (apr_isspace(*pos)) {
        ++pos;
    }

    *line = pos;

    return res;
}
#endif /* USE_APR==1 */

/*****************************************************************************/
const char *sb_get_server_version(void)
{
	return VERSION;
}

int sb_server_root_relative(char *buf, const char *filename)
{
	if (buf == NULL || filename == NULL)
		return FAIL;

	/* length of buf should be longer than MAX_PATH_LEN */
	if (filename[0] == '/')
		snprintf(buf, MAX_PATH_LEN, "%s", filename);
	else
		snprintf(buf, MAX_PATH_LEN, "%s/%s", gSoftBotRoot, filename);

	return SUCCESS;
}

/* XXX: not thread safe */
char* sb_strbin(uint32_t number, int size)
{
    static char strbin[64+1];
    int bitsize=size*8;
    int i=0;
    for (i=0; i<bitsize; i++) {
        strbin[i] = "01"[(number >> (bitsize-i-1))&1];
    }
    strbin[i]='\0';

    return strbin;
}


