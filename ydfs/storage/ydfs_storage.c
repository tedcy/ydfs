#include <stdio.h>

#include "storage_global.h"
#include "storage_net.h"
#include "storage_config.h"
#include "debug.h"

int main(int argc,char *argv[])
{
	init_debug();

	if(load_storage_config() < 0)
		return -1;

	init_storage_service();

	create_storage_service();

	run_storage_service();
	return 0;
}
