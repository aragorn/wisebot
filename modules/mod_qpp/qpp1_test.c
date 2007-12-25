/* $Id$ */
#include "common_core.h"
#include "util.h"
#include "mod_qpp1.h"
#include "qpp1_yacc.h"

int qpp1_yyparse(char *input, int debug);

int main(int argc, char *argv[])
{
	char buffer[STRING_SIZE+1];
	char *input;
	int debug = 0;

	if (argc > 1) debug = 1;

	info("hello, world");

	while (fgets(buffer, STRING_SIZE, stdin) != NULL)
	{
		int n;
		input = sb_trim(buffer);
		info("input[%s]",input);

		init_nodes();
		n = qpp1_yyparse(input, debug);
		print_stack();
		info("result: %d", n);
	}
	info("bye!");

	return 0;
}
