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
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "map.h"
#include "network.h"
#include "netconnect.h"
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
//Wyrmgus end

/**
**  Which indexes to replace with player color
*/
int PlayerColorIndexStart;
int PlayerColorIndexCount;

//Wyrmgus start
std::map<std::string, int> CivilizationStringToIndex;
std::map<std::string, int> FactionStringToIndex[MAX_RACES];
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
			
			for (size_t j = 0; j < 2; ++j) {
				this->Languages[i]->NameTranslations[j].clear();
			}
		}
	}
	//Wyrmgus end
	for (unsigned int i = 0; i != this->Count; ++i) {
		this->Name[i].clear();
		this->Display[i].clear();
		this->Visible[i] = false;
		//Wyrmgus start
		this->Adjective[i].clear();
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
		//clear deities
		for (size_t j = 0; j < this->Deities[i].size(); ++j) {
			delete this->Deities[i][j];
		}
		this->Deities[i].clear();
		this->CivilizationUIFillers[i].clear();
		//Wyrmgus end
	}
	this->Count = 0;
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

int PlayerRace::GetDeityIndexByName(const int civilization, std::string deity_name) const
{
	for (size_t i = 0; i < this->Deities[civilization].size(); ++i) {
		if (deity_name == this->Deities[civilization][i]->Name) {
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
	
	if (faction != -1) {
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
	
	if (language == -1) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	for (size_t i = 0; i < PlayerRaces.Languages[language]->NameTranslations[0].size(); ++i) {
		std::string name_to_be_translated = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[0][i]);
		if (name_to_be_translated == name) {
			std::string name_translation = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[1][i]);
			new_name = name_translation;
			return new_name;
		}
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	for (size_t i = 0; i < PlayerRaces.Languages[language]->NameTranslations[0].size(); ++i) {
		std::string prefix_to_be_translated = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[0][i]);
		if (prefix_to_be_translated == name.substr(0, prefix_to_be_translated.size())) {
			for (size_t j = 0; j < PlayerRaces.Languages[language]->NameTranslations[0].size(); ++j) {
				std::string suffix_to_be_translated = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[0][j]);
				suffix_to_be_translated[0] = tolower(suffix_to_be_translated[0]);
				if (suffix_to_be_translated == name.substr(prefix_to_be_translated.size(), name.size() - prefix_to_be_translated.size())) {
					std::string prefix_translation = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[1][i]);
					std::string suffix_translation = TransliterateText(PlayerRaces.Languages[language]->NameTranslations[1][j]);
					suffix_translation[0] = tolower(suffix_translation[0]);
					if (prefix_translation.substr(prefix_translation.size() - 2, 2) == "gs" && suffix_translation.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
						prefix_translation = FindAndReplaceStringEnding(prefix_translation, "gs", "g");
					}
					if (prefix_translation.substr(prefix_translation.size() - 1, 1) == "s" && suffix_translation.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
						prefix_translation = FindAndReplaceStringEnding(prefix_translation, "s", "");
					}
					new_name = prefix_translation;
					new_name += suffix_translation;
					return new_name;
				}
			}
		}
	}
	
	// if the name contains a space, try to translate each of its elements separately
	if (name.find(" ") != std::string::npos) {
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
	
	return new_name;
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
	if (this->Race != -1 && !GameRunning) {
		if (!PlayerRaces.CivilizationUpgrades[this->Race].empty()) {
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
				snprintf(buf, sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", old_civilization != -1 ? PlayerRaces.Name[old_civilization].c_str() : "", (old_civilization != -1 && old_faction != -1) ? PlayerRaces.Factions[old_civilization][old_faction]->Name.c_str() : "");
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
		if (!PlayerRaces.Factions[this->Race][this->Faction]->FactionUpgrade.empty()) {
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
	
	int old_language = PlayerRaces.GetFactionLanguage(this->Race, this->Faction);
	int new_language = PlayerRaces.GetFactionLanguage(this->Race, faction);
	
	this->Faction = faction;

	if (this->Index == ThisPlayer->Index) {
		UI.Load();
	}
	
	if (this->Faction == -1) {
		return;
	}
	
	if (!IsNetworkGame()) { //only set the faction's name as the player's name if this is a single player game
		this->SetName(faction_name);
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
		if (new_language != old_language) { //if the language changed, update the names of this player's units
			if (!unit.Character) {
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
					if (this->Index != j && Players[j].Type != PlayerNobody && Players[j].Name == PlayerRaces.Factions[this->Race][faction_id]->Name) {
						faction_used = true;
					}		
				}
				if (
					!faction_used
					&& ((PlayerRaces.Factions[this->Race][faction_id]->Type == "tribe" && !this->HasUpgradeClass("writing")) || ((PlayerRaces.Factions[this->Race][faction_id]->Type == "polity" && this->HasUpgradeClass("writing"))))
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
				if (this->Index != j && Players[j].Type != PlayerNobody && Players[j].Name == PlayerRaces.Factions[this->Race][i]->Name) {
					faction_used = true;
				}		
			}
			if (
				!faction_used
				&& ((PlayerRaces.Factions[this->Race][i]->Type == "tribe" && !this->HasUpgradeClass("writing")) || ((PlayerRaces.Factions[this->Race][i]->Type == "polity" && this->HasUpgradeClass("writing"))))
				&& PlayerRaces.Factions[this->Race][i]->Playable
			) {
				local_factions[faction_count] = i;
				faction_count += 1;
			}
		}
	}
	
	if (faction_count > 0) {
		int chosen_faction = local_factions[SyncRand(faction_count)];
		this->SetFaction(PlayerRaces.Factions[this->Race][chosen_faction]->Name);
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
		if (unit.IsAlive() && unit.Type->BoolFlag[HARVESTER_INDEX].value && unit.Type->ResInfo && !unit.Removed) {
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
		if (p.LostTownHallTimer && !p.Revealed && p.LostTownHallTimer < ((int) GameCycle)) {
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
void CPlayer::Notify(int type, const Vec2i &pos, const char *fmt, ...) const
{
	Assert(Map.Info.IsPointOnMap(pos));
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
	UI.Minimap.AddEvent(pos, color);
	if (this == ThisPlayer) {
		SetMessageEvent(pos, "%s", temp);
	} else {
		SetMessageEvent(pos, "(%s): %s", Name.c_str(), temp);
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
}

void CPlayer::SetDiplomacyAlliedWith(const CPlayer &player)
{
	this->Enemy &= ~(1 << player.Index);
	this->Allied |= 1 << player.Index;
}

void CPlayer::SetDiplomacyEnemyWith(const CPlayer &player)
{
	this->Enemy |= 1 << player.Index;
	this->Allied &= ~(1 << player.Index);
}

void CPlayer::SetDiplomacyCrazyWith(const CPlayer &player)
{
	this->Enemy |= 1 << player.Index;
	this->Allied |= 1 << player.Index;
}

void CPlayer::ShareVisionWith(const CPlayer &player)
{
	this->SharedVision |= (1 << player.Index);
}

void CPlayer::UnshareVisionWith(const CPlayer &player)
{
	this->SharedVision &= ~(1 << player.Index);
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
					} else {
						first_element = false;
					}
					
					effect_element_string += UnitTypes[unit_type_id]->Name;
					effect_element_string += " (";
					
					if (UnitTypes[unit_type_id]->Name != UnitTypes[base_unit_type_id]->Name) {
						effect_element_string += FindAndReplaceString(CapitalizeString(UnitTypes[unit_type_id]->Class), "-", " ");
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
								effect_element_string += "Lose ";
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
							
							std::string variable_name = UnitTypeVar.VariableNameLookup[j];
							variable_name = FindAndReplaceString(variable_name, "BasicDamage", "Damage");
							variable_name = FindAndReplaceString(variable_name, "SightRange", "Sight");
							variable_name = FindAndReplaceString(variable_name, "AttackRange", "Range");
							variable_name = SeparateCapitalizedStringElements(variable_name);
							effect_element_string += variable_name;
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
				
				for (int z = 0; z < NumUpgradeModifiers; ++z) {
					if (UpgradeModifiers[z]->UpgradeId == faction_upgrade_id && !UpgradeModifiers[z]->ConvertTo) {
						for (size_t i = 0; i < UnitTypes.size(); ++i) {
							Assert(UpgradeModifiers[z]->ApplyTo[i] == '?' || UpgradeModifiers[z]->ApplyTo[i] == 'X');

							if (UpgradeModifiers[z]->ApplyTo[i] == 'X') {
								bool changed_stats = false;
								std::string effect_element_string;
								
								if (!first_element) {
									effect_element_string += ", ";
								} else {
									first_element = false;
								}
									
								effect_element_string += UnitTypes[i]->Name;
								effect_element_string += " (";

								bool first_var = true;
								for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
									if (j == PRIORITY_INDEX || j == POINTS_INDEX) {
										continue;
									}
						
									if (UpgradeModifiers[z]->Modifier.Variables[j].Value != 0) {
										if (!first_var) {
											effect_element_string += ", ";
										} else {
											first_var = false;
										}
											
										if (IsBooleanVariable(j) && UpgradeModifiers[z]->Modifier.Variables[j].Value < 0) {
											effect_element_string += "Lose ";
										}
										
										if (!IsBooleanVariable(j)) {
											if (UpgradeModifiers[z]->Modifier.Variables[j].Value > 0) {
												effect_element_string += "+";
											}
											effect_element_string += std::to_string((long long) UpgradeModifiers[z]->Modifier.Variables[j].Value);
											if (IsPercentageVariable(j)) {
												effect_element_string += "%";
											}
											effect_element_string += " ";
										}
											
										std::string variable_name = UnitTypeVar.VariableNameLookup[j];
										variable_name = FindAndReplaceString(variable_name, "BasicDamage", "Damage");
										variable_name = FindAndReplaceString(variable_name, "SightRange", "Sight");
										variable_name = FindAndReplaceString(variable_name, "AttackRange", "Range");
										variable_name = SeparateCapitalizedStringElements(variable_name);
										effect_element_string += variable_name;
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
	int name_count = this->NameTypeWords[type].size();
	
	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		name_count += this->NameTypeAffixes[i][AffixTypePrefix][type].size() * this->NameTypeAffixes[i][AffixTypeSuffix][type].size();
		name_count += this->NameTypeAffixes[i][AffixTypePrefix][type].size() * this->NameTypeAffixes[i][AffixTypeInfix][type].size() * this->NameTypeAffixes[i][AffixTypeSuffix][type].size();
	}
	
	return name_count;
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

void CLanguage::AddNameTranslation(std::string translation_from, std::string translation_to)
{
	for (size_t i = 0; i < this->NameTranslations[0].size(); ++i) {
		if (this->NameTranslations[0][i] == translation_from && this->NameTranslations[1][i] == translation_to) { //if translation is already present, return
			return;
		}
	}
	
	this->NameTranslations[0].push_back(translation_from);
	this->NameTranslations[1].push_back(translation_to);
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
	int minimum_desired_names = 10;

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
			if (iterator->first == "river" || iterator->first == "unit-class-castle" || iterator->first == "unit-class-farm" || iterator->first.find("item-") != std::string::npos || iterator->first == "person" || iterator->first == "person-female" || iterator->first == "family" || iterator->first == "noble-family" || iterator->first == "noble-family-predicate") { //don't do this process for name types which aren't actually used by the game yet, to save performance ("person" is actually used by Kobolds, but no language can inherit language data from their language)
				continue;
			}
			if (std::find(types.begin(), types.end(), iterator->first) == types.end()) {
				types.push_back(iterator->first);
			}
		}
		for (int j = 0; j < MaxWordJunctionTypes; ++j) {
			for (int k = 0; k < MaxAffixTypes; ++k) {
				for (std::map<std::string, std::vector<LanguageWord *>>::iterator iterator = PlayerRaces.Languages[i]->NameTypeAffixes[j][k].begin(); iterator != PlayerRaces.Languages[i]->NameTypeAffixes[j][k].end(); ++iterator) {
					if (iterator->first == "river" || iterator->first == "unit-class-castle" || iterator->first == "unit-class-farm" || iterator->first.find("item-") != std::string::npos || iterator->first == "person" || iterator->first == "person-female" || iterator->first == "family" || iterator->first == "noble-family" || iterator->first == "noble-family-predicate") {
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
		std::map<LanguageWord *, std::vector<LanguageWord *>> new_related_words;

		for (size_t k = 0; k < PlayerRaces.Languages[i]->LanguageWords.size(); ++k) {
			LanguageWord *word = PlayerRaces.Languages[i]->LanguageWords[k];
			if (word->DerivesFrom != NULL || word->DerivesTo.size() > 0) {
				related_words[word].push_back(word);
				new_related_words[word].push_back(word);
			}
		}
			
		ShowLoadProgress("Deriving name generation patterns from related words for the %s language", PlayerRaces.Languages[i]->Name.c_str());
		
		while (related_words.size() > 0) {
				
			std::vector<LanguageWord *> words_to_erase;
				
			for (std::map<LanguageWord *, std::vector<LanguageWord *>>::reverse_iterator iterator = related_words.rbegin(); iterator != related_words.rend(); ++iterator) {
				LanguageWord *word = iterator->first;
					
				bool deeper_related_word_level_may_exist = false;
					
				// fill the vector with all the related words for the current relationship depth level
				for (int n = (int) new_related_words[word].size() - 1; n >= 0; --n) {
					if (word != new_related_words[word][n]) { // add name translations for related words (this will be done in order of relationship, so more distantly related words will be less likely to be used for name translations
						PlayerRaces.Languages[i]->AddNameTranslation(new_related_words[word][n]->Word, word->Word);
						
						// see if there is any inflected form of the word, and if so, add it to the name translations as well
						if (new_related_words[word][n]->Type == WordTypeNoun) {
							for (int o = 0; o < MaxGrammaticalNumbers; ++o) {
								for (int p = 0; p < MaxGrammaticalCases; ++p) {
									for (int q = 0; q < MaxWordJunctionTypes; ++q) {
										if (new_related_words[word][n]->GetNounInflection(o, p, q) != new_related_words[word][n]->Word) {
											std::string translation_from = new_related_words[word][n]->GetNounInflection(o, p, q);
											std::string translation_to;
											if (word->Type == new_related_words[word][n]->Type) {
												translation_to = word->GetNounInflection(o, p, q);
											} else {
												translation_to = word->Word;
											}
											PlayerRaces.Languages[i]->AddNameTranslation(translation_from, translation_to);
										}
									}
								}
							}
						} else if (new_related_words[word][n]->Type == WordTypeVerb) {
							for (int o = 0; o < MaxGrammaticalNumbers; ++o) {
								for (int p = 0; p < MaxGrammaticalPersons; ++p) {
									for (int q = 0; q < MaxGrammaticalTenses; ++q) {
										for (int r = 0; r < MaxGrammaticalMoods; ++r) {
											if (new_related_words[word][n]->GetVerbInflection(o, p, q, r) != new_related_words[word][n]->Word) {
												std::string translation_from = new_related_words[word][n]->GetVerbInflection(o, p, q, r);
												std::string translation_to;
												if (word->Type == new_related_words[word][n]->Type) {
													translation_to = word->GetVerbInflection(o, p, q, r);
												} else {
													translation_to = word->Word;
												}
												PlayerRaces.Languages[i]->AddNameTranslation(translation_from, translation_to);
											}
										}
									}
								}
							}
							
							for (int o = 0; o < MaxGrammaticalTenses; ++o) {
								if (new_related_words[word][n]->GetParticiple(o) != new_related_words[word][n]->Word) {
									std::string translation_from = new_related_words[word][n]->GetParticiple(o);
									std::string translation_to;
									if (word->Type == new_related_words[word][n]->Type) {
										translation_to = word->GetParticiple(o);
									} else {
										translation_to = word->Word;
									}
									PlayerRaces.Languages[i]->AddNameTranslation(translation_from, translation_to);
								}
							}
						} else if (new_related_words[word][n]->Type == WordTypeAdjective) {
							for (int o = 0; o < MaxComparisonDegrees; ++o) {
								for (int p = 0; p < MaxArticleTypes; ++p) {
									for (int q = 0; q < MaxGrammaticalCases; ++q) {
										for (int r = 0; r < MaxGrammaticalNumbers; ++r) {
											for (int s = 0; s < MaxGrammaticalGenders; ++s) {
												if (new_related_words[word][n]->GetAdjectiveInflection(o, p, q, r, s) != new_related_words[word][n]->Word) {
													std::string translation_from = new_related_words[word][n]->GetAdjectiveInflection(o, p, q, r, s);
													std::string translation_to;
													if (word->Type == new_related_words[word][n]->Type) {
														translation_to = word->GetAdjectiveInflection(o, p, q, r, s);
													} else {
														translation_to = word->Word;
													}
													PlayerRaces.Languages[i]->AddNameTranslation(translation_from, translation_to);
												}
											}
										}
									}
								}
							}
						}
					}
					if (
						new_related_words[word][n]->DerivesFrom != NULL
						&& std::find(related_words[word].begin(), related_words[word].end(), new_related_words[word][n]->DerivesFrom) == related_words[word].end()
					) {
						related_words[word].push_back(new_related_words[word][n]->DerivesFrom);
						new_related_words[word].push_back(new_related_words[word][n]->DerivesFrom);
						deeper_related_word_level_may_exist = true;
					}
					for (size_t o = 0; o < new_related_words[word][n]->DerivesTo.size(); ++o) {
						if (std::find(related_words[word].begin(), related_words[word].end(), new_related_words[word][n]->DerivesTo[o]) == related_words[word].end()) {
							related_words[word].push_back(new_related_words[word][n]->DerivesTo[o]);
							new_related_words[word].push_back(new_related_words[word][n]->DerivesTo[o]);
							deeper_related_word_level_may_exist = true;
						}
					}
					new_related_words[word].erase(std::remove(new_related_words[word].begin(), new_related_words[word].end(), new_related_words[word][n]), new_related_words[word].end());
				}
					
				//now attach the new type name to the word from its related words, if it is found in them
				for (size_t n = 0; n < types.size(); ++n) {
					if (PlayerRaces.Languages[i]->GetPotentialNameQuantityForType(types[n]) < minimum_desired_names) {
						for (size_t o = 0; o < new_related_words[word].size(); ++o) {
							if (word != new_related_words[word][o]) {
								word->AddNameTypeGenerationFromWord(new_related_words[word][o], types[n]);
							}
						}
					}
				}
					
				if (!deeper_related_word_level_may_exist) { //if relationship levels have been exhausted, don't search this word anymore
					words_to_erase.push_back(word);
				}
			}
				
			for (size_t i = 0; i < words_to_erase.size(); ++i) {
				related_words.erase(words_to_erase[i]);
				new_related_words.erase(words_to_erase[i]);
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
