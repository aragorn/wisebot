#include "mod_vrm.h"

int main(int argc, char* argv[])
{
    vrm_t vrm;	
	vrm_open("dat/test_vrm", &vrm); 
	vrm_close(&vrm);

	return 0;
}
