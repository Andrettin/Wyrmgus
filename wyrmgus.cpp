/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "wyrmgus.h"

#include "stratagus.h"

#include "character.h"
#include "civilization.h"
#include "faction.h"
#include "game/game.h"
#include "hair_color.h"
#include "map/map.h"
#include "language/word.h"
#include "literary_text.h"
#include "map/terrain_type.h"
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
	
	ClassDB::bind_method(D_METHOD("set_user_directory", "user_directory"), +[](Wyrmgus *wyrmgus, const String &user_directory){ wyrmgus->UserDirectory = user_directory; });
	ClassDB::bind_method(D_METHOD("get_user_directory"), &Wyrmgus::GetUserDirectory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "user_directory"), "set_user_directory", "get_user_directory");
	
	ClassDB::bind_method(D_METHOD("get_hair_color", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CHairColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_player_color", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CPlayerColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_skin_color", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CSkinColor::Get(ident); });
	
	ClassDB::bind_method(D_METHOD("get_icon", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CIcon::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_icons"),+[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CIcon::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_civilization", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CCivilization::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_civilizations"),+[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CCivilization::GetAll()); });

	ClassDB::bind_method(D_METHOD("get_faction", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CFaction::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_factions"),+[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CFaction::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_campaign", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CCampaign::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_campaigns"),+[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CCampaign::GetAll()); });
	ClassDB::bind_method(D_METHOD("set_current_campaign", "campaign"), +[](const Wyrmgus *wyrmgus, const String &ident){ CCampaign::SetCurrentCampaign(CCampaign::Get(ident)); });
	ClassDB::bind_method(D_METHOD("get_current_campaign"), +[](const Wyrmgus *wyrmgus){ return CCampaign::GetCurrentCampaign(); });
	
	ClassDB::bind_method(D_METHOD("get_achievements"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CAchievement::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_terrain_types"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CTerrainType::GetAll()); });

	ClassDB::bind_method(D_METHOD("get_unit_type", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CUnitType::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_unit_unit_types"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CUnitType::GetUnitUnitTypes()); });
	ClassDB::bind_method(D_METHOD("get_building_unit_types"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CUnitType::GetBuildingUnitTypes()); });
	ClassDB::bind_method(D_METHOD("get_item_unit_types"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CUnitType::GetItemUnitTypes()); });
	
	ClassDB::bind_method(D_METHOD("get_character", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CCharacter::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_characters"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CCharacter::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_genders"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CGender::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_this_player"), +[](const Wyrmgus *wyrmgus){ return CPlayer::GetThisPlayer(); });
	
	ClassDB::bind_method(D_METHOD("get_literary_text", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CLiteraryText::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_literary_texts"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CLiteraryText::GetAll()); });
	
	ClassDB::bind_method(D_METHOD("get_plane", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CPlane::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_planes"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CPlane::GetAll()); });
	ClassDB::bind_method(D_METHOD("get_world", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CWorld::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_worlds"), +[](const Wyrmgus *wyrmgus){ return ContainerToGodotArray(CWorld::GetAll()); });

	ClassDB::bind_method(D_METHOD("get_word", "ident"), +[](const Wyrmgus *wyrmgus, const String &ident){ return CWord::Get(ident); });

	ClassDB::bind_method(D_METHOD("number_to_roman_numeral", "number"), &Wyrmgus::NumberToRomanNumeral);
	
	ClassDB::bind_method(D_METHOD("set_game_paused", "paused"), +[](const Wyrmgus *wyrmgus, const bool paused){ SetGamePaused(paused); });
	ClassDB::bind_method(D_METHOD("is_game_paused"), +[](const Wyrmgus *wyrmgus){ return IsGamePaused(); });
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "game_paused"), "set_game_paused", "is_game_paused");
	
	ClassDB::bind_method(D_METHOD("get_pixel_tile_size"), +[](const Wyrmgus *wyrmgus){ return Vector2(CMap::Map.PixelTileSize); });
	
	ADD_SIGNAL(MethodInfo("initialized"));
	ADD_SIGNAL(MethodInfo("this_player_changed", PropertyInfo(Variant::OBJECT, "old_player"), PropertyInfo(Variant::OBJECT, "new_player")));
	ADD_SIGNAL(MethodInfo("interface_changed", PropertyInfo(Variant::STRING, "old_interface"), PropertyInfo(Variant::STRING, "new_interface")));
	
	//this signal occurs when the time of day of the map layer being currently seen by the player has changed
	ADD_SIGNAL(MethodInfo("time_of_day_changed", PropertyInfo(Variant::OBJECT, "old_time_of_day"), PropertyInfo(Variant::OBJECT, "new_time_of_day")));
	
	ADD_SIGNAL(MethodInfo("map_layer_created", PropertyInfo(Variant::OBJECT, "map_layer")));
	ADD_SIGNAL(MethodInfo("current_map_layer_changed", PropertyInfo(Variant::OBJECT, "old_map_layer"), PropertyInfo(Variant::OBJECT, "new_map_layer")));
	ADD_SIGNAL(MethodInfo("map_loaded"));

	//this signal is triggered when any unit is placed on the map, so that its graphics can be depicted
	ADD_SIGNAL(MethodInfo("unit_placed", PropertyInfo(Variant::OBJECT, "unit")));
	//this signal occurs when a unit owned by the player has been hit
	ADD_SIGNAL(MethodInfo("unit_hit"));
	
	ADD_SIGNAL(MethodInfo("dialogue_called", PropertyInfo(Variant::OBJECT, "dialogue")));
}
