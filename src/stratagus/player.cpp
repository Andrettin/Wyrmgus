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
#include "depend.h"
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
#include "upgrade.h"
//Wyrmgus end
#include "ui.h"
#include "video.h"

//Wyrmgus start
#include "../ai/ai_local.h"
//Wyrmgus end

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

std::vector<CColor> HairColorsRGB[HairColorMax];
std::string HairColorNames[HairColorMax];
std::vector<int> ConversibleHairColors;
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

CDeity *PlayerRace::GetDeity(std::string deity_ident) const
{
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (deity_ident == this->Deities[i]->Ident) {
			return this->Deities[i];
		}
	}
	return NULL;
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
	
	if (PlayerRaces.Civilizations[civilization]->ParentCivilization != -1) {
		return GetCivilizationClassUnitType(PlayerRaces.Civilizations[civilization]->ParentCivilization, class_id);
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
	
	if (PlayerRaces.Civilizations[civilization]->ParentCivilization != -1) {
		return GetCivilizationClassUpgrade(PlayerRaces.Civilizations[civilization]->ParentCivilization, class_id);
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
	
	if (PlayerRaces.Civilizations[civilization]->ParentCivilization != -1) {
		return GetCivilizationLanguage(PlayerRaces.Civilizations[civilization]->ParentCivilization);
	}
	
	return -1;
}

std::vector<CFiller> PlayerRace::GetCivilizationUIFillers(int civilization)
{
	if (civilization == -1) {
		return std::vector<CFiller>();
	}
	
	if (CivilizationUIFillers[civilization].size() > 0) {
		return CivilizationUIFillers[civilization];
	}
	
	if (PlayerRaces.Civilizations[civilization]->ParentCivilization != -1) {
		return GetCivilizationUIFillers(PlayerRaces.Civilizations[civilization]->ParentCivilization);
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

std::map<int, std::vector<std::string>> &CCivilization::GetPersonalNames()
{
	if (this->PersonalNames.size() > 0) {
		return this->PersonalNames;
	}
	
	if (this->ParentCivilization != -1) {
		return PlayerRaces.Civilizations[this->ParentCivilization]->GetPersonalNames();
	}
	
	return this->PersonalNames;
}

std::vector<std::string> &CCivilization::GetSettlementNames()
{
	if (this->SettlementNames.size() > 0) {
		return this->SettlementNames;
	}
	
	if (this->ParentCivilization != -1) {
		return PlayerRaces.Civilizations[this->ParentCivilization]->GetSettlementNames();
	}
	
	return this->SettlementNames;
}

std::vector<std::string> &CCivilization::GetShipNames()
{
	if (this->ShipNames.size() > 0) {
		return this->ShipNames;
	}
	
	if (this->ParentCivilization != -1) {
		return PlayerRaces.Civilizations[this->ParentCivilization]->GetShipNames();
	}
	
	return this->ShipNames;
}

CFaction::~CFaction()
{
	if (this->Conditions) {
		delete Conditions;
	}

	this->UIFillers.clear();
}

std::vector<std::string> &CFaction::GetSettlementNames()
{
	if (this->SettlementNames.size() > 0) {
		return this->SettlementNames;
	}
	
	if (this->ParentFaction != -1) {
		return PlayerRaces.Factions[this->Civilization][this->ParentFaction]->GetSettlementNames();
	}
	
	return PlayerRaces.Civilizations[this->Civilization]->GetSettlementNames();
}

std::vector<std::string> &CFaction::GetShipNames()
{
	if (this->ShipNames.size() > 0) {
		return this->ShipNames;
	}
	
	if (this->ParentFaction != -1) {
		return PlayerRaces.Factions[this->Civilization][this->ParentFaction]->GetShipNames();
	}
	
	return PlayerRaces.Civilizations[this->Civilization]->GetShipNames();
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
	if (p.Overlord) {
		file.printf("  \"overlord\", %d,\n", p.Overlord->Index);
	}
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
	
	//Wyrmgus start
	file.printf("},\n  \"prices\", {");
	for (int j = 0; j < MaxCosts; ++j) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[j].c_str(), p.Prices[j]);
	}
	//Wyrmgus end

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
	
	file.printf("\n  \"quest-build-units-of-class\", {");
	for (size_t j = 0; j < p.QuestBuildUnitsOfClass.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", std::get<0>(p.QuestBuildUnitsOfClass[j])->Ident.c_str());
		file.printf("\"%s\",", UnitTypeClasses[std::get<1>(p.QuestBuildUnitsOfClass[j])].c_str());
		file.printf("%d,", std::get<2>(p.QuestBuildUnitsOfClass[j]));
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
			Players[i].SetFaction(faction);
			Players[i].AiEnabled = true;
			Players[i].AiName = faction->DefaultAI;
			Players[i].Team = 1;
			Players[i].Resources[CopperCost] = 2500; // give the new player enough resources to start up
			Players[i].Resources[WoodCost] = 2500;
			Players[i].Resources[StoneCost] = 2500;
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
	this->Religion = NULL;
	this->Overlord = NULL;
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
	this->Deities.clear();
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
	if (this->Race != -1 && (GameRunning || GameEstablishing)) {
		if (!PlayerRaces.CivilizationUpgrades[this->Race].empty() && this->Allow.Upgrades[CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race])->ID);
		}
	}

	int old_civilization = this->Race;
	int old_faction = this->Faction;

	if (GameRunning) {
		this->SetFaction(NULL);
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
	
	if (this->Race != -1) {
		CUpgrade *civilization_upgrade = CUpgrade::Get(PlayerRaces.CivilizationUpgrades[this->Race]);
		if (civilization_upgrade && this->Allow.Upgrades[civilization_upgrade->ID] != 'R') {
			UpgradeAcquire(*this, civilization_upgrade);
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
void CPlayer::SetFaction(CFaction *faction)
{
	if (this->Faction != -1) {
		if (!PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade.empty() && this->Allow.Upgrades[CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade)->ID] == 'R') {
			UpgradeLost(*this, CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade)->ID);
		}

		int faction_type_upgrade_id = UpgradeIdByIdent("upgrade-" + GetFactionTypeNameById(PlayerRaces.Factions[this->Race][this->Faction]->Type));
		if (faction_type_upgrade_id != -1 && this->Allow.Upgrades[faction_type_upgrade_id] == 'R') {
			UpgradeLost(*this, faction_type_upgrade_id);
		}
	}

	int faction_id = PlayerRaces.GetFactionIndexByName(this->Race, faction ? faction->Ident : "");
	
	for (size_t i = 0; i < UpgradeClasses.size(); ++i) {
		if (PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i) != PlayerRaces.GetFactionClassUpgrade(this->Race, faction_id, i)) { //if the upgrade for a certain class is different for the new faction than the old faction (and it has been acquired), remove the modifiers of the old upgrade and apply the modifiers of the new
			if (PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i) != -1 && this->Allow.Upgrades[PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i)] == 'R') {
				UpgradeLost(*this, PlayerRaces.GetFactionClassUpgrade(this->Race, this->Faction, i));

				if (PlayerRaces.GetFactionClassUpgrade(this->Race, faction_id, i) != -1) {
					UpgradeAcquire(*this, AllUpgrades[PlayerRaces.GetFactionClassUpgrade(this->Race, faction_id, i)]);
				}
			}
		}
	}
	
	bool personal_names_changed = true;
	bool ship_names_changed = true;
	bool settlement_names_changed = true;
	if (this->Faction != -1 && faction_id != -1) {
		ship_names_changed = PlayerRaces.Factions[this->Race][this->Faction]->GetShipNames() != PlayerRaces.Factions[this->Race][faction_id]->GetShipNames();
		settlement_names_changed = PlayerRaces.Factions[this->Race][this->Faction]->GetSettlementNames() != PlayerRaces.Factions[this->Race][faction_id]->GetSettlementNames();
		personal_names_changed = false; // setting to a faction of the same civilization
	}
	
	this->Faction = faction_id;

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
		for (size_t i = 0; i < PlayerRaces.Factions[this->Race][faction_id]->Colors.size(); ++i) {
			if (!IsPlayerColorUsed(PlayerRaces.Factions[this->Race][faction_id]->Colors[i])) {
				color = PlayerRaces.Factions[this->Race][faction_id]->Colors[i];
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
			CUpgrade *faction_upgrade = CUpgrade::Get(PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade);
			if (faction_upgrade && this->Allow.Upgrades[faction_upgrade->ID] != 'R') {
				if (GameEstablishing) {
					AllowUpgradeId(*this, faction_upgrade->ID, 'R');
				} else {
					UpgradeAcquire(*this, faction_upgrade);
				}
			}
		}
		
		int faction_type_upgrade_id = UpgradeIdByIdent("upgrade-" + GetFactionTypeNameById(PlayerRaces.Factions[this->Race][this->Faction]->Type));
		if (faction_type_upgrade_id != -1 && this->Allow.Upgrades[faction_type_upgrade_id] != 'R') {
			if (GameEstablishing) {
				AllowUpgradeId(*this, faction_type_upgrade_id, 'R');
			} else {
				UpgradeAcquire(*this, AllUpgrades[faction_type_upgrade_id]);
			}
		}
	} else {
		fprintf(stderr, "Invalid faction \"%s\" tried to be set for player %d of civilization \"%s\".\n", faction->Name.c_str(), this->Index, PlayerRaces.Name[this->Race].c_str());
	}
	
	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);
		if (!unit.Unique && unit.Type->PersonalNames.size() == 0) {
			if (!unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->UnitType == UnitTypeNaval && ship_names_changed) {
				unit.UpdatePersonalName();
			}
		}
		if (settlement_names_changed && unit.Type->BoolFlag[TOWNHALL_INDEX].value && !unit.Unique) {
			unit.UpdateSettlementName();
		}
		if (personal_names_changed && unit.Type->BoolFlag[ORGANIC_INDEX].value && !unit.Character) {
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
	int faction_count = 0;
	int local_factions[FactionMax];
	
	// first search for valid factions in the current faction's "develops to" list, and only if that fails search in all factions of the civilization
	if (this->Faction != -1) {
		for (size_t i = 0; i < PlayerRaces.Factions[this->Race][this->Faction]->DevelopsTo.size(); ++i) {
			int faction_id = PlayerRaces.GetFactionIndexByName(this->Race, PlayerRaces.Factions[this->Race][this->Faction]->DevelopsTo[i]);
			if (faction_id != -1) {
				if (
					this->CanFoundFaction(PlayerRaces.Factions[this->Race][faction_id])
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
			if (
				this->CanFoundFaction(PlayerRaces.Factions[this->Race][i])
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
		this->SetFaction(PlayerRaces.Factions[this->Race][chosen_faction]);
	} else {
		this->SetFaction(NULL);
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

/**
**  Check if the player can found a particular faction.
**
**  @param faction    New faction.
*/
bool CPlayer::CanFoundFaction(CFaction *faction, bool pre)
{
	if (CurrentQuest != NULL || GrandStrategy) {
		return false;
	}
	
	for (int i = 0; i < PlayerMax; ++i) {
		if (this->Index != i && Players[i].Type != PlayerNobody && Players[i].Race == faction->Civilization && Players[i].Faction == faction->ID) {
			// faction is already in use
			return false;
		}
	}
	
	if (!pre) {
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
	Religion = NULL;
	Overlord = NULL;
	Vassals.clear();
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
	this->Deities.clear();
	this->AvailableQuests.clear();
	this->CurrentQuests.clear();
	this->CompletedQuests.clear();
	this->QuestBuildUnits.clear();
	this->QuestBuildUnitsOfClass.clear();
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
	UpgradeTimers.Clear();
	for (int i = 0; i < MaxCosts; ++i) {
		SpeedResourcesHarvest[i] = SPEEDUP_FACTOR;
		SpeedResourcesReturn[i] = SPEEDUP_FACTOR;
		//Wyrmgus start
		Prices[i] = DefaultResourcePrices[i];
		//Wyrmgus end
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
	//Wyrmgus start
	if (this->Units[unit.PlayerSlot] != &unit) {
		fprintf(stderr, "Error in CPlayer::RemoveUnit: the unit's PlayerSlot doesn't match its position in the player's units array; Unit's PlayerSlot: %d, Unit Type: \"%s\".\n", unit.PlayerSlot, unit.Type ? unit.Type->Ident.c_str() : "");
	}
	//Wyrmgus end
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
		if (unit.IsAlive() && unit.Variable[LEVELUP_INDEX].Value >= 1) {
			LevelUpUnits.push_back(&unit);
		}
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
	if (this == ThisPlayer && GameCycle >= CYCLES_PER_MINUTE && this->AvailableQuests.size() > 0 && exausted_available_quests && this->NumTownHalls > 0) {
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
	for (size_t i = 0; i < this->CurrentQuests.size();) {
		std::string failed_quest = this->HasFailedQuest(this->CurrentQuests[i]);
		if (!failed_quest.empty()) {
			this->FailQuest(this->CurrentQuests[i], failed_quest);
		} else if (this->HasCompletedQuest(this->CurrentQuests[i])) {
			this->CompleteQuest(this->CurrentQuests[i]);
		} else {
			++i;
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
	
	for (size_t i = 0; i < quest->BuildUnitsOfClass.size(); ++i) {
		this->QuestBuildUnitsOfClass.push_back(std::tuple<CQuest *, int, int>(quest, std::get<0>(quest->BuildUnitsOfClass[i]), std::get<1>(quest->BuildUnitsOfClass[i])));
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
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Completed\", \"You have completed the " + quest->Name + " quest!\\n\\n" + rewards_string + "\", nil, \"" + quest->Icon.Name + "\", \"" + PlayerColorNames[quest->PlayerColor] + "\", \"" + HairColorNames[quest->HairColor] + "\") end;");
	}
}

void CPlayer::FailQuest(CQuest *quest, std::string fail_reason)
{
	this->CurrentQuests.erase(std::remove(this->CurrentQuests.begin(), this->CurrentQuests.end(), quest), this->CurrentQuests.end());
	
	CclCommand("trigger_player = " + std::to_string((long long) this->Index) + ";");
	
	if (quest->FailEffects) {
		quest->FailEffects->pushPreamble();
		quest->FailEffects->run();
	}
	
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
	
	for (size_t i = 0; i < this->QuestBuildUnitsOfClass.size(); ++i) {
		if (std::get<0>(this->QuestBuildUnitsOfClass[i]) == quest && std::get<2>(this->QuestBuildUnitsOfClass[i]) > 0) {
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
	for (size_t i = 0; i < quest->HeroesMustSurvive.size(); ++i) { // put it here, because "unfailable" quests should also fail when a hero which should survive dies
		if (!this->HasHero(quest->HeroesMustSurvive[i])) {
			return "A hero necessary for the quest has died.";
		}
	}
	
	if (quest->Unfailable) {
		return "";
	}

	if (quest->CurrentCompleted) { // quest already completed by someone else
		return "Another faction has completed the quest before you.";
	}

	for (size_t i = 0; i < this->QuestBuildUnits.size(); ++i) {
		if (std::get<0>(this->QuestBuildUnits[i]) == quest && std::get<2>(this->QuestBuildUnits[i]) > 0) {
			bool has_builder = false;
			CUnitType *type = std::get<1>(this->QuestBuildUnits[i]);
			for (size_t j = 0; j < AiHelpers.Build[type->Slot].size(); ++j) {
				if (this->UnitTypesCount[AiHelpers.Build[type->Slot][j]->Slot] > 0) {
					has_builder = true;
					break;
				}
			}
			if (!has_builder || !CheckDependByType(*this, *type)) {
				return "You can no longer produce the required unit.";
			}
		}
	}
	
	for (size_t i = 0; i < this->QuestBuildUnitsOfClass.size(); ++i) {
		if (std::get<0>(this->QuestBuildUnitsOfClass[i]) == quest && std::get<2>(this->QuestBuildUnitsOfClass[i]) > 0) {
			bool has_builder = false;
			int unit_type_id = PlayerRaces.GetFactionClassUnitType(this->Race, this->Faction, std::get<1>(this->QuestBuildUnitsOfClass[i]));
			
			if (unit_type_id == -1) {
				return "You cannot longer produce the required unit.";
			}
			
			CUnitType *type = UnitTypes[unit_type_id];
			for (size_t j = 0; j < AiHelpers.Build[type->Slot].size(); ++j) {
				if (this->UnitTypesCount[AiHelpers.Build[type->Slot][j]->Slot] > 0) {
					has_builder = true;
					break;
				}
			}
			if (!has_builder || !CheckDependByType(*this, *type)) {
				return "You cannot longer produce the required unit.";
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
//Wyrmgus start
//int CPlayer::CheckUnitType(const CUnitType &type) const
int CPlayer::CheckUnitType(const CUnitType &type, bool hire) const
//Wyrmgus end
{
	//Wyrmgus start
//	return this->CheckCosts(type.Stats[this->Index].Costs);
	int modified_costs[MaxCosts];
	memset(modified_costs, 0, sizeof(modified_costs));
	if (hire) {
		modified_costs[CopperCost] = type.Stats[this->Index].GetPrice();
		if (type.TrainQuantity) {
			modified_costs[CopperCost] *= type.TrainQuantity;
		}
	} else {
		for (int i = 1; i < MaxCosts; ++i) {
			modified_costs[i] = type.Stats[this->Index].Costs[i];
			if (type.TrainQuantity) {
				modified_costs[i] *= type.TrainQuantity;
			}
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
//Wyrmgus start
//void CPlayer::AddUnitType(const CUnitType &type)
void CPlayer::AddUnitType(const CUnitType &type, bool hire)
//Wyrmgus end
{
	//Wyrmgus start
//	AddCosts(type.Stats[this->Index].Costs);
	if (hire) {
		int hire_costs[MaxCosts];
		memset(hire_costs, 0, sizeof(hire_costs));
		hire_costs[CopperCost] = type.Stats[this->Index].GetPrice();
		AddCostsFactor(hire_costs, 100 * (type.TrainQuantity ? type.TrainQuantity : 1));
	} else {
		AddCostsFactor(type.Stats[this->Index].Costs, 100 * (type.TrainQuantity ? type.TrainQuantity : 1));
	}
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
	if (hire) {
		int hire_costs[MaxCosts];
		memset(hire_costs, 0, sizeof(hire_costs));
		hire_costs[CopperCost] = type.Stats[this->Index].GetPrice();
		this->SubCostsFactor(hire_costs, 100 * (type.TrainQuantity ? type.TrainQuantity : 1));
	} else {
		this->SubCostsFactor(type.Stats[this->Index].Costs, 100 * (type.TrainQuantity ? type.TrainQuantity : 1));
	}
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
**  Gives the cost of an upgrade for the player
*/
void CPlayer::GetUpgradeCosts(const CUpgrade *upgrade, int *upgrade_costs)
{
	for (int i = 0; i < MaxCosts; ++i) {
		upgrade_costs[i] = upgrade->Costs[i];
		for (size_t j = 0; j < upgrade->ScaledCostUnits.size(); ++j) {
			upgrade_costs[i] += upgrade->ScaledCosts[i] * this->UnitTypesCount[upgrade->ScaledCostUnits[j]->Slot];
		}
	}
}
//Wyrmgus end

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

	player.UpdateQuestPool(); // every minute, update the quest pool
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

//Wyrmgus start
//void CPlayer::SetDiplomacyEnemyWith(const CPlayer &player)
void CPlayer::SetDiplomacyEnemyWith(CPlayer &player)
//Wyrmgus end
{
	this->Enemy |= 1 << player.Index;
	this->Allied &= ~(1 << player.Index);
	
	//Wyrmgus start
	if (GameCycle > 0 && player.Index == ThisPlayer->Index) {
		ThisPlayer->Notify(_("%s changed their diplomatic stance with us to Enemy"), _(this->Name.c_str()));
	}
	
	// if either player is the overlord of another (indirect or otherwise), break the vassalage bond after the declaration of war
	if (this->IsOverlordOf(player, true)) {
		player.SetOverlord(NULL);
	} else if (player.IsOverlordOf(*this, true)) {
		this->SetOverlord(NULL);
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
	return player.StartMapLayer == this->StartMapLayer || (Map.Worlds[player.StartMapLayer] == Map.Worlds[this->StartMapLayer] && Map.Planes[player.StartMapLayer] == Map.Planes[this->StartMapLayer]);
}

/**
**  Check if the player's faction type is a neutral one
*/
bool CPlayer::HasNeutralFactionType() const
{
	if (
		this->Race != -1
		&& this->Faction != -1
		&& (PlayerRaces.Factions[this->Race][this->Faction]->Type == FactionTypeMercenaryCompany || PlayerRaces.Factions[this->Race][this->Faction]->Type == FactionTypeHolyOrder || PlayerRaces.Factions[this->Race][this->Faction]->Type == FactionTypeTradingCompany)
	) {
		return true;
	}

	return false;
}

/**
**  Check if the player can use the buildings of another, for neutral building functions (i.e. unit training)
*/
bool CPlayer::HasBuildingAccess(const CPlayer &player) const
{
	if (player.Type == PlayerNeutral) {
		return true;
	}
	
	if (
		player.HasNeutralFactionType()
		&& player.Overlord == NULL || this->IsOverlordOf(player, true)
	) {
		if (PlayerRaces.Factions[player.Race][player.Faction]->Type != FactionTypeHolyOrder || std::find(this->Deities.begin(), this->Deities.end(), PlayerRaces.Factions[player.Race][player.Faction]->HolyOrderDeity) != this->Deities.end()) { //if the faction is a holy order, the player must have chosen its respective deity
			return true;
		}
	}

	return false;
}

bool CPlayer::HasHero(const CCharacter *hero) const
{
	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		if (this->Heroes[i]->Character == hero) {
			return true;
		}
	}
	
	return false;
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

int GetPlayerColorIndexByName(std::string player_color_name)
{
	for (int c = 0; c < PlayerColorMax; ++c) {
		if (PlayerColorNames[c] == player_color_name) {
			return c;
		}
	}
	return -1;
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
	} else if (faction_type == FactionTypeTradingCompany) {
		return "trading-company";
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

void CLanguage::RemoveWord(LanguageWord *word)
{
	if (std::find(this->LanguageWords.begin(), this->LanguageWords.end(), word) != this->LanguageWords.end()) {
		this->LanguageWords.erase(std::remove(this->LanguageWords.begin(), this->LanguageWords.end(), word), this->LanguageWords.end());
	}
}

bool LanguageWord::HasMeaning(std::string meaning)
{
	return std::find(this->Meanings.begin(), this->Meanings.end(), meaning) != this->Meanings.end();
}

std::string LanguageWord::GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case)) != this->NumberCaseInflections.end()) {
		return this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case))->second;
	}
	
	return this->Word + PlayerRaces.Languages[this->Language]->GetNounEnding(grammatical_number, grammatical_case, word_junction_type);
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
