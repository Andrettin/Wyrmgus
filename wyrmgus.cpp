#include "wyrmgus.h"

#include "stratagus.h"

#include "achievement.h"
#include "campaign.h"
#include "hair_color.h"
#include "player.h"
#include "player_color.h"
#include "script.h"
#include "skin_color.h"
#include "unit/unit_type.h"
#include "src/include/version.h"

Wyrmgus *Wyrmgus::Instance = nullptr;

Wyrmgus *Wyrmgus::GetInstance()
{
	return Wyrmgus::Instance;
}

int Wyrmgus::Run()
{
    int default_argc = 1;
    char *default_argv = "Wyrmsun";
	
	Wyrmgus::Instance = this;
	
	return stratagusMain(default_argc, &default_argv);
}

String Wyrmgus::GetVersion() const
{
	return _version_str2;
}

void Wyrmgus::LuaCommand(String command)
{
	QueueLuaCommand(command.utf8().get_data());
}

CHairColor *Wyrmgus::GetHairColor(String ident) const
{
	return CHairColor::GetHairColor(ident.utf8().get_data());
}

CPlayerColor *Wyrmgus::GetPlayerColor(String ident) const
{
	return CPlayerColor::GetPlayerColor(ident.utf8().get_data());
}

CSkinColor *Wyrmgus::GetSkinColor(String ident) const
{
	return CSkinColor::GetSkinColor(ident.utf8().get_data());
}

CCampaign *Wyrmgus::GetCampaign(String ident) const
{
	return CCampaign::GetCampaign(ident.utf8().get_data());
}

Array Wyrmgus::GetCampaigns() const
{
	Array campaigns;
	
	for (CCampaign *campaign : CCampaign::GetCampaigns()) {
		campaigns.push_back(campaign);
	}
	
	return campaigns;
}

void Wyrmgus::SetCurrentCampaign(String campaign_ident)
{
	CCampaign::SetCurrentCampaign(this->GetCampaign(campaign_ident));
}

CCampaign *Wyrmgus::GetCurrentCampaign() const
{
	return CCampaign::GetCurrentCampaign();
}

Array Wyrmgus::GetAchievements() const
{
	Array achievements;
	
	for (CAchievement *achievement : CAchievement::GetAchievements()) {
		achievements.push_back(achievement);
	}
	
	return achievements;
}

Array Wyrmgus::GetUnitUnitTypes() const
{
	Array unit_types;
	
	for (CUnitType *unit_type : CUnitType::GetUnitUnitTypes()) {
		unit_types.push_back(unit_type);
	}
	
	return unit_types;
}

Array Wyrmgus::GetBuildingUnitTypes() const
{
	Array unit_types;
	
	for (CUnitType *unit_type : CUnitType::GetBuildingUnitTypes()) {
		unit_types.push_back(unit_type);
	}
	
	return unit_types;
}

Array Wyrmgus::GetItemUnitTypes() const
{
	Array unit_types;
	
	for (CUnitType *unit_type : CUnitType::GetItemUnitTypes()) {
		unit_types.push_back(unit_type);
	}
	
	return unit_types;
}

CPlayer *Wyrmgus::GetThisPlayer() const
{
	return CPlayer::GetThisPlayer();
}

void Wyrmgus::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &Wyrmgus::Run);
	ClassDB::bind_method(D_METHOD("get_version"), &Wyrmgus::GetVersion);
	ClassDB::bind_method(D_METHOD("lua_command", "command"), &Wyrmgus::LuaCommand);
	
	ClassDB::bind_method(D_METHOD("get_hair_color", "ident"), &Wyrmgus::GetHairColor);
	ClassDB::bind_method(D_METHOD("get_player_color", "ident"), &Wyrmgus::GetPlayerColor);
	ClassDB::bind_method(D_METHOD("get_skin_color", "ident"), &Wyrmgus::GetSkinColor);
	
	ClassDB::bind_method(D_METHOD("get_campaign", "ident"), &Wyrmgus::GetCampaign);
	ClassDB::bind_method(D_METHOD("get_campaigns"), &Wyrmgus::GetCampaigns);
	ClassDB::bind_method(D_METHOD("set_current_campaign", "campaign"), &Wyrmgus::SetCurrentCampaign);
	ClassDB::bind_method(D_METHOD("get_current_campaign"), &Wyrmgus::GetCurrentCampaign);
	
	ClassDB::bind_method(D_METHOD("get_achievements"), &Wyrmgus::GetAchievements);
	
	ClassDB::bind_method(D_METHOD("get_unit_unit_types"), &Wyrmgus::GetUnitUnitTypes);
	ClassDB::bind_method(D_METHOD("get_building_unit_types"), &Wyrmgus::GetBuildingUnitTypes);
	ClassDB::bind_method(D_METHOD("get_item_unit_types"), &Wyrmgus::GetItemUnitTypes);
	
	ClassDB::bind_method(D_METHOD("get_this_player"), &Wyrmgus::GetThisPlayer);
	
	ADD_SIGNAL(MethodInfo("this_player_changed", PropertyInfo(Variant::OBJECT, "old_player"), PropertyInfo(Variant::OBJECT, "new_player")));
	ADD_SIGNAL(MethodInfo("interface_changed", PropertyInfo(Variant::STRING, "old_interface"), PropertyInfo(Variant::STRING, "new_interface")));
}
