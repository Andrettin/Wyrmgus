/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "wyrmgus.h"

#include "stratagus.h"

#include "character.h"
#include "civilization.h"
#include "hair_color.h"
#include "language/word.h"
#include "literary_text.h"
#include "player.h"
#include "player_color.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "script.h"
#include "skin_color.h"
#include "species/gender.h"
#include "ui/icon.h"
#include "unit/unit_type.h"
#include "util.h"
#include "src/include/version.h"
#include "world/plane.h"
#include "world/world.h"

#include <oamlGodotModule/oamlGodotModule.h>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

Wyrmgus *Wyrmgus::Instance = nullptr;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

Wyrmgus *Wyrmgus::GetInstance()
{
	return Wyrmgus::Instance;
}

void Wyrmgus::Run()
{
    int default_argc = 1;
    char *default_argv = "Wyrmsun";
	
	Wyrmgus::Instance = this;
	
	stratagusMain(default_argc, &default_argv);
}

String Wyrmgus::GetVersion() const
{
	return _version_str2;
}

void Wyrmgus::LuaCommand(const String command)
{
	QueueLuaCommand(command.utf8().get_data());
}

void Wyrmgus::SetOamlModule(Node *oaml_module)
{
	this->OamlModule = static_cast<oamlGodotModule *>(oaml_module);
}

String Wyrmgus::NumberToRomanNumeral(const unsigned number) const
{
	return ::NumberToRomanNumeral(number);
}

void Wyrmgus::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("run"), &Wyrmgus::Run);
	ClassDB::bind_method(D_METHOD("get_version"), &Wyrmgus::GetVersion);
	ClassDB::bind_method(D_METHOD("lua_command", "command"), &Wyrmgus::LuaCommand);
	
	ClassDB::bind_method(D_METHOD("set_oaml_module", "oaml_module"), &Wyrmgus::SetOamlModule);
	
	ClassDB::bind_method(D_METHOD("set_user_directory", "user_directory"), [](Wyrmgus *wyrmgus, const String &user_directory){ wyrmgus->UserDirectory = user_directory; });
	ClassDB::bind_method(D_METHOD("get_user_directory"), &Wyrmgus::GetUserDirectory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "user_directory"), "set_user_directory", "get_user_directory");
	
	ClassDB::bind_method(D_METHOD("get_hair_color", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CHairColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_player_color", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CPlayerColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_skin_color", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CSkinColor::Get(ident); });
	
	ClassDB::bind_method(D_METHOD("get_icon", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CIcon::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_icons"),[](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CIcon::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_civilization", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CCivilization::Get(ident); });
	
	ClassDB::bind_method(D_METHOD("get_campaign", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CCampaign::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_campaigns"),[](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CCampaign::GetAll()); });
	ClassDB::bind_method(D_METHOD("set_current_campaign", "campaign"), [](const Wyrmgus *wyrmgus, const String &ident){ CCampaign::SetCurrentCampaign(CCampaign::Get(ident)); });
	ClassDB::bind_method(D_METHOD("get_current_campaign"), [](const Wyrmgus *wyrmgus){ return CCampaign::GetCurrentCampaign(); });
	
	ClassDB::bind_method(D_METHOD("get_achievements"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CAchievement::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_unit_type", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CUnitType::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_unit_unit_types"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CUnitType::GetUnitUnitTypes()); });
	ClassDB::bind_method(D_METHOD("get_building_unit_types"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CUnitType::GetBuildingUnitTypes()); });
	ClassDB::bind_method(D_METHOD("get_item_unit_types"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CUnitType::GetItemUnitTypes()); });
	
	ClassDB::bind_method(D_METHOD("get_character", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CCharacter::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_characters"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CCharacter::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_genders"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CGender::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_this_player"), [](const Wyrmgus *wyrmgus){ return CPlayer::GetThisPlayer(); });
	
	ClassDB::bind_method(D_METHOD("get_literary_text", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CLiteraryText::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_literary_texts"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CLiteraryText::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_plane", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CPlane::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_planes"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CPlane::GetAll()); });
	ClassDB::bind_method(D_METHOD("get_world", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CWorld::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_worlds"), [](const Wyrmgus *wyrmgus){ return VectorToGodotArray(CWorld::GetAll()); });

	ClassDB::bind_method(D_METHOD("get_word", "ident"), [](const Wyrmgus *wyrmgus, const String &ident){ return CWord::Get(ident); });

	ClassDB::bind_method(D_METHOD("number_to_roman_numeral", "number"), &Wyrmgus::NumberToRomanNumeral);
	
	ADD_SIGNAL(MethodInfo("initialized"));
	ADD_SIGNAL(MethodInfo("this_player_changed", PropertyInfo(Variant::OBJECT, "old_player"), PropertyInfo(Variant::OBJECT, "new_player")));
	ADD_SIGNAL(MethodInfo("interface_changed", PropertyInfo(Variant::STRING, "old_interface"), PropertyInfo(Variant::STRING, "new_interface")));
	
	// this signal occurs when the time of day of the map layer being currently seen by the player has changed
	ADD_SIGNAL(MethodInfo("time_of_day_changed", PropertyInfo(Variant::OBJECT, "old_time_of_day"), PropertyInfo(Variant::OBJECT, "new_time_of_day")));
	
	// this signal occurs when a unit owned by the player has been hit
	ADD_SIGNAL(MethodInfo("unit_hit"));
}
