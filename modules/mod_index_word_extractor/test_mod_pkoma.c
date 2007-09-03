/* $Id$ */
#include <stdio.h>
extern int test_mod_pkoma();

int main(void)
{
#ifdef MOD_PKOMA_ENABLED
	test_mod_pkoma();
#else
	printf("mod_pkoma is not enabled.\n");
#endif

	return 0;
}
