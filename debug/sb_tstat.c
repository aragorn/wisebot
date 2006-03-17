#include "common_core.h"
#include "common_util.h"

char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
module *static_modules[1];

int main () {
	int i;
	tstat_t t;

	sb_tstat_start(&t);
	for (i=0; i<1000000000; i++) {
		if (i == 0) {}
	}
	sb_tstat_finish(&t);

	sb_tstat_print(&t);
	return 0;
}
