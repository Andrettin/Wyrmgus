#include "wyrmgus_module.h"

#include "stratagus.h"

int WyrmgusModule::Run()
{
    int default_argc = 1;
    char *default_argv = "Wyrmsun";
	return stratagusMain(default_argc, &default_argv);
}

void WyrmgusModule::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &WyrmgusModule::Run);
}
