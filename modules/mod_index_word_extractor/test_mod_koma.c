/* $Id$ */
#include <stdio.h>

extern int test_mod_koma();

int main(void)
{
#ifdef MOD_KOMA_ENABLED
	test_mod_koma();
#else
	printf("mod_koma is not enabled.\n");
#endif

	return 0;
}
