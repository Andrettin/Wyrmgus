#include "wyrmgus_module.h"

#include "stratagus.h"

#include "src/include/version.h"

int WyrmgusModule::Run()
{
    int default_argc = 1;
    char *default_argv = "Wyrmsun";
	return stratagusMain(default_argc, &default_argv);
}

String WyrmgusModule::GetVersion()
{
	return _version_str2;
}

void WyrmgusModule::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &WyrmgusModule::Run);
	ClassDB::bind_method(D_METHOD("get_version"), &WyrmgusModule::GetVersion);
}
