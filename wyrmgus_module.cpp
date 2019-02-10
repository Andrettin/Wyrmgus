#include "wyrmgus_module.h"

#include "stratagus.h"

#include "campaign.h"
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

CCampaign *WyrmgusModule::GetCampaign(String ident)
{
	return CCampaign::GetCampaign(ident.utf8().get_data());
}

Array WyrmgusModule::GetCampaigns()
{
	Array campaigns;
	
	for (CCampaign *campaign : CCampaign::GetCampaigns()) {
		campaigns.push_back(campaign);
	}
	
	return campaigns;
}

void WyrmgusModule::SetCurrentCampaign(String campaign_ident)
{
	CCampaign::SetCurrentCampaign(this->GetCampaign(campaign_ident));
}

CCampaign *WyrmgusModule::GetCurrentCampaign()
{
	return CCampaign::GetCurrentCampaign();
}

void WyrmgusModule::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &WyrmgusModule::Run);
	ClassDB::bind_method(D_METHOD("get_version"), &WyrmgusModule::GetVersion);
	ClassDB::bind_method(D_METHOD("lua_command", "command"), &WyrmgusModule::LuaCommand);
	ClassDB::bind_method(D_METHOD("get_campaign", "ident"), &WyrmgusModule::GetCampaign);
	ClassDB::bind_method(D_METHOD("get_campaigns"), &WyrmgusModule::GetCampaigns);
	ClassDB::bind_method(D_METHOD("set_current_campaign", "campaign"), &WyrmgusModule::SetCurrentCampaign);
	ClassDB::bind_method(D_METHOD("get_current_campaign"), &WyrmgusModule::GetCurrentCampaign);
}
