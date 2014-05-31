#include <stdio.h>
#include <stdlib.h>

#include "pfm_trigger_lib.h"

int main(int argc, char ** argv)
{
	int i;
	int m = 1000;
	int n = 20;
	int l = atoi(argv[1]);

	pfm_user_trigger_init();
	pfm_user_trigger_enable_mon(1);
	for(i = 0; i< l; i++){
		m += i + n;
	}
	pfm_user_trigger_enable_mon(0);	

	for(i = 0; i< l; i++){
		m += i + n;
	}

	pfm_user_trigger_cleanup(0);

	printf("m is %d\n", m);

	return 0;
}
