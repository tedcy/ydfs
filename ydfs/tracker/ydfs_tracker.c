#include <stdio.h>

#include "tracker_global.h"
#include "tracker_net.h"
#include "debug.h"
#include "tracker_config.h"


int main(int argc,char *argv[])
{
	init_debug();

	if(load_tracker_config() < 0)
		return -1;

	init_tracker_service();

	create_tracker_service();

	run_tracker_service();

	return 0;
}
