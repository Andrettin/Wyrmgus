#include "wyrmgus_module.h"

#include "stratagus.h"

#include "campaign.h"
#include "player.h"
#include "script.h"
#include "src/include/version.h"

WyrmgusModule *WyrmgusModule::Instance = nullptr;

WyrmgusModule *WyrmgusModule::GetInstance()
{
	return WyrmgusModule::Instance;
}

int WyrmgusModule::Run()
{
    int default_argc = 1;
    char *default_argv = "Wyrmsun";
	
	WyrmgusModule::Instance = this;
	
	return stratagusMain(default_argc, &default_argv);
}

String WyrmgusModule::GetVersion() const
{
	return _version_str2;
}

void WyrmgusModule::LuaCommand(String command)
{
	QueueLuaCommand(command.utf8().get_data());
}

CCampaign *WyrmgusModule::GetCampaign(String ident) const
{
	return CCampaign::GetCampaign(ident.utf8().get_data());
}

Array WyrmgusModule::GetCampaigns() const
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

CCampaign *WyrmgusModule::GetCurrentCampaign() const
{
	return CCampaign::GetCurrentCampaign();
}

CPlayer *WyrmgusModule::GetThisPlayer() const
{
	return CPlayer::GetThisPlayer();
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
	ClassDB::bind_method(D_METHOD("get_this_player"), &WyrmgusModule::GetThisPlayer);
	
	ADD_SIGNAL(MethodInfo("this_player_changed", PropertyInfo(Variant::OBJECT, "old_player"), PropertyInfo(Variant::OBJECT, "new_player")));
	ADD_SIGNAL(MethodInfo("interface_changed", PropertyInfo(Variant::STRING, "old_interface"), PropertyInfo(Variant::STRING, "new_interface")));
}
