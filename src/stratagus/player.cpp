//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name player.cpp - The player source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//		and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdarg.h>

#include "stratagus.h"

#include "player.h"

#include "action/action_upgradeto.h"
#include "action/actions.h"
#include "age.h"
#include "ai/ai.h"
//Wyrmgus start
#include "ai/ai_local.h" //for using AiHelpers
#include "civilization.h"
#include "commands.h" //for faction setting
#include "economy/currency.h"
#include "editor/editor.h"
#include "faction.h"
#include "game/game.h"
//Wyrmgus end
//Wyrmgus start
#include "iocompat.h"
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "network/network.h"
#include "network/netconnect.h"
//Wyrmgus start
#include "parameters.h"
#include "player_color.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/religion.h"
#include "settings.h"
//Wyrmgus end
#include "sound/sound.h"
#include "sound/unit_sound.h"
#include "time/calendar.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
#include "unit/unit_type.h"
//Wyrmgus end
#include "upgrade/dependency.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "upgrade/upgrade_modifier.h"
#include "video/font.h"
#include "video/video.h"
#include "world/world.h"
#include "wyrmgus.h"

#include <mutex>

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CPlayer player.h
**
**  \#include "player.h"
**
**  This structure contains all information about a player in game.
**
**  The player structure members:
**
**  CPlayer::Player
**
**    This is the unique slot number. It is not possible that two
**    players have the same slot number at the same time. The slot
**    numbers are reused in the future. This means if a player is
**    defeated, a new player can join using this slot. Currently
**    #PlayerMax (16) players are supported. This member is used to
**    access bit fields.
**    Slot #PlayerNumNeutral (15) is reserved for the neutral units
**    like gold-mines or critters.
**
**    @note Should call this member Slot?
**
**  CPlayer::Name
**
**    Name of the player used for displays and network game.
**    It is restricted to 15 characters plus final zero.
**
**  CPlayer::Type
**
**    Type of the player. This field is setup from the level (map).
**    We support currently #PlayerNeutral,
**    #PlayerNobody, #PlayerComputer, #PlayerPerson,
**    #PlayerRescuePassive and #PlayerRescueActive.
**    @see #PlayerTypes.
**
**  CPlayer::RaceName
**
**    Name of the race to which the player belongs, used to select
**    the user interface and the AI.
**    We have 'orc', 'human', 'alliance' or 'mythical'. Should
**    only be used during configuration and not during runtime.
**
**  CPlayer::Race
**
**    Race number of the player. This field is setup from the level
**    map. This number is mapped with #PlayerRaces to the symbolic
**    name CPlayer::RaceName.
**
**  CPlayer::AiName
**
**    AI name for computer. This field is setup
**    from the map. Used to select the AI for the computer
**    player.
**
**  CPlayer::Team
**
**    Team of player. Selected during network game setup. All players
**    of the same team are allied and enemy to all other teams.
**    @note It is planned to show the team on the map.
**
**  CPlayer::Enemy
**
**    A bit field which contains the enemies of this player.
**    If CPlayer::Enemy & (1<<CPlayer::Player) != 0 its an enemy.
**    Setup during startup using the CPlayer::Team, can later be
**    changed with diplomacy. CPlayer::Enemy and CPlayer::Allied
**    are combined, if none bit is set, the player is neutral.
**    @note You can be allied to a player, which sees you as enemy.
**
**  CPlayer::Allied
**
**    A bit field which contains the allies of this player.
**    If CPlayer::Allied & (1<<CPlayer::Player) != 0 its an allied.
**    Setup during startup using the Player:Team, can later be
**    changed with diplomacy. CPlayer::Enemy and CPlayer::Allied
**    are combined, if none bit is set, the player is neutral.
**    @note You can be allied to a player, which sees you as enemy.
**
**  CPlayer::SharedVision
**
**    A bit field which contains shared vision for this player.
**    Shared vision only works when it's activated both ways. Really.
**
**  CPlayer::StartX CPlayer::StartY
**
**    The tile map coordinates of the player start position. 0,0 is
**    the upper left on the map. This members are setup from the
**    map and only important for the game start.
**    Ignored if game starts with level settings. Used to place
**    the initial workers if you play with 1 or 3 workers.
**
**  CPlayer::Resources[::MaxCosts]
**
**    How many resources the player owns. Needed for building
**    units and structures.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::MaxResources[::MaxCosts]
**
**    How many resources the player can store at the moment.
**
**  CPlayer::Incomes[::MaxCosts]
**
**    Income of the resources, when they are delivered at a store.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::LastResources[::MaxCosts]
**
**    Keeps track of resources in time (used for calculating
**    CPlayer::Revenue, see below)
**
**  CPlayer::Revenue[::MaxCosts]
**
**    Production of resources per minute (or estimates)
**    Used just as information (statistics) for the player...
**
**  CPlayer::UnitTypesCount[::UnitTypeMax]
**
**    Total count for each different unit type. Used by the AI and
**    for dependencies checks. The addition of all counts should
**    be CPlayer::TotalNumUnits.
**    @note Should not use the maximum number of unit-types here,
**    only the real number of unit-types used.
**
**  CPlayer::AiEnabled
**
**    If the player is controlled by the computer and this flag is
**    true, than the player is handled by the AI on this local
**    computer.
**
**    @note Currently the AI is calculated parallel on all computers
**    in a network play. It is planned to change this.
**
**  CPlayer::Ai
**
**    AI structure pointer. Please look at #PlayerAi for more
**    information.
**
**  CPlayer::Units
**
**    A table of all (CPlayer::TotalNumUnits) units of the player.
**
**  CPlayer::TotalNumUnits
**
**    Total number of units (incl. buildings) in the CPlayer::Units
**    table.
**
**  CPlayer::Demand
**
**    Total unit demand, used to demand limit.
**    A player can only build up to CPlayer::Food units and not more
**    than CPlayer::FoodUnitLimit units.
**
**    @note that CPlayer::NumFoodUnits > CPlayer::Food, when enough
**    farms are destroyed.
**
**  CPlayer::NumBuildings
**
**    Total number buildings, units that don't need food.
**
**  CPlayer::Food
**
**    Number of food available/produced. Player can't train more
**    CPlayer::NumFoodUnits than this.
**    @note that all limits are always checked.
**
**  CPlayer::FoodUnitLimit
**
**    Number of food units allowed. Player can't train more
**    CPlayer::NumFoodUnits than this.
**    @note that all limits are always checked.
**
**  CPlayer::BuildingLimit
**
**    Number of buildings allowed.  Player can't build more
**    CPlayer::NumBuildings than this.
**    @note that all limits are always checked.
**
**  CPlayer::TotalUnitLimit
**
**    Number of total units allowed. Player can't have more
**    CPlayer::NumFoodUnits+CPlayer::NumBuildings=CPlayer::TotalNumUnits
**    this.
**    @note that all limits are always checked.
**
**  CPlayer::Score
**
**    Total number of points. You can get points for killing units,
**    destroying buildings ...
**
**  CPlayer::TotalUnits
**
**    Total number of units made.
**
**  CPlayer::TotalBuildings
**
**    Total number of buildings made.
**
**  CPlayer::TotalResources[::MaxCosts]
**
**    Total number of resources collected.
**    @see _costs_, TimeCost, GoldCost, WoodCost, OilCost, MaxCosts.
**
**  CPlayer::TotalRazings
**
**    Total number of buildings destroyed.
**
**  CPlayer::TotalKills
**
**    Total number of kills.
**
**  CPlayer::Color
**
**    Color of units of this player on the minimap. Index number
**    into the global palette.
**
**  CPlayer::UnitColors
**
**    Unit colors of this player. Contains the hardware dependent
**    pixel values for the player colors (palette index #208-#211).
**    Setup from the global palette.
**    @note Index #208-#211 are various SHADES of the team color
**    (#208 is brightest shade, #211 is darkest shade) .... these
**    numbers are NOT red=#208, blue=#209, etc
**
**  CPlayer::Allow
**
**    Contains which unit-types and upgrades are allowed for the
**    player. Possible values are:
**    @li  'A' -- allowed,
**    @li  'F' -- forbidden,
**    @li  'R' -- acquired, perhaps other values
**    @li  'Q' -- acquired but forbidden (does it make sense?:))
**    @li  'E' -- enabled, allowed by level but currently forbidden
**    @see CAllow
**
**  CPlayer::UpgradeTimers
**
**    Timer for the upgrades. One timer for all possible upgrades.
**    Initial 0 counted up by the upgrade action, until it reaches
**    the upgrade time.
**    @see _upgrade_timers_
**    @note it is planned to combine research for faster upgrades.
*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int NumPlayers;							/// How many player slots used

CPlayer *CPlayer::ThisPlayer = nullptr;	/// Player on this computer
std::vector<CPlayer *> CPlayer::Players;	/// All players in play
std::shared_mutex CPlayer::PlayerMutex;

PlayerRace PlayerRaces;					/// Player races

bool NoRescueCheck;						/// Disable rescue check

/**
**  Colors used for minimap.
*/
//Wyrmgus start
//std::vector<CColor> PlayerColorsRGB[PlayerMax];
//std::vector<IntColor> PlayerColors[PlayerMax];

//std::string PlayerColorNames[PlayerMax];
std::vector<CColor> PlayerColorsRGB[PlayerColorMax];
std::vector<IntColor> PlayerColors[PlayerColorMax];
std::string PlayerColorNames[PlayerColorMax];
std::vector<int> ConversiblePlayerColors;
//Wyrmgus end

/**
**  Which indexes to replace with player color
*/
int PlayerColorIndexStart;
int PlayerColorIndexCount;

//Wyrmgus start
std::map<std::string, int> DynastyStringToIndex;
std::map<std::string, CLanguage *> LanguageIdentToPointer;

bool LanguageCacheOutdated = false;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Clean up the PlayerRaces names.
*/
void PlayerRace::Clean()
{
	//Wyrmgus start
	for (CLanguage *language : this->Languages) {
		for (LanguageWord *language_word : language->LanguageWords) {
			for (CLanguage *dialect : language->Dialects) { //remove word from dialects, so that they don't try to delete it too
				dialect->RemoveWord(language_word);
			}
			
			delete language_word;
		}
		language->LanguageWords.clear();
		
		language->NameTranslations.clear();
	}
	//Wyrmgus end
	for (size_t i = 0; i != CCivilization::Civilizations.size(); ++i) {
		this->Name[i].clear();
		//Wyrmgus start
		this->CivilizationUpgrades[i].clear();
		//Wyrmgus end
	}
	//Wyrmgus start
	for (CDynasty *dynasty : this->Dynasties) {
		delete dynasty;
	}
	this->Dynasties.clear();
	//Wyrmgus end
}

//Wyrmgus start
CDynasty *PlayerRace::GetDynasty(const std::string &dynasty_ident) const
{
	if (dynasty_ident.empty()) {
		return nullptr;
	}
	
	if (DynastyStringToIndex.find(dynasty_ident) != DynastyStringToIndex.end()) {
		return PlayerRaces.Dynasties[DynastyStringToIndex[dynasty_ident]];
	} else {
		return nullptr;
	}
}

CLanguage *PlayerRace::GetLanguage(const std::string &language_ident) const
{
	if (LanguageIdentToPointer.find(language_ident) != LanguageIdentToPointer.end()) {
		return LanguageIdentToPointer[language_ident];
	}
	return nullptr;
}

/**
**  "Translate" (that is, adapt) a proper name from one culture (civilization) to another.
*/
std::string PlayerRace::TranslateName(const std::string &name, CLanguage *language)
{
	std::string new_name;
	
	if (!language || name.empty()) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	if (language->NameTranslations.find(name) != language->NameTranslations.end()) {
		return language->NameTranslations[name][SyncRand(language->NameTranslations[name].size())];
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (name.size() > 1) {
		if (name.find(" ") == std::string::npos) {
			for (size_t i = 0; i < name.size(); ++i) {
				std::string name_prefix = name.substr(0, i + 1);
				std::string name_suffix = CapitalizeString(name.substr(i + 1, name.size() - (i + 1)));
			
	//			fprintf(stdout, "Trying to match prefix \"%s\" and suffix \"%s\" for translating name \"%s\" to the \"%s\" language.\n", name_prefix.c_str(), name_suffix.c_str(), name.c_str(), language->Ident.c_str());
			
				if (language->NameTranslations.find(name_prefix) != language->NameTranslations.end() && language->NameTranslations.find(name_suffix) != language->NameTranslations.end()) { // if both a prefix and suffix have been matched
					name_prefix = language->NameTranslations[name_prefix][SyncRand(language->NameTranslations[name_prefix].size())];
					name_suffix = language->NameTranslations[name_suffix][SyncRand(language->NameTranslations[name_suffix].size())];
					name_suffix = DecapitalizeString(name_suffix);
					if (name_prefix.substr(name_prefix.size() - 2, 2) == "gs" && name_suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
						name_prefix = FindAndReplaceStringEnding(name_prefix, "gs", "g");
					}
					if (name_prefix.substr(name_prefix.size() - 1, 1) == "s" && name_suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
						name_prefix = FindAndReplaceStringEnding(name_prefix, "s", "");
					}

					return name_prefix + name_suffix;
				}
			}
		} else { // if the name contains a space, try to translate each of its elements separately
			size_t previous_pos = 0;
			new_name = name;
			for (size_t i = 0; i < name.size(); ++i) {
				if ((i + 1) == name.size() || name[i + 1] == ' ') {
					std::string name_element = TranslateName(name.substr(previous_pos, i + 1 - previous_pos), language);
				
					if (name_element.empty()) {
						new_name = "";
						break;
					}
				
					new_name = FindAndReplaceString(new_name, name.substr(previous_pos, i + 1 - previous_pos), name_element);
				
					previous_pos = i + 2;
				}
			}
		}
	}
	
	return new_name;
}

CDynasty::~CDynasty()
{
	if (this->Conditions) {
		delete Conditions;
	}
}
//Wyrmgus end

/**
**  Init players.
*/
void InitPlayers()
{
	for (size_t p = 0; p < PlayerMax; ++p) {
		CPlayer::Players[p]->Index = p;
		if (!CPlayer::Players[p]->Type) {
			CPlayer::Players[p]->Type = PlayerNobody;
		}
		//Wyrmgus start
//		for (int x = 0; x < PlayerColorIndexCount; ++x) {
//			PlayerColors[p][x] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[p][x]);
//		}
		//Wyrmgus end
	}
	//Wyrmgus start
	for (int p = 0; p < PlayerColorMax; ++p) {
		for (int x = 0; x < PlayerColorIndexCount; ++x) {
			PlayerColors[p][x] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[p][x]);
		}
	}
	//Wyrmgus end
}

/**
**  Clean up players.
*/
void CleanPlayers()
{
	CPlayer::SetThisPlayer(nullptr);
	for (unsigned int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->Clear();
	}
	NumPlayers = 0;
	NoRescueCheck = false;
}

void FreePlayerColors()
{
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->UnitColors.Colors.clear();
		//Wyrmgus start
//		PlayerColorsRGB[i].clear();
//		PlayerColors[i].clear();
		//Wyrmgus end
	}
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		PlayerColorsRGB[i].clear();
		PlayerColors[i].clear();
	}
	//Wyrmgus end
}

/**
**  Save state of players to file.
**
**  @param file  Output file.
**
**  @note FIXME: Not completely saved.
*/
void SavePlayers(CFile &file)
{
	file.printf("\n--------------------------------------------\n");
	file.printf("--- MODULE: players\n\n");

	//  Dump all players
	for (int i = 0; i < NumPlayers; ++i) {
		CPlayer::Players[i]->Save(file);
	}

	file.printf("SetThisPlayer(%i)\n\n", CPlayer::GetThisPlayer()->Index);
}

void CPlayer::SetThisPlayer(CPlayer *player)
{
	CPlayer *old_player = nullptr;
	
	{
		std::unique_lock<std::shared_mutex> lock(PlayerMutex);
		
		old_player = CPlayer::ThisPlayer;
		
		if (old_player == player) {
			return;
		}
		
		CPlayer::ThisPlayer = player;
	}
	
	Wyrmgus::GetInstance()->emit_signal("this_player_changed", old_player, player);
	
	String old_interface = old_player ? old_player->GetInterface() : "";
	String new_interface = player ? player->GetInterface() : "";
	if (new_interface != old_interface) {
		Wyrmgus::GetInstance()->emit_signal("interface_changed", old_interface, new_interface);
	}
}

CPlayer *CPlayer::GetThisPlayer()
{
	std::shared_lock<std::shared_mutex> lock(PlayerMutex);
	
	return CPlayer::ThisPlayer;
}

CPlayer *CPlayer::GetPlayer(const int index)
{
	std::shared_lock<std::shared_mutex> lock(PlayerMutex);
	
	if (index < 0) {
		fprintf(stderr, "Cannot get player for index %i: the index is negative.\n", index);
		return nullptr;
	}
	
	if (index >= PlayerMax) {
		fprintf(stderr, "Cannot get player for index %i: the maximum value is %i.\n", index, PlayerMax);
		return nullptr;
	}
	
	return CPlayer::Players[index];
}

void CPlayer::Save(CFile &file) const
{
	file.printf("Player(%i,\n", this->Index);
	//Wyrmgus start
	file.printf(" \"race\", \"%s\",", PlayerRaces.Name[this->Race].c_str());
	if (this->Faction != nullptr) {
		file.printf(" \"faction\", \"%s\",", this->Faction->GetIdent().utf8().get_data());
	}
	if (this->Dynasty != nullptr) {
		file.printf(" \"dynasty\", \"%s\",", this->Dynasty->Ident.c_str());
	}
	if (this->Age != nullptr) {
		file.printf(" \"age\", \"%s\",", this->Age->Ident.c_str());
	}
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == this->Color) {
			file.printf(" \"color\", %u,", i);
			break;
		}
	}
	//Wyrmgus end
	file.printf("  \"name\", \"%s\",\n", this->Name.c_str());
	file.printf("  \"type\", ");
	switch (this->Type) {
		case PlayerNeutral:       file.printf("\"neutral\",");         break;
		case PlayerNobody:        file.printf("\"nobody\",");          break;
		case PlayerComputer:      file.printf("\"computer\",");        break;
		case PlayerPerson:        file.printf("\"person\",");          break;
		case PlayerRescuePassive: file.printf("\"rescue-passive\","); break;
		case PlayerRescueActive:  file.printf("\"rescue-active\","); break;
		default:                  file.printf("%i,", this->Type); break;
	}
	//Wyrmgus start
//	file.printf(" \"race\", \"%s\",", PlayerRaces.Name[this->Race].c_str());
	//Wyrmgus end
	file.printf(" \"ai-name\", \"%s\",\n", this->AiName.c_str());
	file.printf("  \"team\", %i,", this->Team);

	file.printf(" \"enemy\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (this->Enemy & (1 << j)) ? 'X' : '_');
	}
	file.printf("\", \"allied\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (this->Allied & (1 << j)) ? 'X' : '_');
	}
	file.printf("\", \"shared-vision\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (this->SharedVision & (1 << j)) ? 'X' : '_');
	}
	file.printf("\",\n  \"start\", {%hi, %hi},\n", this->StartPos.x, this->StartPos.y);
	//Wyrmgus start
	file.printf("  \"start-map-layer\", %i,\n", this->StartMapLayer);
	if (this->Overlord) {
		file.printf("  \"overlord\", %i,\n", this->Overlord->Index);
	}
	//Wyrmgus end

	// Resources
	file.printf("  \"resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		//Wyrmgus start
		if (!this->Resources[j]) {
			continue;
		}
		//Wyrmgus end
		file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->Resources[j]);
	}
	// Stored Resources
	file.printf("},\n  \"stored-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		//Wyrmgus start
		if (!this->StoredResources[j]) {
			continue;
		}
		//Wyrmgus end
		file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->StoredResources[j]);
	}
	// Max Resources
	file.printf("},\n  \"max-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->MaxResources[j]);
	}
	// Last Resources
	file.printf("},\n  \"last-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		//Wyrmgus start
		if (!this->LastResources[j]) {
			continue;
		}
		//Wyrmgus end
		file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->LastResources[j]);
	}
	// Incomes
	file.printf("},\n  \"incomes\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			if (j == MaxCosts / 2) {
				file.printf("\n ");
			} else {
				file.printf(" ");
			}
		}
		file.printf("\"%s\", %i,", DefaultResourceNames[j].c_str(), this->Incomes[j]);
	}
	// Revenue
	file.printf("},\n  \"revenue\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		//Wyrmgus start
//		if (j) {
//			if (j == MaxCosts / 2) {
//				file.printf("\n ");
//			} else {
//				file.printf(" ");
//			}
//		}
//		file.printf("\"%s\", %i,", DefaultResourceNames[j].c_str(), this->Revenue[j]);
		if (this->Revenue[j]) {
			file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->Revenue[j]);
		}
		//Wyrmgus end
	}
	
	//Wyrmgus start
	file.printf("},\n  \"prices\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (this->Prices[j]) {
			file.printf("\"%s\", %i, ", DefaultResourceNames[j].c_str(), this->Prices[j]);
		}
	}
	//Wyrmgus end

	// UnitTypesCount done by load units.

	file.printf("},\n  \"%s\",\n", this->AiEnabled ? "ai-enabled" : "ai-disabled");

	// Ai done by load ais.
	// Units done by load units.
	// TotalNumUnits done by load units.
	// NumBuildings done by load units.
	
	//Wyrmgus start
	if (this->Revealed) {
		file.printf(" \"revealed\",");
	}
	//Wyrmgus end
	
	file.printf(" \"supply\", %i,", this->Supply);
	file.printf(" \"trade-cost\", %i,", this->TradeCost);
	file.printf(" \"unit-limit\", %i,", this->UnitLimit);
	file.printf(" \"building-limit\", %i,", this->BuildingLimit);
	file.printf(" \"total-unit-limit\", %i,", this->TotalUnitLimit);

	file.printf("\n  \"score\", %i,", this->Score);
	file.printf("\n  \"total-units\", %i,", this->TotalUnits);
	file.printf("\n  \"total-buildings\", %i,", this->TotalBuildings);
	file.printf("\n  \"total-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%i,", this->TotalResources[j]);
	}
	file.printf("},");
	file.printf("\n  \"total-razings\", %i,", this->TotalRazings);
	file.printf("\n  \"total-kills\", %i,", this->TotalKills);
	//Wyrmgus start
	file.printf("\n  \"unit-type-kills\", {");
	for (size_t i = 0; i < CUnitType::UnitTypes.size(); ++i) {
		if (this->UnitTypeKills[i] != 0) {
			file.printf("\"%s\", %i, ", CUnitType::UnitTypes[i]->Ident.c_str(), this->UnitTypeKills[i]);
		}
	}
	file.printf("},");
	//Wyrmgus end
	if (this->LostTownHallTimer != 0) {
		file.printf("\n  \"lost-town-hall-timer\", %i,", this->LostTownHallTimer);
	}
	if (this->HeroCooldownTimer != 0) {
		file.printf("\n  \"hero-cooldown-timer\", %i,", this->HeroCooldownTimer);
	}
	//Wyrmgus end

	file.printf("\n  \"speed-resource-harvest\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%i,", this->SpeedResourcesHarvest[j]);
	}
	file.printf("},");
	file.printf("\n  \"speed-resource-return\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%i,", this->SpeedResourcesReturn[j]);
	}
	file.printf("},");
	file.printf("\n  \"speed-build\", %i,", this->SpeedBuild);
	file.printf("\n  \"speed-train\", %i,", this->SpeedTrain);
	file.printf("\n  \"speed-upgrade\", %i,", this->SpeedUpgrade);
	file.printf("\n  \"speed-research\", %i,", this->SpeedResearch);
	
	//Wyrmgus start
	/*
	Uint8 r, g, b;

	SDL_GetRGB(this->Color, TheScreen->format, &r, &g, &b);
	file.printf("\n  \"color\", { %hhu, %hhu, %hhu },", r, g, b);
	*/
	//Wyrmgus end

	//Wyrmgus start
	file.printf("\n  \"current-quests\", {");
	for (size_t j = 0; j < this->CurrentQuests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", this->CurrentQuests[j]->Ident.c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"completed-quests\", {");
	for (size_t j = 0; j < this->CompletedQuests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", this->CompletedQuests[j]->Ident.c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"quest-objectives\", {");
	for (size_t j = 0; j < this->QuestObjectives.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("{");
		file.printf("\"quest\", \"%s\",", this->QuestObjectives[j]->Quest->Ident.c_str());
		file.printf("\"objective-type\", \"%s\",", GetQuestObjectiveTypeNameById(this->QuestObjectives[j]->ObjectiveType).c_str());
		file.printf("\"objective-string\", \"%s\",", this->QuestObjectives[j]->ObjectiveString.c_str());
		file.printf("\"quantity\", %i,", this->QuestObjectives[j]->Quantity);
		file.printf("\"counter\", %i,", this->QuestObjectives[j]->Counter);
		if (this->QuestObjectives[j]->Resource != -1) {
			file.printf("\"resource\", \"%s\",", DefaultResourceNames[this->QuestObjectives[j]->Resource].c_str());
		}
		if (this->QuestObjectives[j]->UnitClass != -1) {
			file.printf("\"unit-class\", \"%s\",", UnitTypeClasses[this->QuestObjectives[j]->UnitClass].c_str());
		}
		for (const CUnitType *unit_type : this->QuestObjectives[j]->UnitTypes) {
			file.printf("\"unit-type\", \"%s\",", unit_type->Ident.c_str());
		}
		if (this->QuestObjectives[j]->Upgrade) {
			file.printf("\"upgrade\", \"%s\",", this->QuestObjectives[j]->Upgrade->Ident.c_str());
		}
		if (this->QuestObjectives[j]->Character) {
			file.printf("\"character\", \"%s\",", this->QuestObjectives[j]->Character->Ident.c_str());
		}
		if (this->QuestObjectives[j]->Unique) {
			file.printf("\"unique\", \"%s\",", this->QuestObjectives[j]->Unique->Ident.c_str());
		}
		if (this->QuestObjectives[j]->Settlement) {
			file.printf("\"settlement\", \"%s\",", this->QuestObjectives[j]->Settlement->Ident.c_str());
		}
		if (this->QuestObjectives[j]->Faction) {
			file.printf("\"faction\", \"%s\",", this->QuestObjectives[j]->Faction->GetIdent().utf8().get_data());
		}
		file.printf("},");
	}
	file.printf("},");
	
	file.printf("\n  \"modifiers\", {");
	for (size_t j = 0; j < this->Modifiers.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\", %i,", this->Modifiers[j].first->Ident.c_str(), this->Modifiers[j].second);
	}
	file.printf("},");
	//Wyrmgus end

	file.printf("\n  \"autosell-resources\", {");
	for (size_t j = 0; j < this->AutosellResources.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", DefaultResourceNames[this->AutosellResources[j]].c_str());
	}
	file.printf("},");
	
	// UnitColors done by init code.
	// Allow saved by allow.

	file.printf("\n  \"timers\", {");
	//Wyrmgus start
	bool first = true;
	//Wyrmgus end
	for (int j = 0; j < UpgradeMax; ++j) {
		//Wyrmgus start
//		if (j) {
//			file.printf(" ,");
//		}
//		file.printf("%i", this->UpgradeTimers.Upgrades[j]);
		if (this->UpgradeTimers.Upgrades[j]) {
			if (first) {
				first = false;
			} else {
				file.printf(", ");
			}
			file.printf("\"%s\", %i", AllUpgrades[j]->Ident.c_str(), this->UpgradeTimers.Upgrades[j]);
		}
		//Wyrmgus end
	}
	file.printf("}");

	file.printf(")\n\n");

	DebugPrint("FIXME: must save unit-stats?\n");
}

/**
**  Create a new player.
**
**  @param type  Player type (Computer,Human,...).
*/
void CreatePlayer(int type)
{
	if (NumPlayers == PlayerMax) { // already done for bigmaps!
		return;
	}
	CPlayer &player = *CPlayer::Players[NumPlayers];
	player.Index = NumPlayers;

	player.Init(type);
}

//Wyrmgus start
CPlayer *GetFactionPlayer(const CFaction *faction)
{
	if (!faction) {
		return nullptr;
	}
	
	for (int i = 0; i < NumPlayers; ++i) {
		if (CPlayer::Players[i]->Race == faction->Civilization->ID && CPlayer::Players[i]->GetFaction() == faction) {
			return CPlayer::Players[i];
		}
	}
	
	return nullptr;
}

CPlayer *GetOrAddFactionPlayer(const CFaction *faction)
{
	CPlayer *faction_player = GetFactionPlayer(faction);
	if (faction_player) {
		return faction_player;
	}
	
	// no player belonging to this faction, so let's make an unused player slot be created for it
	
	for (int i = 0; i < NumPlayers; ++i) {
		if (CPlayer::Players[i]->Type == PlayerNobody) {
			CPlayer::Players[i]->Type = PlayerComputer;
			CPlayer::Players[i]->SetCivilization(faction->Civilization->ID);
			CPlayer::Players[i]->SetFaction(faction);
			CPlayer::Players[i]->AiEnabled = true;
			CPlayer::Players[i]->AiName = faction->DefaultAI;
			CPlayer::Players[i]->Team = 1;
			CPlayer::Players[i]->Resources[CopperCost] = 2500; // give the new player enough resources to start up
			CPlayer::Players[i]->Resources[WoodCost] = 2500;
			CPlayer::Players[i]->Resources[StoneCost] = 2500;
			return CPlayer::Players[i];
		}
	}
	
	fprintf(stderr, "Cannot add player for faction \"%s\": no player slots available.\n", faction->GetIdent().utf8().get_data());
	
	return nullptr;
}
//Wyrmgus end

void CPlayer::Init(/* PlayerTypes */ int type)
{
	//  Take first slot for person on this computer,
	//  fill other with computer players.
	if (type == PlayerPerson && !NetPlayers) {
		if (!CPlayer::GetThisPlayer()) {
			CPlayer::SetThisPlayer(this);
		} else {
			type = PlayerComputer;
		}
	}
	if (NetPlayers && NumPlayers == NetLocalPlayerNumber) {
		CPlayer::SetThisPlayer(CPlayer::Players[NetLocalPlayerNumber]);
	}

	if (NumPlayers == PlayerMax) {
		static int already_warned;

		if (!already_warned) {
			DebugPrint("Too many players\n");
			already_warned = 1;
		}
		return;
	}

	std::unique_lock<std::shared_mutex> lock(this->Mutex);
	
	std::vector<CUnit *>().swap(this->Units);
	std::vector<CUnit *>().swap(this->FreeWorkers);
	//Wyrmgus start
	std::vector<CUnit *>().swap(this->LevelUpUnits);
	//Wyrmgus end

	//  Make simple teams:
	//  All person players are enemies.
	int team;
	switch (type) {
		case PlayerNeutral:
		case PlayerNobody:
		default:
			team = 0;
			this->SetName("Neutral");
			break;
		case PlayerComputer:
			team = 1;
			this->SetName("Computer");
			break;
		case PlayerPerson:
			team = 2 + NumPlayers;
			this->SetName("Person");
			break;
		case PlayerRescuePassive:
		case PlayerRescueActive:
			// FIXME: correct for multiplayer games?
			this->SetName("Computer");
			team = 2 + NumPlayers;
			break;
	}
	DebugPrint("CreatePlayer name %s\n" _C_ this->Name.c_str());

	this->Type = type;
	this->Race = 0;
	this->Faction = nullptr;
	this->Religion = nullptr;
	this->Dynasty = nullptr;
	this->Age = nullptr;
	this->Overlord = nullptr;
	this->Team = team;
	this->Enemy = 0;
	this->Allied = 0;
	this->AiName = "ai-passive";

	//  Calculate enemy/allied mask.
	for (int i = 0; i < NumPlayers; ++i) {
		switch (type) {
			case PlayerNeutral:
			case PlayerNobody:
			default:
				break;
			case PlayerComputer:
				// Computer allied with computer and enemy of all persons.
				//Wyrmgus start
				/*
				if (CPlayer::Players[i]->Type == PlayerComputer) {
					this->Allied |= (1 << i);
					CPlayer::Players[i]->Allied |= (1 << NumPlayers);
				} else if (CPlayer::Players[i]->Type == PlayerPerson || CPlayer::Players[i]->Type == PlayerRescueActive) {
				*/
				// make computer players be hostile to each other by default
				if (CPlayer::Players[i]->Type == PlayerComputer || CPlayer::Players[i]->Type == PlayerPerson || CPlayer::Players[i]->Type == PlayerRescueActive) {
				//Wyrmgus end
					this->Enemy |= (1 << i);
					CPlayer::Players[i]->Enemy |= (1 << NumPlayers);
				}
				break;
			case PlayerPerson:
				// Humans are enemy of all?
				if (CPlayer::Players[i]->Type == PlayerComputer || CPlayer::Players[i]->Type == PlayerPerson) {
					this->Enemy |= (1 << i);
					CPlayer::Players[i]->Enemy |= (1 << NumPlayers);
				} else if (CPlayer::Players[i]->Type == PlayerRescueActive || CPlayer::Players[i]->Type == PlayerRescuePassive) {
					this->Allied |= (1 << i);
					CPlayer::Players[i]->Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescuePassive:
				// Rescue passive are allied with persons
				if (CPlayer::Players[i]->Type == PlayerPerson) {
					this->Allied |= (1 << i);
					CPlayer::Players[i]->Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescueActive:
				// Rescue active are allied with persons and enemies of computer
				if (CPlayer::Players[i]->Type == PlayerComputer) {
					this->Enemy |= (1 << i);
					CPlayer::Players[i]->Enemy |= (1 << NumPlayers);
				} else if (CPlayer::Players[i]->Type == PlayerPerson) {
					this->Allied |= (1 << i);
					CPlayer::Players[i]->Allied |= (1 << NumPlayers);
				}
				break;
		}
	}

	//  Initial default incomes.
	for (int i = 0; i < MaxCosts; ++i) {
		this->Incomes[i] = CResource::GetAll()[i]->DefaultIncome;
	}
	
	this->TradeCost = DefaultTradeCost;

	//  Initial max resource amounts.
	for (int i = 0; i < MaxCosts; ++i) {
		this->MaxResources[i] = CResource::GetAll()[i]->DefaultMaxAmount;
	}

	//Wyrmgus start
	this->UnitTypesCount.clear();
	this->UnitTypesUnderConstructionCount.clear();
	this->UnitTypesAiActiveCount.clear();
	this->Heroes.clear();
	this->Deities.clear();
	this->UnitsByType.clear();
	this->AiActiveUnitsByType.clear();
	//Wyrmgus end

	this->Supply = 0;
	this->Demand = 0;
	this->NumBuildings = 0;
	//Wyrmgus start
	this->NumBuildingsUnderConstruction = 0;
	this->NumTownHalls = 0;
	//Wyrmgus end
	this->Score = 0;
	//Wyrmgus start
	this->LostTownHallTimer = 0;
	this->HeroCooldownTimer = 0;
	//Wyrmgus end

	this->Color = PlayerColors[NumPlayers][0];

	if (CPlayer::Players[NumPlayers]->Type == PlayerComputer || CPlayer::Players[NumPlayers]->Type == PlayerRescueActive) {
		this->AiEnabled = true;
	} else {
		this->AiEnabled = false;
	}
	//Wyrmgus start
	this->Revealed = false;
	//Wyrmgus end
	++NumPlayers;
}

/**
**  Change player name.
**
**  @param name    New name.
*/
void CPlayer::SetName(const std::string &name)
{
	Name = name;
}

//Wyrmgus start
void CPlayer::SetCivilization(int civilization)
{
	String old_interface = this->GetInterface();
	
	if (this->Race != -1 && (GameRunning || GameEstablishing)) {
		if (!PlayerRaces.CivilizationUpgrades[this->Race].empty() && this->Allow.Upgrades[CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID);
		}
	}
	
	CCivilization *old_civilization = this->GetCivilization();
	CCivilization *new_civilization = civilization != -1 ? CCivilization::Civilizations[civilization] : nullptr;

	if (GameRunning) {
		this->SetFaction(nullptr);
	} else {
		this->Faction = nullptr;
	}

	{
		std::unique_lock<std::shared_mutex> lock(this->Mutex);
		
		this->Race = civilization;
	}

	//if the civilization of the person player changed, update the UI
	if (CPlayer::GetThisPlayer() == this || (!CPlayer::GetThisPlayer() && this->Index == 0)) {
		//load proper UI
		char buf[256];
		snprintf(buf, sizeof(buf), "if (LoadCivilizationUI ~= nil) then LoadCivilizationUI(\"%s\") end;", PlayerRaces.Name[this->Race].c_str());
		CclCommand(buf);
		
		UI.Load();
		SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
	}
	
	if (this->Race != -1) {
		CUpgrade *civilization_upgrade = CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race]);
		if (civilization_upgrade && this->Allow.Upgrades[civilization_upgrade->ID] != 'R') {
			UpgradeAcquire(*this, civilization_upgrade);
		}
	}
	
	emit_signal("civilization_changed", old_civilization, new_civilization);
	
	String new_interface = this->GetInterface();
	if (new_interface != old_interface) {
		emit_signal("interface_changed", old_interface, new_interface);
		
		if (CPlayer::GetThisPlayer() == this) {
			Wyrmgus::GetInstance()->emit_signal("interface_changed", old_interface, new_interface);
		}
	}
}

CCivilization *CPlayer::GetCivilization() const
{
	std::shared_lock<std::shared_mutex> lock(this->Mutex);
	
	if (this->Race != -1 && this->Race < (int) CCivilization::Civilizations.size()) {
		return CCivilization::Civilizations[this->Race];
	}
		
	return nullptr;
}
	
String CPlayer::GetInterface() const
{
	CCivilization *civilization = this->GetCivilization();
	if (civilization) {
		return civilization->GetInterface();
	}
	
	return "";
}

/**
**  Change player faction.
**
**  @param faction    New faction.
*/
void CPlayer::SetFaction(const CFaction *faction)
{
	const CFaction *old_faction = this->Faction;
	
	if (faction && faction->Civilization->ID != this->Race) {
		this->SetCivilization(faction->Civilization->ID);
	}

	if (this->Faction != nullptr) {
		if (!this->Faction->FactionUpgrade.empty() && this->Allow.Upgrades[CUpgrade::Get(this->Faction->FactionUpgrade)->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(this->Faction->FactionUpgrade)->ID);
		}

		int faction_type_upgrade_id = UpgradeIdByIdent("upgrade-" + GetFactionTypeNameById(this->Faction->Type));
		if (faction_type_upgrade_id != -1 && this->Allow.Upgrades[faction_type_upgrade_id] == 'R') {
			UpgradeLost(*this, faction_type_upgrade_id);
		}
	}

	if (old_faction != nullptr && faction != nullptr) {
		for (size_t i = 0; i < UpgradeClasses.size(); ++i) {
			if (CFaction::GetFactionClassUpgrade(old_faction, i) != CFaction::GetFactionClassUpgrade(faction, i)) { //if the upgrade for a certain class is different for the new faction than the old faction (and it has been acquired), remove the modifiers of the old upgrade and apply the modifiers of the new
				if (CFaction::GetFactionClassUpgrade(old_faction, i) != -1 && this->Allow.Upgrades[CFaction::GetFactionClassUpgrade(old_faction, i)] == 'R') {
					UpgradeLost(*this, CFaction::GetFactionClassUpgrade(old_faction, i));

					if (CFaction::GetFactionClassUpgrade(faction, i) != -1) {
						UpgradeAcquire(*this, AllUpgrades[CFaction::GetFactionClassUpgrade(faction, i)]);
					}
				}
			}
		}
	}
	
	bool personal_names_changed = true;
	bool ship_names_changed = true;
	if (old_faction != nullptr && faction != nullptr) {
		ship_names_changed = old_faction->GetShipNames() != faction->GetShipNames();
		personal_names_changed = false; // setting to a faction of the same civilization
	}
	
	this->Faction = faction;

	if (this->Index == CPlayer::GetThisPlayer()->Index) {
		UI.Load();
	}
	
	if (this->Faction == nullptr) {
		return;
	}
	
	if (!IsNetworkGame()) { //only set the faction's name as the player's name if this is a single player game
		this->SetName(this->Faction->Name);
	}
	if (this->Faction != nullptr) {
		int color = -1;
		for (CPlayerColor *player_color : faction->GetPrimaryColors()) {
			int primary_color = GetPlayerColorIndexByName(player_color->GetIdent().utf8().get_data());
			if (!IsPlayerColorUsed(primary_color)) {
				color = primary_color;
				break;
			}
		}
		if (color == -1) { //if all of the faction's colors are used, get a unused player color
			for (int i = 0; i < PlayerColorMax; ++i) {
				if (!IsPlayerColorUsed(i)) {
					color = i;
					break;
				}
			}
		}
		
		if (color != -1) {
			if (this->Color != PlayerColors[color][0]) {
				this->Color = PlayerColors[color][0];
				this->UnitColors.Colors = PlayerColorsRGB[color];
			}
		}
	
		if (!this->Faction->FactionUpgrade.empty()) {
			CUpgrade *faction_upgrade = CUpgrade::Get(this->Faction->FactionUpgrade);
			if (faction_upgrade && this->Allow.Upgrades[faction_upgrade->ID] != 'R') {
				if (GameEstablishing) {
					AllowUpgradeId(*this, faction_upgrade->ID, 'R');
				} else {
					UpgradeAcquire(*this, faction_upgrade);
				}
			}
		}
		
		int faction_type_upgrade_id = UpgradeIdByIdent("upgrade-" + GetFactionTypeNameById(this->Faction->Type));
		if (faction_type_upgrade_id != -1 && this->Allow.Upgrades[faction_type_upgrade_id] != 'R') {
			if (GameEstablishing) {
				AllowUpgradeId(*this, faction_type_upgrade_id, 'R');
			} else {
				UpgradeAcquire(*this, AllUpgrades[faction_type_upgrade_id]);
			}
		}
	} else {
		fprintf(stderr, "Invalid faction \"%s\" tried to be set for player %i of civilization \"%s\".\n", faction->Name.c_str(), this->Index, PlayerRaces.Name[this->Race].c_str());
	}
	
	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);
		if (!unit.Unique && unit.Type->PersonalNames.size() == 0) {
			if (!unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->UnitType == UnitTypeNaval && ship_names_changed) {
				unit.UpdatePersonalName();
			}
		}
		if (personal_names_changed && unit.Type->BoolFlag[ORGANIC_INDEX].value && !unit.Character && unit.Type->GetCivilization() != nullptr && unit.Type->GetCivilization()->GetSpecies() == faction->Civilization->GetSpecies() && unit.Type->Slot == CFaction::GetFactionClassUnitType(faction, unit.Type->Class)) {
			unit.UpdatePersonalName();
		}
		unit.UpdateSoldUnits();
		unit.UpdateButtonIcons();
	}
}

/**
**  Change player faction to a randomly chosen one.
**
**  @param faction    New faction.
*/
void CPlayer::SetRandomFaction()
{
	// set random one from the civilization's factions
	std::vector<const CFaction *> local_factions;
	
	for (const CFaction *faction : CFaction::GetAll()) {
		if (faction->Civilization->ID != this->Race) {
			continue;
		}
		if (!faction->Playable) {
			continue;
		}
		if (!this->CanFoundFaction(faction)) {
			continue;
		}

		int faction_type = faction->Type;
		bool has_writing = this->HasUpgradeClass(GetUpgradeClassIndexByName("writing"));
		if (
			!(faction_type == FactionTypeTribe && !has_writing)
			&& !(faction_type == FactionTypePolity && has_writing)
		) {
			continue;
		}

		local_factions.push_back(faction);
	}
	
	if (local_factions.size() > 0) {
		const CFaction *chosen_faction = local_factions[SyncRand(local_factions.size())];
		this->SetFaction(chosen_faction);
	} else {
		this->SetFaction(nullptr);
	}
}

/**
**	@brief	Change player dynasty.
**
**	@param	dynasty	New dynasty.
*/
void CPlayer::SetDynasty(CDynasty *dynasty)
{
	CDynasty *old_dynasty = this->Dynasty;
	
	if (this->Dynasty) {
		if (this->Dynasty->DynastyUpgrade && this->Allow.Upgrades[this->Dynasty->DynastyUpgrade->ID] == 'R') {
			UpgradeLost(*this, this->Dynasty->DynastyUpgrade->ID);
		}
	}

	this->Dynasty = dynasty;

	if (!this->Dynasty) {
		return;
	}
	
	if (this->Dynasty->DynastyUpgrade) {
		if (this->Allow.Upgrades[this->Dynasty->DynastyUpgrade->ID] != 'R') {
			if (GameEstablishing) {
				AllowUpgradeId(*this, this->Dynasty->DynastyUpgrade->ID, 'R');
			} else {
				UpgradeAcquire(*this, this->Dynasty->DynastyUpgrade);
			}
		}
	}

	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);
		unit.UpdateSoldUnits(); //in case conditions changed (i.e. some heroes may require a certain dynasty)
	}
}

/**
**	@brief	Check which age fits the player's current situation best, and set it as the player's age
*/
void CPlayer::CheckAge()
{
	//pick an age which fits the player, giving priority to the first ones (ages are already sorted by priority)
	
	for (CAge *potential_age : CAge::GetAll()) {
		if (!CheckDependencies(potential_age, this)) {
			continue;
		}
		
		this->SetAge(potential_age);
		return;
	}
	
	this->SetAge(nullptr);
}

/**
**	@brief	Set the player's age
**
**	@param	age	The age to be set for the player
*/
void CPlayer::SetAge(CAge *age)
{
	if (this->Age == age) {
		return;
	}
	
	this->Age = age;
	
	if (this == CPlayer::GetThisPlayer()) {
		if (this->Age) {
			UI.AgePanel.Text = this->Age->Name;
			UI.AgePanel.G = this->Age->G;
			
			if (GameCycle > 0 && !SaveGameLoading) {
				this->Notify(_("The %s has dawned upon us."), this->Age->Name.c_str());
			}
		} else {
			UI.AgePanel.Text.clear();
			UI.AgePanel.G = nullptr;
		}
	}
	
	CAge::CheckCurrentAge();
}

/**
**	@brief	Get the player's currency
**
**	@return	The player's currency
*/
CCurrency *CPlayer::GetCurrency() const
{
	if (this->Faction != nullptr) {
		return this->Faction->GetCurrency();
	}
	
	if (this->Race != -1) {
		return CCivilization::Civilizations[this->Race]->GetCurrency();
	}
	
	return nullptr;
}

void CPlayer::ShareUpgradeProgress(CPlayer &player, CUnit &unit)
{
	std::vector<CUpgrade *> upgrade_list = this->GetResearchableUpgrades();
	std::vector<CUpgrade *> potential_upgrades;

	for (size_t i = 0; i < upgrade_list.size(); ++i) {
		if (this->Allow.Upgrades[upgrade_list[i]->ID] != 'R') {
			continue;
		}
		
		if (upgrade_list[i]->Class == -1) {
			continue;
		}
		
		int upgrade_id = CFaction::GetFactionClassUpgrade(player.GetFaction(), upgrade_list[i]->Class);
		if (upgrade_id == -1) {
			continue;
		}
		
		CUpgrade *upgrade = AllUpgrades[upgrade_id];
		
		if (player.Allow.Upgrades[upgrade->ID] != 'A' || !CheckDependencies(upgrade, &player)) {
			continue;
		}
	
		if (player.UpgradeRemovesExistingUpgrade(upgrade, player.AiEnabled)) {
			continue;
		}
		
		potential_upgrades.push_back(upgrade);
	}
	
	if (potential_upgrades.size() > 0) {
		CUpgrade *chosen_upgrade = potential_upgrades[SyncRand(potential_upgrades.size())];
		
		if (!chosen_upgrade->Name.empty()) {
			player.Notify(NotifyGreen, unit.tilePos, unit.MapLayer->ID, _("%s acquired through contact with %s"), chosen_upgrade->Name.c_str(), this->Name.c_str());
		}
		if (&player == CPlayer::GetThisPlayer()) {
			CSound *sound = GameSounds.ResearchComplete[player.Race].Sound;

			if (sound) {
				PlayGameSound(sound, MaxSampleVolume);
			}
		}
		if (player.AiEnabled) {
			AiResearchComplete(unit, chosen_upgrade);
		}
		UpgradeAcquire(player, chosen_upgrade);
	}
}

bool CPlayer::IsPlayerColorUsed(int color)
{
	bool color_used = false;
	for (int i = 0; i < PlayerMax; ++i) {
		if (this->Index != i && CPlayer::Players[i]->GetFaction() != nullptr && CPlayer::Players[i]->Type != PlayerNobody && CPlayer::Players[i]->Color == PlayerColors[color][0]) {
			color_used = true;
		}		
	}
	return color_used;
}

bool CPlayer::HasUpgradeClass(const int upgrade_class) const
{
	if (this->Race == -1 || upgrade_class == -1) {
		return false;
	}
	
	int upgrade_id = -1;
	
	if (this->Faction != nullptr) {
		upgrade_id = CFaction::GetFactionClassUpgrade(this->Faction, upgrade_class);
	} else {
		upgrade_id = CCivilization::GetCivilizationClassUpgrade(CCivilization::Civilizations[this->Race], upgrade_class);
	}
	
	if (upgrade_id != -1 && this->Allow.Upgrades[upgrade_id] == 'R') {
		return true;
	}

	return false;
}

bool CPlayer::HasSettlement(const CSite *settlement) const
{
	if (!settlement) {
		return false;
	}
	
	if (settlement->SiteUnit && settlement->SiteUnit->Player == this) {
		return true;
	}

	return false;
}

bool CPlayer::HasSettlementNearWaterZone(int water_zone) const
{
	std::vector<CUnit *> settlement_unit_table;
	
	int town_hall_type_id = CFaction::GetFactionClassUnitType(this->Faction, GetUnitTypeClassIndexByName("town-hall"));			
	if (town_hall_type_id == -1) {
		return false;
	}
	CUnitType *town_hall_type = CUnitType::UnitTypes[town_hall_type_id];
	
	int stronghold_type_id = CFaction::GetFactionClassUnitType(this->Faction, GetUnitTypeClassIndexByName("stronghold"));			
	CUnitType *stronghold_type = nullptr;
	if (stronghold_type_id != -1) {
		stronghold_type = CUnitType::UnitTypes[stronghold_type_id];
	}
	
	FindPlayerUnitsByType(*this, *town_hall_type, settlement_unit_table, true);
	
	if (stronghold_type) {
		FindPlayerUnitsByType(*this, *stronghold_type, settlement_unit_table, true); //adds strongholds to the table
	}
	for (size_t i = 0; i < settlement_unit_table.size(); ++i) {
		CUnit *settlement_unit = settlement_unit_table[i];
		if (!settlement_unit->IsAliveOnMap()) {
			continue;
		}
		
		int settlement_landmass = CMap::Map.GetTileLandmass(settlement_unit->tilePos, settlement_unit->MapLayer->ID);
		if (std::find(CMap::Map.BorderLandmasses[settlement_landmass].begin(), CMap::Map.BorderLandmasses[settlement_landmass].end(), water_zone) == CMap::Map.BorderLandmasses[settlement_landmass].end()) { //settlement's landmass doesn't even border the water zone, continue
			continue;
		}
		
		Vec2i pos(0, 0);
		if (FindTerrainType(0, 0, 8, *this, settlement_unit->tilePos, &pos, settlement_unit->MapLayer->ID, water_zone)) {
			return true;
		}
	}

	return false;
}

CSite *CPlayer::GetNearestSettlement(const Vec2i &pos, int z, const Vec2i &size) const
{
	CUnit *best_hall = nullptr;
	int best_distance = -1;
	
	for (CUnit *settlement_unit : CMap::Map.SiteUnits) {
		if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->Type->BoolFlag[TOWNHALL_INDEX].value || z != settlement_unit->MapLayer->ID) {
			continue;
		}
		if (!this->HasNeutralFactionType() && this != settlement_unit->Player) {
			continue;
		}
		int distance = MapDistance(size, pos, z, settlement_unit->Type->TileSize, settlement_unit->tilePos, settlement_unit->MapLayer->ID);
		if (!best_hall || distance < best_distance) {
			best_hall = settlement_unit;
			best_distance = distance;
		}
	}
	
	if (best_hall) {
		return best_hall->Settlement;
	} else {
		return nullptr;
	}
}

bool CPlayer::HasUnitBuilder(const CUnitType *type, const CSite *settlement) const
{
	if (type->BoolFlag[BUILDING_INDEX].value && type->Slot < (int) AiHelpers.Build.size()) {
		for (size_t j = 0; j < AiHelpers.Build[type->Slot].size(); ++j) {
			if (this->GetUnitTypeCount(AiHelpers.Build[type->Slot][j]) > 0) {
				return true;
			}
		}
	} else if (!type->BoolFlag[BUILDING_INDEX].value && type->Slot < (int) AiHelpers.Train.size()) {
		for (size_t j = 0; j < AiHelpers.Train[type->Slot].size(); ++j) {
			if (this->GetUnitTypeCount(AiHelpers.Train[type->Slot][j]) > 0) {
				return true;
			}
		}
	}
	if (type->Slot < (int) AiHelpers.Upgrade.size()) {
		for (size_t j = 0; j < AiHelpers.Upgrade[type->Slot].size(); ++j) {
			if (this->GetUnitTypeCount(AiHelpers.Upgrade[type->Slot][j]) > 0) {
				if (!settlement) {
					return true;
				} else {
					for (int i = 0; i < this->GetUnitCount(); ++i) {
						CUnit &unit = this->GetUnit(i);
						if (unit.Type == AiHelpers.Upgrade[type->Slot][j] && unit.Settlement == settlement) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool CPlayer::HasUpgradeResearcher(const CUpgrade *upgrade) const
{
	if (upgrade->ID < (int) AiHelpers.Research.size()) {
		for (size_t j = 0; j < AiHelpers.Research[upgrade->ID].size(); ++j) {
			CUnitType *researcher_type = AiHelpers.Research[upgrade->ID][j];
			if (this->GetUnitTypeCount(researcher_type) > 0 || HasUnitBuilder(researcher_type)) {
				return true;
			}
		}
	}
	return false;
}

/**
**  Check if the player can found a particular faction.
**
**  @param faction    New faction.
*/
bool CPlayer::CanFoundFaction(const CFaction *faction, bool pre)
{
	if (CurrentQuest != nullptr) {
		return false;
	}
	
	if (!faction->FactionUpgrade.empty()) {
		CUpgrade *faction_upgrade = CUpgrade::Get(faction->FactionUpgrade);
		
		if (faction_upgrade) {
			if (!CheckDependencies(faction_upgrade, this, false, pre)) {
				return false;
			}
		} else {
			fprintf(stderr, "Faction upgrade \"%s\" doesn't exist.\n", faction->FactionUpgrade.c_str());
		}
	}

	for (int i = 0; i < PlayerMax; ++i) {
		if (this->Index != i && CPlayer::Players[i]->Type != PlayerNobody && CPlayer::Players[i]->Race == faction->Civilization->ID && CPlayer::Players[i]->GetFaction() == faction) {
			// faction is already in use
			return false;
		}
	}
	
	if (!pre) {
		//check if the required core settlements are owned by the player
		if (CCampaign::GetCurrentCampaign() != nullptr) { //only check for settlements in the Scenario mode
			for (size_t i = 0; i < faction->Cores.size(); ++i) {
				if (!faction->Cores[i]->SiteUnit || faction->Cores[i]->SiteUnit->Player != this || faction->Cores[i]->SiteUnit->CurrentAction() == UnitActionBuilt) {
					return false;
				}
			}
		}
		
		if (faction->Conditions) {
			CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
			faction->Conditions->pushPreamble();
			faction->Conditions->run(1);
			if (faction->Conditions->popBoolean() == false) {
				return false;
			}
		}
	}
	
	return true;
}

/**
**  Check if the player can choose a particular dynasty.
**
**  @param dynasty    New dynasty.
*/
bool CPlayer::CanChooseDynasty(const CDynasty *dynasty, bool pre)
{
	if (CurrentQuest != nullptr) {
		return false;
	}
	
	if (dynasty->DynastyUpgrade) {
		if (!CheckDependencies(dynasty->DynastyUpgrade, this, false, pre)) {
			return false;
		}
	} else {
		return false;
	}

	if (!pre) {
		if (dynasty->Conditions) {
			CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
			dynasty->Conditions->pushPreamble();
			dynasty->Conditions->run(1);
			if (dynasty->Conditions->popBoolean() == false) {
				return false;
			}
		}
	}
	
	return true;
}

/**
**  Check if the player can recruit a particular hero.
**
**  @param character    Hero.
*/
bool CPlayer::CanRecruitHero(const CCharacter *character, bool ignore_neutral) const
{
	if (character->Deity != nullptr) { //character is a deity
		return false;
	}
	
	if (!character->Civilization || character->Civilization->ID != this->Race) {
		return false;
	}
	
	if (!CheckDependencies(character->Type, this, true)) {
		return false;
	}
	
	if (!character->Factions.empty() && (this->Faction == nullptr || std::find(character->Factions.begin(), character->Factions.end(), this->Faction) == character->Factions.end())) {
		return false;
	}
	
	if (character->Conditions) {
		CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
		character->Conditions->pushPreamble();
		character->Conditions->run(1);
		if (character->Conditions->popBoolean() == false) {
			return false;
		}
	}
	
	if (!character->CanAppear(ignore_neutral)) {
		return false;
	}
	
	return true;
}

/**
**  Check if the upgrade removes an existing upgrade of the player.
**
**  @param upgrade    Upgrade.
*/
bool CPlayer::UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade, bool ignore_lower_priority) const
{
	for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
		for (size_t j = 0; j < upgrade->UpgradeModifiers[z]->RemoveUpgrades.size(); ++j) {
			const CUpgrade *removed_upgrade = upgrade->UpgradeModifiers[z]->RemoveUpgrades[j];
			bool has_upgrade = this->AiEnabled ? AiHasUpgrade(*this->Ai, removed_upgrade, true) : (UpgradeIdAllowed(*this, removed_upgrade->ID) == 'R');
			if (has_upgrade) {
				if (ignore_lower_priority && this->Faction != nullptr && this->Faction->GetUpgradePriority(removed_upgrade) < this->Faction->GetUpgradePriority(upgrade)) {
					continue;
				}
				return true;
			}
		}
	}
	
	return false;
}

std::string CPlayer::GetFactionTitleName() const
{
	if (this->Race == -1 || this->Faction == nullptr) {
		return "";
	}
	
	const CFaction *faction = this->Faction;
	int faction_tier = faction->DefaultTier;
	int government_type = faction->DefaultGovernmentType;
	
	if (faction->Type == FactionTypePolity) {
		if (!faction->Titles[government_type][faction_tier].empty()) {
			return faction->Titles[government_type][faction_tier];
		} else {
			if (government_type == GovernmentTypeMonarchy) {
				if (faction_tier == FactionTierBarony) {
					return "Barony";
				} else if (faction_tier == FactionTierCounty) {
					return "County";
				} else if (faction_tier == FactionTierDuchy) {
					return "Duchy";
				} else if (faction_tier == FactionTierGrandDuchy) {
					return "Grand Duchy";
				} else if (faction_tier == FactionTierKingdom) {
					return "Kingdom";
				} else if (faction_tier == FactionTierEmpire) {
					return "Empire";
				}
			} else if (government_type == GovernmentTypeRepublic) {
				return "Republic";
			} else if (government_type == GovernmentTypeTheocracy) {
				return "Theocracy";
			}
		}
	}
	
	return "";
}

std::string CPlayer::GetCharacterTitleName(int title_type, int gender) const
{
	if (this->Race == -1 || this->Faction == nullptr || title_type == -1 || gender == -1) {
		return "";
	}
	
	CCivilization *civilization = CCivilization::Civilizations[this->Race];
	const CFaction *faction = this->Faction;
	int faction_tier = faction->DefaultTier;
	int government_type = faction->DefaultGovernmentType;
	
	if (faction->Type == FactionTypePolity) {
		if (!faction->MinisterTitles[title_type][gender][government_type][faction_tier].empty()) {
			return faction->MinisterTitles[title_type][gender][government_type][faction_tier];
		} else if (!faction->MinisterTitles[title_type][NoGender][government_type][faction_tier].empty()) {
			return faction->MinisterTitles[title_type][NoGender][government_type][faction_tier];
		} else if (!faction->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][faction_tier].empty()) {
			return faction->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][faction_tier];
		} else if (!faction->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][faction_tier].empty()) {
			return faction->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][faction_tier];
		} else if (!faction->MinisterTitles[title_type][gender][government_type][FactionTierNoFactionTier].empty()) {
			return faction->MinisterTitles[title_type][gender][government_type][FactionTierNoFactionTier];
		} else if (!faction->MinisterTitles[title_type][NoGender][government_type][FactionTierNoFactionTier].empty()) {
			return faction->MinisterTitles[title_type][NoGender][government_type][FactionTierNoFactionTier];
		} else if (!faction->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier].empty()) {
			return faction->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier];
		} else if (!faction->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier].empty()) {
			return faction->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier];
		} else if (!civilization->MinisterTitles[title_type][gender][government_type][faction_tier].empty()) {
			return civilization->MinisterTitles[title_type][gender][government_type][faction_tier];
		} else if (!civilization->MinisterTitles[title_type][NoGender][government_type][faction_tier].empty()) {
			return civilization->MinisterTitles[title_type][NoGender][government_type][faction_tier];
		} else if (!civilization->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][faction_tier].empty()) {
			return civilization->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][faction_tier];
		} else if (!civilization->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][faction_tier].empty()) {
			return civilization->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][faction_tier];
		} else if (!civilization->MinisterTitles[title_type][gender][government_type][FactionTierNoFactionTier].empty()) {
			return civilization->MinisterTitles[title_type][gender][government_type][FactionTierNoFactionTier];
		} else if (!civilization->MinisterTitles[title_type][NoGender][government_type][FactionTierNoFactionTier].empty()) {
			return civilization->MinisterTitles[title_type][NoGender][government_type][FactionTierNoFactionTier];
		} else if (!civilization->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier].empty()) {
			return civilization->MinisterTitles[title_type][gender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier];
		} else if (!civilization->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier].empty()) {
			return civilization->MinisterTitles[title_type][NoGender][GovernmentTypeNoGovernmentType][FactionTierNoFactionTier];
		}
	}

	if (title_type == CharacterTitleHeadOfState) {
		if (faction->Type == FactionTypeTribe) {
			if (gender != FemaleGender) {
				return "Chieftain";
			} else {
				return "Chieftess";
			}
		} else if (faction->Type == FactionTypePolity) {
			std::string faction_title = this->GetFactionTitleName();
			
			if (faction_title == "Barony") {
				if (gender != FemaleGender) {
					return "Baron";
				} else {
					return "Baroness";
				}
			} else if (faction_title == "Lordship") {
				if (gender != FemaleGender) {
					return "Lord";
				} else {
					return "Lady";
				}
			} else if (faction_title == "County") {
				if (gender != FemaleGender) {
					return "Count";
				} else {
					return "Countess";
				}
			} else if (faction_title == "City-State") {
				return "Archon";
			} else if (faction_title == "Duchy") {
				if (gender != FemaleGender) {
					return "Duke";
				} else {
					return "Duchess";
				}
			} else if (faction_title == "Principality") {
				if (gender != FemaleGender) {
					return "Prince";
				} else {
					return "Princess";
				}
			} else if (faction_title == "Margraviate") {
				return "Margrave";
			} else if (faction_title == "Landgraviate") {
				return "Landgrave";
			} else if (faction_title == "Grand Duchy") {
				if (gender != FemaleGender) {
					return "Grand Duke";
				} else {
					return "Grand Duchess";
				}
			} else if (faction_title == "Archduchy") {
				if (gender != FemaleGender) {
					return "Archduke";
				} else {
					return "Archduchess";
				}
			} else if (faction_title == "Kingdom") {
				if (gender != FemaleGender) {
					return "King";
				} else {
					return "Queen";
				}
			} else if (faction_title == "Khanate") {
				return "Khan";
			} else if (faction_title == "Empire") {
				if (gender != FemaleGender) {
					return "Emperor";
				} else {
					return "Empress";
				}
			} else if (faction_title == "Republic") {
				return "Consul";
			} else if (faction_title == "Confederation") {
				return "Chancellor";
			} else if (faction_title == "Theocracy") {
				if (gender != FemaleGender) {
					return "High Priest";
				} else {
					return "High Priestess";
				}
			} else if (faction_title == "Bishopric") {
				return "Bishop";
			} else if (faction_title == "Archbishopric") {
				return "Archbishop";
			}
		}
	} else if (title_type == CharacterTitleHeadOfGovernment) {
		return "Prime Minister";
	} else if (title_type == CharacterTitleEducationMinister) {
//		return "Education Minister"; //education minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Master Educator";
	} else if (title_type == CharacterTitleFinanceMinister) {
//		return "Finance Minister"; //finance minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Treasurer";
	} else if (title_type == CharacterTitleForeignMinister) {
//		return "Foreign Minister"; //foreign minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Chancellor";
	} else if (title_type == CharacterTitleIntelligenceMinister) {
//		return "Intelligence Minister"; //intelligence minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Spymaster";
	} else if (title_type == CharacterTitleInteriorMinister) {
//		return "Interior Minister"; //interior minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "High Constable";
	} else if (title_type == CharacterTitleJusticeMinister) {
//		return "Justice Minister"; //justice minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Master of Laws";
	} else if (title_type == CharacterTitleWarMinister) {
//		return "War Minister"; //war minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
		return "Marshal";
	} else if (title_type == CharacterTitleGovernor) {
		return "Governor";
	} else if (title_type == CharacterTitleMayor) {
		return "Mayor";
	}
	
	return "";
}

void CPlayer::GetWorkerLandmasses(std::vector<int>& worker_landmasses, const CUnitType *building)
{
	for (unsigned int i = 0; i < AiHelpers.Build[building->Slot].size(); ++i) {
		if (this->GetUnitTypeAiActiveCount(AiHelpers.Build[building->Slot][i])) {
			std::vector<CUnit *> worker_table;

			FindPlayerUnitsByType(*this, *AiHelpers.Build[building->Slot][i], worker_table, true);

			for (size_t j = 0; j != worker_table.size(); ++j) {
				int worker_landmass = CMap::Map.GetTileLandmass(worker_table[j]->tilePos, worker_table[j]->MapLayer->ID);
				if (std::find(worker_landmasses.begin(), worker_landmasses.end(), worker_landmass) == worker_landmasses.end()) {
					worker_landmasses.push_back(worker_landmass);
				}
			}
		}
	}
}

std::vector<CUpgrade *> CPlayer::GetResearchableUpgrades()
{
	std::vector<CUpgrade *> researchable_upgrades;
	for (std::map<const CUnitType *, int>::iterator iterator = this->UnitTypesAiActiveCount.begin(); iterator != this->UnitTypesAiActiveCount.end(); ++iterator) {
		const CUnitType *type = iterator->first;
		if (type->Slot < ((int) AiHelpers.ResearchedUpgrades.size())) {
			for (size_t i = 0; i < AiHelpers.ResearchedUpgrades[type->Slot].size(); ++i) {
				CUpgrade *upgrade = AiHelpers.ResearchedUpgrades[type->Slot][i];
				if (std::find(researchable_upgrades.begin(), researchable_upgrades.end(), upgrade) == researchable_upgrades.end()) {
					researchable_upgrades.push_back(upgrade);
				}
			}
		}
	}
	
	return researchable_upgrades;
}
//Wyrmgus end

/**
**  Clear all player data excepts members which don't change.
**
**  The fields that are not cleared are
**  UnitLimit, BuildingLimit, TotalUnitLimit and Allow.
*/
void CPlayer::Clear()
{
	std::unique_lock<std::shared_mutex> lock(this->Mutex);
	
	this->Index = 0;
	this->Name.clear();
	this->Type = 0;
	this->Race = 0;
	this->Faction = nullptr;
	this->Religion = nullptr;
	this->Dynasty = nullptr;
	this->Age = nullptr;
	this->Overlord = nullptr;
	this->Vassals.clear();
	this->AiName.clear();
	this->Team = 0;
	this->Enemy = 0;
	this->Allied = 0;
	this->SharedVision = 0;
	this->StartPos.x = 0;
	this->StartPos.y = 0;
	//Wyrmgus start
	this->StartMapLayer = 0;
	//Wyrmgus end
	memset(this->Resources, 0, sizeof(this->Resources));
	memset(this->StoredResources, 0, sizeof(this->StoredResources));
	memset(this->MaxResources, 0, sizeof(this->MaxResources));
	memset(this->LastResources, 0, sizeof(this->LastResources));
	memset(this->Incomes, 0, sizeof(this->Incomes));
	memset(this->Revenue, 0, sizeof(this->Revenue));
	//Wyrmgus start
	memset(this->ResourceDemand, 0, sizeof(this->ResourceDemand));
	memset(this->StoredResourceDemand, 0, sizeof(this->StoredResourceDemand));
	//Wyrmgus end
	this->UnitTypesCount.clear();
	this->UnitTypesUnderConstructionCount.clear();
	this->UnitTypesAiActiveCount.clear();
	//Wyrmgus start
	this->Heroes.clear();
	this->Deities.clear();
	this->UnitsByType.clear();
	this->AiActiveUnitsByType.clear();
	this->AvailableQuests.clear();
	this->CurrentQuests.clear();
	this->CompletedQuests.clear();
	this->AutosellResources.clear();
	for (CPlayerQuestObjective *quest_objective : this->QuestObjectives) {
		delete quest_objective;
	}
	this->QuestObjectives.clear();
	this->Modifiers.clear();
	//Wyrmgus end
	this->AiEnabled = false;
	//Wyrmgus start
	this->Revealed = false;
	//Wyrmgus end
	this->Ai = nullptr;
	this->Units.resize(0);
	this->FreeWorkers.resize(0);
	//Wyrmgus start
	this->LevelUpUnits.resize(0);
	//Wyrmgus end
	this->NumBuildings = 0;
	//Wyrmgus start
	this->NumBuildingsUnderConstruction = 0;
	this->NumTownHalls = 0;
	//Wyrmgus end
	this->Supply = 0;
	this->Demand = 0;
	this->TradeCost = 0;
	// FIXME: can't clear limits since it's initialized already
	//	UnitLimit = 0;
	//	BuildingLimit = 0;
	//	TotalUnitLimit = 0;
	this->Score = 0;
	this->TotalUnits = 0;
	this->TotalBuildings = 0;
	memset(this->TotalResources, 0, sizeof(this->TotalResources));
	this->TotalRazings = 0;
	this->TotalKills = 0;
	//Wyrmgus start
	memset(this->UnitTypeKills, 0, sizeof(this->UnitTypeKills));
	this->LostTownHallTimer = 0;
	this->HeroCooldownTimer = 0;
	//Wyrmgus end
	this->Color = 0;
	this->UpgradeTimers.Clear();
	for (int i = 0; i < MaxCosts; ++i) {
		this->SpeedResourcesHarvest[i] = SPEEDUP_FACTOR;
		this->SpeedResourcesReturn[i] = SPEEDUP_FACTOR;
		//Wyrmgus start
		this->Prices[i] = CResource::GetAll()[i]->BasePrice;
		//Wyrmgus end
	}
	this->SpeedBuild = SPEEDUP_FACTOR;
	this->SpeedTrain = SPEEDUP_FACTOR;
	this->SpeedUpgrade = SPEEDUP_FACTOR;
	this->SpeedResearch = SPEEDUP_FACTOR;
}


void CPlayer::AddUnit(CUnit &unit)
{
	Assert(unit.Player != this);
	Assert(unit.PlayerSlot == static_cast<size_t>(-1));
	unit.PlayerSlot = this->Units.size();
	this->Units.push_back(&unit);
	unit.Player = this;
	Assert(this->Units[unit.PlayerSlot] == &unit);
}

void CPlayer::RemoveUnit(CUnit &unit)
{
	Assert(unit.Player == this);
	//Wyrmgus start
	if (unit.PlayerSlot == -1 || this->Units[unit.PlayerSlot] != &unit) {
		fprintf(stderr, "Error in CPlayer::RemoveUnit: the unit's PlayerSlot doesn't match its position in the player's units array; Unit's PlayerSlot: %zu, Unit Type: \"%s\".\n", unit.PlayerSlot, unit.Type ? unit.Type->Ident.c_str() : "");
		return;
	}
	//Wyrmgus end
	Assert(this->Units[unit.PlayerSlot] == &unit);

	//	unit.Player = nullptr; // we can remove dying unit...
	CUnit *last = this->Units.back();

	this->Units[unit.PlayerSlot] = last;
	last->PlayerSlot = unit.PlayerSlot;
	this->Units.pop_back();
	unit.PlayerSlot = static_cast<size_t>(-1);
	Assert(last == &unit || this->Units[last->PlayerSlot] == last);
}

void CPlayer::UpdateFreeWorkers()
{
	FreeWorkers.clear();
	if (FreeWorkers.capacity() != 0) {
		// Just calling FreeWorkers.clear() is not always appropriate.
		// Certain paths may leave FreeWorkers in an invalid state, so
		// it's safer to re-initialize.
		std::vector<CUnit*>().swap(FreeWorkers);
	}
	const int nunits = this->GetUnitCount();

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = this->GetUnit(i);
		//Wyrmgus start
//		if (unit.IsAlive() && unit.Type->BoolFlag[HARVESTER_INDEX].value && !unit.Removed) {
		if (unit.IsAlive() && unit.Type->BoolFlag[HARVESTER_INDEX].value && !unit.Removed && !unit.Type->BoolFlag[TRADER_INDEX].value) {
		//Wyrmgus end
			if (unit.CurrentAction() == UnitActionStill) {
				FreeWorkers.push_back(&unit);
			}
		}
	}
}

//Wyrmgus start
void CPlayer::PerformResourceTrade()
{
	CUnit *market_unit = this->GetMarketUnit();
	
	if (!market_unit) {
		return;
	}
	
	for (size_t i = 0; i < this->AutosellResources.size(); ++i) {
		const int res = this->AutosellResources[i];
		
		if ((this->Resources[res] + this->StoredResources[res]) >= 100) { //sell 100 per second, as long as there is enough of the resource stored
			market_unit->SellResource(res, this->Index);
		}
		
		//increase price due to domestic demand
		this->StoredResourceDemand[res] += this->GetEffectiveResourceDemand(res);
		while (this->StoredResourceDemand[res] >= 100) {
			this->IncreaseResourcePrice(res);
			this->StoredResourceDemand[res] -= 100;
		}
	}
	
	for (size_t i = 0; i < LuxuryResources.size(); ++i) {
		const int res = LuxuryResources[i];
		
		while ((this->Resources[res] + this->StoredResources[res]) >= 100) {
			market_unit->SellResource(res, this->Index);
		}
		
		//increase price due to domestic demand
		this->StoredResourceDemand[res] += this->GetEffectiveResourceDemand(res);
		while (this->StoredResourceDemand[res] >= 100) {
			this->IncreaseResourcePrice(res);
			this->StoredResourceDemand[res] -= 100;
		}
	}
}

/**
**	@brief	Get whether the player has a market unit
**
**	@return	True if the player has a market unit, or false otherwise
*/
bool CPlayer::HasMarketUnit() const
{
	const int n_m = AiHelpers.SellMarkets[0].size();

	for (int i = 0; i < n_m; ++i) {
		CUnitType &market_type = *AiHelpers.SellMarkets[0][i];

		if (this->GetUnitTypeCount(&market_type)) {
			return true;
		}
	}
	
	return false;
}

/**
**	@brief	Get the player's market unit, if any
**
**	@return	The market unit if present, or null otherwise
*/
CUnit *CPlayer::GetMarketUnit() const
{
	CUnit *market_unit = nullptr;
	
	const int n_m = AiHelpers.SellMarkets[0].size();

	for (int i = 0; i < n_m; ++i) {
		CUnitType &market_type = *AiHelpers.SellMarkets[0][i];

		if (this->GetUnitTypeCount(&market_type)) {
			std::vector<CUnit *> market_table;
			FindPlayerUnitsByType(*this, market_type, market_table);
			if (market_table.size() > 0) {
				market_unit = market_table[SyncRand() % market_table.size()];
				break;
			}
		}
	}
	
	return market_unit;
}

std::vector<int> CPlayer::GetAutosellResources() const
{
	return this->AutosellResources;
}

void CPlayer::AutosellResource(const int resource)
{
	if (std::find(this->AutosellResources.begin(), this->AutosellResources.end(), resource) != this->AutosellResources.end()) {
		this->AutosellResources.erase(std::remove(this->AutosellResources.begin(), this->AutosellResources.end(), resource), this->AutosellResources.end());
	} else {
		this->AutosellResources.push_back(resource);
	}
}

void CPlayer::UpdateLevelUpUnits()
{
	LevelUpUnits.clear();
	if (LevelUpUnits.capacity() != 0) {
		// Just calling LevelUpUnits.clear() is not always appropriate.
		// Certain paths may leave LevelUpUnits in an invalid state, so
		// it's safer to re-initialize.
		std::vector<CUnit*>().swap(LevelUpUnits);
	}
	const int nunits = this->GetUnitCount();

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = this->GetUnit(i);
		if (unit.IsAlive() && unit.Variable[LEVELUP_INDEX].Value >= 1) {
			LevelUpUnits.push_back(&unit);
		}
	}
}

void CPlayer::UpdateQuestPool()
{
	if (CCampaign::GetCurrentCampaign() == nullptr) { // in-game quests only while playing the campaign mode
		return;
	}
	
	bool exausted_available_quests = (this->AvailableQuests.size() == 0);
	
	this->AvailableQuests.clear();
	
	std::vector<CQuest *> potential_quests;
	for (size_t i = 0; i < Quests.size(); ++i) {
		if (this->CanAcceptQuest(Quests[i])) {
			potential_quests.push_back(Quests[i]);
		}
	}
	
	int max_quest_pool_size = 3 - ((int) this->CurrentQuests.size());
	for (int i = 0; i < max_quest_pool_size; ++i) { // fill the quest pool with up to three quests
		if (potential_quests.size() == 0) {
			break;
		}
		this->AvailableQuests.push_back(potential_quests[SyncRand(potential_quests.size())]);
		int available_quest_quantity = this->AvailableQuests.size();
		potential_quests.erase(std::remove(potential_quests.begin(), potential_quests.end(), this->AvailableQuests[available_quest_quantity - 1]), potential_quests.end());
	}
	
	this->AvailableQuestsChanged();

	// notify the player when new quests are available (but only if the player has already exausted the quests available to him, so that they aren't bothered if they choose not to engage with the quest system)
	if (this == CPlayer::GetThisPlayer() && GameCycle >= CYCLES_PER_MINUTE && this->AvailableQuests.size() > 0 && exausted_available_quests && this->NumTownHalls > 0) {
		CPlayer::GetThisPlayer()->Notify("%s", _("New quests available"));
	}
	
	if (this->AiEnabled) { // if is an AI player, accept all quests that it can
		int available_quest_quantity = this->AvailableQuests.size();
		for (int i = (available_quest_quantity  - 1); i >= 0; --i) {
			if (this->CanAcceptQuest(this->AvailableQuests[i])) { // something may have changed, so recheck if the player is able to accept the quest
				this->AcceptQuest(this->AvailableQuests[i]);
			}
		}
	}
}

void CPlayer::AvailableQuestsChanged()
{
	if (this == CPlayer::GetThisPlayer()) {
		for (int i = 0; i < (int) UnitButtonTable.size(); ++i) {
			if (UnitButtonTable[i]->Action != ButtonQuest || UnitButtonTable[i]->Value >= (int) this->AvailableQuests.size()) {
				continue;
			}
			
			const CQuest *quest = this->AvailableQuests[UnitButtonTable[i]->Value];
			UnitButtonTable[i]->Hint = "Quest: " + quest->Name;
			UnitButtonTable[i]->Description = quest->Description + "\n \nObjectives:";
			for (size_t j = 0; j < quest->Objectives.size(); ++j) {
				UnitButtonTable[i]->Description += "\n- " + quest->Objectives[j]->ObjectiveString;
			}
			for (size_t j = 0; j < quest->ObjectiveStrings.size(); ++j) {
				UnitButtonTable[i]->Description += "\n" + quest->ObjectiveStrings[j];
			}
			if (!quest->Rewards.empty()) {
				UnitButtonTable[i]->Description += "\n \nRewards: " + quest->Rewards;
			}
			if (!quest->Hint.empty()) {
				UnitButtonTable[i]->Description += "\n \nHint: " + quest->Hint;
			}
			if (quest->HighestCompletedDifficulty > DifficultyNoDifficulty) {
				std::string highest_completed_difficulty;
				if (quest->HighestCompletedDifficulty == DifficultyEasy) {
					highest_completed_difficulty = "Easy";
				} else if (quest->HighestCompletedDifficulty == DifficultyNormal) {
					highest_completed_difficulty = "Normal";
				} else if (quest->HighestCompletedDifficulty == DifficultyHard) {
					highest_completed_difficulty = "Hard";
				} else if (quest->HighestCompletedDifficulty == DifficultyBrutal) {
					highest_completed_difficulty = "Brutal";
				}
				UnitButtonTable[i]->Description += "\n \nHighest Completed Difficulty: " + highest_completed_difficulty;
			}
			
		}
		
		if (!Selected.empty() && Selected[0]->Type->BoolFlag[TOWNHALL_INDEX].value) {
			UI.ButtonPanel.Update();
		}
	}
}

void CPlayer::UpdateCurrentQuests()
{
	for (size_t i = 0; i < this->QuestObjectives.size(); ++i) {
		CPlayerQuestObjective *objective = this->QuestObjectives[i];
		if (objective->ObjectiveType == HaveResourceObjectiveType) {
			objective->Counter = std::min(this->GetResource(objective->Resource, STORE_BOTH), objective->Quantity);
		} else if (objective->ObjectiveType == ResearchUpgradeObjectiveType) {
			objective->Counter = UpgradeIdAllowed(*this, objective->Upgrade->ID) == 'R' ? 1 : 0;
		} else if (objective->ObjectiveType == RecruitHeroObjectiveType) {
			objective->Counter = this->HasHero(objective->Character) ? 1 : 0;
		}
	}
	
	for (int i = (this->CurrentQuests.size()  - 1); i >= 0; --i) {
		std::string failed_quest = this->HasFailedQuest(this->CurrentQuests[i]);
		if (!failed_quest.empty()) {
			this->FailQuest(this->CurrentQuests[i], failed_quest);
		} else if (this->HasCompletedQuest(this->CurrentQuests[i])) {
			this->CompleteQuest(this->CurrentQuests[i]);
		}
	}
}

void CPlayer::AcceptQuest(CQuest *quest)
{
	if (!quest) {
		return;
	}
	
	this->AvailableQuests.erase(std::remove(this->AvailableQuests.begin(), this->AvailableQuests.end(), quest), this->AvailableQuests.end());
	this->CurrentQuests.push_back(quest);
	
	for (size_t i = 0; i < quest->Objectives.size(); ++i) {
		CPlayerQuestObjective *objective = new CPlayerQuestObjective;
		objective->ObjectiveType = quest->Objectives[i]->ObjectiveType;
		objective->Quest = quest->Objectives[i]->Quest;
		objective->ObjectiveString = quest->Objectives[i]->ObjectiveString;
		objective->Quantity = quest->Objectives[i]->Quantity;
		objective->Resource = quest->Objectives[i]->Resource;
		objective->UnitClass = quest->Objectives[i]->UnitClass;
		objective->UnitTypes = quest->Objectives[i]->UnitTypes;
		objective->Upgrade = quest->Objectives[i]->Upgrade;
		objective->Character = quest->Objectives[i]->Character;
		objective->Unique = quest->Objectives[i]->Unique;
		objective->Settlement = quest->Objectives[i]->Settlement;
		objective->Faction = quest->Objectives[i]->Faction;
		this->QuestObjectives.push_back(objective);
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
	
	if (quest->AcceptEffects) {
		quest->AcceptEffects->pushPreamble();
		quest->AcceptEffects->run();
	}
	
	this->AvailableQuestsChanged();
	
	this->UpdateCurrentQuests();
}

void CPlayer::CompleteQuest(CQuest *quest)
{
	if (std::find(this->CompletedQuests.begin(), this->CompletedQuests.end(), quest) != this->CompletedQuests.end()) {
		return;
	}
	
	RemoveCurrentQuest(quest);
	
	this->CompletedQuests.push_back(quest);
	if (quest->Competitive) {
		quest->CurrentCompleted = true;
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
	
	if (quest->CompletionEffects) {
		quest->CompletionEffects->pushPreamble();
		quest->CompletionEffects->run();
	}
	
	if (this == CPlayer::GetThisPlayer()) {
		SetQuestCompleted(quest->Ident, GameSettings.Difficulty);
		SaveQuestCompletion();
		std::string rewards_string;
		if (!quest->Rewards.empty()) {
			rewards_string = "Rewards: " + quest->Rewards;
		}
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Completed\", \"You have completed the " + quest->Name + " quest!\\n\\n" + rewards_string + "\", nil, \"" + quest->Icon.Name + "\", \"" + PlayerColorNames[quest->PlayerColor] + "\", " + std::to_string((long long) (quest->Icon.Icon ? quest->Icon.Icon->Frame : 0)) + ") end;");
	}
}

void CPlayer::FailQuest(CQuest *quest, std::string fail_reason)
{
	this->RemoveCurrentQuest(quest);
	
	CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
	
	if (quest->FailEffects) {
		quest->FailEffects->pushPreamble();
		quest->FailEffects->run();
	}
	
	if (this == CPlayer::GetThisPlayer()) {
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Failed\", \"You have failed the " + quest->Name + " quest! " + fail_reason + "\", nil, \"" + quest->Icon.Name + "\", \"" + PlayerColorNames[quest->PlayerColor] + "\") end;");
	}
}

void CPlayer::RemoveCurrentQuest(CQuest *quest)
{
	this->CurrentQuests.erase(std::remove(this->CurrentQuests.begin(), this->CurrentQuests.end(), quest), this->CurrentQuests.end());
	
	for (int i = (this->QuestObjectives.size()  - 1); i >= 0; --i) {
		if (this->QuestObjectives[i]->Quest == quest) {
			this->QuestObjectives.erase(std::remove(this->QuestObjectives.begin(), this->QuestObjectives.end(), this->QuestObjectives[i]), this->QuestObjectives.end());
		}
	}
}

bool CPlayer::CanAcceptQuest(CQuest *quest)
{
	if (quest->Hidden || quest->CurrentCompleted || quest->Unobtainable) {
		return false;
	}
	
	if (std::find(this->CurrentQuests.begin(), this->CurrentQuests.end(), quest) != this->CurrentQuests.end() || std::find(this->CompletedQuests.begin(), this->CompletedQuests.end(), quest) != this->CompletedQuests.end()) {
		return false;
	}
	
	int recruit_heroes_quantity = 0;
	for (size_t i = 0; i < quest->Objectives.size(); ++i) {
		CQuestObjective *objective = quest->Objectives[i];
		if (objective->ObjectiveType == BuildUnitsObjectiveType || objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
			std::vector<const CUnitType *> unit_types = objective->UnitTypes;
			if (objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
				int unit_type_id = CFaction::GetFactionClassUnitType(this->Faction, objective->UnitClass);
				if (unit_type_id == -1) {
					return false;
				}
				unit_types.clear();
				unit_types.push_back(CUnitType::UnitTypes[unit_type_id]);
			}

			bool validated = false;
			for (const CUnitType *unit_type : unit_types) {
				if (objective->Settlement && !this->HasSettlement(objective->Settlement) && !unit_type->BoolFlag[TOWNHALL_INDEX].value) {
					continue;
				}

				if (!this->HasUnitBuilder(unit_type, objective->Settlement) || !CheckDependencies(unit_type, this)) {
					continue;
				}

				validated = true;
			}

			if (!validated) {
				return false;
			}
		} else if (objective->ObjectiveType == ResearchUpgradeObjectiveType) {
			const CUpgrade *upgrade = objective->Upgrade;
			
			bool has_researcher = this->HasUpgradeResearcher(upgrade);
				
			if (!has_researcher && upgrade->ID < (int) AiHelpers.Research.size()) { //check if the quest includes an objective to build a researcher of the upgrade
				for (CQuestObjective *second_objective : quest->Objectives) {
					if (second_objective == objective) {
						continue;
					}
						
					if (second_objective->ObjectiveType == BuildUnitsObjectiveType || second_objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
						std::vector<const CUnitType *> unit_types = second_objective->UnitTypes;
						if (second_objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
							int unit_type_id = CFaction::GetFactionClassUnitType(this->Faction, second_objective->UnitClass);
							if (unit_type_id == -1) {
								continue;
							}
							unit_types.clear();
							unit_types.push_back(CUnitType::UnitTypes[unit_type_id]);
						}

						for (const CUnitType *unit_type : unit_types) {
							if (std::find(AiHelpers.Research[upgrade->ID].begin(), AiHelpers.Research[upgrade->ID].end(), unit_type) != AiHelpers.Research[upgrade->ID].end()) { //if the unit type of the other objective is a researcher of this upgrade
								has_researcher = true;
								break;
							}
						}

						if (has_researcher) {
							break;
						}
					}
				}
			}
				
			if (!has_researcher || this->Allow.Upgrades[upgrade->ID] != 'A' || !CheckDependencies(upgrade, this)) {
				return false;
			}
		} else if (objective->ObjectiveType == RecruitHeroObjectiveType) {
			if (!this->CanRecruitHero(objective->Character, true)) {
				return false;
			}
			recruit_heroes_quantity++;
		} else if (objective->ObjectiveType == DestroyUnitsObjectiveType || objective->ObjectiveType == DestroyHeroObjectiveType || objective->ObjectiveType == DestroyUniqueObjectiveType) {
			if (objective->Faction) {
				CPlayer *faction_player = GetFactionPlayer(objective->Faction);
				if (faction_player == nullptr || faction_player->GetUnitCount() == 0) {
					return false;
				}
				
				if (objective->Settlement && !faction_player->HasSettlement(objective->Settlement)) {
					return false;
				}
			}
			
			if (objective->ObjectiveType == DestroyHeroObjectiveType) {
				if (objective->Character->CanAppear()) { //if the character "can appear" it doesn't already exist, and thus can't be destroyed
					return false;
				}
			} else if (objective->ObjectiveType == DestroyUniqueObjectiveType) {
				if (objective->Unique->CanDrop()) { //if the unique "can drop" it doesn't already exist, and thus can't be destroyed
					return false;
				}
			}
		} else if (objective->ObjectiveType == DestroyFactionObjectiveType) {
			CPlayer *faction_player = GetFactionPlayer(objective->Faction);
			if (faction_player == nullptr || faction_player->GetUnitCount() == 0) {
				return false;
			}
		}
	}
	
	if (recruit_heroes_quantity > 0 && (this->Heroes.size() + recruit_heroes_quantity) > PlayerHeroMax) {
		return false;
	}
	
	for (size_t i = 0; i < quest->HeroesMustSurvive.size(); ++i) {
		if (!this->HasHero(quest->HeroesMustSurvive[i])) {
			return false;
		}
	}

	if (quest->Conditions) {
		CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
		quest->Conditions->pushPreamble();
		quest->Conditions->run(1);
		return quest->Conditions->popBoolean();
	} else {
		return true;
	}
}

bool CPlayer::HasCompletedQuest(CQuest *quest)
{
	if (quest->Uncompleteable) {
		return false;
	}
	
	for (size_t i = 0; i < this->QuestObjectives.size(); ++i) {
		if (this->QuestObjectives[i]->Quest != quest) {
			continue;
		}
		if (this->QuestObjectives[i]->Quantity > 0 && this->QuestObjectives[i]->Counter < this->QuestObjectives[i]->Quantity) {
			return false;
		}
	}
	
	return true;
}

std::string CPlayer::HasFailedQuest(CQuest *quest) // returns the reason for failure (empty if none)
{
	for (size_t i = 0; i < quest->HeroesMustSurvive.size(); ++i) { // put it here, because "unfailable" quests should also fail when a hero which should survive dies
		if (!this->HasHero(quest->HeroesMustSurvive[i])) {
			return "A hero necessary for the quest has died.";
		}
	}
	
	if (quest->CurrentCompleted) { // quest already completed by someone else
		return "Another faction has completed the quest before you could.";
	}

	if (quest->Unfailable) {
		return "";
	}

	for (size_t i = 0; i < this->QuestObjectives.size(); ++i) {
		CPlayerQuestObjective *objective = this->QuestObjectives[i];
		if (objective->Quest != quest) {
			continue;
		}
		if (objective->ObjectiveType == BuildUnitsObjectiveType || objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
			if (objective->Counter < objective->Quantity) {
				std::vector<const CUnitType *> unit_types = objective->UnitTypes;
				if (objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
					int unit_type_id = CFaction::GetFactionClassUnitType(this->Faction, objective->UnitClass);
					if (unit_type_id == -1) {
						return "You can no longer produce the required unit.";
					}
					unit_types.clear();
					unit_types.push_back(CUnitType::UnitTypes[unit_type_id]);
				}
				
				bool validated = false;
				std::string validation_error;
				for (const CUnitType *unit_type : unit_types) {
					if (objective->Settlement && !this->HasSettlement(objective->Settlement) && !unit_type->BoolFlag[TOWNHALL_INDEX].value) {
						validation_error = "You no longer hold the required settlement.";
						continue;
					}

					if (!this->HasUnitBuilder(unit_type, objective->Settlement) || !CheckDependencies(unit_type, this)) {
						validation_error = "You can no longer produce the required unit.";
						continue;
					}

					validated = true;
				}

				if (!validated) {
					return validation_error;
				}
			}
		} else if (objective->ObjectiveType == ResearchUpgradeObjectiveType) {
			const CUpgrade *upgrade = objective->Upgrade;
			
			if (this->Allow.Upgrades[upgrade->ID] != 'R') {
				bool has_researcher = this->HasUpgradeResearcher(upgrade);
				
				if (!has_researcher && upgrade->ID < (int) AiHelpers.Research.size()) { //check if the quest includes an objective to build a researcher of the upgrade
					for (CPlayerQuestObjective *second_objective : this->QuestObjectives) {
						if (second_objective->Quest != quest || second_objective == objective || second_objective->Counter >= second_objective->Quantity) { //if the objective has been fulfilled, then there should be a researcher, if there isn't it is due to i.e. the researcher having been destroyed later on, or upgraded to another type, and then the quest should fail if the upgrade can no longer be researched
							continue;
						}
						
						if (second_objective->ObjectiveType == BuildUnitsObjectiveType || second_objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
							std::vector<const CUnitType *> unit_types = second_objective->UnitTypes;
							if (second_objective->ObjectiveType == BuildUnitsOfClassObjectiveType) {
								int unit_type_id = CFaction::GetFactionClassUnitType(this->Faction, second_objective->UnitClass);
								if (unit_type_id == -1) {
									continue;
								}
								unit_types.clear();
								unit_types.push_back(CUnitType::UnitTypes[unit_type_id]);
							}

							for (const CUnitType *unit_type : unit_types) {
								if (std::find(AiHelpers.Research[upgrade->ID].begin(), AiHelpers.Research[upgrade->ID].end(), unit_type) != AiHelpers.Research[upgrade->ID].end()) { //if the unit type of the other objective is a researcher of this upgrade
									has_researcher = true;
									break;
								}
							}

							if (has_researcher) {
								break;
							}
						}
					}
				}
				
				if (!has_researcher || this->Allow.Upgrades[upgrade->ID] != 'A' || !CheckDependencies(upgrade, this)) {
					return "You can no longer research the required upgrade.";
				}
			}
		} else if (objective->ObjectiveType == RecruitHeroObjectiveType) {
			if (!this->HasHero(objective->Character) && !this->CanRecruitHero(objective->Character, true)) {
				return "The hero can no longer be recruited.";
			}
		} else if (objective->ObjectiveType == DestroyUnitsObjectiveType || objective->ObjectiveType == DestroyHeroObjectiveType || objective->ObjectiveType == DestroyUniqueObjectiveType) {
			if (objective->Faction && objective->Counter < objective->Quantity) {
				CPlayer *faction_player = GetFactionPlayer(objective->Faction);
				if (faction_player == nullptr || faction_player->GetUnitCount() == 0) {
					return "The target no longer exists.";
				}
				
				if (objective->Settlement && !faction_player->HasSettlement(objective->Settlement)) {
					return "The target no longer exists.";
				}
			}
			
			if (objective->ObjectiveType == DestroyHeroObjectiveType) {
				if (objective->Counter == 0 && objective->Character->CanAppear()) {  // if is supposed to destroy a character, but it is nowhere to be found, fail the quest
					return "The target no longer exists.";
				}
			} else if (objective->ObjectiveType == DestroyUniqueObjectiveType) {
				if (objective->Counter == 0 && objective->Unique->CanDrop()) {  // if is supposed to destroy a unique, but it is nowhere to be found, fail the quest
					return "The target no longer exists.";
				}
			}
		} else if (objective->ObjectiveType == DestroyFactionObjectiveType) {
			if (objective->Counter == 0) {  // if is supposed to destroy a faction, but it is nowhere to be found, fail the quest
				CPlayer *faction_player = GetFactionPlayer(objective->Faction);
				if (faction_player == nullptr || faction_player->GetUnitCount() == 0) {
					return "The target no longer exists.";
				}
			}
		}
	}
	
	return "";
}

void CPlayer::AddModifier(CUpgrade *modifier, int cycles)
{
	if (this->Allow.Upgrades[modifier->ID] == 'R') {
		for (size_t i = 0; i < this->Modifiers.size(); ++i) { //if already has the modifier, make it have the greater duration of the new or old one
			if (this->Modifiers[i].first == modifier) {
				this->Modifiers[i].second = std::max(this->Modifiers[i].second, (int) (GameCycle + cycles));
			}
		}
	} else {
		this->Modifiers.push_back(std::pair<CUpgrade *, int>(modifier, GameCycle + cycles));
		UpgradeAcquire(*this, modifier);
	}
	
}

void CPlayer::RemoveModifier(CUpgrade *modifier)
{
	if (this->Allow.Upgrades[modifier->ID] == 'R') {
		UpgradeLost(*this, modifier->ID);
		for (size_t i = 0; i < this->Modifiers.size(); ++i) { //if already has the modifier, make it have the greater duration of the new or old one
			if (this->Modifiers[i].first == modifier) {
				this->Modifiers.erase(std::remove(this->Modifiers.begin(), this->Modifiers.end(), this->Modifiers[i]), this->Modifiers.end());
				break;
			}
		}
	}
	
}

bool CPlayer::AtPeace() const
{
	for (int i = 0; i < PlayerNumNeutral; ++i) {
		if (this->IsEnemy(*CPlayer::Players[i]) && this->HasContactWith(*CPlayer::Players[i]) && CPlayer::Players[i]->GetUnitCount() > 0) {
			return false;
		}
	}
	
	return true;
}
//Wyrmgus end

std::vector<CUnit *>::const_iterator CPlayer::UnitBegin() const
{
	return Units.begin();
}

std::vector<CUnit *>::iterator CPlayer::UnitBegin()
{
	return Units.begin();
}

std::vector<CUnit *>::const_iterator CPlayer::UnitEnd() const
{
	return Units.end();
}

std::vector<CUnit *>::iterator CPlayer::UnitEnd()
{
	return Units.end();
}

CUnit &CPlayer::GetUnit(int index) const
{
	return *Units[index];
}

int CPlayer::GetUnitCount() const
{
	return static_cast<int>(Units.size());
}



/*----------------------------------------------------------------------------
--  Resource management
----------------------------------------------------------------------------*/

/**
**  Gets the player resource.
**
**  @param resource  Resource to get.
**  @param type      Storing type
**
**  @note Storing types: 0 - overall store, 1 - store buildings, 2 - both
*/
int CPlayer::GetResource(const int resource, const int type)
{
	switch (type) {
		case STORE_OVERALL:
			return this->Resources[resource];
		case STORE_BUILDING:
			return this->StoredResources[resource];
		case STORE_BOTH:
			return this->Resources[resource] + this->StoredResources[resource];
		default:
			DebugPrint("Wrong resource type\n");
			return -1;
	}
}

/**
**  Adds/subtracts some resources to/from the player store
**
**  @param resource  Resource to add/subtract.
**  @param value     How many of this resource (can be negative).
**  @param store     If true, sets the building store resources, else the overall resources.
*/
void CPlayer::ChangeResource(const int resource, const int value, const bool store)
{
	if (value < 0) {
		const int fromStore = std::min(this->StoredResources[resource], abs(value));
		this->StoredResources[resource] -= fromStore;
		this->Resources[resource] -= abs(value) - fromStore;
		this->Resources[resource] = std::max(this->Resources[resource], 0);
	} else {
		if (store && this->MaxResources[resource] != -1) {
			this->StoredResources[resource] += std::min(value, this->MaxResources[resource] - this->StoredResources[resource]);
		} else {
			this->Resources[resource] += value;
		}
	}
}

/**
**  Change the player resource.
**
**  @param resource  Resource to change.
**  @param value     How many of this resource.
**  @param type      Resource types: 0 - overall store, 1 - store buildings, 2 - both
*/
void CPlayer::SetResource(const int resource, const int value, const int type)
{
	if (type == STORE_BOTH) {
		if (this->MaxResources[resource] != -1) {
			const int toRes = std::max(0, value - this->StoredResources[resource]);
			this->Resources[resource] = std::max(0, toRes);
			this->StoredResources[resource] = std::min(value - toRes, this->MaxResources[resource]);
		} else {
			this->Resources[resource] = value;
		}
	} else if (type == STORE_BUILDING && this->MaxResources[resource] != -1) {
		this->StoredResources[resource] = std::min(value, this->MaxResources[resource]);
	} else if (type == STORE_OVERALL) {
		this->Resources[resource] = value;
	}
}

/**
**  Check, if there enough resources for action.
**
**  @param resource  Resource to change.
**  @param value     How many of this resource.
*/
bool CPlayer::CheckResource(const int resource, const int value)
{
	int result = this->Resources[resource];
	if (this->MaxResources[resource] != -1) {
		result += this->StoredResources[resource];
	}
	return result < value ? false : true;
}

//Wyrmgus start
/**
**  Increase resource price
**
**  @param resource  Resource.
*/
void CPlayer::IncreaseResourcePrice(const int resource)
{
	int price_change = CResource::GetAll()[resource]->BasePrice / std::max(this->Prices[resource], 100);
	price_change = std::max(1, price_change);
	this->Prices[resource] += price_change;
}

/**
**  Decrease resource price
**
**  @param resource  Resource.
*/
void CPlayer::DecreaseResourcePrice(const int resource)
{
	int price_change = this->Prices[resource] / CResource::GetAll()[resource]->BasePrice;
	price_change = std::max(1, price_change);
	this->Prices[resource] -= price_change;
	this->Prices[resource] = std::max(1, this->Prices[resource]);
}

/**
**  Converges prices with another player (and returns how many convergences were effected)
*/
int CPlayer::ConvergePricesWith(CPlayer &player, int max_convergences)
{
	int convergences = 0;
	
	bool converged = true;
	while (converged) {
		converged = false;

		for (int i = 1; i < MaxCosts; ++i) {
			if (!CResource::GetAll()[i]->BasePrice) {
				continue;
			}
			
			int convergence_increase = 100;
			
			if (this->Prices[i] < player.Prices[i] && convergences < max_convergences) {
				this->IncreaseResourcePrice(i);
				convergences += convergence_increase;
				converged = true;
				
				if (this->Prices[i] < player.Prices[i] && convergences < max_convergences) { //now do the convergence for the other side as well, if possible
					player.DecreaseResourcePrice(i);
					convergences += convergence_increase;
					converged = true;
				}
			} else if (this->Prices[i] > player.Prices[i] && convergences < max_convergences) {
				this->DecreaseResourcePrice(i);
				convergences += convergence_increase;
				converged = true;

				if (this->Prices[i] > player.Prices[i] && convergences < max_convergences) { //do the convergence for the other side as well, if possible
					player.IncreaseResourcePrice(i);
					convergences += convergence_increase;
					converged = true;
				}
			}
		}
	}
	
	return convergences;
}

/**
**  Get the price of a resource for the player
**
**  @param resource  Resource.
*/
int CPlayer::GetResourcePrice(const int resource) const
{
	if (resource == CopperCost) {
		return 100;
	}
	
	return this->Prices[resource];
}

/**
**  Get the effective resource demand for the player, given the current prices
**
**  @param resource  Resource.
*/
int CPlayer::GetEffectiveResourceDemand(const int resource) const
{
	int resource_demand = this->ResourceDemand[resource];
	
	if (this->Prices[resource]) {
		resource_demand *= CResource::GetAll()[resource]->BasePrice;
		resource_demand /= this->Prices[resource];
	}
	
	if (CResource::GetAll()[resource]->DemandElasticity != 100) {
		resource_demand = this->ResourceDemand[resource] + ((resource_demand - this->ResourceDemand[resource]) * CResource::GetAll()[resource]->DemandElasticity / 100);
	}
	
	resource_demand = std::max(resource_demand, 0);

	return resource_demand;
}

/**
**  Get the effective sell price of a resource
*/
int CPlayer::GetEffectiveResourceSellPrice(const int resource, int traded_quantity) const
{
	if (resource == CopperCost) {
		return 100;
	}
	
	int price = traded_quantity * this->Prices[resource] / 100 * (100 - this->TradeCost) / 100;
	price = std::max(1, price);
	return price;
}

/**
**  Get the effective buy quantity of a resource
*/
int CPlayer::GetEffectiveResourceBuyPrice(const int resource, int traded_quantity) const
{
	int price = traded_quantity * this->Prices[resource] / 100 * 100 / (100 - this->TradeCost);
	price = std::max(1, price);
	return price;
}

/**
**  Get the total price difference between this player and another one
*/
int CPlayer::GetTotalPriceDifferenceWith(const CPlayer &player) const
{
	int difference = 0;
	for (int i = 1; i < MaxCosts; ++i) {
		if (!CResource::GetAll()[i]->BasePrice) {
			continue;
		}
		difference += abs(this->Prices[i] - player.Prices[i]);
	}

	return difference;
}

/**
**  Get the trade potential between this player and another one
*/
int CPlayer::GetTradePotentialWith(const CPlayer &player) const
{
	int trade_potential = 0;
	for (int i = 1; i < MaxCosts; ++i) {
		if (!CResource::GetAll()[i]->BasePrice) {
			continue;
		}
		int price_difference = abs(this->Prices[i] - player.Prices[i]);
		trade_potential += price_difference * 100;
	}
	
	trade_potential = std::max(trade_potential, 10);
	
	return trade_potential;
}
//Wyrmgus end

int CPlayer::GetUnitTotalCount(const CUnitType &type) const
{
	int count = this->GetUnitTypeCount(&type);
	for (std::vector<CUnit *>::const_iterator it = this->UnitBegin(); it != this->UnitEnd(); ++it) {
		//Wyrmgus start
		if (*it == nullptr) {
			fprintf(stderr, "Error in CPlayer::GetUnitTotalCount: unit of player %i is null.\n", this->Index);
			continue;
		}
		//Wyrmgus end
		CUnit &unit = **it;

		if (unit.CurrentAction() == UnitActionUpgradeTo) {
			COrder_UpgradeTo &order = dynamic_cast<COrder_UpgradeTo &>(*unit.CurrentOrder());
			if (order.GetUnitType().Slot == type.Slot) {
				++count;
			}
		}
	}
	return count;
}

/**
**  Check if the unit-type didn't break any unit limits.
**
**  @param type    Type of unit.
**
**  @return        True if enough, negative on problem.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckLimits(const CUnitType &type) const
{
	//  Check game limits.
	if (type.BoolFlag[BUILDING_INDEX].value && NumBuildings >= BuildingLimit) {
		Notify("%s", _("Building Limit Reached"));
		return -1;
	}
	if (!type.BoolFlag[BUILDING_INDEX].value && (this->GetUnitCount() - NumBuildings) >= UnitLimit) {
		Notify("%s", _("Unit Limit Reached"));
		return -2;
	}
	//Wyrmgus start
//	if (this->Demand + type.Stats[this->Index].Variables[DEMAND_INDEX].Value > this->Supply && type.Stats[this->Index].Variables[DEMAND_INDEX].Value) {
	if (this->Demand + (type.Stats[this->Index].Variables[DEMAND_INDEX].Value * (type.TrainQuantity ? type.TrainQuantity : 1)) > this->Supply && type.Stats[this->Index].Variables[DEMAND_INDEX].Value) {
	//Wyrmgus end
		//Wyrmgus start
//		Notify("%s", _("Insufficient Supply, increase Supply."));
		Notify("%s", _("Insufficient Food Supply, increase Food Supply."));
		//Wyrmgus end
		return -3;
	}
	if (this->GetUnitCount() >= TotalUnitLimit) {
		Notify("%s", _("Total Unit Limit Reached"));
		return -4;
	}
	if (GetUnitTotalCount(type) >= Allow.Units[type.Slot]) {
		Notify(_("Limit of %i reached for this unit type"), Allow.Units[type.Slot]);
		return -6;
	}
	return 1;
}

/**
**  Check if enough resources for are available.
**
**  @param costs   How many costs.
**
**  @return        False if all enough, otherwise a bit mask.
**
**  @note The return values of the PlayerCheck functions are inconsistent.
*/
int CPlayer::CheckCosts(const int *costs, bool notify) const
{
	//Wyrmgus start
	bool sound_played = false;
	//Wyrmgus end
	int err = 0;
	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Resources[i] + this->StoredResources[i] >= costs[i]) {
			continue;
		}
		if (notify) {
			const char *name = DefaultResourceNames[i].c_str();
			const char *actionName = CResource::GetAll()[i]->ActionName.c_str();

			Notify(_("Not enough %s... %s more %s."), _(name), _(actionName), _(name));

			//Wyrmgus start
//			if (this == CPlayer::GetThisPlayer() && GameSounds.NotEnoughRes[this->Race][i].Sound) {
			if (this == CPlayer::GetThisPlayer() && GameSounds.NotEnoughRes[this->Race][i].Sound && !sound_played) {
				sound_played = true;
			//Wyrmgus end
				PlayGameSound(GameSounds.NotEnoughRes[this->Race][i].Sound, MaxSampleVolume);
			}
		}
		err |= 1 << i;
	}
	return err;
}

/**
**  Check if enough resources for new unit is available.
**
**  @param type    Type of unit.
**
**  @return        False if all enough, otherwise a bit mask.
*/
//Wyrmgus start
//int CPlayer::CheckUnitType(const CUnitType &type) const
int CPlayer::CheckUnitType(const CUnitType &type, bool hire) const
//Wyrmgus end
{
	//Wyrmgus start
//	return this->CheckCosts(type.Stats[this->Index].Costs);
	int type_costs[MaxCosts];
	this->GetUnitTypeCosts(&type, type_costs, hire);
	return this->CheckCosts(type_costs);
	//Wyrmgus end
}

/**
**  Add costs to the resources
**
**  @param costs   How many costs.
*/
void CPlayer::AddCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		ChangeResource(i, costs[i], false);
	}
}

/**
**  Add the costs of an unit type to resources
**
**  @param type    Type of unit.
*/
//Wyrmgus start
//void CPlayer::AddUnitType(const CUnitType &type)
void CPlayer::AddUnitType(const CUnitType &type, bool hire)
//Wyrmgus end
{
	//Wyrmgus start
//	AddCosts(type.Stats[this->Index].Costs);
	int type_costs[MaxCosts];
	this->GetUnitTypeCosts(&type, type_costs, hire);
	AddCostsFactor(type_costs, 100);
	//Wyrmgus end
}

/**
**  Add a factor of costs to the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::AddCostsFactor(const int *costs, int factor)
{
	if (!factor) {
		return;
	}
	
	for (int i = 1; i < MaxCosts; ++i) {
		ChangeResource(i, costs[i] * factor / 100, true);
	}
}

/**
**  Subtract costs from the resources
**
**  @param costs   How many costs.
*/
void CPlayer::SubCosts(const int *costs)
{
	for (int i = 1; i < MaxCosts; ++i) {
		ChangeResource(i, -costs[i], true);
	}
}

/**
**  Subtract the costs of new unit from resources
**
**  @param type    Type of unit.
*/
//Wyrmgus start
//void CPlayer::SubUnitType(const CUnitType &type)
void CPlayer::SubUnitType(const CUnitType &type, bool hire)
//Wyrmgus end
{
	//Wyrmgus start
//	this->SubCosts(type.Stats[this->Index].Costs);
	int type_costs[MaxCosts];
	this->GetUnitTypeCosts(&type, type_costs, hire);
	this->SubCostsFactor(type_costs, 100);
	//Wyrmgus end
}

/**
**  Subtract a factor of costs from the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::SubCostsFactor(const int *costs, int factor)
{
	for (int i = 1; i < MaxCosts; ++i) {
		ChangeResource(i, -costs[i] * 100 / factor);
	}
}

//Wyrmgus start
/**
**  Gives the cost of a unit type for the player
*/
void CPlayer::GetUnitTypeCosts(const CUnitType *type, int *type_costs, bool hire, bool ignore_one) const
{
	for (int i = 0; i < MaxCosts; ++i) {
		type_costs[i] = 0;
	}
	if (hire) {
		type_costs[CopperCost] = type->Stats[this->Index].GetPrice();
	} else {
		for (int i = 0; i < MaxCosts; ++i) {
			type_costs[i] = type->Stats[this->Index].Costs[i];
		}
	}
	for (int i = 0; i < MaxCosts; ++i) {
		if (type->TrainQuantity) {
			type_costs[i] *= type->TrainQuantity;
		}
		if (type->CostModifier) {
			int type_count = this->GetUnitTypeCount(type) + this->GetUnitTypeUnderConstructionCount(type);
			if (ignore_one) {
				type_count--;
			}
			for (int j = 0; j < type_count; ++j) {
				type_costs[i] *= 100 + type->CostModifier;
				type_costs[i] /= 100;
			}
		}
	}
}

int CPlayer::GetUnitTypeCostsMask(const CUnitType *type, bool hire) const
{
	int costs_mask = 0;
	
	int type_costs[MaxCosts];
	AiPlayer->Player->GetUnitTypeCosts(type, type_costs, hire);
	
	for (int i = 1; i < MaxCosts; ++i) {
		if (type_costs[i] > 0) {
			costs_mask |= 1 << i;
		}
	}
	
	return costs_mask;
}

/**
**  Gives the cost of an upgrade for the player
*/
void CPlayer::GetUpgradeCosts(const CUpgrade *upgrade, int *upgrade_costs)
{
	for (int i = 0; i < MaxCosts; ++i) {
		upgrade_costs[i] = upgrade->Costs[i];
		for (size_t j = 0; j < upgrade->ScaledCostUnits.size(); ++j) {
			upgrade_costs[i] += upgrade->ScaledCosts[i] * this->GetUnitTypeCount(upgrade->ScaledCostUnits[j]);
		}
	}
}

int CPlayer::GetUpgradeCostsMask(const CUpgrade *upgrade) const
{
	int costs_mask = 0;
	
	int upgrade_costs[MaxCosts];
	AiPlayer->Player->GetUpgradeCosts(upgrade, upgrade_costs);
	
	for (int i = 1; i < MaxCosts; ++i) {
		if (upgrade_costs[i] > 0) {
			costs_mask |= 1 << i;
		}
	}
	
	return costs_mask;
}

//Wyrmgus end

void CPlayer::SetUnitTypeCount(const CUnitType *type, int quantity)
{
	if (!type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitTypesCount.find(type) != this->UnitTypesCount.end()) {
			this->UnitTypesCount.erase(type);
		}
	} else {
		this->UnitTypesCount[type] = quantity;
	}
}

void CPlayer::ChangeUnitTypeCount(const CUnitType *type, int quantity)
{
	this->SetUnitTypeCount(type, this->GetUnitTypeCount(type) + quantity);
}

int CPlayer::GetUnitTypeCount(const CUnitType *type) const
{
	if (type && this->UnitTypesCount.find(type) != this->UnitTypesCount.end()) {
		return this->UnitTypesCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::SetUnitTypeUnderConstructionCount(const CUnitType *type, int quantity)
{
	if (!type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitTypesUnderConstructionCount.find(type) != this->UnitTypesUnderConstructionCount.end()) {
			this->UnitTypesUnderConstructionCount.erase(type);
		}
	} else {
		this->UnitTypesUnderConstructionCount[type] = quantity;
	}
}

void CPlayer::ChangeUnitTypeUnderConstructionCount(const CUnitType *type, int quantity)
{
	this->SetUnitTypeUnderConstructionCount(type, this->GetUnitTypeUnderConstructionCount(type) + quantity);
}

int CPlayer::GetUnitTypeUnderConstructionCount(const CUnitType *type) const
{
	if (type && this->UnitTypesUnderConstructionCount.find(type) != this->UnitTypesUnderConstructionCount.end()) {
		return this->UnitTypesUnderConstructionCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::SetUnitTypeAiActiveCount(const CUnitType *type, int quantity)
{
	if (!type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitTypesAiActiveCount.find(type) != this->UnitTypesAiActiveCount.end()) {
			this->UnitTypesAiActiveCount.erase(type);
		}
	} else {
		this->UnitTypesAiActiveCount[type] = quantity;
	}
}

void CPlayer::ChangeUnitTypeAiActiveCount(const CUnitType *type, int quantity)
{
	this->SetUnitTypeAiActiveCount(type, this->GetUnitTypeAiActiveCount(type) + quantity);
}

int CPlayer::GetUnitTypeAiActiveCount(const CUnitType *type) const
{
	if (type && this->UnitTypesAiActiveCount.find(type) != this->UnitTypesAiActiveCount.end()) {
		return this->UnitTypesAiActiveCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::IncreaseCountsForUnit(CUnit *unit, bool type_change)
{
	const CUnitType *type = unit->Type;

	this->ChangeUnitTypeCount(type, 1);
	this->UnitsByType[type].push_back(unit);
	
	if (unit->Active) {
		this->ChangeUnitTypeAiActiveCount(type, 1);
		this->AiActiveUnitsByType[type].push_back(unit);
	}

	if (type->BoolFlag[TOWNHALL_INDEX].value) {
		this->NumTownHalls++;
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		this->ResourceDemand[i] += type->Stats[this->Index].ResourceDemand[i];
	}
	
	if (this->AiEnabled && type->BoolFlag[COWARD_INDEX].value && !type->BoolFlag[HARVESTER_INDEX].value && !type->CanTransport() && type->Spells.size() == 0 && CMap::Map.Info.IsPointOnMap(unit->tilePos, unit->MapLayer) && unit->CanMove() && unit->Active && unit->GroupId != 0 && unit->Variable[SIGHTRANGE_INDEX].Value > 0) { //assign coward, non-worker, non-transporter, non-spellcaster units to be scouts
		this->Ai->Scouts.push_back(unit);
	}
	
	if (!type_change) {
		if (unit->Character != nullptr) {
			this->Heroes.push_back(unit);
		}
	}
}

void CPlayer::DecreaseCountsForUnit(CUnit *unit, bool type_change)
{
	const CUnitType *type = unit->Type;

	this->ChangeUnitTypeCount(type, -1);
	
	this->UnitsByType[type].erase(std::remove(this->UnitsByType[type].begin(), this->UnitsByType[type].end(), unit), this->UnitsByType[type].end());
			
	if (this->UnitsByType[type].empty()) {
		this->UnitsByType.erase(type);
	}
	
	if (unit->Active) {
		this->ChangeUnitTypeAiActiveCount(type, -1);
		
		this->AiActiveUnitsByType[type].erase(std::remove(this->AiActiveUnitsByType[type].begin(), this->AiActiveUnitsByType[type].end(), unit), this->AiActiveUnitsByType[type].end());
		
		if (this->AiActiveUnitsByType[type].empty()) {
			this->AiActiveUnitsByType.erase(type);
		}
	}
	
	if (type->BoolFlag[TOWNHALL_INDEX].value) {
		this->NumTownHalls--;
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		this->ResourceDemand[i] -= type->Stats[this->Index].ResourceDemand[i];
	}
	
	if (this->AiEnabled && this->Ai && std::find(this->Ai->Scouts.begin(), this->Ai->Scouts.end(), unit) != this->Ai->Scouts.end()) {
		this->Ai->Scouts.erase(std::remove(this->Ai->Scouts.begin(), this->Ai->Scouts.end(), unit), this->Ai->Scouts.end());
	}
	
	if (!type_change) {
		if (unit->Character != nullptr) {
			this->Heroes.erase(std::remove(this->Heroes.begin(), this->Heroes.end(), unit), this->Heroes.end());
		}
	}
}

/**
**  Have unit of type.
**
**  @param type    Type of unit.
**
**  @return        How many exists, false otherwise.
*/
int CPlayer::HaveUnitTypeByType(const CUnitType &type) const
{
	return this->GetUnitTypeCount(&type);
}

/**
**  Have unit of type.
**
**  @param ident   Identifier of unit-type that should be lookuped.
**
**  @return        How many exists, false otherwise.
**
**  @note This function should not be used during run time.
*/
int CPlayer::HaveUnitTypeByIdent(const std::string &ident) const
{
	return this->GetUnitTypeCount(UnitTypeByIdent(ident));
}

/**
**  Initialize the Ai for all players.
*/
void PlayersInitAi()
{
	for (int player = 0; player < NumPlayers; ++player) {
		if (CPlayer::Players[player]->AiEnabled) {
			AiInit(*CPlayer::Players[player]);
		}
	}
}

/**
**  Handle AI of all players each game cycle.
*/
void PlayersEachCycle()
{
	for (int player = 0; player < NumPlayers; ++player) {
		CPlayer &p = *CPlayer::Players[player];
		
		//Wyrmgus start
		if (p.LostTownHallTimer && !p.Revealed && p.LostTownHallTimer < ((int) GameCycle) && CPlayer::GetThisPlayer()->HasContactWith(p)) {
			p.Revealed = true;
			for (int j = 0; j < NumPlayers; ++j) {
				if (player != j && CPlayer::Players[j]->Type != PlayerNobody) {
					CPlayer::Players[j]->Notify(_("%s's units have been revealed!"), p.Name.c_str());
				} else {
					CPlayer::Players[j]->Notify("%s", _("Your units have been revealed!"));
				}
			}
		}
		
		
		for (size_t i = 0; i < p.Modifiers.size(); ++i) { //if already has the modifier, make it have the greater duration of the new or old one
			if ((unsigned long) p.Modifiers[i].second < GameCycle) {
				p.RemoveModifier(p.Modifiers[i].first); //only remove one modifier per cycle, to prevent too many upgrade changes from happening at the same cycle (for performance reasons)
				break;
			}
		}
		
		if (p.HeroCooldownTimer) {
			p.HeroCooldownTimer--;
		}
		//Wyrmgus end

		if (p.AiEnabled) {
			AiEachCycle(p);
		}
	}
}

/**
**  Handle AI of a player each second.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachSecond(int playerIdx)
{
	CPlayer &player = *CPlayer::Players[playerIdx];

	if ((GameCycle / CYCLES_PER_SECOND) % 10 == 0) {
		for (int res = 0; res < MaxCosts; ++res) {
			player.Revenue[res] = player.Resources[res] + player.StoredResources[res] - player.LastResources[res];
			player.Revenue[res] *= 6;  // estimate per minute
			player.LastResources[res] = player.Resources[res] + player.StoredResources[res];
		}
	}
	if (player.AiEnabled) {
		AiEachSecond(player);
	}

	player.UpdateFreeWorkers();
	//Wyrmgus start
	player.PerformResourceTrade();
	player.UpdateCurrentQuests();
	//Wyrmgus end
}

/**
**  Handle AI of a player each half minute.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachHalfMinute(int playerIdx)
{
	CPlayer &player = *CPlayer::Players[playerIdx];

	if (player.AiEnabled) {
		AiEachHalfMinute(player);
	}

	player.UpdateQuestPool(); // every half minute, update the quest pool
}

/**
**  Handle AI of a player each minute.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachMinute(int playerIdx)
{
	CPlayer &player = *CPlayer::Players[playerIdx];

	if (player.AiEnabled) {
		AiEachMinute(player);
	}
}

/**
**  Change current color set to new player.
**
**  FIXME: use function pointer here.
**
**  @param player  Pointer to player.
**  @param sprite  The sprite in which the colors should be changed.
*/
//Wyrmgus start
//void GraphicPlayerPixels(CPlayer &player, const CGraphic &sprite)
void GraphicPlayerPixels(int player, const CGraphic &sprite)
//Wyrmgus end
{
	//Wyrmgus start
	if (sprite.Grayscale) {
		return;
	}
	//Wyrmgus end
	
	Assert(PlayerColorIndexCount);

	SDL_LockSurface(sprite.Surface);
	//Wyrmgus start
//	std::vector<SDL_Color> sdlColors(player.UnitColors.Colors.begin(), player.UnitColors.Colors.end());
	std::vector<SDL_Color> sdlColors(PlayerColorsRGB[player].begin(), PlayerColorsRGB[player].end());
	
	//convert colors according to time of day
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (sprite.TimeOfDay) {
		if (sprite.TimeOfDay->Dawn) {
			time_of_day_red = -20;
			time_of_day_green = -20;
			time_of_day_blue = 0;
		} else if (sprite.TimeOfDay->Day) {
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (sprite.TimeOfDay->Dusk) {
			time_of_day_red = 0;
			time_of_day_green = -20;
			time_of_day_blue = -20;
		} else if (sprite.TimeOfDay->Night) {
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		}
	}
	
	if (sprite.TimeOfDay && (time_of_day_red != 0 || time_of_day_green != 0 || time_of_day_blue != 0)) {
		for (int i = 0; i < PlayerColorIndexCount; ++i) {
			sdlColors[i].r = std::max<int>(0,std::min<int>(255,int(sdlColors[i].r) + time_of_day_red));
			sdlColors[i].g = std::max<int>(0,std::min<int>(255,int(sdlColors[i].g) + time_of_day_green));
			sdlColors[i].b = std::max<int>(0,std::min<int>(255,int(sdlColors[i].b) + time_of_day_blue));
		}
	}
	//Wyrmgus end
	SDL_SetColors(sprite.Surface, &sdlColors[0], PlayerColorIndexStart, PlayerColorIndexCount);
	if (sprite.SurfaceFlip) {
		SDL_SetColors(sprite.SurfaceFlip, &sdlColors[0], PlayerColorIndexStart, PlayerColorIndexCount);
	}
	SDL_UnlockSurface(sprite.Surface);
}

/**
**  Setup the player colors for the current palette.
**
**  @todo  FIXME: could be called before PixelsXX is setup.
*/
void SetPlayersPalette()
{
	for (int i = 0; i < PlayerMax; ++i) {
		//Wyrmgus start
//		CPlayer::Players[i]->UnitColors.Colors = PlayerColorsRGB[i];
		if (CPlayer::Players[i]->GetFaction() == nullptr) {
			CPlayer::Players[i]->UnitColors.Colors = PlayerColorsRGB[i];
		}
		//Wyrmgus end
	}
}

/**
**  Output debug information for players.
*/
void DebugPlayers()
{
#ifdef DEBUG
	DebugPrint("Nr   Color   I Name     Type         Race    Ai\n");
	DebugPrint("--  -------- - -------- ------------ ------- -----\n");
	for (int i = 0; i < PlayerMax; ++i) {
		if (CPlayer::Players[i]->Type == PlayerNobody) {
			continue;
		}
		const char *playertype;

		switch (CPlayer::Players[i]->Type) {
			case 0: playertype = "Don't know 0"; break;
			case 1: playertype = "Don't know 1"; break;
			case 2: playertype = "neutral     "; break;
			case 3: playertype = "nobody      "; break;
			case 4: playertype = "computer    "; break;
			case 5: playertype = "person      "; break;
			case 6: playertype = "rescue pas. "; break;
			case 7: playertype = "rescue akt. "; break;
			default : playertype = "?unknown?   "; break;
		}
		DebugPrint("%2d: %8.8s %c %-8.8s %s %7s %s\n" _C_ i _C_ PlayerColorNames[i].c_str() _C_
				   CPlayer::GetThisPlayer() == CPlayer::Players[i] ? '*' :
				   CPlayer::Players[i]->AiEnabled ? '+' : ' ' _C_
				   CPlayer::Players[i]->Name.c_str() _C_ playertype _C_
				   PlayerRaces.Name[CPlayer::Players[i]->Race].c_str() _C_
				   CPlayer::Players[i]->AiName.c_str());
	}
#endif
}

/**
**  Notify player about a problem.
**
**  @param type    Problem type
**  @param pos     Map tile position
**  @param fmt     Message format
**  @param ...     Message varargs
**
**  @todo FIXME: We must also notfiy allied players.
*/
void CPlayer::Notify(int type, const Vec2i &pos, int z, const char *fmt, ...) const
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));
	char temp[128];
	Uint32 color;
	va_list va;

	// Notify me, and my TEAM members
	if (this != CPlayer::GetThisPlayer() && !IsTeamed(*CPlayer::GetThisPlayer())) {
		return;
	}

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);
	switch (type) {
		case NotifyRed:
			color = ColorRed;
			break;
		case NotifyYellow:
			color = ColorYellow;
			break;
		case NotifyGreen:
			color = ColorGreen;
			break;
		default: color = ColorWhite;
	}
	UI.Minimap.AddEvent(pos, z, color);
	if (this == CPlayer::GetThisPlayer()) {
		SetMessageEvent(pos, z, "%s", temp);
	} else {
		SetMessageEvent(pos, z, "(%s): %s", Name.c_str(), temp);
	}
}

/**
**  Notify player about a problem.
**
**  @param type    Problem type
**  @param pos     Map tile position
**  @param fmt     Message format
**  @param ...     Message varargs
**
**  @todo FIXME: We must also notfiy allied players.
*/
void CPlayer::Notify(const char *fmt, ...) const
{
	// Notify me, and my TEAM members
	if (this != CPlayer::GetThisPlayer() && !IsTeamed(*CPlayer::GetThisPlayer())) {
		return;
	}
	char temp[128];
	va_list va;

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);
	if (this == CPlayer::GetThisPlayer()) {
		SetMessage("%s", temp);
	} else {
		SetMessage("(%s): %s", Name.c_str(), temp);
	}
}

void CPlayer::SetDiplomacyNeutralWith(const CPlayer &player)
{
	this->Enemy &= ~(1 << player.Index);
	this->Allied &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to Neutral"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::SetDiplomacyAlliedWith(const CPlayer &player)
{
	this->Enemy &= ~(1 << player.Index);
	this->Allied |= 1 << player.Index;
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to Ally"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

//Wyrmgus start
//void CPlayer::SetDiplomacyEnemyWith(const CPlayer &player)
void CPlayer::SetDiplomacyEnemyWith(CPlayer &player)
//Wyrmgus end
{
	this->Enemy |= 1 << player.Index;
	this->Allied &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to Enemy"), _(this->Name.c_str()));
	}
	
	// if either player is the overlord of another (indirect or otherwise), break the vassalage bond after the declaration of war
	if (this->IsOverlordOf(player, true)) {
		player.SetOverlord(nullptr);
	} else if (player.IsOverlordOf(*this, true)) {
		this->SetOverlord(nullptr);
	}
	//Wyrmgus end
}

void CPlayer::SetDiplomacyCrazyWith(const CPlayer &player)
{
	this->Enemy |= 1 << player.Index;
	this->Allied |= 1 << player.Index;
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to Crazy"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::ShareVisionWith(const CPlayer &player)
{
	this->SharedVision |= (1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s is now sharing vision with us"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::UnshareVisionWith(const CPlayer &player)
{
	this->SharedVision &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == CPlayer::GetThisPlayer()->Index) {
		CPlayer::GetThisPlayer()->Notify(_("%s is no longer sharing vision with us"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

//Wyrmgus start
void CPlayer::SetOverlord(CPlayer *player)
{
	if (this->Overlord) {
		this->Overlord->Vassals.erase(std::remove(this->Overlord->Vassals.begin(), this->Overlord->Vassals.end(), this), this->Overlord->Vassals.end());
	}

	this->Overlord = player;
	
	if (this->Overlord) {
		this->Overlord->Vassals.push_back(this);
		if (!SaveGameLoading) {
			this->SetDiplomacyAlliedWith(*this->Overlord);
			this->Overlord->SetDiplomacyAlliedWith(*this);
			CommandDiplomacy(this->Index, DiplomacyAllied, this->Overlord->Index);
			CommandDiplomacy(this->Overlord->Index, DiplomacyAllied, this->Index);
			CommandSharedVision(this->Index, true, this->Overlord->Index);
			CommandSharedVision(this->Overlord->Index, true, this->Index);
		}
	}
}
//Wyrmgus end

/**
**  Check if the player is an enemy
*/
bool CPlayer::IsEnemy(const CPlayer &player) const
{
	//Wyrmgus start
//	return IsEnemy(player.Index);
	return IsEnemy(player.Index) || player.IsEnemy(this->Index); // be hostile to the other player if they are hostile, even if the diplomatic stance hasn't been changed
	//Wyrmgus end
}

/**
**  Check if the unit is an enemy
*/
bool CPlayer::IsEnemy(const CUnit &unit) const
{
	//Wyrmgus start
	if (
		unit.Player->Type == PlayerNeutral
		&& unit.Type->BoolFlag[PREDATOR_INDEX].value
		&& this->Type != PlayerNeutral
	) {
		return true;
	}
	
	if (
		this != unit.Player
		&& this->Type != PlayerNeutral
		&& unit.CurrentAction() == UnitActionAttack
		&& unit.CurrentOrder()->HasGoal()
		&& unit.CurrentOrder()->GetGoal()->Player == this
		&& !unit.CurrentOrder()->GetGoal()->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value
	) {
		return true;
	}
	
	if (unit.Player->Index != this->Index && this->Type != PlayerNeutral && unit.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive() && !this->HasNeutralFactionType()) {
		return true;
	}
	//Wyrmgus end
	
	return IsEnemy(*unit.Player);
}

/**
**  Check if the player is an ally
*/
bool CPlayer::IsAllied(const CPlayer &player) const
{
	return (Allied & (1 << player.Index)) != 0;
}

/**
**  Check if the unit is an ally
*/
bool CPlayer::IsAllied(const CUnit &unit) const
{
	return IsAllied(*unit.Player);
}


bool CPlayer::IsVisionSharing() const
{
	return SharedVision != 0;
}

/**
**  Check if the player shares vision with the player
*/
bool CPlayer::IsSharedVision(const CPlayer &player) const
{
	return (SharedVision & (1 << player.Index)) != 0;
}

/**
**  Check if the player shares vision with the unit
*/
bool CPlayer::IsSharedVision(const CUnit &unit) const
{
	return IsSharedVision(*unit.Player);
}

/**
**  Check if the both players share vision
*/
bool CPlayer::IsBothSharedVision(const CPlayer &player) const
{
	return (SharedVision & (1 << player.Index)) != 0
		   && (player.SharedVision & (1 << Index)) != 0;
}

/**
**  Check if the player and the unit share vision
*/
bool CPlayer::IsBothSharedVision(const CUnit &unit) const
{
	return IsBothSharedVision(*unit.Player);
}

/**
**  Check if the player is teamed
*/
bool CPlayer::IsTeamed(const CPlayer &player) const
{
	return Team == player.Team;
}

/**
**  Check if the unit is teamed
*/
bool CPlayer::IsTeamed(const CUnit &unit) const
{
	return IsTeamed(*unit.Player);
}

//Wyrmgus start
/**
**  Check if the player is the overlord of another
*/
bool CPlayer::IsOverlordOf(const CPlayer &player, bool include_indirect) const
{
	if (!player.Overlord) {
		return false;
	}
	
	if (this == player.Overlord) {
		return true;
	}

	if (include_indirect) { //if include_indirect is true, search this player's other vassals to see if the player is an indirect overlord of the other
		for (size_t i = 0; i < this->Vassals.size(); ++i) {
			if (this->Vassals[i]->IsOverlordOf(player, include_indirect)) {
				return true;
			}
		}
	}
	
	return false;
}

/**
**  Check if the player is the vassal of another
*/
bool CPlayer::IsVassalOf(const CPlayer &player, bool include_indirect) const
{
	if (!this->Overlord) {
		return false;
	}
	
	if (this->Overlord == &player) {
		return true;
	}

	if (include_indirect) { //if include_indirect is true, search this player's other vassals to see if the player is an indirect overlord of the other
		if (this->Overlord->IsVassalOf(player, include_indirect)) {
			return true;
		}
	}
	
	return false;
}

/**
**  Check if the player has contact with another (used for determining which players show up in the player list and etc.)
*/
bool CPlayer::HasContactWith(const CPlayer &player) const
{
	return player.StartMapLayer == this->StartMapLayer || (player.StartMapLayer < (int) CMap::Map.MapLayers.size() && this->StartMapLayer < (int) CMap::Map.MapLayers.size() && CMap::Map.MapLayers[player.StartMapLayer]->World == CMap::Map.MapLayers[this->StartMapLayer]->World && CMap::Map.MapLayers[player.StartMapLayer]->Plane == CMap::Map.MapLayers[this->StartMapLayer]->Plane);
}

/**
**  Check if the player's faction type is a neutral one
*/
bool CPlayer::HasNeutralFactionType() const
{
	if (
		this->Race != -1
		&& this->Faction != nullptr
		&& (this->Faction->Type == FactionTypeMercenaryCompany || this->Faction->Type == FactionTypeHolyOrder || this->Faction->Type == FactionTypeTradingCompany)
	) {
		return true;
	}

	return false;
}

/**
**  Check if the player can use the buildings of another, for neutral building functions (i.e. unit training)
*/
bool CPlayer::HasBuildingAccess(const CPlayer &player, int button_action) const
{
	if (player.IsEnemy(*this)) {
		return false;
	}
	
	if (player.Type == PlayerNeutral) {
		return true;
	}
	
	if (
		player.HasNeutralFactionType()
		&& (player.Overlord == nullptr || this->IsOverlordOf(player, true) || player.Overlord->IsAllied(*this))
	) {
		if (player.GetFaction()->Type != FactionTypeHolyOrder || (button_action != ButtonTrain && button_action != ButtonBuy) || std::find(this->Deities.begin(), this->Deities.end(), player.GetFaction()->HolyOrderDeity) != this->Deities.end()) { //if the faction is a holy order, the player must have chosen its respective deity
			return true;
		}
	}

	return false;
}

bool CPlayer::HasHero(const CCharacter *hero) const
{
	if (!hero) {
		return false;
	}
	
	for (const CUnit *hero_unit : this->Heroes) {
		if (hero_unit->Character == hero) {
			return true;
		}
	}
	
	return false;
}

void CPlayer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_civilization"), &CPlayer::GetCivilization);
	ClassDB::bind_method(D_METHOD("get_faction"), &CPlayer::GetFaction);
	ClassDB::bind_method(D_METHOD("get_interface"), &CPlayer::GetInterface);
	
	ADD_SIGNAL(MethodInfo("civilization_changed", PropertyInfo(Variant::OBJECT, "old_civilization"), PropertyInfo(Variant::OBJECT, "new_civilization")));
	ADD_SIGNAL(MethodInfo("interface_changed", PropertyInfo(Variant::STRING, "old_interface"), PropertyInfo(Variant::STRING, "new_interface")));
}

void NetworkSetFaction(int player, const std::string &faction_name)
{
	const int faction = CFaction::GetIndex(faction_name);
	SendCommandSetFaction(player, faction);
}

int GetPlayerColorIndexByName(const std::string &player_color_name)
{
	for (int c = 0; c < PlayerColorMax; ++c) {
		if (PlayerColorNames[c] == player_color_name) {
			return c;
		}
	}
	return -1;
}

std::string GetFactionTypeNameById(int faction_type)
{
	if (faction_type == FactionTypeNoFactionType) {
		return "no-faction-type";
	} else if (faction_type == FactionTypeTribe) {
		return "tribe";
	} else if (faction_type == FactionTypePolity) {
		return "polity";
	} else if (faction_type == FactionTypeMercenaryCompany) {
		return "mercenary-company";
	} else if (faction_type == FactionTypeHolyOrder) {
		return "holy-order";
	} else if (faction_type == FactionTypeTradingCompany) {
		return "trading-company";
	}

	return "";
}

int GetFactionTypeIdByName(const std::string &faction_type)
{
	if (faction_type == "no-faction-type") {
		return FactionTypeNoFactionType;
	} else if (faction_type == "tribe") {
		return FactionTypeTribe;
	} else if (faction_type == "polity") {
		return FactionTypePolity;
	} else if (faction_type == "mercenary-company") {
		return FactionTypeMercenaryCompany;
	} else if (faction_type == "holy-order") {
		return FactionTypeHolyOrder;
	} else if (faction_type == "trading-company") {
		return FactionTypeTradingCompany;
	}

	return -1;
}

std::string GetGovernmentTypeNameById(int government_type)
{
	if (government_type == GovernmentTypeNoGovernmentType) {
		return "no-government-type";
	} else if (government_type == GovernmentTypeMonarchy) {
		return "monarchy";
	} else if (government_type == GovernmentTypeRepublic) {
		return "republic";
	} else if (government_type == GovernmentTypeTheocracy) {
		return "theocracy";
	}

	return "";
}

int GetGovernmentTypeIdByName(const std::string &government_type)
{
	if (government_type == "no-government-type") {
		return GovernmentTypeNoGovernmentType;
	} else if (government_type == "monarchy") {
		return GovernmentTypeMonarchy;
	} else if (government_type == "republic") {
		return GovernmentTypeRepublic;
	} else if (government_type == "theocracy") {
		return GovernmentTypeTheocracy;
	}

	return -1;
}

std::string GetForceTypeNameById(int force_type)
{
	if (force_type == LandForceType) {
		return "land-force";
	} else if (force_type == NavalForceType) {
		return "naval-force";
	} else if (force_type == AirForceType) {
		return "air-force";
	}

	return "";
}

int GetForceTypeIdByName(const std::string &force_type)
{
	if (force_type == "land-force") {
		return LandForceType;
	} else if (force_type == "naval-force") {
		return NavalForceType;
	} else if (force_type == "air-force") {
		return AirForceType;
	}

	return -1;
}

std::string GetWordTypeNameById(int word_type)
{
	if (word_type == WordTypeNoun) {
		return "noun";
	} else if (word_type == WordTypeVerb) {
		return "verb";
	} else if (word_type == WordTypeAdjective) {
		return "adjective";
	} else if (word_type == WordTypePronoun) {
		return "pronoun";
	} else if (word_type == WordTypeAdverb) {
		return "adverb";
	} else if (word_type == WordTypeConjunction) {
		return "conjunction";
	} else if (word_type == WordTypeAdposition) {
		return "adposition";
	} else if (word_type == WordTypeArticle) {
		return "article";
	} else if (word_type == WordTypeNumeral) {
		return "numeral";
	} else if (word_type == WordTypeAffix) {
		return "affix";
	}

	return "";
}

int GetWordTypeIdByName(const std::string &word_type)
{
	if (word_type == "noun") {
		return WordTypeNoun;
	} else if (word_type == "verb") {
		return WordTypeVerb;
	} else if (word_type == "adjective") {
		return WordTypeAdjective;
	} else if (word_type == "pronoun") {
		return WordTypePronoun;
	} else if (word_type == "adverb") {
		return WordTypeAdverb;
	} else if (word_type == "conjunction") {
		return WordTypeConjunction;
	} else if (word_type == "adposition") {
		return WordTypeAdposition;
	} else if (word_type == "article") {
		return WordTypeArticle;
	} else if (word_type == "numeral") {
		return WordTypeNumeral;
	} else if (word_type == "affix") {
		return WordTypeAffix;
	}

	return -1;
}

std::string GetArticleTypeNameById(int article_type)
{
	if (article_type == ArticleTypeNoArticle) {
		return "no-article";
	} else if (article_type == ArticleTypeDefinite) {
		return "definite";
	} else if (article_type == ArticleTypeIndefinite) {
		return "indefinite";
	}

	return "";
}

int GetArticleTypeIdByName(const std::string &article_type)
{
	if (article_type == "no-article") {
		return ArticleTypeNoArticle;
	} else if (article_type == "definite") {
		return ArticleTypeDefinite;
	} else if (article_type == "indefinite") {
		return ArticleTypeIndefinite;
	}

	return -1;
}

std::string GetGrammaticalCaseNameById(int grammatical_case)
{
	if (grammatical_case == GrammaticalCaseNoCase) {
		return "no-case";
	} else if (grammatical_case == GrammaticalCaseNominative) {
		return "nominative";
	} else if (grammatical_case == GrammaticalCaseAccusative) {
		return "accusative";
	} else if (grammatical_case == GrammaticalCaseDative) {
		return "dative";
	} else if (grammatical_case == GrammaticalCaseGenitive) {
		return "genitive";
	}

	return "";
}

int GetGrammaticalCaseIdByName(const std::string &grammatical_case)
{
	if (grammatical_case == "no-case") {
		return GrammaticalCaseNoCase;
	} else if (grammatical_case == "nominative") {
		return GrammaticalCaseNominative;
	} else if (grammatical_case == "accusative") {
		return GrammaticalCaseAccusative;
	} else if (grammatical_case == "dative") {
		return GrammaticalCaseDative;
	} else if (grammatical_case == "genitive") {
		return GrammaticalCaseGenitive;
	}

	return -1;
}

std::string GetGrammaticalNumberNameById(int grammatical_number)
{
	if (grammatical_number == GrammaticalNumberNoNumber) {
		return "no-number";
	} else if (grammatical_number == GrammaticalNumberSingular) {
		return "singular";
	} else if (grammatical_number == GrammaticalNumberPlural) {
		return "plural";
	}

	return "";
}

int GetGrammaticalNumberIdByName(const std::string &grammatical_number)
{
	if (grammatical_number == "no-number") {
		return GrammaticalNumberNoNumber;
	} else if (grammatical_number == "singular") {
		return GrammaticalNumberSingular;
	} else if (grammatical_number == "plural") {
		return GrammaticalNumberPlural;
	}

	return -1;
}

std::string GetGrammaticalPersonNameById(int grammatical_person)
{
	if (grammatical_person == GrammaticalPersonFirstPerson) {
		return "first-person";
	} else if (grammatical_person == GrammaticalPersonSecondPerson) {
		return "second-person";
	} else if (grammatical_person == GrammaticalPersonThirdPerson) {
		return "third-person";
	}

	return "";
}

int GetGrammaticalPersonIdByName(const std::string &grammatical_person)
{
	if (grammatical_person == "first-person") {
		return GrammaticalPersonFirstPerson;
	} else if (grammatical_person == "second-person") {
		return GrammaticalPersonSecondPerson;
	} else if (grammatical_person == "third-person") {
		return GrammaticalPersonThirdPerson;
	}

	return -1;
}

std::string GetGrammaticalGenderNameById(int grammatical_gender)
{
	if (grammatical_gender == GrammaticalGenderNoGender) {
		return "no-gender";
	} else if (grammatical_gender == GrammaticalGenderMasculine) {
		return "masculine";
	} else if (grammatical_gender == GrammaticalGenderFeminine) {
		return "feminine";
	} else if (grammatical_gender == GrammaticalGenderNeuter) {
		return "neuter";
	}

	return "";
}

int GetGrammaticalGenderIdByName(const std::string &grammatical_gender)
{
	if (grammatical_gender == "no-gender") {
		return GrammaticalGenderNoGender;
	} else if (grammatical_gender == "masculine") {
		return GrammaticalGenderMasculine;
	} else if (grammatical_gender == "feminine") {
		return GrammaticalGenderFeminine;
	} else if (grammatical_gender == "neuter") {
		return GrammaticalGenderNeuter;
	}

	return -1;
}

std::string GetGrammaticalTenseNameById(int grammatical_tense)
{
	if (grammatical_tense == GrammaticalTenseNoTense) {
		return "no-tense";
	} else if (grammatical_tense == GrammaticalTensePresent) {
		return "present";
	} else if (grammatical_tense == GrammaticalTensePast) {
		return "past";
	} else if (grammatical_tense == GrammaticalTenseFuture) {
		return "future";
	}

	return "";
}

int GetGrammaticalTenseIdByName(const std::string &grammatical_tense)
{
	if (grammatical_tense == "no-tense") {
		return GrammaticalTenseNoTense;
	} else if (grammatical_tense == "present") {
		return GrammaticalTensePresent;
	} else if (grammatical_tense == "past") {
		return GrammaticalTensePast;
	} else if (grammatical_tense == "future") {
		return GrammaticalTenseFuture;
	}

	return -1;
}

std::string GetGrammaticalMoodNameById(int grammatical_mood)
{
	if (grammatical_mood == GrammaticalMoodIndicative) {
		return "indicative";
	} else if (grammatical_mood == GrammaticalMoodSubjunctive) {
		return "subjunctive";
	}

	return "";
}

int GetGrammaticalMoodIdByName(const std::string &grammatical_mood)
{
	if (grammatical_mood == "indicative") {
		return GrammaticalMoodIndicative;
	} else if (grammatical_mood == "subjunctive") {
		return GrammaticalMoodSubjunctive;
	}

	return -1;
}

std::string GetComparisonDegreeNameById(int comparison_degree)
{
	if (comparison_degree == ComparisonDegreePositive) {
		return "positive";
	} else if (comparison_degree == ComparisonDegreeComparative) {
		return "comparative";
	} else if (comparison_degree == ComparisonDegreeSuperlative) {
		return "superlative";
	}

	return "";
}

int GetComparisonDegreeIdByName(const std::string &comparison_degree)
{
	if (comparison_degree == "positive") {
		return ComparisonDegreePositive;
	} else if (comparison_degree == "comparative") {
		return ComparisonDegreeComparative;
	} else if (comparison_degree == "superlative") {
		return ComparisonDegreeSuperlative;
	}

	return -1;
}

std::string GetAffixTypeNameById(int affix_type)
{
	if (affix_type == AffixTypePrefix) {
		return "prefix";
	} else if (affix_type == AffixTypeSuffix) {
		return "suffix";
	} else if (affix_type == AffixTypeInfix) {
		return "infix";
	}

	return "";
}

int GetAffixTypeIdByName(const std::string &affix_type)
{
	if (affix_type == "prefix") {
		return AffixTypePrefix;
	} else if (affix_type == "suffix") {
		return AffixTypeSuffix;
	} else if (affix_type == "infix") {
		return AffixTypeInfix;
	}

	return -1;
}

std::string GetWordJunctionTypeNameById(int word_junction_type)
{
	if (word_junction_type == WordJunctionTypeNoWordJunction) {
		return "no-word-junction";
	} else if (word_junction_type == WordJunctionTypeCompound) {
		return "compound";
	} else if (word_junction_type == WordJunctionTypeSeparate) {
		return "separate";
	}

	return "";
}

int GetWordJunctionTypeIdByName(const std::string &word_junction_type)
{
	if (word_junction_type == "no-word-junction") {
		return WordJunctionTypeNoWordJunction;
	} else if (word_junction_type == "compound") {
		return WordJunctionTypeCompound;
	} else if (word_junction_type == "separate") {
		return WordJunctionTypeSeparate;
	}

	return -1;
}

LanguageWord *CLanguage::GetWord(const std::string word, int word_type, std::vector<std::string>& word_meanings) const
{
	for (size_t i = 0; i < this->LanguageWords.size(); ++i) {
		if (
			this->LanguageWords[i]->Word == word
			&& (word_type == -1 || this->LanguageWords[i]->Type == word_type)
			&& (word_meanings.size() == 0 || this->LanguageWords[i]->Meanings == word_meanings)
		) {
			return this->LanguageWords[i];
		}
	}

	return nullptr;
}

std::string CLanguage::GetArticle(int gender, int grammatical_case, int article_type, int grammatical_number)
{
	for (size_t i = 0; i < this->LanguageWords.size(); ++i) {
		if (this->LanguageWords[i]->Type != WordTypeArticle || this->LanguageWords[i]->ArticleType != article_type) {
			continue;
		}
		
		if (grammatical_number != -1 && this->LanguageWords[i]->GrammaticalNumber != -1 && this->LanguageWords[i]->GrammaticalNumber != grammatical_number) {
			continue;
		}
		
		if (gender == -1 || this->LanguageWords[i]->Gender == -1 || gender == this->LanguageWords[i]->Gender) {
			if (grammatical_case == GrammaticalCaseNominative && !this->LanguageWords[i]->Nominative.empty()) {
				return this->LanguageWords[i]->Nominative;
			} else if (grammatical_case == GrammaticalCaseAccusative && !this->LanguageWords[i]->Accusative.empty()) {
				return this->LanguageWords[i]->Accusative;
			} else if (grammatical_case == GrammaticalCaseDative && !this->LanguageWords[i]->Dative.empty()) {
				return this->LanguageWords[i]->Dative;
			} else if (grammatical_case == GrammaticalCaseGenitive && !this->LanguageWords[i]->Genitive.empty()) {
				return this->LanguageWords[i]->Genitive;
			}
		}
	}
	return "";
}

std::string CLanguage::GetNounEnding(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (word_junction_type == -1) {
		word_junction_type = WordJunctionTypeNoWordJunction;
	}
	
	if (!this->NounEndings[grammatical_number][grammatical_case][word_junction_type].empty()) {
		return this->NounEndings[grammatical_number][grammatical_case][word_junction_type];
	} else if (!this->NounEndings[grammatical_number][grammatical_case][WordJunctionTypeNoWordJunction].empty()) {
		return this->NounEndings[grammatical_number][grammatical_case][WordJunctionTypeNoWordJunction];
	}
	
	return "";
}

std::string CLanguage::GetAdjectiveEnding(int article_type, int grammatical_case, int grammatical_number, int grammatical_gender)
{
	if (grammatical_number == -1) {
		grammatical_number = GrammaticalNumberNoNumber;
	}
	
	if (grammatical_gender == -1) {
		grammatical_gender = GrammaticalGenderNoGender;
	}
	
	if (!this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender];
	} else if (!this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][GrammaticalGenderNoGender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][GrammaticalGenderNoGender];
	} else if (!this->AdjectiveEndings[article_type][grammatical_case][GrammaticalNumberNoNumber][GrammaticalGenderNoGender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][GrammaticalNumberNoNumber][GrammaticalGenderNoGender];
	}
	
	return "";
}

void CLanguage::RemoveWord(LanguageWord *word)
{
	if (std::find(this->LanguageWords.begin(), this->LanguageWords.end(), word) != this->LanguageWords.end()) {
		this->LanguageWords.erase(std::remove(this->LanguageWords.begin(), this->LanguageWords.end(), word), this->LanguageWords.end());
	}
}

bool LanguageWord::HasMeaning(const std::string &meaning)
{
	return std::find(this->Meanings.begin(), this->Meanings.end(), meaning) != this->Meanings.end();
}

std::string LanguageWord::GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case)) != this->NumberCaseInflections.end()) {
		return this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case))->second;
	}
	
	return this->Word + this->Language->GetNounEnding(grammatical_number, grammatical_case, word_junction_type);
}

std::string LanguageWord::GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood)
{
	if (this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)) != this->NumberPersonTenseMoodInflections.end()) {
		return this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood))->second;
	}

	return this->Word;
}

std::string LanguageWord::GetAdjectiveInflection(int comparison_degree, int article_type, int grammatical_case, int grammatical_number, int grammatical_gender)
{
	std::string inflected_word;
	
	if (grammatical_case == -1) {
		grammatical_case = GrammaticalCaseNoCase;
	}
	
	if (!this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case];
	} else if (!this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase];
	} else {
		inflected_word = this->Word;
	}
	
	if (article_type != -1 && grammatical_case != GrammaticalCaseNoCase && this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word += this->Language->GetAdjectiveEnding(article_type, grammatical_case, grammatical_number, grammatical_gender);
	}
	
	return inflected_word;
}

std::string LanguageWord::GetParticiple(int grammatical_tense)
{
	if (!this->Participles[grammatical_tense].empty()) {
		return this->Participles[grammatical_tense];
	}
	
	return this->Word;
}

void LanguageWord::RemoveFromVector(std::vector<LanguageWord *>& word_vector)
{
	std::vector<LanguageWord *> word_vector_copy = word_vector;
	
	if (std::find(word_vector.begin(), word_vector.end(), this) != word_vector.end()) {
		word_vector.erase(std::remove(word_vector.begin(), word_vector.end(), this), word_vector.end());
	}
	
	if (word_vector.size() == 0) { // if removing the word from the vector left it empty, undo the removal
		word_vector = word_vector_copy;
	}
}

bool IsNameValidForWord(const std::string &word_name)
{
	if (word_name.empty()) {
		return false;
	}
	
	if (
		word_name.find('\n') != -1
		|| word_name.find('\\') != -1
		|| word_name.find('/') != -1
		|| word_name.find('.') != -1
		|| word_name.find('*') != -1
		|| word_name.find('[') != -1
		|| word_name.find(']') != -1
		|| word_name.find(':') != -1
		|| word_name.find(';') != -1
		|| word_name.find('=') != -1
		|| word_name.find(',') != -1
		|| word_name.find('<') != -1
		|| word_name.find('>') != -1
		|| word_name.find('?') != -1
		|| word_name.find('|') != -1
	) {
		return false;
	}
	
	if (word_name.find_first_not_of(' ') == std::string::npos) {
		return false; //name contains only spaces
	}
	
	return true;
}
//Wyrmgus end
