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
/**@name player.cpp - The players. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
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

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdarg.h>

#include "stratagus.h"

#include "player.h"

#include "action/action_upgradeto.h"
#include "actions.h"
#include "ai.h"
//Wyrmgus start
#include "commands.h" //for faction setting
#include "editor.h"
#include "font.h"
#include "game.h"
#include "iocompat.h"
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "luacallback.h"
//Wyrmgus end
#include "map.h"
#include "network.h"
#include "netconnect.h"
//Wyrmgus start
#include "parameters.h"
#include "quest.h"
#include "settings.h"
//Wyrmgus end
#include "sound.h"
#include "translate.h"
#include "unitsound.h"
#include "unittype.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h" // for faction splitters
#include "upgrade.h"
//Wyrmgus end
#include "ui.h"
#include "video.h"

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

int NumPlayers;                  /// How many player slots used
CPlayer Players[PlayerMax];       /// All players in play
CPlayer *ThisPlayer;              /// Player on this computer
PlayerRace PlayerRaces;          /// Player races

bool NoRescueCheck;               /// Disable rescue check

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

std::vector<CColor> SkinColorsRGB[SkinColorMax];
std::string SkinColorNames[SkinColorMax];
std::vector<CColor> HairColorsRGB[HairColorMax];
std::string HairColorNames[HairColorMax];
//Wyrmgus end

/**
**  Which indexes to replace with player color
*/
int PlayerColorIndexStart;
int PlayerColorIndexCount;

//Wyrmgus start
std::map<std::string, int> CivilizationStringToIndex;
std::map<std::string, int> FactionStringToIndex[MAX_RACES];

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
	if (this->Count > 0) { //don't clean the languages if first defining the civilizations
		for (size_t i = 0; i < this->Languages.size(); ++i) {
			for (size_t j = 0; j < this->Languages[i]->LanguageWords.size(); ++j) {
				for (size_t k = 0; k < this->Languages[i]->Dialects.size(); ++k) { //remove word from dialects, so that they don't try to delete it too
					this->Languages[i]->Dialects[k]->RemoveWord(this->Languages[i]->LanguageWords[j]);
				}
				
				delete this->Languages[i]->LanguageWords[j];
			}
			this->Languages[i]->LanguageWords.clear();
			this->Languages[i]->ModWords.clear();
			
			this->Languages[i]->NameTranslations.clear();
		}
	}
	for (size_t i = 0; i < this->Religions.size(); ++i) {
		delete this->Religions[i];
	}
	this->Religions.clear();
	for (size_t i = 0; i < this->DeityDomains.size(); ++i) {
		delete this->DeityDomains[i];
	}
	this->DeityDomains.clear();
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		delete this->Deities[i];
	}
	this->Deities.clear();
	//Wyrmgus end
	for (unsigned int i = 0; i != this->Count; ++i) {
		this->Name[i].clear();
		this->Display[i].clear();
		this->Visible[i] = false;
		//Wyrmgus start
		this->CivilizationUpgrades[i].clear();
		this->CivilizationClassUnitTypes[i].clear();
		this->CivilizationClassUpgrades[i].clear();
		this->Playable[i] = false;
		this->Species[i].clear();
		this->DefaultColor[i].clear();
		this->ParentCivilization[i] = -1;
		this->CivilizationLanguage[i] = -1;
		for (size_t j = 0; j < PlayerRaces.Factions[i].size(); ++j) {
			delete this->Factions[i][j];
		}
		PlayerRaces.Factions[i].clear();
		this->DevelopsFrom[i].clear();
		this->DevelopsTo[i].clear();
		this->LiteraryWorks[i].clear();
		this->CivilizationUIFillers[i].clear();
		//Wyrmgus end
	}
	this->Count = 0;
	//Wyrmgus start
	for (size_t i = 0; i < PlayerRaces.Civilizations.size(); ++i) {
		delete this->Civilizations[i];
	}
	PlayerRaces.Civilizations.clear();
	//Wyrmgus end
}

int PlayerRace::GetRaceIndexByName(const char *raceName) const
{
	//Wyrmgus start
	/*
	for (unsigned int i = 0; i != this->Count; ++i) {
		if (this->Name[i].compare(raceName) == 0) {
			return i;
		}
	}
	return -1;
	*/
	std::string civilization_name(raceName);
	
	if (civilization_name.empty()) {
		return -1;
	}
	
	if (CivilizationStringToIndex.find(civilization_name) != CivilizationStringToIndex.end()) {
		return CivilizationStringToIndex[civilization_name];
	}
	
	return -1;
	//Wyrmgus end
}

//Wyrmgus start
int PlayerRace::GetFactionIndexByName(const int civilization, const std::string faction_name) const
{
	if (faction_name.empty()) {
		return -1;
	}
	
	if (civilization == -1) { //if the civilization is -1, then search all civilizations for this faction
		for (int i = 0; i < MAX_RACES; ++i) {
			int civilization_faction_index = PlayerRaces.GetFactionIndexByName(i, faction_name);
			if (civilization_faction_index != -1) {
				return civilization_faction_index;
			}
		}
		return -1; //return -1 if found nothing
	}
	
	if (FactionStringToIndex[civilization].find(faction_name) != FactionStringToIndex[civilization].end()) {
		return FactionStringToIndex[civilization][faction_name];
	} else {
		return -1;
	}		
}

CFaction *PlayerRace::GetFaction(const int civilization, const std::string faction_name) const
{
	if (faction_name.empty()) {
		return NULL;
	}
	
	if (civilization == -1) { //if the civilization is -1, then search all civilizations for this faction
		for (int i = 0; i < MAX_RACES; ++i) {
			int civilization_faction_index = PlayerRaces.GetFactionIndexByName(i, faction_name);
			if (civilization_faction_index != -1) {
				return PlayerRaces.Factions[i][civilization_faction_index];
			}
		}
		return NULL; //return NULL if found nothing
	}
	
	if (FactionStringToIndex[civilization].find(faction_name) != FactionStringToIndex[civilization].end()) {
		return PlayerRaces.Factions[civilization][FactionStringToIndex[civilization][faction_name]];
	} else {
		return NULL;
	}		
}

int PlayerRace::GetReligionIndexByIdent(std::string religion_ident) const
{
	for (size_t i = 0; i < this->Religions.size(); ++i) {
		if (religion_ident == this->Religions[i]->Ident) {
			return i;
		}
	}
	return -1;
}

int PlayerRace::GetDeityDomainIndexByIdent(std::string deity_domain_ident) const
{
	for (size_t i = 0; i < this->DeityDomains.size(); ++i) {
		if (deity_domain_ident == this->DeityDomains[i]->Ident) {
			return i;
		}
	}
	return -1;
}

int PlayerRace::GetDeityIndexByIdent(std::string deity_ident) const
{
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (deity_ident == this->Deities[i]->Ident) {
			return i;
		}
	}
	return -1;
}

int PlayerRace::GetLanguageIndexByIdent(std::string language_ident) const
{
	for (size_t i = 0; i < this->Languages.size(); ++i) {
		if (language_ident == this->Languages[i]->Ident) {
			return i;
		}
	}
	return -1;
}

int PlayerRace::GetCivilizationClassUnitType(int civilization, int class_id)
{
	if (civilization == -1 || class_id == -1) {
		return -1;
	}
	
	if (CivilizationClassUnitTypes[civilization].find(class_id) != CivilizationClassUnitTypes[civilization].end()) {
		return CivilizationClassUnitTypes[civilization][class_id];
	}
	
	if (PlayerRaces.ParentCivilization[civilization] != -1) {
		return GetCivilizationClassUnitType(PlayerRaces.ParentCivilization[civilization], class_id);
	}
	
	return -1;
}

int PlayerRace::GetCivilizationClassUpgrade(int civilization, int class_id)
{
	if (civilization == -1 || class_id == -1) {
		return -1;
	}
	
	if (CivilizationClassUpgrades[civilization].find(class_id) != CivilizationClassUpgrades[civilization].end()) {
		return CivilizationClassUpgrades[civilization][class_id];
	}
	
	if (PlayerRaces.ParentCivilization[civilization] != -1) {
		return GetCivilizationClassUpgrade(PlayerRaces.ParentCivilization[civilization], class_id);
	}
	
	return -1;
}

int PlayerRace::GetFactionClassUnitType(int civilization, int faction, int class_id)
{
	if (civilization == -1 || class_id == -1) {
		return -1;
	}
	
	if (faction != -1) {
		if (PlayerRaces.Factions[civilization][faction]->ClassUnitTypes.find(class_id) != PlayerRaces.Factions[civilization][faction]->ClassUnitTypes.end()) {
			return PlayerRaces.Factions[civilization][faction]->ClassUnitTypes[class_id];
		}
		
		if (PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
			return GetFactionClassUnitType(civilization, PlayerRaces.Factions[civilization][faction]->ParentFaction, class_id);
		}
	}
	
	return GetCivilizationClassUnitType(civilization, class_id);
}

int PlayerRace::GetFactionClassUpgrade(int civilization, int faction, int class_id)
{
	if (civilization == -1 || class_id == -1) {
		return -1;
	}
	
	if (faction != -1) {
		if (PlayerRaces.Factions[civilization][faction]->ClassUpgrades.find(class_id) != PlayerRaces.Factions[civilization][faction]->ClassUpgrades.end()) {
			return PlayerRaces.Factions[civilization][faction]->ClassUpgrades[class_id];
		}
		
		if (PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
			return GetFactionClassUpgrade(civilization, PlayerRaces.Factions[civilization][faction]->ParentFaction, class_id);
		}
	}
	
	return GetCivilizationClassUpgrade(civilization, class_id);
}

int PlayerRace::GetCivilizationLanguage(int civilization)
{
	if (civilization == -1) {
		return -1;
	}
	
	if (CivilizationLanguage[civilization] != -1) {
		return CivilizationLanguage[civilization];
	}
	
	if (PlayerRaces.ParentCivilization[civilization] != -1) {
		return GetCivilizationLanguage(PlayerRaces.ParentCivilization[civilization]);
	}
	
	return -1;
}

int PlayerRace::GetFactionLanguage(int civilization, int faction)
{
	if (civilization == -1) {
		return -1;
	}
	
	if (faction != -1) {
		if (Factions[civilization][faction]->Language != -1) {
			return Factions[civilization][faction]->Language;
		}
		
		if (PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
			return GetFactionLanguage(civilization, PlayerRaces.Factions[civilization][faction]->ParentFaction);
		}
	}
	
	return GetCivilizationLanguage(civilization);
}

std::vector<CFiller> PlayerRace::GetCivilizationUIFillers(int civilization)
{
	if (civilization == -1) {
		return std::vector<CFiller>();
	}
	
	if (CivilizationUIFillers[civilization].size() > 0) {
		return CivilizationUIFillers[civilization];
	}
	
	if (PlayerRaces.ParentCivilization[civilization] != -1) {
		return GetCivilizationUIFillers(PlayerRaces.ParentCivilization[civilization]);
	}
	
	return std::vector<CFiller>();
}

std::vector<CFiller> PlayerRace::GetFactionUIFillers(int civilization, int faction)
{
	if (civilization == -1) {
		return std::vector<CFiller>();
	}
	
	if (faction != -1 && Factions[civilization].size() > 0) {
		if (Factions[civilization][faction]->UIFillers.size() > 0) {
			return Factions[civilization][faction]->UIFillers;
		}
		
		if (PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
			return GetFactionUIFillers(civilization, PlayerRaces.Factions[civilization][faction]->ParentFaction);
		}
	}
	
	return GetCivilizationUIFillers(civilization);
}

/**
**  "Translate" (that is, adapt) a proper name from one culture (civilization) to another.
*/
std::string PlayerRace::TranslateName(std::string name, int language)
{
	std::string new_name;
	
	if (language == -1 || name.empty()) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	if (PlayerRaces.Languages[language]->NameTranslations.find(name) != PlayerRaces.Languages[language]->NameTranslations.end()) {
		return PlayerRaces.Languages[language]->NameTranslations[name][SyncRand(PlayerRaces.Languages[language]->NameTranslations[name].size())];
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (name.size() > 1) {
		if (name.find(" ") == std::string::npos) {
			for (size_t i = 0; i < name.size(); ++i) {
				std::string name_prefix = name.substr(0, i + 1);
				std::string name_suffix = CapitalizeString(name.substr(i + 1, name.size() - (i + 1)));
			
	//			fprintf(stdout, "Trying to match prefix \"%s\" and suffix \"%s\" for translating name \"%s\" to the \"%s\" language.\n", name_prefix.c_str(), name_suffix.c_str(), name.c_str(), PlayerRaces.Languages[language]->Name.c_str());
			
				if (PlayerRaces.Languages[language]->NameTranslations.find(name_prefix) != PlayerRaces.Languages[language]->NameTranslations.end() && PlayerRaces.Languages[language]->NameTranslations.find(name_suffix) != PlayerRaces.Languages[language]->NameTranslations.end()) { // if both a prefix and suffix have been matched
					name_prefix = PlayerRaces.Languages[language]->NameTranslations[name_prefix][SyncRand(PlayerRaces.Languages[language]->NameTranslations[name_prefix].size())];
					name_suffix = PlayerRaces.Languages[language]->NameTranslations[name_suffix][SyncRand(PlayerRaces.Languages[language]->NameTranslations[name_suffix].size())];
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

std::string CCivilization::GetMonthName(int month)
{
	if (this->Months.find(month) != this->Months.end()) {
		return this->Months.find(month)->second;
	}
	
	return CapitalizeString(GetMonthNameById(month));
}

CFaction::~CFaction()
{
	this->UIFillers.clear();
}
//Wyrmgus end

/**
**  Init players.
*/
void InitPlayers()
{
	for (int p = 0; p < PlayerMax; ++p) {
		Players[p].Index = p;
		if (!Players[p].Type) {
			Players[p].Type = PlayerNobody;
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
	ThisPlayer = NULL;
	for (unsigned int i = 0; i < PlayerMax; ++i) {
		Players[i].Clear();
	}
	NumPlayers = 0;
	NoRescueCheck = false;
}

void FreePlayerColors()
{
	for (int i = 0; i < PlayerMax; ++i) {
		Players[i].UnitColors.Colors.clear();
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
		Players[i].Save(file);
	}

	file.printf("SetThisPlayer(%d)\n\n", ThisPlayer->Index);
}


void CPlayer::Save(CFile &file) const
{
	const CPlayer &p = *this;
	file.printf("Player(%d,\n", this->Index);
	//Wyrmgus start
	file.printf(" \"race\", \"%s\",", PlayerRaces.Name[p.Race].c_str());
	if (p.Faction != -1) {
		file.printf(" \"faction\", %d,", p.Faction);
	}
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == this->Color) {
			file.printf(" \"color\", %d,", i);
			break;
		}
	}
	//Wyrmgus end
	file.printf("  \"name\", \"%s\",\n", p.Name.c_str());
	file.printf("  \"type\", ");
	switch (p.Type) {
		case PlayerNeutral:       file.printf("\"neutral\",");         break;
		case PlayerNobody:        file.printf("\"nobody\",");          break;
		case PlayerComputer:      file.printf("\"computer\",");        break;
		case PlayerPerson:        file.printf("\"person\",");          break;
		case PlayerRescuePassive: file.printf("\"rescue-passive\","); break;
		case PlayerRescueActive:  file.printf("\"rescue-active\","); break;
		default:                  file.printf("%d,", p.Type); break;
	}
	//Wyrmgus start
//	file.printf(" \"race\", \"%s\",", PlayerRaces.Name[p.Race].c_str());
	//Wyrmgus end
	file.printf(" \"ai-name\", \"%s\",\n", p.AiName.c_str());
	file.printf("  \"team\", %d,", p.Team);

	file.printf(" \"enemy\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (p.Enemy & (1 << j)) ? 'X' : '_');
	}
	file.printf("\", \"allied\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (p.Allied & (1 << j)) ? 'X' : '_');
	}
	file.printf("\", \"shared-vision\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", (p.SharedVision & (1 << j)) ? 'X' : '_');
	}
	file.printf("\",\n  \"start\", {%d, %d},\n", p.StartPos.x, p.StartPos.y);
	//Wyrmgus start
	file.printf("  \"start-map-layer\", %d,\n", p.StartMapLayer);
	//Wyrmgus end

	// Resources
	file.printf("  \"resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.Resources[j]);
	}
	// Stored Resources
	file.printf("},\n  \"stored-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.StoredResources[j]);
	}
	// Max Resources
	file.printf("},\n  \"max-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.MaxResources[j]);
	}
	// Last Resources
	file.printf("},\n  \"last-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.LastResources[j]);
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
		file.printf("\"%s\", %d,", DefaultResourceNames[j].c_str(), p.Incomes[j]);
	}
	// Revenue
	file.printf("},\n  \"revenue\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			if (j == MaxCosts / 2) {
				file.printf("\n ");
			} else {
				file.printf(" ");
			}
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[j].c_str(), p.Revenue[j]);
	}

	// UnitTypesCount done by load units.

	file.printf("},\n  \"%s\",\n", p.AiEnabled ? "ai-enabled" : "ai-disabled");

	// Ai done by load ais.
	// Units done by load units.
	// TotalNumUnits done by load units.
	// NumBuildings done by load units.
	
	//Wyrmgus start
	if (p.Revealed) {
		file.printf(" \"revealed\",");
	}
	//Wyrmgus end
	
	file.printf(" \"supply\", %d,", p.Supply);
	file.printf(" \"unit-limit\", %d,", p.UnitLimit);
	file.printf(" \"building-limit\", %d,", p.BuildingLimit);
	file.printf(" \"total-unit-limit\", %d,", p.TotalUnitLimit);

	file.printf("\n  \"score\", %d,", p.Score);
	file.printf("\n  \"total-units\", %d,", p.TotalUnits);
	file.printf("\n  \"total-buildings\", %d,", p.TotalBuildings);
	file.printf("\n  \"total-resources\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%d,", p.TotalResources[j]);
	}
	file.printf("},");
	file.printf("\n  \"total-razings\", %d,", p.TotalRazings);
	file.printf("\n  \"total-kills\", %d,", p.TotalKills);
	//Wyrmgus start
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		if (p.UnitTypeKills[i] != 0) {
			file.printf("\n  \"unit-type-kills\", \"%s\", %d,", UnitTypes[i]->Ident.c_str(), p.UnitTypeKills[i]);
		}
	}
	//Wyrmgus end
	if (p.LostTownHallTimer != 0) {
		file.printf("\n  \"lost-town-hall-timer\", %d,", p.LostTownHallTimer);
	}
	//Wyrmgus end

	file.printf("\n  \"speed-resource-harvest\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%d,", p.SpeedResourcesHarvest[j]);
	}
	file.printf("},");
	file.printf("\n  \"speed-resource-return\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("%d,", p.SpeedResourcesReturn[j]);
	}
	file.printf("},");
	file.printf("\n  \"speed-build\", %d,", p.SpeedBuild);
	file.printf("\n  \"speed-train\", %d,", p.SpeedTrain);
	file.printf("\n  \"speed-upgrade\", %d,", p.SpeedUpgrade);
	file.printf("\n  \"speed-research\", %d,", p.SpeedResearch);
	
	//Wyrmgus start
	/*
	Uint8 r, g, b;

	SDL_GetRGB(p.Color, TheScreen->format, &r, &g, &b);
	file.printf("\n  \"color\", { %d, %d, %d },", r, g, b);
	*/
	//Wyrmgus end

	//Wyrmgus start
	file.printf("\n  \"current-quests\", {");
	for (size_t j = 0; j < p.CurrentQuests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", p.CurrentQuests[j]->Ident.c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"completed-quests\", {");
	for (size_t j = 0; j < p.CompletedQuests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", p.CompletedQuests[j]->Ident.c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"quest-build-units\", {");
	for (size_t j = 0; j < p.QuestBuildUnits.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestBuildUnits[j])->Ident.c_str());
		file.printf("\"%s\",", std::get<1>(p.QuestBuildUnits[j])->Ident.c_str());
		file.printf("%d,", std::get<2>(p.QuestBuildUnits[j]));
	}
	file.printf("},");
	
	file.printf("\n  \"quest-research-upgrades\", {");
	for (size_t j = 0; j < p.QuestResearchUpgrades.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestResearchUpgrades[j])->Ident.c_str());
		file.printf("\"%s\",", std::get<1>(p.QuestResearchUpgrades[j])->Ident.c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"quest-destroy-units\", {");
	for (size_t j = 0; j < p.QuestDestroyUnits.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestDestroyUnits[j])->Ident.c_str());
		file.printf("\"%s\",", std::get<1>(p.QuestDestroyUnits[j])->Ident.c_str());
		if (std::get<2>(p.QuestDestroyUnits[j])) {
			file.printf("\"%s\",", std::get<2>(p.QuestDestroyUnits[j])->Ident.c_str());
		} else {
			file.printf("\"%s\",", "");
		}
		file.printf("%d,", std::get<3>(p.QuestDestroyUnits[j]));
	}
	file.printf("},");
	
	file.printf("\n  \"quest-destroy-uniques\", {");
	for (size_t j = 0; j < p.QuestDestroyUniques.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestDestroyUniques[j])->Ident.c_str());
		file.printf("\"%s\",", std::get<1>(p.QuestDestroyUniques[j])->Ident.c_str());
		file.printf("%s,", std::get<2>(p.QuestDestroyUniques[j]) ? "true" : "false");
	}
	file.printf("},");
	
	file.printf("\n  \"quest-gather-resources\", {");
	for (size_t j = 0; j < p.QuestGatherResources.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestGatherResources[j])->Ident.c_str());
		file.printf("\"%s\",", DefaultResourceNames[std::get<1>(p.QuestGatherResources[j])].c_str());
		file.printf("%d,", std::get<2>(p.QuestGatherResources[j]));
	}
	file.printf("},");
	//Wyrmgus end

	// UnitColors done by init code.
	// Allow saved by allow.

	file.printf("\n  \"timers\", {");
	for (int j = 0; j < UpgradeMax; ++j) {
		if (j) {
			file.printf(" ,");
		}
		file.printf("%d", p.UpgradeTimers.Upgrades[j]);
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
	CPlayer &player = Players[NumPlayers];
	player.Index = NumPlayers;

	player.Init(type);
}

//Wyrmgus start
CPlayer *GetFactionPlayer(CFaction *faction)
{
	if (!faction) {
		return NULL;
	}
	
	for (int i = 0; i < NumPlayers; ++i) {
		if (Players[i].Race == faction->Civilization && Players[i].Faction == faction->ID) {
			return &Players[i];
		}
	}
	
	return NULL;
}

CPlayer *GetOrAddFactionPlayer(CFaction *faction)
{
	CPlayer *faction_player = GetFactionPlayer(faction);
	if (faction_player) {
		return faction_player;
	}
	
	// no player belonging to this faction, so let's make an unused player slot be created for it
	
	for (int i = 0; i < NumPlayers; ++i) {
		if (Players[i].Type == PlayerNobody) {
			Players[i].Type = PlayerComputer;
			Players[i].SetCivilization(faction->Civilization);
			Players[i].SetFaction(faction->Ident);
			Players[i].AiEnabled = true;
			Players[i].AiName = faction->DefaultAI;
			Players[i].Team = 1;
			Players[i].Resources[GoldCost] = 1000; // give the new player enough resources to start up
			Players[i].Resources[WoodCost] = 1000;
			Players[i].Resources[StoneCost] = 1000;
			return &Players[i];
		}
	}
	
	return NULL;
}
//Wyrmgus end

void CPlayer::Init(/* PlayerTypes */ int type)
{
	std::vector<CUnit *>().swap(this->Units);
	std::vector<CUnit *>().swap(this->FreeWorkers);
	//Wyrmgus start
	std::vector<CUnit *>().swap(this->LevelUpUnits);
	//Wyrmgus end

	//  Take first slot for person on this computer,
	//  fill other with computer players.
	if (type == PlayerPerson && !NetPlayers) {
		if (!ThisPlayer) {
			ThisPlayer = this;
		} else {
			type = PlayerComputer;
		}
	}
	if (NetPlayers && NumPlayers == NetLocalPlayerNumber) {
		ThisPlayer = &Players[NetLocalPlayerNumber];
	}

	if (NumPlayers == PlayerMax) {
		static int already_warned;

		if (!already_warned) {
			DebugPrint("Too many players\n");
			already_warned = 1;
		}
		return;
	}

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
	//Wyrmgus start
	this->Faction = -1;
	//Wyrmgus end
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
				if (Players[i].Type == PlayerComputer) {
					this->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson || Players[i].Type == PlayerRescueActive) {
				*/
				// make computer players be hostile to each other by default
				if (Players[i].Type == PlayerComputer || Players[i].Type == PlayerPerson || Players[i].Type == PlayerRescueActive) {
				//Wyrmgus end
					this->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				}
				break;
			case PlayerPerson:
				// Humans are enemy of all?
				if (Players[i].Type == PlayerComputer || Players[i].Type == PlayerPerson) {
					this->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerRescueActive || Players[i].Type == PlayerRescuePassive) {
					this->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescuePassive:
				// Rescue passive are allied with persons
				if (Players[i].Type == PlayerPerson) {
					this->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
			case PlayerRescueActive:
				// Rescue active are allied with persons and enemies of computer
				if (Players[i].Type == PlayerComputer) {
					this->Enemy |= (1 << i);
					Players[i].Enemy |= (1 << NumPlayers);
				} else if (Players[i].Type == PlayerPerson) {
					this->Allied |= (1 << i);
					Players[i].Allied |= (1 << NumPlayers);
				}
				break;
		}
	}

	//  Initial default incomes.
	for (int i = 0; i < MaxCosts; ++i) {
		this->Incomes[i] = DefaultIncomes[i];
	}

	//  Initial max resource amounts.
	for (int i = 0; i < MaxCosts; ++i) {
		this->MaxResources[i] = DefaultResourceMaxAmounts[i];
	}

	memset(this->UnitTypesCount, 0, sizeof(this->UnitTypesCount));
	memset(this->UnitTypesAiActiveCount, 0, sizeof(this->UnitTypesAiActiveCount));
	//Wyrmgus start
	memset(this->UnitTypesNonHeroCount, 0, sizeof(this->UnitTypesNonHeroCount));
	memset(this->UnitTypesStartingNonHeroCount, 0, sizeof(this->UnitTypesStartingNonHeroCount));
	this->Heroes.clear();
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
	//Wyrmgus end

	this->Color = PlayerColors[NumPlayers][0];

	if (Players[NumPlayers].Type == PlayerComputer || Players[NumPlayers].Type == PlayerRescueActive) {
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
	if (this->Race != -1 && !GameRunning && GameEstablishing) {
		if (!PlayerRaces.CivilizationUpgrades[this->Race].empty() && this->Allow.Upgrades[CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID);
		}
	}

	int old_civilization = this->Race;
	int old_faction = this->Faction;

	if (GameRunning) {
		this->SetFaction("");
	} else {
		this->Faction = -1;
	}

	this->Race = civilization;

	//if the civilization of the person player changed, update the UI
	if ((ThisPlayer && ThisPlayer->Index == this->Index) || (!ThisPlayer && this->Index == 0)) {
		//load proper UI
		char buf[256];
		snprintf(buf, sizeof(buf), "if (LoadCivilizationUI ~= nil) then LoadCivilizationUI(\"%s\") end;", PlayerRaces.Name[this->Race].c_str());
		CclCommand(buf);
		
		UI.Load();
		SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
	}
	
	if (this->Race != -1 && !GameRunning) {
		if (!PlayerRaces.CivilizationUpgrades[this->Race].empty()) {
			UpgradeAcquire(*this, CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race]));
		}
	}
		
	// set new faction from new civilization
	if (GameRunning && !GrandStrategy && Editor.Running == EditorNotRunning) {
		if (ThisPlayer && ThisPlayer->Index == this->Index) {
			if (GameCycle != 0) {
				char buf[256];
				snprintf(buf, sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", old_civilization != -1 ? PlayerRaces.Name[old_civilization].c_str() : "", (old_civilization != -1 && old_faction != -1) ? PlayerRaces.Factions[old_civilization][old_faction]->Ident.c_str() : "");
				CclCommand(buf);
			}
		} else if (this->AiEnabled) {
			this->SetRandomFaction();
		}
	}
		
	if (GrandStrategy && ThisPlayer) {
		this->SetRandomFaction();
	}
}

/**
**  Change player faction.
**
**  @param faction    New faction.
*/
void CPlayer::SetFaction(const std::string faction_name)
{
	if (this->Faction != -1) {
		if (!PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade.empty() && this->Allow.Upgrades[CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade)->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade)->ID);
		}
	}

	int faction = PlayerRaces.GetFactionIndexByName(this->Race, faction_name);
	
	for (size_t i = 0; i < UpgradeClasses.size(); ++i) {
		if (PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i) != PlayerRaces.GetFactionClassUpgrade(this->Race, faction, i)) { //if the upgrade for a certain class is different for the new faction than the old faction (and it has been acquired), remove the modifiers of the old upgrade and apply the modifiers of the new
			if (PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i) != -1 && this->Allow.Upgrades[PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i)] == 'R') {
				UpgradeLost(*this, PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i));

				if (PlayerRaces.GetFactionClassUpgrade(this->Race, faction, i) != -1) {
					UpgradeAcquire(*this, AllUpgrades[PlayerRaces.GetFactionClassUpgrade(this->Race, faction, i)]);
				}
			}
		}
	}
	
	bool personal_names_changed = true;
	bool ship_names_changed = true;
	if (this->Faction != -1 && faction != -1) {
		personal_names_changed = PlayerRaces.Factions[this->Race][this->Faction]->PersonalNames != PlayerRaces.Factions[this->Race][faction]->PersonalNames;
		ship_names_changed = PlayerRaces.Factions[this->Race][this->Faction]->ShipNames != PlayerRaces.Factions[this->Race][faction]->ShipNames;
	}
	
	this->Faction = faction;

	if (this->Index == ThisPlayer->Index) {
		UI.Load();
	}
	
	if (this->Faction == -1) {
		return;
	}
	
	if (!IsNetworkGame()) { //only set the faction's name as the player's name if this is a single player game
		this->SetName(PlayerRaces.Factions[this->Race][this->Faction]->Name);
	}
	if (this->Faction != -1) {
		int color = -1;
		for (size_t i = 0; i < PlayerRaces.Factions[this->Race][faction]->Colors.size(); ++i) {
			if (!IsPlayerColorUsed(PlayerRaces.Factions[this->Race][faction]->Colors[i])) {
				color = PlayerRaces.Factions[this->Race][faction]->Colors[i];
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
			this->Color = PlayerColors[color][0];
			this->UnitColors.Colors = PlayerColorsRGB[color];
		}
	
		if (!PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade.empty()) {
			UpgradeAcquire(*this, CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade));
		}
	} else {
		fprintf(stderr, "Invalid faction \"%s\" tried to be set for player %d of civilization \"%s\".\n", faction_name.c_str(), this->Index, PlayerRaces.Name[this->Race].c_str());
	}
	
	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);
		if (!unit.Character && unit.Type->PersonalNames.size() == 0) {
			if ((unit.Type->BoolFlag[ORGANIC_INDEX].value && personal_names_changed) || (!unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->UnitType == UnitTypeNaval) && ship_names_changed) {
				unit.UpdatePersonalName();
			}
		}
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
	int faction_count = 0;
	int local_factions[FactionMax];
	
	// first search for valid factions in the current faction's "develops to" list, and only if that fails search in all factions of the civilization
	if (this->Faction != -1) {
		for (size_t i = 0; i < PlayerRaces.Factions[this->Race][this->Faction]->DevelopsTo.size(); ++i) {
			int faction_id = PlayerRaces.GetFactionIndexByName(this->Race, PlayerRaces.Factions[this->Race][this->Faction]->DevelopsTo[i]);
			if (faction_id != -1) {
				bool faction_used = false;
				for (int j = 0; j < PlayerMax; ++j) {
					if (this->Index != j && Players[j].Type != PlayerNobody && Players[j].Name == PlayerRaces.Factions[this->Race][faction_id]->Ident) {
						faction_used = true;
					}		
				}
				if (
					!faction_used
					&& ((PlayerRaces.Factions[this->Race][faction_id]->Type == FactionTypeTribe && !this->HasUpgradeClass("writing")) || ((PlayerRaces.Factions[this->Race][faction_id]->Type == FactionTypePolity && this->HasUpgradeClass("writing"))))
					&& PlayerRaces.Factions[this->Race][faction_id]->Playable
				) {
					local_factions[faction_count] = faction_id;
					faction_count += 1;
				}
			}
		}
	}
	
	if (faction_count == 0) {
		for (size_t i = 0; i < PlayerRaces.Factions[this->Race].size(); ++i) {
			bool faction_used = false;
			for (int j = 0; j < PlayerMax; ++j) {
				if (this->Index != j && Players[j].Type != PlayerNobody && Players[j].Name == PlayerRaces.Factions[this->Race][i]->Ident) {
					faction_used = true;
				}		
			}
			if (
				!faction_used
				&& ((PlayerRaces.Factions[this->Race][i]->Type == FactionTypeTribe && !this->HasUpgradeClass("writing")) || ((PlayerRaces.Factions[this->Race][i]->Type == FactionTypePolity && this->HasUpgradeClass("writing"))))
				&& PlayerRaces.Factions[this->Race][i]->Playable
			) {
				local_factions[faction_count] = i;
				faction_count += 1;
			}
		}
	}
	
	if (faction_count > 0) {
		int chosen_faction = local_factions[SyncRand(faction_count)];
		this->SetFaction(PlayerRaces.Factions[this->Race][chosen_faction]->Ident);
	} else {
		this->SetFaction("");
	}
}

bool CPlayer::IsPlayerColorUsed(int color)
{
	bool color_used = false;
	for (int i = 0; i < PlayerMax; ++i) {
		if (this->Index != i && Players[i].Faction != -1 && Players[i].Type != PlayerNobody && Players[i].Color == PlayerColors[color][0]) {
			color_used = true;
		}		
	}
	return color_used;
}

bool CPlayer::HasUpgradeClass(std::string upgrade_class_name)
{
	if (this->Race == -1 || this->Faction == -1 || upgrade_class_name.empty()) {
		return false;
	}
	
	int upgrade_id = PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, GetUpgradeClassIndexByName(upgrade_class_name));
	
	if (upgrade_id != -1 && this->Allow.Upgrades[upgrade_id] == 'R') {
		return true;
	}

	return false;
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
	Index = 0;
	Name.clear();
	Type = 0;
	Race = 0;
	//Wyrmgus start
	Faction = -1;
	//Wyrmgus end
	AiName.clear();
	Team = 0;
	Enemy = 0;
	Allied = 0;
	SharedVision = 0;
	StartPos.x = 0;
	StartPos.y = 0;
	//Wyrmgus start
	StartMapLayer = 0;
	//Wyrmgus end
	memset(Resources, 0, sizeof(Resources));
	memset(StoredResources, 0, sizeof(StoredResources));
	memset(MaxResources, 0, sizeof(MaxResources));
	memset(LastResources, 0, sizeof(LastResources));
	memset(Incomes, 0, sizeof(Incomes));
	memset(Revenue, 0, sizeof(Revenue));
	memset(UnitTypesCount, 0, sizeof(UnitTypesCount));
	memset(UnitTypesAiActiveCount, 0, sizeof(UnitTypesAiActiveCount));
	//Wyrmgus start
	memset(UnitTypesNonHeroCount, 0, sizeof(UnitTypesNonHeroCount));
	memset(UnitTypesStartingNonHeroCount, 0, sizeof(UnitTypesStartingNonHeroCount));
	this->Heroes.clear();
	this->AvailableHeroes.clear();
	this->AvailableQuests.clear();
	this->CurrentQuests.clear();
	this->CompletedQuests.clear();
	this->QuestBuildUnits.clear();
	this->QuestResearchUpgrades.clear();
	this->QuestDestroyUnits.clear();
	this->QuestDestroyUniques.clear();
	this->QuestGatherResources.clear();
	//Wyrmgus end
	AiEnabled = false;
	//Wyrmgus start
	Revealed = false;
	//Wyrmgus end
	Ai = 0;
	this->Units.resize(0);
	this->FreeWorkers.resize(0);
	//Wyrmgus start
	this->LevelUpUnits.resize(0);
	//Wyrmgus end
	NumBuildings = 0;
	//Wyrmgus start
	NumBuildingsUnderConstruction = 0;
	NumTownHalls = 0;
	//Wyrmgus end
	Supply = 0;
	Demand = 0;
	// FIXME: can't clear limits since it's initialized already
	//	UnitLimit = 0;
	//	BuildingLimit = 0;
	//	TotalUnitLimit = 0;
	Score = 0;
	TotalUnits = 0;
	TotalBuildings = 0;
	memset(TotalResources, 0, sizeof(TotalResources));
	TotalRazings = 0;
	TotalKills = 0;
	//Wyrmgus start
	memset(UnitTypeKills, 0, sizeof(UnitTypeKills));
	LostTownHallTimer = 0;
	//Wyrmgus end
	Color = 0;
	//Wyrmgus start
	CustomHeroUnit = NULL;
	//Wyrmgus end
	UpgradeTimers.Clear();
	for (int i = 0; i < MaxCosts; ++i) {
		SpeedResourcesHarvest[i] = SPEEDUP_FACTOR;
		SpeedResourcesReturn[i] = SPEEDUP_FACTOR;
	}
	SpeedBuild = SPEEDUP_FACTOR;
	SpeedTrain = SPEEDUP_FACTOR;
	SpeedUpgrade = SPEEDUP_FACTOR;
	SpeedResearch = SPEEDUP_FACTOR;
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
	Assert(this->Units[unit.PlayerSlot] == &unit);

	//	unit.Player = NULL; // we can remove dying unit...
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
		if (unit.IsAlive() && unit.Type->BoolFlag[HARVESTER_INDEX].value && !unit.Removed) {
			if (unit.CurrentAction() == UnitActionStill) {
				FreeWorkers.push_back(&unit);
			}
		}
	}
}

//Wyrmgus start
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
		if (unit.IsAlive() && unit.Variable[LEVELUP_INDEX].Value >= 1 && !unit.Removed) {
			LevelUpUnits.push_back(&unit);
		}
	}
}

void CPlayer::UpdateHeroPool()
{
	if (CurrentCampaign == NULL) { // hero recruitment only while playing the campaign mode
		return;
	}
	
	this->AvailableHeroes.clear();
	
	std::vector<CCharacter *> potential_heroes;
	
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		if (iterator->second->Persistent) {
			potential_heroes.push_back(iterator->second);
		}
	}
	
	for (int i = 0; i < 1; ++i) { // fill the hero pool with up to one hero
		if (potential_heroes.size() == 0) {
			break;
		}
		this->AvailableHeroes.push_back(potential_heroes[SyncRand(potential_heroes.size())]);
		int available_hero_quantity = this->AvailableHeroes.size();
		potential_heroes.erase(std::remove(potential_heroes.begin(), potential_heroes.end(), this->AvailableHeroes[available_hero_quantity - 1]), potential_heroes.end());
	}
}

void CPlayer::UpdateQuestPool()
{
	if (CurrentCampaign == NULL) { // in-game quests only while playing the campaign mode
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
	if (this == ThisPlayer && GameCycle >= CYCLES_PER_MINUTE && this->AvailableQuests.size() > 0 && exausted_available_quests) {
		ThisPlayer->Notify("%s", _("New quests available"));
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
	if (this == ThisPlayer) {
		for (int i = 0; i < (int) UnitButtonTable.size(); ++i) {
			if (UnitButtonTable[i]->Action != ButtonQuest || UnitButtonTable[i]->Value >= (int) this->AvailableQuests.size()) {
				continue;
			}
			
			UnitButtonTable[i]->Hint = "Quest: " + this->AvailableQuests[UnitButtonTable[i]->Value]->Name;
			UnitButtonTable[i]->Description = this->AvailableQuests[UnitButtonTable[i]->Value]->Description + "\n \nObjectives:";
			for (size_t j = 0; j < this->AvailableQuests[UnitButtonTable[i]->Value]->Objectives.size(); ++j) {
				UnitButtonTable[i]->Description += "\n" + this->AvailableQuests[UnitButtonTable[i]->Value]->Objectives[j];
			}
			if (!this->AvailableQuests[UnitButtonTable[i]->Value]->Rewards.empty()) {
				UnitButtonTable[i]->Description += "\n \nRewards: " + this->AvailableQuests[UnitButtonTable[i]->Value]->Rewards;
			}
			if (!this->AvailableQuests[UnitButtonTable[i]->Value]->Hint.empty()) {
				UnitButtonTable[i]->Description += "\n \nHint: " + this->AvailableQuests[UnitButtonTable[i]->Value]->Hint;
			}
		}
	}
}

void CPlayer::UpdateCurrentQuests()
{
	for (int i = ((int) this->CurrentQuests.size() - 1); i >= 0; --i) {
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
	
	for (size_t i = 0; i < quest->BuildUnits.size(); ++i) {
		this->QuestBuildUnits.push_back(std::tuple<CQuest *, CUnitType *, int>(quest, std::get<0>(quest->BuildUnits[i]), std::get<1>(quest->BuildUnits[i])));
	}
	
	for (size_t i = 0; i < quest->ResearchUpgrades.size(); ++i) {
		this->QuestResearchUpgrades.push_back(std::tuple<CQuest *, CUpgrade *>(quest, quest->ResearchUpgrades[i]));
	}
	
	for (size_t i = 0; i < quest->DestroyUnits.size(); ++i) {
		this->QuestDestroyUnits.push_back(std::tuple<CQuest *, CUnitType *, CFaction *, int>(quest, std::get<0>(quest->DestroyUnits[i]), std::get<1>(quest->DestroyUnits[i]), std::get<2>(quest->DestroyUnits[i])));
	}
	
	for (size_t i = 0; i < quest->DestroyUniques.size(); ++i) {
		this->QuestDestroyUniques.push_back(std::tuple<CQuest *, CUniqueItem *, bool>(quest, quest->DestroyUniques[i], false));
	}
	
	for (size_t i = 0; i < quest->GatherResources.size(); ++i) {
		this->QuestGatherResources.push_back(std::tuple<CQuest *, int, int>(quest, std::get<0>(quest->GatherResources[i]), std::get<1>(quest->GatherResources[i])));
	}
	
	if (this == ThisPlayer) {
		for (size_t i = 0; i < quest->Objectives.size(); ++i) {
//			SetObjective(quest->Objectives[i].c_str());
			CclCommand("AddPlayerObjective(" + std::to_string((long long) this->Index) + ", \"" + quest->Objectives[i] + "\");");
		}
	}
	
	this->AvailableQuestsChanged();
}

void CPlayer::CompleteQuest(CQuest *quest)
{
	this->CurrentQuests.erase(std::remove(this->CurrentQuests.begin(), this->CurrentQuests.end(), quest), this->CurrentQuests.end());
	this->CompletedQuests.push_back(quest);
	if (quest->Competitive) {
		quest->CurrentCompleted = true;
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
	
	if (quest->CompletionEffects) {
		quest->CompletionEffects->pushPreamble();
		quest->CompletionEffects->run();
	}
	
	if (this == ThisPlayer) {
		for (size_t i = 0; i < quest->Objectives.size(); ++i) {
//			SetObjective(quest->Objectives[i].c_str());
			CclCommand("RemovePlayerObjective(" + std::to_string((long long) this->Index) + ", \"" + quest->Objectives[i] + "\");");
		}
	}
	
	if (this == ThisPlayer) {
		SetQuestCompleted(quest->Ident, GameSettings.Difficulty);
		SaveQuestCompletion();
		std::string rewards_string;
		if (!quest->Rewards.empty()) {
			rewards_string = "Rewards: " + quest->Rewards;
		}
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Completed\", \"You have completed the " + quest->Name + " quest!\\n\\n" + rewards_string + "\", nil, \"" + quest->Icon.Name + "\", \"" + PlayerColorNames[quest->PlayerColor] + "\") end;");
	}
}

void CPlayer::FailQuest(CQuest *quest, std::string fail_reason)
{
	this->CurrentQuests.erase(std::remove(this->CurrentQuests.begin(), this->CurrentQuests.end(), quest), this->CurrentQuests.end());
	
	if (this == ThisPlayer) {
		for (size_t i = 0; i < quest->Objectives.size(); ++i) {
//			SetObjective(quest->Objectives[i].c_str());
			CclCommand("RemovePlayerObjective(" + std::to_string((long long) this->Index) + ", \"" + quest->Objectives[i] + "\");");
		}

		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Failed\", \"You have failed the " + quest->Name + " quest! " + fail_reason + "\", nil, \"" + quest->Icon.Name + "\", \"" + PlayerColorNames[quest->PlayerColor] + "\") end;");
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
	
	if (quest->Conditions) {
		CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
		quest->Conditions->pushPreamble();
		quest->Conditions->run(1);
		return quest->Conditions->popBoolean();
	} else {
		return false;
	}
}

bool CPlayer::HasCompletedQuest(CQuest *quest)
{
	if (quest->Uncompleteable) {
		return false;
	}
	
	for (size_t i = 0; i < this->QuestBuildUnits.size(); ++i) {
		if (std::get<0>(this->QuestBuildUnits[i]) == quest && std::get<2>(this->QuestBuildUnits[i]) > 0) {
			return false;
		}
	}
	
	for (size_t i = 0; i < this->QuestResearchUpgrades.size(); ++i) {
		if (std::get<0>(this->QuestResearchUpgrades[i]) == quest && UpgradeIdentAllowed(*this, std::get<1>(this->QuestResearchUpgrades[i])->Ident) != 'R') {
			return false;
		}
	}
	
	for (size_t i = 0; i < this->QuestDestroyUnits.size(); ++i) {
		if (std::get<0>(this->QuestDestroyUnits[i]) == quest && std::get<3>(this->QuestDestroyUnits[i]) > 0) {
			return false;
		}
	}
	
	for (size_t i = 0; i < this->QuestDestroyUniques.size(); ++i) {
		if (std::get<0>(this->QuestDestroyUniques[i]) == quest && std::get<2>(this->QuestDestroyUniques[i]) == false) {
			return false;
		}
	}
	
	for (size_t i = 0; i < this->QuestGatherResources.size(); ++i) {
		if (std::get<0>(this->QuestGatherResources[i]) == quest && std::get<2>(this->QuestGatherResources[i]) > 0) {
			return false;
		}
	}
	
	return true;
}

std::string CPlayer::HasFailedQuest(CQuest *quest) // returns the reason for failure (empty if none)
{
	if (quest->CurrentCompleted) { // quest already completed by someone else
		return "Another faction has completed the quest before you.";
	}

	for (size_t i = 0; i < this->QuestBuildUnits.size(); ++i) {
		if (std::get<0>(this->QuestBuildUnits[i]) == quest && std::get<2>(this->QuestBuildUnits[i]) > 0) {
			bool has_builder = false;
			CUnitType *type = std::get<1>(this->QuestBuildUnits[i]);
			for (size_t j = 0; j < type->TrainedBy.size(); ++j) {
				if (this->UnitTypesCount[type->TrainedBy[j]->Slot] > 0) {
					has_builder = true;
					break;
				}
			}
			if (!has_builder) {
				return "You can no longer produce the required unit.";
			}
		}
	}
	
	for (size_t i = 0; i < this->QuestDestroyUniques.size(); ++i) {
		if (std::get<0>(this->QuestDestroyUniques[i]) == quest && std::get<2>(this->QuestDestroyUniques[i]) == true && std::get<1>(this->QuestDestroyUniques[i])->CanDrop()) { // if is supposed to destroy a unique, but it is nowhere to be found, fail the quest
			return "The target no longer exists.";
		}
	}
	
	return "";
}

bool CPlayer::AtPeace() const
{
	for (int i = 0; i < PlayerNumNeutral; ++i) {
		if (this->IsEnemy(Players[i]) && this->HasContactWith(Players[i]) && Players[i].GetUnitCount() > 0) {
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


int CPlayer::GetUnitTotalCount(const CUnitType &type) const
{
	int count = UnitTypesCount[type.Slot];
	for (std::vector<CUnit *>::const_iterator it = this->UnitBegin(); it != this->UnitEnd(); ++it) {
		//Wyrmgus start
		if (*it == NULL) {
			fprintf(stderr, "Error in CPlayer::GetUnitTotalCount: unit of player %d is NULL.\n", this->Index);
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
	if (type.Building && NumBuildings >= BuildingLimit) {
		Notify("%s", _("Building Limit Reached"));
		return -1;
	}
	if (!type.Building && (this->GetUnitCount() - NumBuildings) >= UnitLimit) {
		Notify("%s", _("Unit Limit Reached"));
		return -2;
	}
	//Wyrmgus start
//	if (this->Demand + type.Stats[this->Index].Variables[DEMAND_INDEX].Value > this->Supply && type.Stats[this->Index].Variables[DEMAND_INDEX].Value) {
	if (this->Demand + (type.Stats[this->Index].Variables[DEMAND_INDEX].Value * (type.TrainQuantity ? type.TrainQuantity : 1)) > this->Supply && type.Stats[this->Index].Variables[DEMAND_INDEX].Value) {
	//Wyrmgus end
		Notify("%s", _("Insufficient Supply, increase Supply."));
		return -3;
	}
	if (this->GetUnitCount() >= TotalUnitLimit) {
		Notify("%s", _("Total Unit Limit Reached"));
		return -4;
	}
	if (GetUnitTotalCount(type) >= Allow.Units[type.Slot]) {
		Notify(_("Limit of %d reached for this unit type"), Allow.Units[type.Slot]);
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
			const char *actionName = DefaultActions[i].c_str();

			//Wyrmgus start
//			Notify(_("Not enough %s...%s more %s."), _(name), _(actionName), _(name));
			Notify(_("Not enough %s... %s more %s."), _(name), _(actionName), _(name)); //added extra space to look better
			//Wyrmgus end

			//Wyrmgus start
//			if (this == ThisPlayer && GameSounds.NotEnoughRes[this->Race][i].Sound) {
			if (this == ThisPlayer && GameSounds.NotEnoughRes[this->Race][i].Sound && !sound_played) {
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
int CPlayer::CheckUnitType(const CUnitType &type) const
{
	//Wyrmgus start
//	return this->CheckCosts(type.Stats[this->Index].Costs);
	int modified_costs[MaxCosts];
	for (int i = 1; i < MaxCosts; ++i) {
		modified_costs[i] = type.Stats[this->Index].Costs[i];
		if (type.TrainQuantity) 
		{
			modified_costs[i] *= type.TrainQuantity;
		}
	}
	return this->CheckCosts(modified_costs);
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
void CPlayer::AddUnitType(const CUnitType &type)
{
	AddCosts(type.Stats[this->Index].Costs);
}

/**
**  Add a factor of costs to the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::AddCostsFactor(const int *costs, int factor)
{
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
void CPlayer::SubUnitType(const CUnitType &type)
{
	//Wyrmgus start
//	this->SubCosts(type.Stats[this->Index].Costs);
	this->SubCostsFactor(type.Stats[this->Index].Costs, 100 * (type.TrainQuantity ? type.TrainQuantity : 1));
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

/**
**  Have unit of type.
**
**  @param type    Type of unit.
**
**  @return        How many exists, false otherwise.
*/
int CPlayer::HaveUnitTypeByType(const CUnitType &type) const
{
	return UnitTypesCount[type.Slot];
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
	return UnitTypesCount[UnitTypeByIdent(ident)->Slot];
}

/**
**  Initialize the Ai for all players.
*/
void PlayersInitAi()
{
	for (int player = 0; player < NumPlayers; ++player) {
		if (Players[player].AiEnabled) {
			AiInit(Players[player]);
		}
	}
}

/**
**  Handle AI of all players each game cycle.
*/
void PlayersEachCycle()
{
	for (int player = 0; player < NumPlayers; ++player) {
		CPlayer &p = Players[player];
		
		//Wyrmgus start
		if (p.LostTownHallTimer && !p.Revealed && p.LostTownHallTimer < ((int) GameCycle) && ThisPlayer->HasContactWith(p)) {
			p.Revealed = true;
			for (int j = 0; j < NumPlayers; ++j) {
				if (player != j && Players[j].Type != PlayerNobody) {
					Players[j].Notify(_("%s's units have been revealed!"), p.Name.c_str());
				} else {
					Players[j].Notify("%s", _("Your units have been revealed!"));
				}
			}
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
	CPlayer &player = Players[playerIdx];

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
	player.UpdateCurrentQuests();
	//Wyrmgus end
}

//Wyrmgus start
/**
**  Handle AI of a player each minute.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachMinute(int playerIdx)
{
	CPlayer &player = Players[playerIdx];

//	player.UpdateHeroPool();
	player.UpdateQuestPool(); // every minute, update the quest pool
	
	// split off factions
	if (GameCycle >= CYCLES_PER_MINUTE && player.Faction != -1 && CurrentCampaign != NULL) { // only do this after the first minute has passed, and only for the campaign mode
		CFaction *faction = PlayerRaces.Factions[player.Race][player.Faction];
		for (size_t i = 0; i < faction->SplitsTo.size(); ++i) {
			int splitter_faction_id = PlayerRaces.GetFactionIndexByName(player.Race, faction->SplitsTo[i]);
			if (splitter_faction_id == -1) {
				continue;
			}
			CFaction *splitter_faction = PlayerRaces.Factions[player.Race][splitter_faction_id];
			if (GetFactionPlayer(splitter_faction) == NULL && SyncRand(10) == 0) { // 10% chance a particular faction will split off in a given minute
				CUnit *worker_unit = NULL;
				const int nunits = player.GetUnitCount();
				for (int j = 0; j < nunits; ++j) {
					CUnit &unit = player.GetUnit(j);
					if (unit.IsAliveOnMap() && unit.Type->BoolFlag[HARVESTER_INDEX].value) {
						worker_unit = &unit;
						break;
					}
				}
				if (!worker_unit) { // no worker unit that can be used to check a path to a deposit from
					break;
				}
				
				CUnit *depot = FindDeposit(*worker_unit, 1000, GoldCost);
				if (!depot) {
					break;
				}
				
				// generate the new faction's units near a gold deposit
				CUnit *deposit = NULL;
				int resource_range = 0;
				for (int j = 0; j < 3; ++j) { //search for resources first in a 64 tile radius, then in a 128 tile radius, and then in the whole map
					resource_range += 64;
					if (j == 2) {
						resource_range = 1000;
					}
					deposit = UnitFindResource(*worker_unit, depot ? *depot : *worker_unit, resource_range, GoldCost, true, NULL, false, true, true);
					if (deposit) {
						break; // found deposit
					}
				}
				if (!deposit) { // no gold deposit available
					break;
				}
				
				Vec2i splitter_start_pos = Map.GenerateUnitLocation(depot->Type, splitter_faction, deposit->tilePos - Vec2i(depot->Type->TileWidth - 1, depot->Type->TileHeight - 1) - Vec2i(8, 8), deposit->tilePos + Vec2i(deposit->Type->TileWidth - 1, deposit->Type->TileHeight - 1) + Vec2i(8, 8), deposit->MapLayer);
				if (!Map.Info.IsPointOnMap(splitter_start_pos, deposit->MapLayer)) {
					break;
				}
				
				int new_player_id = -1;
				for (int j = 0; j < NumPlayers; ++j) {
					if (Players[j].Type == PlayerNobody) {
						Players[j].Type = PlayerComputer;
						Players[j].SetCivilization(splitter_faction->Civilization);
						Players[j].SetFaction(splitter_faction->Ident);
						Players[j].AiEnabled = true;
						Players[j].AiName = faction->DefaultAI;
						Players[j].Team = 1;
						Players[j].Resources[GoldCost] = 5000; // give the new player enough resources to start up
						Players[j].Resources[WoodCost] = 5000;
						Players[j].Resources[StoneCost] = 5000;
						AiInit(Players[j]);
						new_player_id = j;
						break;
					}
				}
				if (new_player_id == -1) { // no player slot left
					break;
				}				
				
				Players[new_player_id].SetStartView(splitter_start_pos, deposit->MapLayer);
				CUnit *new_depot = CreateUnit(splitter_start_pos, *depot->Type, &Players[new_player_id], deposit->MapLayer); // create town hall for the new tribe
				for (int j = 0; j < 5; ++j) { // create five workers for the new tribe
					CUnit *new_worker_unit = CreateUnit(splitter_start_pos, *worker_unit->Type, &Players[new_player_id], deposit->MapLayer);
				}
				
				//acquire all technologies of the parent player
				for (size_t j = 0; j < AllUpgrades.size(); ++j) {
					if (UpgradeIdentAllowed(player, AllUpgrades[j]->Ident) == 'R') {
						UpgradeAcquire(Players[new_player_id], AllUpgrades[j]);
					}
				}
				
				if (&player == ThisPlayer) {
					std::string dialog_text = "The " + splitter_faction->Name + " has split off from our people, settling in new lands!";
					CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"The " + splitter_faction->Name + "\", \"" + dialog_text + "\") end;");
				}
			}
		}
	}
}
//Wyrmgus end

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
	
	if (sprite.TimeOfDay == DawnTimeOfDay) {
		time_of_day_red = -20;
		time_of_day_green = -20;
		time_of_day_blue = 0;
	} else if (sprite.TimeOfDay == MorningTimeOfDay || sprite.TimeOfDay == MiddayTimeOfDay || sprite.TimeOfDay == AfternoonTimeOfDay) {
		time_of_day_red = 0;
		time_of_day_green = 0;
		time_of_day_blue = 0;
	} else if (sprite.TimeOfDay == DuskTimeOfDay) {
		time_of_day_red = 0;
		time_of_day_green = -20;
		time_of_day_blue = -20;
	} else if (sprite.TimeOfDay == FirstWatchTimeOfDay || sprite.TimeOfDay == MidnightTimeOfDay || sprite.TimeOfDay == SecondWatchTimeOfDay) {
		time_of_day_red = -45;
		time_of_day_green = -35;
		time_of_day_blue = -10;
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
//		Players[i].UnitColors.Colors = PlayerColorsRGB[i];
		if (Players[i].Faction == -1) {
			Players[i].UnitColors.Colors = PlayerColorsRGB[i];
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
		if (Players[i].Type == PlayerNobody) {
			continue;
		}
		const char *playertype;

		switch (Players[i].Type) {
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
				   ThisPlayer == &Players[i] ? '*' :
				   Players[i].AiEnabled ? '+' : ' ' _C_
				   Players[i].Name.c_str() _C_ playertype _C_
				   PlayerRaces.Name[Players[i].Race].c_str() _C_
				   Players[i].AiName.c_str());
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
//Wyrmgus start
//void CPlayer::Notify(int type, const Vec2i &pos, const char *fmt, ...) const
void CPlayer::Notify(int type, const Vec2i &pos, int z, const char *fmt, ...) const
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
	Assert(Map.Info.IsPointOnMap(pos, z));
	//Wyrmgus end
	char temp[128];
	Uint32 color;
	va_list va;

	// Notify me, and my TEAM members
	if (this != ThisPlayer && !IsTeamed(*ThisPlayer)) {
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
	//Wyrmgus start
//	UI.Minimap.AddEvent(pos, color);
	UI.Minimap.AddEvent(pos, z, color);
	//Wyrmgus end
	if (this == ThisPlayer) {
		//Wyrmgus start
//		SetMessageEvent(pos, "%s", temp);
		SetMessageEvent(pos, z, "%s", temp);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		SetMessageEvent(pos, "(%s): %s", Name.c_str(), temp);
		SetMessageEvent(pos, z, "(%s): %s", Name.c_str(), temp);
		//Wyrmgus end
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
	if (this != ThisPlayer && !IsTeamed(*ThisPlayer)) {
		return;
	}
	char temp[128];
	va_list va;

	va_start(va, fmt);
	temp[sizeof(temp) - 1] = '\0';
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	va_end(va);
	if (this == ThisPlayer) {
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
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s changed their diplomatic stance with us to Neutral"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::SetDiplomacyAlliedWith(const CPlayer &player)
{
	this->Enemy &= ~(1 << player.Index);
	this->Allied |= 1 << player.Index;
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s changed their diplomatic stance with us to Ally"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::SetDiplomacyEnemyWith(const CPlayer &player)
{
	this->Enemy |= 1 << player.Index;
	this->Allied &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s changed their diplomatic stance with us to Enemy"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::SetDiplomacyCrazyWith(const CPlayer &player)
{
	this->Enemy |= 1 << player.Index;
	this->Allied |= 1 << player.Index;
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s changed their diplomatic stance with us to Crazy"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::ShareVisionWith(const CPlayer &player)
{
	this->SharedVision |= (1 << player.Index);
	
	//Wyrmgus start
	if (player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s is now sharing vision with us"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}

void CPlayer::UnshareVisionWith(const CPlayer &player)
{
	this->SharedVision &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s is no longer sharing vision with us"), _(this->Name.c_str()));
	}
	//Wyrmgus end
}


/**
**  Check if the player is an enemy
*/
bool CPlayer::IsEnemy(const CPlayer &player) const
{
	return IsEnemy(player.Index);
}

/**
**  Check if the unit is an enemy
*/
bool CPlayer::IsEnemy(const CUnit &unit) const
{
	//Wyrmgus start
	if (unit.Player->Index != this->Index && this->Type != PlayerNeutral && unit.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive()) {
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
**  Check if the player has contact with another (used for determining which players show up in the player list and etc.)
*/
bool CPlayer::HasContactWith(const CPlayer &player) const
{
	return player.StartMapLayer == this->StartMapLayer || (Map.Worlds[player.StartMapLayer] == Map.Worlds[this->StartMapLayer] && Map.Planes[player.StartMapLayer] == Map.Planes[this->StartMapLayer]);
}

void SetCivilizationStringToIndex(std::string civilization_name, int civilization_id)
{
	CivilizationStringToIndex[civilization_name] = civilization_id;
}

void SetFactionStringToIndex(int civilization, std::string faction_name, int faction_id)
{
	FactionStringToIndex[civilization][faction_name] = faction_id;
}

void NetworkSetFaction(int player, std::string faction_name)
{
	int faction = PlayerRaces.GetFactionIndexByName(Players[player].Race, faction_name);
	SendCommandSetFaction(player, faction);
}

std::string GetFactionEffectsString(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			std::string faction_effects_string;
			
			//check if the faction has a different unit type from its civilization
			bool first_element = true;
			for (size_t i = 0; i < UnitTypeClasses.size(); ++i) {
				int unit_type_id = PlayerRaces.GetFactionClassUnitType(civilization, faction, i);
				int base_unit_type_id = PlayerRaces.GetCivilizationClassUnitType(civilization, i);
				if (unit_type_id != -1 && unit_type_id != base_unit_type_id) {
					bool changed_stats = false;
					std::string effect_element_string;
					
					if (!first_element) {
						effect_element_string += ", ";
					}
					
					effect_element_string += UnitTypes[unit_type_id]->Name;
					effect_element_string += " (";
					
					if (UnitTypes[unit_type_id]->Name != UnitTypes[base_unit_type_id]->Name) {
						effect_element_string += _(FullyCapitalizeString(FindAndReplaceString(UnitTypes[unit_type_id]->Class, "-", " ")).c_str());
						effect_element_string += ", ";
						changed_stats = true;
					}
					
					bool first_var = true;
					for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
						if (j == PRIORITY_INDEX || j == POINTS_INDEX) {
							continue;
						}
						
						if (UnitTypes[unit_type_id]->DefaultStat.Variables[j].Value != UnitTypes[base_unit_type_id]->DefaultStat.Variables[j].Value) {
							if (!first_var) {
								effect_element_string += ", ";
							} else {
								first_var = false;
							}
							
							int variable_difference = UnitTypes[unit_type_id]->DefaultStat.Variables[j].Value - UnitTypes[base_unit_type_id]->DefaultStat.Variables[j].Value;
							
							if (IsBooleanVariable(j) && variable_difference < 0) {
								effect_element_string += _("Lose");
								effect_element_string += " ";
							}
							
							if (!IsBooleanVariable(j)) {
								if (variable_difference > 0) {
									effect_element_string += "+";
								}

								effect_element_string += std::to_string((long long) variable_difference);
								if (IsPercentageVariable(j)) {
									effect_element_string += "%";
								}
								effect_element_string += " ";
							}
							
							effect_element_string += GetVariableDisplayName(j);
							changed_stats = true;
						}
					}
					
					effect_element_string += ")";
					
					if (changed_stats) {
						faction_effects_string += effect_element_string;
						if (first_element) {
							first_element = false;
						}
					}
				}
			}
			
			//check if the faction's upgrade makes modifications to any units
			if (!PlayerRaces.Factions[civilization][faction]->FactionUpgrade.empty()) {
				int faction_upgrade_id = CUpgrade::Get(PlayerRaces.Factions[civilization][faction]->FactionUpgrade)->ID;
				
				for (size_t z = 0; z < AllUpgrades[faction_upgrade_id]->UpgradeModifiers.size(); ++z) {
					if (!AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->ConvertTo) {
						for (size_t i = 0; i < UnitTypes.size(); ++i) {
							Assert(AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == '?' || AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == 'X');

							if (AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
								bool changed_stats = false;
								std::string effect_element_string;
								
								if (!first_element) {
									effect_element_string += ", ";
								} else {
									first_element = false;
								}
									
								effect_element_string += _(UnitTypes[i]->Name.c_str());
								effect_element_string += " (";

								bool first_var = true;
								for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
									if (j == PRIORITY_INDEX || j == POINTS_INDEX) {
										continue;
									}
						
									if (AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[j].Value != 0) {
										if (!first_var) {
											effect_element_string += ", ";
										} else {
											first_var = false;
										}
											
										if (IsBooleanVariable(j) && AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[j].Value < 0) {
											effect_element_string += _("Lose");
											effect_element_string += " ";
										}
										
										if (!IsBooleanVariable(j)) {
											if (AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[j].Value > 0) {
												effect_element_string += "+";
											}
											effect_element_string += std::to_string((long long) AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[j].Value);
											if (IsPercentageVariable(j)) {
												effect_element_string += "%";
											}
											effect_element_string += " ";
										}
											
										effect_element_string += GetVariableDisplayName(j);
										changed_stats = true;
									}
								}

								for (int j = 0; j < MaxCosts; ++j) {
									if (AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.ImproveIncomes[j]) {
										if (!first_var) {
											effect_element_string += ", ";
										} else {
											first_var = false;
										}
										
										if (AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.ImproveIncomes[j] > 0) {
											effect_element_string += "+";
										}
										effect_element_string += std::to_string((long long) AllUpgrades[faction_upgrade_id]->UpgradeModifiers[z]->Modifier.ImproveIncomes[j]);
										effect_element_string += "%";
										effect_element_string += " ";
										effect_element_string += _(CapitalizeString(DefaultResourceNames[j]).c_str());
										effect_element_string += " ";
										effect_element_string += _("Processing");
										changed_stats = true;
									}
								}
						
								effect_element_string += ")";
								if (changed_stats) {
									faction_effects_string += effect_element_string;
								}
							}
						}
					}
				}
			}
			
			return faction_effects_string;
		}
	}
	
	return "";
}

int GetPlayerColorIndexByName(std::string player_color_name)
{
	for (int c = 0; c < PlayerColorMax; ++c) {
		if (PlayerColorNames[c] == player_color_name) {
			return c;
		}
	}
	return -1;
}

int GetSkinColorIndexByName(std::string skin_color_name)
{
	for (int c = 1; c < SkinColorMax; ++c) {
		if (SkinColorNames[c].empty()) {
			break;
		}
		if (SkinColorNames[c] == skin_color_name) {
			return c;
		}
	}
	return 0;
}

int GetHairColorIndexByName(std::string hair_color_name)
{
	for (int c = 1; c < HairColorMax; ++c) {
		if (HairColorNames[c].empty()) {
			break;
		}
		if (HairColorNames[c] == hair_color_name) {
			return c;
		}
	}
	return 0;
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
	}

	return "";
}

int GetFactionTypeIdByName(std::string faction_type)
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

int GetGovernmentTypeIdByName(std::string government_type)
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

int GetWordTypeIdByName(std::string word_type)
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

int GetArticleTypeIdByName(std::string article_type)
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

int GetGrammaticalCaseIdByName(std::string grammatical_case)
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

int GetGrammaticalNumberIdByName(std::string grammatical_number)
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

int GetGrammaticalPersonIdByName(std::string grammatical_person)
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

int GetGrammaticalGenderIdByName(std::string grammatical_gender)
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

int GetGrammaticalTenseIdByName(std::string grammatical_tense)
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

int GetGrammaticalMoodIdByName(std::string grammatical_mood)
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

int GetComparisonDegreeIdByName(std::string comparison_degree)
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

int GetAffixTypeIdByName(std::string affix_type)
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

int GetWordJunctionTypeIdByName(std::string word_junction_type)
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

	return NULL;
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

int CLanguage::GetPotentialNameQuantityForType(std::string type)
{
	if (this->PotentialNameQuantityForType.find(type) != this->PotentialNameQuantityForType.end()) {
		return this->PotentialNameQuantityForType.find(type)->second;
	}
	
	return 0;
}

void CLanguage::CalculatePotentialNameQuantityForType(std::string type)
{
	int name_count = this->NameTypeWords[type].size();
	
	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		name_count += this->NameTypeAffixes[i][AffixTypePrefix][type].size() * this->NameTypeAffixes[i][AffixTypeSuffix][type].size();
		name_count += this->NameTypeAffixes[i][AffixTypePrefix][type].size() * this->NameTypeAffixes[i][AffixTypeInfix][type].size() * this->NameTypeAffixes[i][AffixTypeSuffix][type].size();
	}
	
	this->PotentialNameQuantityForType[type] = name_count;
}

void CLanguage::RemoveWord(LanguageWord *word)
{
	if (std::find(this->LanguageWords.begin(), this->LanguageWords.end(), word) != this->LanguageWords.end()) {
		this->LanguageWords.erase(std::remove(this->LanguageWords.begin(), this->LanguageWords.end(), word), this->LanguageWords.end());
	}

	for (std::map<std::string, std::vector<LanguageWord *>>::iterator iterator = this->NameTypeWords.begin(); iterator != this->NameTypeWords.end(); ++iterator) {
		if (std::find(this->NameTypeWords[iterator->first].begin(), this->NameTypeWords[iterator->first].end(), word) != this->NameTypeWords[iterator->first].end()) {
			this->NameTypeWords[iterator->first].erase(std::remove(this->NameTypeWords[iterator->first].begin(), this->NameTypeWords[iterator->first].end(), word), this->NameTypeWords[iterator->first].end());
		}
	}

	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		for (int j = 0; j < MaxAffixTypes; ++j) {
			for (std::map<std::string, std::vector<LanguageWord *>>::iterator iterator = this->NameTypeAffixes[i][j].begin(); iterator != this->NameTypeAffixes[i][j].end(); ++iterator) {
				if (std::find(this->NameTypeAffixes[i][j][iterator->first].begin(), this->NameTypeAffixes[i][j][iterator->first].end(), word) != this->NameTypeAffixes[i][j][iterator->first].end()) {
					this->NameTypeAffixes[i][j][iterator->first].erase(std::remove(this->NameTypeAffixes[i][j][iterator->first].begin(), this->NameTypeAffixes[i][j][iterator->first].end(), word), this->NameTypeAffixes[i][j][iterator->first].end());
				}
			}
		}
	}
}

int LanguageWord::HasNameType(std::string type, int grammatical_number, int grammatical_case, int grammatical_tense)
{
	int name_type_count = 0;
	
	if (grammatical_number == -1 || grammatical_case == -1 || grammatical_tense == -1) {
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			if (grammatical_number != -1 && i != grammatical_number) {
				continue;
			}
			for (int j = 0; j < MaxGrammaticalCases; ++j) {
				if (grammatical_case != -1 && i != grammatical_case) {
					continue;
				}
				for (int k = 0; k < MaxGrammaticalTenses; ++k) {
					if (grammatical_tense != -1 && i != grammatical_tense) {
						continue;
					}
					if (this->NameTypes[i][j][k].find(type) != this->NameTypes[i][j][k].end()) {
						name_type_count += this->NameTypes[i][j][k][type];
					}
				}
			}
		}
	} else {
		if (this->NameTypes[grammatical_number][grammatical_case][grammatical_tense].find(type) != this->NameTypes[grammatical_number][grammatical_case][grammatical_tense].end()) {
			name_type_count += this->NameTypes[grammatical_number][grammatical_case][grammatical_tense][type];
		}
	}
	
	return name_type_count;
}

int LanguageWord::HasAffixNameType(std::string type, int word_junction_type, int affix_type, int grammatical_number, int grammatical_case, int grammatical_tense)
{
	if (affix_type == -1) {
		return this->HasNameType(type, grammatical_number, grammatical_case, grammatical_tense);
	}
	
	int name_type_count = 0;
	
	if (grammatical_number == -1 || grammatical_case == -1 || grammatical_tense == -1) {
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			if (grammatical_number != -1 && i != grammatical_number) {
				continue;
			}
			for (int j = 0; j < MaxGrammaticalCases; ++j) {
				if (grammatical_case != -1 && i != grammatical_case) {
					continue;
				}
				for (int k = 0; k < MaxGrammaticalTenses; ++k) {
					if (grammatical_tense != -1 && i != grammatical_tense) {
						continue;
					}
					if (this->AffixNameTypes[word_junction_type][affix_type][i][j][k].find(type) != this->AffixNameTypes[word_junction_type][affix_type][i][j][k].end()) {
						name_type_count += this->AffixNameTypes[word_junction_type][affix_type][i][j][k][type];
					}
				}
			}
		}
	} else {
		if (this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].find(type) != this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].end()) {
			name_type_count += this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense][type];
		}
	}
	
	return name_type_count;
}

bool LanguageWord::HasMeaning(std::string meaning)
{
	return std::find(this->Meanings.begin(), this->Meanings.end(), meaning) != this->Meanings.end();
}

std::string LanguageWord::GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (!this->NumberCaseInflections[grammatical_number][grammatical_case].empty()) {
		return this->NumberCaseInflections[grammatical_number][grammatical_case];
	}
	
	return this->Word + PlayerRaces.Languages[this->Language]->GetNounEnding(grammatical_number, grammatical_case, word_junction_type);
}

std::string LanguageWord::GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood)
{
	if (!this->NumberPersonTenseMoodInflections[grammatical_number][grammatical_person][grammatical_tense][grammatical_mood].empty()) {
		return this->NumberPersonTenseMoodInflections[grammatical_number][grammatical_person][grammatical_tense][grammatical_mood];
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
		inflected_word += PlayerRaces.Languages[this->Language]->GetAdjectiveEnding(article_type, grammatical_case, grammatical_number, grammatical_gender);
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

int LanguageWord::GetAffixGrammaticalNumber(LanguageWord *prefix, LanguageWord *infix, LanguageWord *suffix, std::string type, int word_junction_type, int affix_type)
{
	int grammatical_number = GrammaticalNumberSingular;
	if (this->Type == WordTypeNoun) {
		std::vector<int> potential_grammatical_numbers;
		
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			if (this->HasAffixNameType(type, word_junction_type, affix_type, i, -1, -1)) {
				potential_grammatical_numbers.push_back(i);
			}
		}
		
		if (potential_grammatical_numbers.size() > 0) {
			grammatical_number = potential_grammatical_numbers[SyncRand(potential_grammatical_numbers.size())];
		}
		
		if (this != prefix && this != infix && prefix != NULL && prefix->Type == WordTypeNumeral && prefix->Number > 1) { //if prefix is a numeral that is greater than one, the grammatical number of this word must be plural
			if (grammatical_number == GrammaticalNumberSingular) {
				grammatical_number = GrammaticalNumberPlural;
			}
		}
	}
	
	return grammatical_number;
}

std::string LanguageWord::GetAffixForm(LanguageWord *prefix, LanguageWord *infix, LanguageWord *suffix, std::string type, int word_junction_type, int affix_type, int affix_grammatical_numbers[MaxAffixTypes])
{
	int grammatical_number = affix_type != -1 ? affix_grammatical_numbers[affix_type] : GrammaticalNumberSingular;
	int grammatical_case = GrammaticalCaseNominative;
	int grammatical_tense = GrammaticalTenseNoTense;
	std::string affix_form;
	
	if (affix_type == -1) { // if is a single name, get the grammatical number now (since it isn't included in the affix_grammatical_numbers array)
		std::vector<int> potential_grammatical_numbers;
			
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			if (this->HasNameType(type, i)) {
				potential_grammatical_numbers.push_back(i);
			}
		}
			
		if (potential_grammatical_numbers.size() > 0) {
			grammatical_number = potential_grammatical_numbers[SyncRand(potential_grammatical_numbers.size())];
		}
	}
	
	if (this->Type == WordTypeNoun) {
		std::vector<int> potential_grammatical_cases;
			
		for (int i = 0; i < MaxGrammaticalCases; ++i) {
			if (this->HasAffixNameType(type, word_junction_type, affix_type, grammatical_number, i, grammatical_tense)) {
				potential_grammatical_cases.push_back(i);
			}
		}
			
		if (potential_grammatical_cases.size() > 0) {
			grammatical_case = potential_grammatical_cases[SyncRand(potential_grammatical_cases.size())];
		}
		
		affix_form = this->GetNounInflection(grammatical_number, grammatical_case, word_junction_type);
	} else if (this->Type == WordTypeVerb) {
		std::vector<int> potential_grammatical_tenses;
			
		for (int i = 0; i < MaxGrammaticalTenses; ++i) {
			if (this->HasAffixNameType(type, word_junction_type, affix_type, grammatical_number, grammatical_case, i)) {
				potential_grammatical_tenses.push_back(i);
			}
		}
			
		if (potential_grammatical_tenses.size() > 0) {
			grammatical_tense = potential_grammatical_tenses[SyncRand(potential_grammatical_tenses.size())];
		}
		
		if (grammatical_tense == GrammaticalTenseNoTense) { //if the grammatical tense is no tense, then give the verb's infinitive form
			affix_form = this->Word;
		} else { //else, get the participle for this tense
			affix_form = this->GetParticiple(grammatical_tense);
		}
	} else if (this->Type == WordTypeAdjective) {
		affix_form = this->GetAdjectiveInflection(ComparisonDegreePositive);

		std::vector<int> potential_grammatical_cases;
			
		for (int i = 0; i < MaxGrammaticalCases; ++i) {
			if (this->HasAffixNameType(type, word_junction_type, affix_type, grammatical_number, i, grammatical_tense)) {
				potential_grammatical_cases.push_back(i);
			}
		}
			
		if (potential_grammatical_cases.size() > 0) {
			grammatical_case = potential_grammatical_cases[SyncRand(potential_grammatical_cases.size())];
		}
			
		if (word_junction_type == WordJunctionTypeSeparate || grammatical_case != GrammaticalCaseNominative) {
			grammatical_number = affix_grammatical_numbers[AffixTypeSuffix];
			affix_form = this->GetAdjectiveInflection(ComparisonDegreePositive, ArticleTypeDefinite, grammatical_case, grammatical_number, suffix != NULL ? suffix->Gender : -1);
		}
	} else {
		affix_form = this->Word;
	}
	
	return affix_form;
}

void LanguageWord::AddNameTypeGenerationFromWord(LanguageWord *word, std::string type)
{
	for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
		for (int j = 0; j < MaxGrammaticalCases; ++j) {
			for (int k = 0; k < MaxGrammaticalTenses; ++k) {
				if (word->HasNameType(type, i, j, k) && !this->HasNameType(type, i, j, k)) {
					this->NameTypes[i][j][k][type] = word->HasNameType(type, i, j, k);
					this->AddToLanguageNameTypes(type);
				}
			}
		}
	}
	
	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		for (int j = 0; j < MaxAffixTypes; ++j) {
			for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
				for (int n = 0; n < MaxGrammaticalCases; ++n) {
					for (int o = 0; o < MaxGrammaticalTenses; ++o) {
						if ((this->Type == word->Type || i != WordJunctionTypeSeparate) && word->HasAffixNameType(type, i, j, k, n, o) && !this->HasAffixNameType(type, i, j, k, n, o)) {
							this->AffixNameTypes[i][j][k][n][o][type] = word->HasAffixNameType(type, i, j, k, n, o);
							this->AddToLanguageAffixNameTypes(type, i, j);
						}
					}
				}
			}
		}
	}
	
	//if the word being inherited from is used as part of a compound word, inherit relevant type name generation (i.e. if the related word is "Alf", and it is set as the prefix in the compound "Alfred", and "Alfred" as NameType generation for persons, then add PrefixNameType generation for the word inheriting type name generation from "Alf")
	for (int i = 0; i < MaxAffixTypes; ++i) {
		for (int j = 0; j < MaxGrammaticalNumbers; ++j) {
			for (int k = 0; k < MaxGrammaticalCases; ++k) {
				for (int n = 0; n < MaxGrammaticalTenses; ++n) {
					if (!this->HasAffixNameType(type, WordJunctionTypeCompound, i, j, k, n)) {
						this->AffixNameTypes[WordJunctionTypeCompound][i][j][k][n][type] = 0;
						for (size_t o = 0; o < word->CompoundElementOf[i].size(); ++o) {
							if (word->CompoundElementOf[i][o]->HasNameType(type, j, k, n)) {
								this->AffixNameTypes[WordJunctionTypeCompound][i][j][k][n][type] += word->CompoundElementOf[i][o]->HasNameType(type, j, k, n);
								this->AddToLanguageAffixNameTypes(type, WordJunctionTypeCompound, i);
							}
						}
					}
				}
			}
		}
	}
}

void LanguageWord::AddToLanguageNameTypes(std::string type)
{
	if (std::find(PlayerRaces.Languages[this->Language]->NameTypeWords[type].begin(), PlayerRaces.Languages[this->Language]->NameTypeWords[type].end(), this) == PlayerRaces.Languages[this->Language]->NameTypeWords[type].end()) {
		PlayerRaces.Languages[this->Language]->NameTypeWords[type].push_back(this);
	}
	for (size_t i = 0; i < PlayerRaces.Languages[this->Language]->Dialects.size(); ++i) { //do the same for the dialects
		if (std::find(PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeWords[type].begin(), PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeWords[type].end(), this) == PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeWords[type].end()) {
			PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeWords[type].push_back(this);
		}
	}
	
	PlayerRaces.Languages[this->Language]->CalculatePotentialNameQuantityForType(type);
}

void LanguageWord::AddToLanguageAffixNameTypes(std::string type, int word_junction_type, int affix_type)
{
	if (std::find(PlayerRaces.Languages[this->Language]->NameTypeAffixes[word_junction_type][affix_type][type].begin(), PlayerRaces.Languages[this->Language]->NameTypeAffixes[word_junction_type][affix_type][type].end(), this) == PlayerRaces.Languages[this->Language]->NameTypeAffixes[word_junction_type][affix_type][type].end()) {
		PlayerRaces.Languages[this->Language]->NameTypeAffixes[word_junction_type][affix_type][type].push_back(this);
	}
	for (size_t i = 0; i < PlayerRaces.Languages[this->Language]->Dialects.size(); ++i) { //do the same for the dialects
		if (std::find(PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeAffixes[word_junction_type][affix_type][type].begin(), PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeAffixes[word_junction_type][affix_type][type].end(), this) == PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeAffixes[word_junction_type][affix_type][type].end()) {
			PlayerRaces.Languages[this->Language]->Dialects[i]->NameTypeAffixes[word_junction_type][affix_type][type].push_back(this);
		}
	}
	
	PlayerRaces.Languages[this->Language]->CalculatePotentialNameQuantityForType(type);
}

void LanguageWord::IncreaseNameType(std::string type, int grammatical_number, int grammatical_case, int grammatical_tense)
{
	if (this->NameTypes[grammatical_number][grammatical_case][grammatical_tense].find(type) == this->NameTypes[grammatical_number][grammatical_case][grammatical_tense].end()) {
		this->NameTypes[grammatical_number][grammatical_case][grammatical_tense][type] = 0;
	}
	this->NameTypes[grammatical_number][grammatical_case][grammatical_tense][type] += 1;
}

void LanguageWord::IncreaseAffixNameType(std::string type, int word_junction_type, int affix_type, int grammatical_number, int grammatical_case, int grammatical_tense)
{
	if (this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].find(type) == this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].end()) {
		this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense][type] = 0;
	}
	this->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense][type] += 1;
}

void LanguageWord::StripNameTypeGeneration(std::string type)
{
	for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
		for (int j = 0; j < MaxGrammaticalCases; ++j) {
			for (int k = 0; k < MaxGrammaticalTenses; ++k) {
				if (this->HasNameType(type, i, j, k)) {
					this->NameTypes[i][j][k][type] = 0;
				}
			}
		}
	}
	
	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		for (int j = 0; j < MaxAffixTypes; ++j) {
			for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
				for (int n = 0; n < MaxGrammaticalCases; ++n) {
					for (int o = 0; o < MaxGrammaticalTenses; ++o) {
						if (this->HasAffixNameType(type, i, j, k, n, o)) {
							this->AffixNameTypes[i][j][k][n][o][type] = 0;
						}
					}
				}
			}
		}
	}
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

void GenerateMissingLanguageData()
{
	std::vector<std::string> types;
	int minimum_desired_names = 25;

	int default_language = PlayerRaces.GetLanguageIndexByIdent("english");
	size_t markov_chain_size = 2;
	
	// generate "missing" words (missing equivalent words for English words) for languages which have that enabled, with a Markov chain process
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		if (default_language == -1) {
			break;
		}

		if (PlayerRaces.Languages[i]->GenerateMissingWords) {
			ShowLoadProgress("Generating words for the %s language", PlayerRaces.Languages[i]->Name.c_str());
			
			// produce a list with the Markov chain elements
			size_t maximum_word_length = 0;
			std::map<std::string, std::vector<std::string>> markov_elements;
			std::map<std::string, std::vector<std::string>> markov_elements_per_type[MaxWordTypes];
			for (size_t j = 0; j < PlayerRaces.Languages[i]->LanguageWords.size(); ++j) {
				std::string word = DecapitalizeString(PlayerRaces.Languages[i]->LanguageWords[j]->Word);
				if (word.size() > maximum_word_length) {
					maximum_word_length = word.size();
				}
				
				for(size_t k = 0; k < word.size(); ++k) {
					std::string element = word.substr(k, markov_chain_size);

					const std::string following_letter = k < (word.size() - 1) ? word.substr(k + markov_chain_size, 1) : ""; // an empty string indicates ending the name
					markov_elements[element].push_back(following_letter);
					markov_elements_per_type[PlayerRaces.Languages[i]->LanguageWords[j]->Type][element].push_back(following_letter);

					if (k == 0) {
						markov_elements["^"].push_back(element); // "^" indicates beginning the name
						markov_elements_per_type[PlayerRaces.Languages[i]->LanguageWords[j]->Type]["^"].push_back(element);
					
						const std::string starting_following_letter = k < (word.size() - 1) ? word.substr(k + markov_chain_size - 1, 1) : "";
						std::string starting_element = "^" + word.substr(k, markov_chain_size - 1);
						markov_elements[starting_element].push_back(starting_following_letter);
						markov_elements_per_type[PlayerRaces.Languages[i]->LanguageWords[j]->Type][starting_element].push_back(starting_following_letter);
					}
				}
			}

			for (size_t j = 0; j < PlayerRaces.Languages[default_language]->LanguageWords.size(); ++j) {
				if (!PlayerRaces.Languages[default_language]->LanguageWords[j]->Archaic && (PlayerRaces.Languages[default_language]->LanguageWords[j]->Meanings.size() > 0 || PlayerRaces.Languages[default_language]->LanguageWords[j]->Number != -1)) {
					bool language_has_equivalent = false;
					for (size_t k = 0; k < PlayerRaces.Languages[i]->LanguageWords.size(); ++k) {
						for (size_t n = 0; n < PlayerRaces.Languages[default_language]->LanguageWords[j]->Meanings.size(); ++n) {
							if (PlayerRaces.Languages[i]->LanguageWords[k]->HasMeaning(PlayerRaces.Languages[default_language]->LanguageWords[j]->Meanings[n])) {
								language_has_equivalent = true;
								break;
							}
						}
						if (language_has_equivalent) {
							break;
						}
					}
					if (language_has_equivalent) {
						continue;
					}
					
					//generate the word from the markov elements
					std::string previous_element = "^";
					std::string new_word = "^";
					int word_type = PlayerRaces.Languages[default_language]->LanguageWords[j]->Type;
					
					while ((new_word.size() - 1) < maximum_word_length) {
						std::string next_element;
						
						if (markov_elements_per_type[word_type].size() > 0 && markov_elements_per_type[word_type][previous_element].size() > 0) {
							next_element = markov_elements_per_type[word_type][previous_element][SyncRand(markov_elements_per_type[word_type][previous_element].size())];
						} else if (markov_elements[previous_element].size() > 0) {
							next_element = markov_elements[previous_element][SyncRand(markov_elements[previous_element].size())];
						} else {
							fprintf(stderr, "Markov element \"%s\" has no elements it leads to.\n", previous_element.c_str());
						}
						
						if (next_element == "") {
							break;
						}
						
						new_word += next_element;
						previous_element = new_word.substr(new_word.size() - markov_chain_size, markov_chain_size);

						if (word_type == WordTypeArticle && (new_word.size() - 1) >= 3) { //articles can only have up to three letters
							break;
						}
					}
					
					if ((new_word.size() - 1) >= maximum_word_length || (word_type == WordTypeArticle && (new_word.size() - 1) >= 3)) { // if the word stopped being generated because it hit the character limit, take away characters until the last component of the name has an end of string as its next element
						std::string fixed_new_word = new_word;
						while (!fixed_new_word.empty()) {
							previous_element = fixed_new_word.substr(fixed_new_word.size() - std::min(markov_chain_size, fixed_new_word.size()), std::min(markov_chain_size, fixed_new_word.size()));
							bool has_string_ending = false;
							if (markov_elements_per_type[word_type].size() > 0 && markov_elements_per_type[word_type][previous_element].size()) {
								has_string_ending = std::find(markov_elements_per_type[word_type][previous_element].begin(), markov_elements_per_type[word_type][previous_element].end(), "") != markov_elements_per_type[word_type][previous_element].end();
							} else if (markov_elements[previous_element].size() > 0) {
								has_string_ending = std::find(markov_elements[previous_element].begin(), markov_elements[previous_element].end(), "") != markov_elements[previous_element].end();
							} else {
								fprintf(stderr, "Markov element \"%s\" has no elements it leads to.\n", previous_element.c_str());
							}
							
							if (has_string_ending) {
								break;
							} else {
								fixed_new_word = fixed_new_word.substr(0, fixed_new_word.size() - 1);
							}
						}
						if (!fixed_new_word.empty()) {
							new_word = fixed_new_word;
						}
					}
					
					new_word = FindAndReplaceStringBeginning(new_word, "^", "");
					new_word = CapitalizeString(new_word);
					
					LanguageWord *word = new LanguageWord;
					word->Word = new_word;
					word->Language = i;
					PlayerRaces.Languages[i]->LanguageWords.push_back(word);
					word->Type = word_type;
					word->Gender = PlayerRaces.Languages[default_language]->LanguageWords[j]->Gender;
					word->GrammaticalNumber = PlayerRaces.Languages[default_language]->LanguageWords[j]->GrammaticalNumber;
					word->Uncountable = PlayerRaces.Languages[default_language]->LanguageWords[j]->Uncountable;
					word->ArticleType = PlayerRaces.Languages[default_language]->LanguageWords[j]->ArticleType;
					word->Number = PlayerRaces.Languages[default_language]->LanguageWords[j]->Number;
					word->Nominative = word->Word;
					for (size_t k = 0; k < PlayerRaces.Languages[default_language]->LanguageWords[j]->Meanings.size(); ++k) {
						word->Meanings.push_back(PlayerRaces.Languages[default_language]->LanguageWords[j]->Meanings[k]);
					}
					
					//fprintf(stdout, "Generated word: \"%s\" (\"%s\"), %s, %s language.\n", new_word.c_str(), PlayerRaces.Languages[default_language]->LanguageWords[j]->Word.c_str(), GetWordTypeNameById(word_type).c_str(), PlayerRaces.Languages[i]->Name.c_str());
				}
			}
		}
	}

	// now, moving on to dealing with word type names
	// first build a vector with all the types
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		for (std::map<std::string, std::vector<LanguageWord *>>::iterator iterator = PlayerRaces.Languages[i]->NameTypeWords.begin(); iterator != PlayerRaces.Languages[i]->NameTypeWords.end(); ++iterator) {
			if (iterator->first == "river" || iterator->first == "unit-class-castle" || iterator->first == "unit-class-farm" || iterator->first.find("item-") != std::string::npos || iterator->first == "person" || iterator->first == "person-female" || iterator->first == "family" || iterator->first == "noble-family" || iterator->first == "noble-family-predicate" || iterator->first.find("species-") != std::string::npos) { //don't do this process for name types which aren't actually used by the game yet, to save performance ("person" is actually used by Kobolds, but no language can inherit language data from their language), and don't do this process for proper names for animals (it's better to have animals with the names of historical or mythological beings in the original language in which they appeared, than have names made from related words)
				continue;
			}
			if (std::find(types.begin(), types.end(), iterator->first) == types.end()) {
				types.push_back(iterator->first);
			}
		}
		for (int j = 0; j < MaxWordJunctionTypes; ++j) {
			for (int k = 0; k < MaxAffixTypes; ++k) {
				for (std::map<std::string, std::vector<LanguageWord *>>::iterator iterator = PlayerRaces.Languages[i]->NameTypeAffixes[j][k].begin(); iterator != PlayerRaces.Languages[i]->NameTypeAffixes[j][k].end(); ++iterator) {
					if (iterator->first == "river" || iterator->first == "unit-class-castle" || iterator->first == "unit-class-farm" || iterator->first.find("item-") != std::string::npos || iterator->first == "person" || iterator->first == "person-female" || iterator->first == "family" || iterator->first == "noble-family" || iterator->first == "noble-family-predicate" || iterator->first.find("species-") != std::string::npos) {
						continue;
					}
					if (std::find(types.begin(), types.end(), iterator->first) == types.end()) {
						types.push_back(iterator->first);
					}
				}
			}
		}
	}
	
	// now, try to get a minimum quantity of names per language for each type; when failing in one of them, try to assign type name settings based on those of words derived from and to the words in the failing language
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		if (!PlayerRaces.Languages[i]->UsedByCivilizationOrFaction || PlayerRaces.Languages[i]->SkipNameTypeInheritance) {
			continue;
		}
		
		std::map<LanguageWord *, std::vector<LanguageWord *>> related_words;

		for (size_t j = 0; j < PlayerRaces.Languages[i]->LanguageWords.size(); ++j) {
			LanguageWord *word = PlayerRaces.Languages[i]->LanguageWords[j];
			related_words[word].push_back(word);
		}
			
		ShowLoadProgress("Deriving name generation patterns from related words for the %s language", PlayerRaces.Languages[i]->Name.c_str());
		
		for (std::map<LanguageWord *, std::vector<LanguageWord *>>::reverse_iterator iterator = related_words.rbegin(); iterator != related_words.rend(); ++iterator) {
			LanguageWord *word = iterator->first;

			// fill the vector with all the related words for the current relationship depth level
			for (size_t n = 0; n < related_words[word].size(); ++n) {
				if (
					related_words[word][n]->DerivesFrom != NULL
					&& std::find(related_words[word].begin(), related_words[word].end(), related_words[word][n]->DerivesFrom) == related_words[word].end()
				) {
					related_words[word].push_back(related_words[word][n]->DerivesFrom);
				}
				for (size_t o = 0; o < related_words[word][n]->DerivesTo.size(); ++o) {
					if (std::find(related_words[word].begin(), related_words[word].end(), related_words[word][n]->DerivesTo[o]) == related_words[word].end()) {
						related_words[word].push_back(related_words[word][n]->DerivesTo[o]);
					}
				}
			}
					
			//now attach the new type name to the word from its related words, if it is found in them
			for (size_t n = 0; n < types.size(); ++n) {
				if (PlayerRaces.Languages[i]->GetPotentialNameQuantityForType(types[n]) < minimum_desired_names) {
					for (size_t o = 0; o < related_words[word].size(); ++o) {
						if (word != related_words[word][o]) {
							word->AddNameTypeGenerationFromWord(related_words[word][o], types[n]);
						}
					}
				}
			}
		}
	}
			
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		if (!PlayerRaces.Languages[i]->UsedByCivilizationOrFaction || PlayerRaces.Languages[i]->SkipNameTypeInheritance) {
			continue;
		}

		for (size_t j = 0; j < types.size(); ++j) {
			if (PlayerRaces.Languages[i]->GetPotentialNameQuantityForType(types[j]) < minimum_desired_names && default_language != -1) { //if the quantity of names is still too low, try to add name generation of this type for this language based on the default language, for words which share a meaning
				ShowLoadProgress("Deriving \"%s\" name generation patterns by matching word meanings for the %s language", types[j].c_str(), PlayerRaces.Languages[i]->Name.c_str());
				for (size_t k = 0; k < PlayerRaces.Languages[i]->LanguageWords.size(); ++k) {
					for (size_t n = 0; n < PlayerRaces.Languages[default_language]->LanguageWords.size(); ++n) {
						if (PlayerRaces.Languages[default_language]->LanguageWords[n]->Type == PlayerRaces.Languages[i]->LanguageWords[k]->Type) {
							if (PlayerRaces.Languages[default_language]->LanguageWords[n]->Number != -1 && PlayerRaces.Languages[i]->LanguageWords[k]->Number != -1 && PlayerRaces.Languages[default_language]->LanguageWords[n]->Number == PlayerRaces.Languages[i]->LanguageWords[k]->Number) {
								PlayerRaces.Languages[i]->LanguageWords[k]->AddNameTypeGenerationFromWord(PlayerRaces.Languages[default_language]->LanguageWords[n], types[j]);
								continue;
							}
							for (size_t o = 0; o < PlayerRaces.Languages[i]->LanguageWords[k]->Meanings.size(); ++o) {
								if (PlayerRaces.Languages[default_language]->LanguageWords[n]->HasMeaning(PlayerRaces.Languages[i]->LanguageWords[k]->Meanings[o])) {
									PlayerRaces.Languages[i]->LanguageWords[k]->AddNameTypeGenerationFromWord(PlayerRaces.Languages[default_language]->LanguageWords[n], types[j]);
									break;
								}
							}
						}
					}
				}
			}
			
			// for debugging purposes
			// now, as a test, generate 10 names for each name type for the language
			for (int k = 0; k < 10; ++k) {
				std::string generated_name = GenerateName(i, types[j]);
				if (!generated_name.empty()) {
					fprintf(stdout, "Generated name: \"%s\" (\"%s\", %s language).\n", generated_name.c_str(), types[j].c_str(), PlayerRaces.Languages[i]->Name.c_str());
				}
			}
		}
	}
	
	CreateLanguageCache();
	
	//this is for debugging purposes
	int minimum_names = 5;
	int desired_names = 25;
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		if (!PlayerRaces.Languages[i]->UsedByCivilizationOrFaction) {
			continue;
		}

		for (size_t j = 0; j < types.size(); ++j) {
			int final_name_quantity = PlayerRaces.Languages[i]->GetPotentialNameQuantityForType(types[j]);
			if (final_name_quantity > 0 && final_name_quantity < minimum_names) { //if the name quantity is very low, then don't generate that sort of name for the language
				fprintf(stdout, "%s language could only generate %d names out of %d for type \"%s\".\n", PlayerRaces.Languages[i]->Name.c_str(), final_name_quantity, minimum_names, types[j].c_str());
			} else if (final_name_quantity > 0 && final_name_quantity < minimum_desired_names) { //if the name quantity is below the minimum desired amount, note that in the output
				fprintf(stdout, "%s language can only generate %d names out of %d for type \"%s\", below the minimum desired amount.\n", PlayerRaces.Languages[i]->Name.c_str(), final_name_quantity, minimum_desired_names, types[j].c_str());
			} else if (final_name_quantity > 0 && final_name_quantity < desired_names) { //if the name quantity is below the minimum desired amount, note that in the output
				fprintf(stdout, "%s language can only generate %d names out of %d for type \"%s\", below the desired amount.\n", PlayerRaces.Languages[i]->Name.c_str(), final_name_quantity, desired_names, types[j].c_str());
			}
		}
	}
}

void CreateLanguageCache()
{
	struct stat tmp;
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "cache/";
	if (stat(path.c_str(), &tmp) < 0) {
		makedir(path.c_str(), 0777);
	}
	path += "languages";
	path += ".lua";

	FILE *fd = fopen(path.c_str(), "w");
	if (!fd) {
		fprintf(stderr, "Cannot open file %s for writing.\n", path.c_str());
		return;
	}
	
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		CLanguage *language = PlayerRaces.Languages[i];
		fprintf(fd, "DefineLanguage(\"%s\", {\n", language->Ident.c_str());
		fprintf(fd, "\tName = \"%s\",\n", language->Name.c_str());
		if (language->DialectOf) {
			fprintf(fd, "\tDialectOf = \"%s\",\n", language->DialectOf->Ident.c_str());
		}
		
		fprintf(fd, "\tNounEndings = {");
		for (int j = 0; j < MaxGrammaticalNumbers; ++j) {
			for (int k = 0; k < MaxGrammaticalCases; ++k) {
				for (int n = 0; n < MaxWordJunctionTypes; ++n) {
					if (!language->NounEndings[j][k][n].empty()) {
						fprintf(fd, "\"%s\", ", GetGrammaticalNumberNameById(j).c_str());
						fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(k).c_str());
						fprintf(fd, "\"%s\", ", GetWordJunctionTypeNameById(n).c_str());
						fprintf(fd, "\"%s\", ", language->NounEndings[j][k][n].c_str());
					}
				}
			}
		}
		fprintf(fd, "},\n");
		
		fprintf(fd, "\tAdjectiveEndings = {");
		for (int j = 0; j < MaxArticleTypes; ++j) {
			for (int k = 0; k < MaxGrammaticalCases; ++k) {
				for (int n = 0; n < MaxGrammaticalNumbers; ++n) {
					for (int o = 0; o < MaxGrammaticalGenders; ++o) {
						if (!language->AdjectiveEndings[j][k][n][o].empty()) {
							fprintf(fd, "\"%s\", ", GetArticleTypeNameById(j).c_str());
							fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(k).c_str());
							fprintf(fd, "\"%s\", ", GetGrammaticalNumberNameById(n).c_str());
							fprintf(fd, "\"%s\", ", GetGrammaticalGenderNameById(o).c_str());
							fprintf(fd, "\"%s\", ", language->AdjectiveEndings[j][k][n][o].c_str());
						}
					}
				}
			}
		}
		fprintf(fd, "},\n");
		
		fprintf(fd, "\tNameTranslations = {");
		for (std::map<std::string, std::vector<std::string>>::iterator iterator = language->NameTranslations.begin(); iterator != language->NameTranslations.end(); ++iterator) {
			for (size_t j = 0; j < iterator->second.size(); ++j) {
				fprintf(fd, "\n\t\t\"%s\", ", iterator->first.c_str());
				fprintf(fd, "\"%s\", ", iterator->second[j].c_str());
			}
		}
		fprintf(fd, "\n\t},\n");

		
		fprintf(fd, "})\n\n");
	}
	
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		ShowLoadProgress("Creating cache for the %s language", PlayerRaces.Languages[i]->Name.c_str());
		for (size_t j = 0; j < PlayerRaces.Languages[i]->LanguageWords.size(); ++j) {
			LanguageWord *word = PlayerRaces.Languages[i]->LanguageWords[j];
			
			if (!word->Mod.empty()) {
				continue;
			}
		
			fprintf(fd, "DefineLanguageWord(\"%s\", {\n", word->Word.c_str());
			fprintf(fd, "\tLanguage = \"%s\",\n", PlayerRaces.Languages[i]->Ident.c_str());
			if (word->Type != -1) {
				fprintf(fd, "\tType = \"%s\",\n", GetWordTypeNameById(word->Type).c_str());
			}
			if (word->Gender != -1) {
				fprintf(fd, "\tGender = \"%s\",\n", GetGrammaticalGenderNameById(word->Gender).c_str());
			}
			if (word->GrammaticalNumber != -1) {
				fprintf(fd, "\tGrammaticalNumber = \"%s\",\n", GetGrammaticalNumberNameById(word->GrammaticalNumber).c_str());
			}
			if (word->Archaic) {
				fprintf(fd, "\tArchaic = true,\n");
			}

			fprintf(fd, "\tMeanings = {");
			for (size_t k = 0; k < word->Meanings.size(); ++k) {
				fprintf(fd, "\"%s\", ", word->Meanings[k].c_str());
			}
			fprintf(fd, "},\n");
			
			if (word->Type == WordTypeNoun) {
				fprintf(fd, "\tNumberCaseInflections = {");
				for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
					for (int n = 0; n < MaxGrammaticalCases; ++n) {
						if (!word->NumberCaseInflections[k][n].empty()) {
							fprintf(fd, "\"%s\", ", GetGrammaticalNumberNameById(k).c_str());
							fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(n).c_str());
							fprintf(fd, "\"%s\", ", word->NumberCaseInflections[k][n].c_str());
						}
					}
				}
				fprintf(fd, "},\n");
			} else if (word->Type == WordTypeVerb) {
				fprintf(fd, "\tNumberPersonTenseMoodInflections = {");
				for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
					for (int n = 0; n < MaxGrammaticalPersons; ++n) {
						for (int o = 0; o < MaxGrammaticalTenses; ++o) {
							for (int p = 0; p < MaxGrammaticalMoods; ++p) {
								if (!word->NumberPersonTenseMoodInflections[k][n][o][p].empty()) {
									fprintf(fd, "\"%s\", ", GetGrammaticalNumberNameById(k).c_str());
									fprintf(fd, "\"%s\", ", GetGrammaticalPersonNameById(n).c_str());
									fprintf(fd, "\"%s\", ", GetGrammaticalTenseNameById(o).c_str());
									fprintf(fd, "\"%s\", ", GetGrammaticalMoodNameById(p).c_str());
									fprintf(fd, "\"%s\", ", word->NumberPersonTenseMoodInflections[k][n][o][p].c_str());
								}
							}
						}
					}
				}
				fprintf(fd, "},\n");
				
				fprintf(fd, "\tParticiples = {");
				for (int k = 0; k < MaxGrammaticalTenses; ++k) {
					if (!word->Participles[k].empty()) {
						fprintf(fd, "\"%s\", ", GetGrammaticalTenseNameById(k).c_str());
						fprintf(fd, "\"%s\", ", word->Participles[k].c_str());
					}
				}
				fprintf(fd, "},\n");
			} else if (word->Type == WordTypeAdjective) {
				fprintf(fd, "\tComparisonDegreeCaseInflections = {");
				for (int k = 0; k < MaxComparisonDegrees; ++k) {
					for (int n = 0; n < MaxGrammaticalCases; ++n) {
						if (!word->ComparisonDegreeCaseInflections[k][n].empty()) {
							fprintf(fd, "\"%s\", ", GetComparisonDegreeNameById(k).c_str());
							fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(n).c_str());
							fprintf(fd, "\"%s\", ", word->ComparisonDegreeCaseInflections[k][n].c_str());
						}
					}
				}
				fprintf(fd, "},\n");
			}
			
			if (word->Uncountable) {
				fprintf(fd, "\tUncountable = true,\n");
			}
			if (!word->Nominative.empty()) {
				fprintf(fd, "\tNominative = \"%s\",\n", word->Nominative.c_str());
			}
			if (!word->Accusative.empty()) {
				fprintf(fd, "\tAccusative = \"%s\",\n", word->Accusative.c_str());
			}
			if (!word->Dative.empty()) {
				fprintf(fd, "\tDative = \"%s\",\n", word->Dative.c_str());
			}
			if (!word->Genitive.empty()) {
				fprintf(fd, "\tGenitive = \"%s\",\n", word->Genitive.c_str());
			}
			if (word->ArticleType != -1) {
				fprintf(fd, "\tArticleType = \"%s\",\n", GetArticleTypeNameById(word->ArticleType).c_str());
			}
			if (word->Number != -1) {
				fprintf(fd, "\tNumber = %d,\n", word->Number);
			}
			
			fprintf(fd, "\tNameTypes = {");
			for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
				for (int n = 0; n < MaxGrammaticalCases; ++n) {
					for (int o = 0; o < MaxGrammaticalTenses; ++o) {
						for (std::map<std::string, int>::iterator iterator = word->NameTypes[k][n][o].begin(); iterator != word->NameTypes[k][n][o].end(); ++iterator) {
							for (int p = 0; p < iterator->second; ++p) {
								fprintf(fd, "\n\t\t\"%s\", ", GetGrammaticalNumberNameById(k).c_str());
								fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(n).c_str());
								fprintf(fd, "\"%s\", ", GetGrammaticalTenseNameById(o).c_str());
								fprintf(fd, "\"%s\", ", iterator->first.c_str());
							}
						}
					}
				}
			}
			fprintf(fd, "\n\t},\n");
			fprintf(fd, "\tAffixNameTypes = {");
			for (int k = 0; k < MaxWordJunctionTypes; ++k) {
				for (int n = 0; n < MaxAffixTypes; ++n) {
					for (int o = 0; o < MaxGrammaticalNumbers; ++o) {
						for (int p = 0; p < MaxGrammaticalCases; ++p) {
							for (int q = 0; q < MaxGrammaticalTenses; ++q) {
								for (std::map<std::string, int>::iterator iterator = word->AffixNameTypes[k][n][o][p][q].begin(); iterator != word->AffixNameTypes[k][n][o][p][q].end(); ++iterator) {
									for (int r = 0; r < iterator->second; ++r) {
										fprintf(fd, "\n\t\t\"%s\", ", GetWordJunctionTypeNameById(k).c_str());
										fprintf(fd, "\"%s\", ", GetAffixTypeNameById(n).c_str());
										fprintf(fd, "\"%s\", ", GetGrammaticalNumberNameById(o).c_str());
										fprintf(fd, "\"%s\", ", GetGrammaticalCaseNameById(p).c_str());
										fprintf(fd, "\"%s\", ", GetGrammaticalTenseNameById(q).c_str());
										fprintf(fd, "\"%s\", ", iterator->first.c_str());
									}
								}
							}
						}
					}
				}
			}
			fprintf(fd, "\n\t}\n");
			fprintf(fd, "})\n\n");
		}
	}
	
	fclose(fd);
}

void DeleteModWord(std::string language_name, std::string word_name)
{
	int language = PlayerRaces.GetLanguageIndexByIdent(language_name.c_str());
	std::vector<std::string> meanings;
	LanguageWord *word = PlayerRaces.Languages[language]->GetWord(word_name, -1, meanings);
	if (word != NULL && !word->Mod.empty()) {
		PlayerRaces.Languages[language]->RemoveWord(word);
		delete word;
		PlayerRaces.Languages[language]->ModWords.erase(std::remove(PlayerRaces.Languages[language]->ModWords.begin(), PlayerRaces.Languages[language]->ModWords.end(), word), PlayerRaces.Languages[language]->ModWords.end());
	}
}

void CleanLanguageModWords(std::string mod_file)
{
	for (size_t i = 0; i < PlayerRaces.Languages.size(); ++i) {
		int mod_words_size = PlayerRaces.Languages[i]->ModWords.size();
		for (int j = (mod_words_size - 1); j >= 0; --j) {
			if (mod_file == PlayerRaces.Languages[i]->ModWords[j]->Mod) {
				PlayerRaces.Languages[i]->RemoveWord(PlayerRaces.Languages[i]->ModWords[j]);
				delete PlayerRaces.Languages[i]->ModWords[j];
				PlayerRaces.Languages[i]->ModWords.erase(std::remove(PlayerRaces.Languages[i]->ModWords.begin(), PlayerRaces.Languages[i]->ModWords.end(), PlayerRaces.Languages[i]->ModWords[j]), PlayerRaces.Languages[i]->ModWords.end());
			}
		}
	}
}

bool IsNameValidForWord(std::string word_name)
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

//@}
