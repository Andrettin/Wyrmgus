#include "wyrmgus_module.h"

#include "stratagus.h"

//#include "campaign.h"
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

/*
Vector<CCampaign *> WyrmgusModule::GetCampaigns()
{
	Vector<CCampaign *> campaigns;
	
	for (CCampaign *campaign : CCampaign::GetCampaigns()) {
		campaigns.push_back(campaign);
	}
	
	return campaigns;
}
*/

void WyrmgusModule::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &WyrmgusModule::Run);
	ClassDB::bind_method(D_METHOD("get_version"), &WyrmgusModule::GetVersion);
//	ClassDB::bind_method(D_METHOD("get_campaigns"), &WyrmgusModule::GetCampaigns);
}
