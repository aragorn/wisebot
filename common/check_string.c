/* $Id$ */
#include "common_core.h"
#include "hangul.h"
#include <string.h>

char *haystacks[] =
{ "a quick brown fox jumps over a lazy dog.",
   "�ѱ� ���ڿ��Դϴ�.",
   "abc def �ѱ� ���ڿ��Դϴ�.",
   "abc �ѱ�abc ���ڿ��Դϴ�.",
   NULL
};

char *needles[] =
{ "jump",
  "a",
  "�ѱ�",
  "abc",
  "123",
  "�ѱ�abc",
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
