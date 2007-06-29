/* $Id$ */
#define CORE_PRIVATE 1
#include "common_core.h"
#include "util.h"
#include <string.h>
#include <time.h>
#include <ctype.h>

const char *sb_get_server_version(void)
{
	return PACKAGE_VERSION;
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


char* sb_trim(char* s)
{
    return sb_right_trim(sb_left_trim(s));
}

char* sb_left_trim(char* s)
{
    int i = 0;
    int len = 0;

    if(s == NULL) return s;

    len = strlen(s);

    for(i = 0; i < len; i++) {
        if(*s == ' ' || *s == '\n' ||
           *s == '\r' || *s == '\t')
            s++;
        else break;
    }

    return s;
}

char* sb_right_trim(char* s)
{
    int i = 0;
    int len = 0;

    if(s == NULL) return s;

    len = strlen(s);

    for(i = len-1; i >= 0; i--) {
        if(*(s+i) == ' ' || *(s+i) == '\n' ||
           *(s+i) == '\r' || *(s+i) == '\t')
            *(s+i) = '\0';
        else break;
    }

    return s;
}

char *replace(char *str, char s, char d) {
    char *ch;
    ch = str;
    while ( (ch = strchr(ch, s)) != NULL ) {
        *ch = d;
    }
    return str;
}

/* XXX: not thread safe */
char* get_time(const char* format) {
    static char strtime[128];
	time_t now;
    struct tm *tm;

	time(&now);
    tm = localtime(&now);

    strftime(strtime, 128, format, tm);

    return strtime;
}

/*http://sourceware.org/ml/libc-alpha/2004-04/msg00106.html*/
char* __strcasestr (const char* phaystack, const char* pneedle)
{
  register const unsigned char *haystack, *needle;
  register char bl, bu, cl, cu;

  haystack = (const unsigned char *) phaystack;
  needle = (const unsigned char *) pneedle;

  bl = tolower (*needle);
  if (bl != '\0')
    {
      bu = toupper (bl);
      haystack--;                               /* possible ANSI violation */
      do
        {
          cl = *++haystack;
          if (cl == '\0')
            goto ret0;
        }
      while ((cl != bl) && (cl != bu));

      cl = tolower (*++needle);
      if (cl == '\0')
        goto foundneedle;
      cu = toupper (cl);
      ++needle;
      goto jin;

      for (;;)
        {
          register char a;
          register const unsigned char *rhaystack, *rneedle;

          do
            {
              a = *++haystack;
              if (a == '\0')
                goto ret0;
              if ((a == bl) || (a == bu))
                break;
              a = *++haystack;
              if (a == '\0')
                goto ret0;
shloop:
              ;
            }
          while ((a != bl) && (a != bu));

jin:      a = *++haystack;
          if (a == '\0')
            goto ret0;

          if ((a != cl) && (a != cu))
            goto shloop;

          rhaystack = haystack-- + 1;
          rneedle = needle;
          a = tolower (*rneedle);

          if (tolower (*rhaystack) == (int) a)
            do
              {
                if (a == '\0')
                  goto foundneedle;
                ++rhaystack;
                a = tolower (*++needle);
                if (tolower (*rhaystack) != (int) a)
                  break;
                if (a == '\0')
                  goto foundneedle;
                ++rhaystack;
                a = tolower (*++needle);
              }
            while (tolower (*rhaystack) == (int) a);

          needle = rneedle;             /* took the register-poor approach */

          if (a == '\0')
            break;
        }
    }
foundneedle:
  return (char*) haystack;
ret0:
  return 0;
}

