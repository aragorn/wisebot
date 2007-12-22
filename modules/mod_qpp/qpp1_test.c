/* $Id$ */
#include "common_core.h"
#include "util.h"
#include "mod_qpp1.h"
#include "qpp1_yacc.h"

extern int qpp1_parse(char *input, int debug);

int main(int argc, char *argv[])
{
	char buffer[STRING_SIZE+1];
	char *input;
	int debug;

	if (argc > 1) debug = 1;

	printf("hello, world\n");
	while (fgets(buffer, STRING_SIZE, stdin) != NULL)
	{
		input = sb_trim(buffer);
		printf("input[%s]\n",input);
		qpp1_parse(input, debug);
	}
	printf("bye!\n");

	return 0;
}
