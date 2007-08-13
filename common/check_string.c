/* $Id$ */
#include "common_core.h"
#include "hangul.h"
#include <string.h>

char *haystacks[] =
{ "a quick brown fox jumps over a lazy dog.",
   "한글 문자열입니다.",
   "abc def 한글 문자열입니다.",
   "abc 한글abc 문자열입니다.",
   NULL
};

char *needles[] =
{ "jump",
  "a",
  "한글",
  "abc",
  "123",
  "한글abc",
  NULL
};

int main(int argc, char *argv[])
{
	int i = 0;
	char *str;

	for (str=haystacks[i]; str != NULL; str=haystacks[++i])
	{
		int j = 0;
		char *n;

		printf("%s\n", str);
		for (n=needles[j]; n != NULL; n=needles[++j])
		{
			char *f1 = __strcasestr(str, n);
			char *f2 = strcasestrh(str, n);
			if (f1 != f2)
				printf("  ERROR [%s] strcasestr[%s] strcasestrh[%s]\n", n, f1, f2);
			else if (f1 == NULL)
				printf("  NOT FOUND [%s]\n", n);
			else
				printf("  FOUND [%s]\n", n);
		}
	}

	return 0;
}
