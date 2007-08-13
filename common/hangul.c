/* $Id$ */
#define CORE_PRIVATE 1
#include "common_core.h"
#include "hangul.h"

/* http://sourceware.org/ml/libc-alpha/2004-04/msg00106.html */

static char toupperh(const char c)
{
	if (97 <= c && c <= 122) return c - 97 + 65;
	else return c;
}

static char tolowerh(const char c)
{
	if (65 <= c && c <= 90) return c + 97 - 65;
	else return c;
}

char* strcasestrh (const char* phaystack, const char* pneedle)
{
  register const unsigned char *haystack, *needle;
  register char bl, bu, cl, cu;

  haystack = (const unsigned char *) phaystack;
  needle = (const unsigned char *) pneedle;

  bl = tolowerh (*needle);
  if (bl != '\0')
    {
      bu = toupperh (bl);
      haystack--;                               /* possible ANSI violation */
      do
        {
          cl = *++haystack;
          if (cl == '\0')
            goto ret0;
        }
      while ((cl != bl) && (cl != bu));

      cl = tolowerh (*++needle);
      if (cl == '\0')
        goto foundneedle;
      cu = toupperh (cl);
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
          a = tolowerh (*rneedle);

          if (tolowerh (*rhaystack) == (int) a)
            do
              {
                if (a == '\0')
                  goto foundneedle;
                ++rhaystack;
                a = tolowerh (*++needle);
                if (tolowerh (*rhaystack) != (int) a)
                  break;
                if (a == '\0')
                  goto foundneedle;
                ++rhaystack;
                a = tolowerh (*++needle);
              }
            while (tolowerh (*rhaystack) == (int) a);

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

