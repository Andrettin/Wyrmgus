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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
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

#include "stratagus.h"

#include "player/player.h"

#include "action/action_upgradeto.h"
#include "actions.h"
#include "age.h"
#include "ai.h"
#include "ai/ai_local.h" //for using AiHelpers
#include "character_title.h"
#include "commands.h" //for faction setting
#include "currency.h"
#include "database/defines.h"
#include "dialogue.h"
#include "economy/resource_storage_type.h"
#include "editor.h"
#include "engine_interface.h"
//Wyrmgus start
#include "game/game.h"
//Wyrmgus end
#include "game/difficulty.h"
#include "gender.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "iocompat.h"
//Wyrmgus end
#include "iolib.h"
#include "item/unique_item.h"
#include "language/language.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/landmass.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "network.h"
#include "netconnect.h"
//Wyrmgus start
#include "parameters.h"
//Wyrmgus end
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/civilization_history.h"
#include "player/diplomacy_state.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/faction_history.h"
#include "player/faction_type.h"
#include "player/player_color.h"
#include "player/player_type.h"
#include "player/vassalage_type.h"
#include "quest/campaign.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "quest/quest.h"
//Wyrmgus start
#include "religion/deity.h"
#include "religion/religion.h"
//Wyrmgus end
#include "script.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
//Wyrmgus start
#include "settings.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "text_processor.h"
#include "time/calendar.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/set_util.h"
#include "util/string_util.h"
#include "util/util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"
#include "video/font.h"
#include "video/video.h"

/**
**  @class CPlayer
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
**    for condition checks. The addition of all counts should
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

int NumPlayers; //how many player slots used

CPlayer *CPlayer::ThisPlayer = nullptr;
std::vector<qunique_ptr<CPlayer>> CPlayer::Players; //all players in play

PlayerRace PlayerRaces; //player races

bool NoRescueCheck; //disable rescue check

/**
**  "Translate" (that is, adapt) a proper name from one culture (civilization) to another.
*/
std::string PlayerRace::TranslateName(const std::string &name, const wyrmgus::language *language)
{
	std::string new_name;
	
	if (!language || name.empty()) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	const auto find_iterator = language->NameTranslations.find(name);
	if (find_iterator != language->NameTranslations.end()) {
		return find_iterator->second[SyncRand(find_iterator->second.size())];
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (name.size() > 1) {
		if (name.find(" ") == std::string::npos) {
			for (size_t i = 0; i < name.size(); ++i) {
				std::string name_prefix = name.substr(0, i + 1);
				std::string name_suffix = CapitalizeString(name.substr(i + 1, name.size() - (i + 1)));
			
	//			fprintf(stdout, "Trying to match prefix \"%s\" and suffix \"%s\" for translating name \"%s\" to the \"%s\" language.\n", name_prefix.c_str(), name_suffix.c_str(), name.c_str(), language->Ident.c_str());
			
				const auto prefix_find_iterator = language->NameTranslations.find(name_prefix);
				const auto suffix_find_iterator = language->NameTranslations.find(name_suffix);
				if (prefix_find_iterator != language->NameTranslations.end() && suffix_find_iterator != language->NameTranslations.end()) { // if both a prefix and suffix have been matched
					name_prefix = prefix_find_iterator->second[SyncRand(prefix_find_iterator->second.size())];
					name_suffix = suffix_find_iterator->second[SyncRand(suffix_find_iterator->second.size())];
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
//Wyrmgus end

/**
**  Init players.
*/
void InitPlayers()
{
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->get_type() == player_type::none) {
			CPlayer::Players[p]->set_type(player_type::nobody);
		}
	}
}

/**
**  Clean up players.
*/
void CleanPlayers()
{
	CPlayer::SetThisPlayer(nullptr);
	CPlayer::revealed_player_indexes.clear();
	for (unsigned int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->Clear();
	}
	NumPlayers = 0;
	NoRescueCheck = false;
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

	file.printf("SetThisPlayer(%d)\n\n", CPlayer::GetThisPlayer()->get_index());
}

CPlayer::CPlayer(const int index) : index(index), type(player_type::none)
{
	if (index != PlayerNumNeutral) {
		CPlayer::non_neutral_players.push_back(this);
	}
}

CPlayer::~CPlayer()
{
}

void CPlayer::SetThisPlayer(CPlayer *player)
{
	if (player == CPlayer::GetThisPlayer()) {
		return;
	}

	CPlayer::ThisPlayer = player;

	interface_style *interface_style = nullptr;
	if (player != nullptr) {
		interface_style = player->get_interface_style();
	}

	QMetaObject::invokeMethod(QApplication::instance(), [interface_style] {
		engine_interface::get()->set_current_interface_style(interface_style);
	}, Qt::QueuedConnection);

	emit engine_interface::get()->this_player_changed();
}

CPlayer *CPlayer::GetThisPlayer()
{
	return CPlayer::ThisPlayer;
}

const QColor &CPlayer::get_minimap_color() const
{
	return this->get_player_color()->get_minimap_color();
}

void CPlayer::set_revealed(const bool revealed)
{
	if (revealed == this->is_revealed()) {
		return;
	}

	this->revealed = revealed;

	if (revealed) {
		CPlayer::revealed_player_indexes.push_back(this->get_index());
	} else {
		vector::remove(CPlayer::revealed_player_indexes, this->get_index());
	}
}

void CPlayer::Save(CFile &file) const
{
	const CPlayer &p = *this;
	file.printf("Player(%d,\n", this->get_index());
	//Wyrmgus start
	file.printf(" \"civilization\", \"%s\",", p.get_civilization()->get_identifier().c_str());
	if (p.get_faction() != nullptr) {
		file.printf(" \"faction\", \"%s\",", p.get_faction()->get_identifier().c_str());
	}
	if (p.get_faction_tier() != faction_tier::none) {
		file.printf(" \"faction-tier\", \"%s\",", faction_tier_to_string(this->get_faction_tier()).c_str());
	}
	if (p.get_government_type() != government_type::none) {
		file.printf(" \"government-type\", \"%s\",", government_type_to_string(this->get_government_type()).c_str());
	}
	if (p.get_dynasty() != nullptr) {
		file.printf(" \"dynasty\", \"%s\",", p.get_dynasty()->get_identifier().c_str());
	}
	if (p.age) {
		file.printf(" \"age\", \"%s\",", p.age->get_identifier().c_str());
	}
	if (p.get_player_color() != nullptr) {
		file.printf(" \"player-color\", \"%s\",", p.get_player_color()->get_identifier().c_str());
	}
	//Wyrmgus end
	file.printf("  \"name\", \"%s\",\n", p.get_name().c_str());
	file.printf("  \"type\", \"%s\",", player_type_to_string(p.get_type()).c_str());
	//Wyrmgus start
//	file.printf(" \"race\", \"%s\",", PlayerRaces.Name[p.Race].c_str());
	//Wyrmgus end
	file.printf(" \"ai-name\", \"%s\",\n", p.AiName.c_str());
	file.printf("  \"team\", %d,", p.Team);

	file.printf(" \"enemy\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", p.enemies.contains(j) ? 'X' : '_');
	}
	file.printf("\", \"allied\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", p.allies.contains(j) ? 'X' : '_');
	}
	file.printf("\", \"shared-vision\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", p.shared_vision.contains(j) ? 'X' : '_');
	}
	file.printf("\", \"mutual-shared-vision\", \"");
	for (int j = 0; j < PlayerMax; ++j) {
		file.printf("%c", p.mutual_shared_vision.contains(j) ? 'X' : '_');
	}
	file.printf("\",\n  \"start\", {%d, %d},\n", p.StartPos.x, p.StartPos.y);
	//Wyrmgus start
	file.printf("  \"start-map-layer\", %d,\n", p.StartMapLayer);
	//Wyrmgus end
	if (p.get_overlord() != nullptr) {
		file.printf("  \"overlord\", %d, \"%s\",\n", p.get_overlord()->get_index(), vassalage_type_to_string(p.vassalage_type).c_str());
	}

	// Resources
	file.printf("  \"resources\", {");
	for (const auto &[resource, quantity] : p.resources) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	// Stored Resources
	file.printf("},\n  \"stored-resources\", {");
	for (const auto &[resource, quantity] : p.stored_resources) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	// Max Resources
	file.printf("},\n  \"max-resources\", {");
	for (const auto &[resource, quantity] : p.max_resources) {
		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	// Last Resources
	file.printf("},\n  \"last-resources\", {");
	for (const auto &[resource, quantity] : p.last_resources) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	// Incomes
	file.printf("},\n  \"incomes\", {");
	bool first = true;
	for (const auto &[resource, quantity] : p.incomes) {
		if (first) {
			first = false;
		} else {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", resource->get_identifier().c_str(), quantity);
	}

	// Revenue
	file.printf("},\n  \"revenue\", {");
	for (const auto &[resource, quantity] : p.revenues) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	file.printf("},\n  \"prices\", {");
	for (const auto &[resource, quantity] : p.prices) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}

	// UnitTypesCount done by load units.

	file.printf("},\n  \"%s\",\n", p.AiEnabled ? "ai-enabled" : "ai-disabled");

	// Ai done by load ais.
	// Units done by load units.
	// TotalNumUnits done by load units.
	// NumBuildings done by load units.
	
	if (p.is_revealed()) {
		file.printf(" \"revealed\",");
	}
	
	file.printf(" \"supply\", %d,", p.Supply);
	file.printf(" \"trade-cost\", %d,", p.TradeCost);
	file.printf(" \"unit-limit\", %d,", p.UnitLimit);
	file.printf(" \"building-limit\", %d,", p.BuildingLimit);
	file.printf(" \"total-unit-limit\", %d,", p.TotalUnitLimit);

	file.printf("\n  \"score\", %d,", p.Score);
	file.printf("\n  \"total-units\", %d,", p.TotalUnits);
	file.printf("\n  \"total-buildings\", %d,", p.TotalBuildings);
	file.printf("\n  \"total-resources\", {");
	for (const auto &[resource, quantity] : p.resource_totals) {
		if (quantity == 0) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}
	file.printf("},");
	file.printf("\n  \"total-razings\", %d,", p.TotalRazings);
	file.printf("\n  \"total-kills\", %d,", p.TotalKills);
	//Wyrmgus start
	file.printf("\n  \"unit-type-kills\", {");
	for (const unit_type *unit_type : unit_type::get_all()) {
		if (p.UnitTypeKills[unit_type->Slot] != 0) {
			file.printf("\"%s\", %d, ", unit_type->get_identifier().c_str(), p.UnitTypeKills[unit_type->Slot]);
		}
	}
	file.printf("},");
	//Wyrmgus end
	if (p.LostTownHallTimer != 0) {
		file.printf("\n  \"lost-town-hall-timer\", %d,", p.LostTownHallTimer);
	}
	if (p.HeroCooldownTimer != 0) {
		file.printf("\n  \"hero-cooldown-timer\", %d,", p.HeroCooldownTimer);
	}
	//Wyrmgus end

	file.printf("\n  \"speed-resource-harvest\", {");
	for (const auto &[resource, speed] : p.resource_harvest_speeds) {
		if (speed == CPlayer::base_speed_factor) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), speed);
	}
	file.printf("},");
	file.printf("\n  \"speed-resource-return\", {");
	for (const auto &[resource, speed] : p.resource_return_speeds) {
		if (speed == CPlayer::base_speed_factor) {
			continue;
		}

		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), speed);
	}
	file.printf("},");
	file.printf("\n  \"speed-build\", %d,", p.SpeedBuild);
	file.printf("\n  \"speed-train\", %d,", p.SpeedTrain);
	file.printf("\n  \"speed-upgrade\", %d,", p.SpeedUpgrade);
	file.printf("\n  \"speed-research\", %d,", p.SpeedResearch);
	
	//Wyrmgus start
	file.printf("\n  \"current-quests\", {");
	for (size_t j = 0; j < p.current_quests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", p.current_quests[j]->get_identifier().c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"completed-quests\", {");
	for (size_t j = 0; j < p.completed_quests.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", p.completed_quests[j]->get_identifier().c_str());
	}
	file.printf("},");
	
	file.printf("\n  \"quest-objectives\", {");
	for (size_t j = 0; j < p.get_quest_objectives().size(); ++j) {
		const auto &objective = p.get_quest_objectives()[j];
		if (j != 0) {
			file.printf(" ");
		}
		file.printf("{");
		file.printf("\"quest\", \"%s\",", objective->get_quest_objective()->get_quest()->get_identifier().c_str());
		file.printf("\"objective-index\", %d,", objective->get_quest_objective()->get_index());
		file.printf("\"counter\", %d,", objective->get_counter());
		file.printf("},");
	}
	file.printf("},");
	
	file.printf("\n  \"modifiers\", {");
	for (size_t j = 0; j < p.Modifiers.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", p.Modifiers[j].first->get_identifier().c_str(), p.Modifiers[j].second);
	}
	file.printf("},");
	//Wyrmgus end

	file.printf("\n  \"autosell-resources\", {");
	for (size_t j = 0; j < p.AutosellResources.size(); ++j) {
		if (j) {
			file.printf(" ");
		}
		file.printf("\"%s\",", DefaultResourceNames[p.AutosellResources[j]].c_str());
	}
	file.printf("},");
	
	// Allow saved by allow.

	file.printf("\n  \"timers\", {");
	//Wyrmgus start
	first = true;
	//Wyrmgus end
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		//Wyrmgus start
//		if (upgrade->ID) {
//			file.printf(" ,");
//		}
//		file.printf("%d", p.UpgradeTimers.Upgrades[upgrade->ID]);
		if (p.UpgradeTimers.Upgrades[upgrade->ID]) {
			if (first) {
				first = false;
			} else {
				file.printf(", ");
			}
			file.printf("\"%s\", %d", upgrade->get_identifier().c_str(), p.UpgradeTimers.Upgrades[upgrade->ID]);
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
void CreatePlayer(const player_type type)
{
	if (NumPlayers == PlayerMax) {
		return;
	}

	const qunique_ptr<CPlayer> &player = CPlayer::Players[NumPlayers];

	player->Init(type);
}

CPlayer *GetFactionPlayer(const wyrmgus::faction *faction)
{
	if (faction == nullptr) {
		return nullptr;
	}
	
	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		if (player->get_faction() == faction) {
			return player.get();
		}
	}
	
	return nullptr;
}

CPlayer *GetOrAddFactionPlayer(const wyrmgus::faction *faction)
{
	CPlayer *faction_player = GetFactionPlayer(faction);
	if (faction_player != nullptr) {
		return faction_player;
	}
	
	// no player belonging to this faction, so let's make an unused player slot be created for it
	
	for (int i = 0; i < NumPlayers; ++i) {
		const qunique_ptr<CPlayer> &player = CPlayer::Players[i];
		if (player->get_type() == player_type::nobody) {
			player->set_type(player_type::computer);
			player->set_civilization(faction->get_civilization());
			player->SetFaction(faction);
			player->AiEnabled = true;
			player->AiName = faction->DefaultAI;
			player->Team = 1;
			player->set_resource(defines::get()->get_wealth_resource(), 2500); // give the new player enough resources to start up
			player->set_resource(resource::get_all()[WoodCost], 2500);
			player->set_resource(resource::get_all()[StoneCost], 2500);

			return player.get();
		}
	}
	
	throw std::runtime_error("Cannot add player for faction \"" + faction->get_identifier() + "\": no player slots available.");
}

void CPlayer::Init(player_type type)
{
	std::vector<CUnit *>().swap(this->Units);
	std::vector<CUnit *>().swap(this->FreeWorkers);
	std::vector<CUnit *>().swap(this->LevelUpUnits);

	//  Take first slot for person on this computer,
	//  fill other with computer players.
	if (type == player_type::person && !NetPlayers) {
		if (!CPlayer::GetThisPlayer()) {
			CPlayer::SetThisPlayer(this);
		} else {
			type = player_type::computer;
		}
	}
	if (NetPlayers && NumPlayers == NetLocalPlayerNumber) {
		CPlayer::SetThisPlayer(CPlayer::Players[NetLocalPlayerNumber].get());
	}

	if (NumPlayers == PlayerMax) {
		static bool already_warned = false;

		if (!already_warned) {
			DebugPrint("Too many players\n");
			already_warned = true;
		}
		return;
	}

	//  Make simple teams:
	//  All person players are enemies.
	int team;
	switch (type) {
		case player_type::neutral:
		case player_type::nobody:
		default:
			team = 0;
			this->set_name("Neutral");
			break;
		case player_type::computer:
			team = 1;
			this->set_name("Computer");
			break;
		case player_type::person:
			team = 2 + NumPlayers;
			this->set_name("Person");
			break;
		case player_type::rescue_passive:
		case player_type::rescue_active:
			// FIXME: correct for multiplayer games?
			this->set_name("Computer");
			team = 2 + NumPlayers;
			break;
	}
	DebugPrint("CreatePlayer name %s\n" _C_ this->get_name().c_str());

	this->set_type(type);
	this->Race = wyrmgus::defines::get()->get_neutral_civilization()->ID;
	this->faction = nullptr;
	this->faction_tier = wyrmgus::faction_tier::none;
	this->government_type = wyrmgus::government_type::none;
	this->religion = nullptr;
	this->dynasty = nullptr;
	this->age = nullptr;
	this->overlord = nullptr;
	this->vassalage_type = wyrmgus::vassalage_type::none;
	this->Team = team;
	this->enemies.clear();
	this->allies.clear();
	this->AiName = "ai-passive";

	//  Calculate enemy/allied mask.
	for (int i = 0; i < NumPlayers; ++i) {
		const qunique_ptr<CPlayer> &other_player = CPlayer::Players[i];
		switch (type) {
			case player_type::neutral:
			case player_type::nobody:
			default:
				break;
			case player_type::computer:
				// Computer allied with computer and enemy of all persons.
				// make computer players be hostile to each other by default
				if (other_player->get_type() == player_type::computer || other_player->get_type() == player_type::person || other_player->get_type() == player_type::rescue_active) {
					this->enemies.insert(i);
					other_player->enemies.insert(NumPlayers);
				}
				break;
			case player_type::person:
				// Humans are enemy of all?
				if (other_player->get_type() == player_type::computer || other_player->get_type() == player_type::person) {
					this->enemies.insert(i);
					other_player->enemies.insert(NumPlayers);
				} else if (other_player->get_type() == player_type::rescue_active || other_player->get_type() == player_type::rescue_passive) {
					this->allies.insert(i);
					other_player->allies.insert(NumPlayers);
				}
				break;
			case player_type::rescue_passive:
				// Rescue passive are allied with persons
				if (other_player->get_type() == player_type::person) {
					this->allies.insert(i);
					other_player->allies.insert(NumPlayers);
				}
				break;
			case player_type::rescue_active:
				// Rescue active are allied with persons and enemies of computer
				if (other_player->get_type() == player_type::computer) {
					this->enemies.insert(i);
					other_player->enemies.insert(NumPlayers);
				} else if (other_player->get_type() == player_type::person) {
					this->allies.insert(i);
					other_player->allies.insert(NumPlayers);
				}
				break;
		}
	}

	//initial default incomes.
	for (const resource *resource : resource::get_all()) {
		this->set_income(resource, resource->get_default_income());
	}
	
	this->TradeCost = DefaultTradeCost;

	//initial max resource amounts.
	for (const resource *resource : resource::get_all()) {
		this->max_resources[resource] = resource->DefaultMaxAmount;
	}

	//Wyrmgus start
	this->UnitTypesCount.clear();
	this->UnitTypesUnderConstructionCount.clear();
	this->UnitTypesAiActiveCount.clear();
	this->Heroes.clear();
	this->Deities.clear();
	this->units_by_type.clear();
	this->units_by_class.clear();
	this->AiActiveUnitsByType.clear();
	this->last_created_unit = nullptr;
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

	if (CPlayer::Players[NumPlayers]->get_type() == player_type::computer || CPlayer::Players[NumPlayers]->get_type() == player_type::rescue_active) {
		this->AiEnabled = true;
	} else {
		this->AiEnabled = false;
	}
	this->revealed = false;
	++NumPlayers;

	emit diplomatic_stances_changed();
}

void CPlayer::apply_history(const CDate &start_date)
{
	const wyrmgus::civilization *civilization = this->get_civilization();

	this->apply_civilization_history(civilization);

	const wyrmgus::faction *faction = this->get_faction();
	const wyrmgus::faction_history *faction_history = faction->get_history();

	this->set_faction_tier(faction_history->get_tier());
	this->set_government_type(faction_history->get_government_type());
	this->set_dynasty(faction_history->get_dynasty());

	for (const wyrmgus::upgrade_class *upgrade_class : faction_history->get_acquired_upgrade_classes()) {
		const CUpgrade *upgrade = faction->get_class_upgrade(upgrade_class);

		if (upgrade == nullptr) {
			continue;
		}

		if (UpgradeIdAllowed(*this, upgrade->ID) != 'R') {
			UpgradeAcquire(*this, upgrade);
		}
	}

	for (const CUpgrade *upgrade : faction_history->get_acquired_upgrades()) {
		if (UpgradeIdAllowed(*this, upgrade->ID) != 'R') {
			UpgradeAcquire(*this, upgrade);
		}
	}

	for (const auto &kv_pair : faction->HistoricalDiplomacyStates) { //set the appropriate historical diplomacy states to other factions
		if (kv_pair.first.first.Year == 0 || start_date.ContainsDate(kv_pair.first.first)) {
			CPlayer *diplomacy_state_player = GetFactionPlayer(kv_pair.first.second);
			if (diplomacy_state_player) {
				CommandDiplomacy(this->get_index(), kv_pair.second, diplomacy_state_player->get_index());
				CommandDiplomacy(diplomacy_state_player->get_index(), kv_pair.second, this->get_index());
				if (kv_pair.second == wyrmgus::diplomacy_state::allied) {
					CommandSharedVision(this->get_index(), true, diplomacy_state_player->get_index());
					CommandSharedVision(diplomacy_state_player->get_index(), true, this->get_index());
				}
			}
		}
	}

	for (const auto &kv_pair : faction_history->get_diplomacy_states()) {
		const wyrmgus::faction *other_faction = kv_pair.first;
		const wyrmgus::diplomacy_state state = kv_pair.second;

		CPlayer *diplomacy_state_player = GetFactionPlayer(other_faction);
		if (diplomacy_state_player != nullptr) {
			switch (state) {
				case wyrmgus::diplomacy_state::overlord:
				case wyrmgus::diplomacy_state::personal_union_overlord:
				case wyrmgus::diplomacy_state::vassal:
				case wyrmgus::diplomacy_state::personal_union_vassal:
					CommandDiplomacy(this->get_index(), state, diplomacy_state_player->get_index());
					break;
				case wyrmgus::diplomacy_state::allied:
					CommandSharedVision(this->get_index(), true, diplomacy_state_player->get_index());
					CommandSharedVision(diplomacy_state_player->get_index(), true, this->get_index());
					//fallthrough
				default:
					CommandDiplomacy(this->get_index(), state, diplomacy_state_player->get_index());
					CommandDiplomacy(diplomacy_state_player->get_index(), state, this->get_index());
					break;
			}
		}
	}

	for (const auto &kv_pair : faction->HistoricalResources) { //set the appropriate historical resource quantities
		if (kv_pair.first.first.Year == 0 || start_date.ContainsDate(kv_pair.first.first)) {
			this->set_resource(wyrmgus::resource::get_all()[kv_pair.first.second], kv_pair.second, resource_storage_type::overall);
		}
	}

	for (const auto &kv_pair : faction_history->get_resources()) {
		const wyrmgus::resource *resource = kv_pair.first;
		const int quantity = kv_pair.second;
		this->set_resource(resource, quantity, resource_storage_type::overall);
	}

	for (const site *settlement : faction_history->get_explored_settlements()) {
		this->add_settlement_to_explored_territory(settlement);
	}
}

void CPlayer::apply_civilization_history(const wyrmgus::civilization_base *civilization)
{
	if (civilization->get_group() != nullptr) {
		this->apply_civilization_history(civilization->get_group());
	}

	const wyrmgus::civilization_history *civilization_history = civilization->get_history();

	const wyrmgus::faction *faction = this->get_faction();

	for (const wyrmgus::upgrade_class *upgrade_class : civilization_history->get_acquired_upgrade_classes()) {
		const CUpgrade *upgrade = faction->get_class_upgrade(upgrade_class);

		if (upgrade == nullptr) {
			continue;
		}

		if (UpgradeIdAllowed(*this, upgrade->ID) != 'R') {
			UpgradeAcquire(*this, upgrade);
		}
	}

	for (const CUpgrade *upgrade : civilization_history->get_acquired_upgrades()) {
		if (UpgradeIdAllowed(*this, upgrade->ID) != 'R') {
			UpgradeAcquire(*this, upgrade);
		}
	}

	for (const site *settlement : civilization_history->get_explored_settlements()) {
		this->add_settlement_to_explored_territory(settlement);
	}
}

void CPlayer::add_settlement_to_explored_territory(const site *settlement)
{
	if (!settlement->is_settlement()) {
		return;
	}

	const site_game_data *settlement_data = settlement->get_game_data();

	if (!settlement_data->is_on_map()) {
		return;
	}

	const CUnit *settlement_unit = settlement_data->get_site_unit();
	const CMapLayer *map_layer = settlement_unit->MapLayer;
	const int z = map_layer->ID;
	const int player_index = this->get_index();

	std::vector<CUnit *> units;

	for (int x = settlement_data->get_territory_rect().x(); x <= settlement_data->get_territory_rect().right(); ++x) {
		for (int y = settlement_data->get_territory_rect().y(); y <= settlement_data->get_territory_rect().bottom(); ++y) {
			const QPoint tile_pos(x, y);
			tile *tile = map_layer->Field(tile_pos);

			if (tile->get_settlement() != settlement) {
				continue;
			}

			const std::unique_ptr<tile_player_info> &tile_player_info = tile->player_info;

			if (tile_player_info->get_visibility_state(player_index) == 0) {
				tile_player_info->get_visibility_state_ref(player_index) = 1;

				if (this == CPlayer::GetThisPlayer()) {
					if (GameRunning) {
						UI.get_minimap()->update_exploration_xy(tile_pos, z);
					}
				}
			}

			CMap::get()->MarkSeenTile(*tile);

			const CUnitCache &unit_cache = tile->UnitCache;

			for (CUnit *unit : unit_cache) {
				if (!unit->Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
					continue;
				}

				if (unit->CacheLock == 0) {
					unit->CacheLock = 1;
					units.push_back(unit);
				}
			}
		}
	}

	for (CUnit *unit : units) {
		unit->CacheLock = 0;

		if (!unit->is_seen_by_player(player_index)) {
			UnitGoesOutOfFog(*unit, *this);
			UnitGoesUnderFog(*unit, *this);
		}
		UnitCountSeen(*unit);
	}
}

bool CPlayer::is_active() const
{
	return this->get_type() != player_type::nobody;
}

bool CPlayer::is_neutral_player() const
{
	return this->get_index() == PlayerNumNeutral;
}

void CPlayer::set_name(const std::string &name)
{
	if (name == this->get_name()) {
		return;
	}

	std::unique_lock<std::shared_mutex> lock(this->mutex);

	this->name = name;

	emit name_changed();
}

const wyrmgus::civilization *CPlayer::get_civilization() const
{
	if (this->Race != -1) {
		return wyrmgus::civilization::get_all()[this->Race];
	}

	return nullptr;
}

//Wyrmgus start
void CPlayer::set_civilization(const wyrmgus::civilization *civilization)
{
	if (this->get_civilization() != nullptr && (GameRunning || GameEstablishing)) {
		const wyrmgus::civilization *old_civilization = this->get_civilization();
		if (old_civilization->get_upgrade() != nullptr && this->Allow.Upgrades[old_civilization->get_upgrade()->ID] == 'R') {
			UpgradeLost(*this, old_civilization->get_upgrade()->ID);
		}
	}

	if (GameRunning) {
		this->SetFaction(nullptr);
	} else {
		this->faction = nullptr;
	}

	this->Race = civilization->ID;

	if (this->get_civilization() != nullptr) {
		//if the civilization of the person player changed, update the UI
		if ((CPlayer::GetThisPlayer() && CPlayer::GetThisPlayer() == this) || (!CPlayer::GetThisPlayer() && this->get_index() == 0)) {
			//load proper UI
			std::array<char, 256> buf{};
			snprintf(buf.data(), sizeof(buf), "if (LoadCivilizationUI ~= nil) then LoadCivilizationUI(\"%s\") end;", this->get_civilization()->get_identifier().c_str());
			CclCommand(buf.data());

			UI.Load();
		}

		const wyrmgus::civilization *new_civilization = this->get_civilization();
		CUpgrade *civilization_upgrade = new_civilization->get_upgrade();
		if (civilization_upgrade != nullptr && this->Allow.Upgrades[civilization_upgrade->ID] != 'R') {
			UpgradeAcquire(*this, civilization_upgrade);
		}
	}

	if (this == CPlayer::GetThisPlayer()) {
		//update the current interface style if it changed
		QMetaObject::invokeMethod(QApplication::instance(), [interface_style = this->get_interface_style()] {
			engine_interface::get()->set_current_interface_style(interface_style);
		}, Qt::QueuedConnection);
	}
}

void CPlayer::SetFaction(const wyrmgus::faction *faction)
{
	if (faction == this->get_faction()) {
		return;
	}

	const wyrmgus::faction *old_faction = this->get_faction();

	if (faction != nullptr && faction->get_civilization() != this->get_civilization()) {
		this->set_civilization(faction->get_civilization());
	}

	if (this->get_faction() != nullptr) {
		if (this->get_faction()->get_upgrade() != nullptr && this->Allow.Upgrades[this->get_faction()->get_upgrade()->ID] == 'R') {
			UpgradeLost(*this, this->get_faction()->get_upgrade()->ID);
		}

		const CUpgrade *faction_type_upgrade = wyrmgus::defines::get()->get_faction_type_upgrade(this->get_faction()->get_type());
		if (faction_type_upgrade != nullptr && this->Allow.Upgrades[faction_type_upgrade->ID] == 'R') {
			UpgradeLost(*this, faction_type_upgrade->ID);
		}
	}

	if (old_faction != nullptr && faction != nullptr) {
		for (const wyrmgus::upgrade_class *upgrade_class : wyrmgus::upgrade_class::get_all()) {
			const CUpgrade *old_faction_class_upgrade = old_faction->get_class_upgrade(upgrade_class);
			const CUpgrade *new_faction_class_upgrade = faction->get_class_upgrade(upgrade_class);
			if (old_faction_class_upgrade != new_faction_class_upgrade) { //if the upgrade for a certain class is different for the new faction than the old faction (and it has been acquired), remove the modifiers of the old upgrade and apply the modifiers of the new
				if (old_faction_class_upgrade != nullptr && this->Allow.Upgrades[old_faction_class_upgrade->ID] == 'R') {
					UpgradeLost(*this, old_faction_class_upgrade->ID);

					if (new_faction_class_upgrade != nullptr) {
						UpgradeAcquire(*this, new_faction_class_upgrade);
					}
				}
			}
		}
	}
	
	bool personal_names_changed = true;
	if (this->get_faction() != nullptr && faction != nullptr) {
		//setting to a faction of the same civilization (if the civilization were different, then the set_civilization() call above would have led to the current faction becoming null)
		personal_names_changed = false;
	}
	
	this->faction = faction;

	if (this == CPlayer::GetThisPlayer()) {
		UI.Load();
	}
	
	if (this->get_faction() == nullptr) {
		return;
	}
	
	if (!IsNetworkGame()) { //only set the faction's name as the player's name if this is a single player game
		this->set_name(this->get_faction()->get_name());
	}
	if (this->get_faction() != nullptr) {
		const wyrmgus::player_color *player_color = nullptr;

		this->set_faction_tier(faction->get_default_tier());
		this->set_government_type(faction->get_default_government_type());

		const wyrmgus::player_color *faction_color = faction->get_color();
		if (faction_color != nullptr) {
			if (this->get_player_color_usage_count(faction_color) == 0) {
				player_color = faction_color;
			}
		}

		if (player_color == nullptr) {
			//if all of the faction's colors are used, get one of the least used player colors
			//out of those colors, give priority to the one closest (in RGB values) to the faction's color
			int best_usage_count = -1;
			int best_rgb_difference = -1;
			std::vector<const wyrmgus::player_color *> available_colors;
			for (const wyrmgus::player_color *pc : wyrmgus::player_color::get_all()) {
				if (pc == wyrmgus::defines::get()->get_neutral_player_color()) {
					continue;
				}

				if (pc->is_hidden()) {
					continue;
				}

				const int usage_count = this->get_player_color_usage_count(pc);
				if (best_usage_count != -1 && usage_count > best_usage_count) {
					continue;
				}

				if (usage_count < best_usage_count) {
					available_colors.clear();
					best_rgb_difference = -1;
				}

				int rgb_difference = -1;

				if (faction_color != nullptr) {
					for (size_t i = 0; i < faction_color->get_colors().size(); ++i) {
						const QColor &faction_color_shade = faction_color->get_colors()[i];
						const QColor &pc_color_shade = pc->get_colors()[i];
						rgb_difference += std::abs(faction_color_shade.red() - pc_color_shade.red());
						rgb_difference += std::abs(faction_color_shade.green() - pc_color_shade.green());
						rgb_difference += std::abs(faction_color_shade.blue() - pc_color_shade.blue());
					}
				}

				if (best_rgb_difference != -1 && rgb_difference > best_rgb_difference) {
					continue;
				}

				if (rgb_difference < best_rgb_difference) {
					available_colors.clear();
				}

				available_colors.push_back(pc);
				best_usage_count = usage_count;
				best_rgb_difference = rgb_difference;
			}

			if (!available_colors.empty()) {
				player_color = available_colors[SyncRand(available_colors.size())];
			}
		}
		
		if (player_color == nullptr) {
			throw std::runtime_error("No player color chosen for player \"" + this->get_name() + "\" (" + std::to_string(this->get_index()) + ").");
		}

		this->player_color = player_color;

		if (!CEditor::get()->is_running()) {
			//update the territory on the minimap for the new color
			this->update_territory_tiles();
		}

		if (this->get_faction()->get_upgrade() != nullptr) {
			const CUpgrade *faction_upgrade = this->get_faction()->get_upgrade();
			if (faction_upgrade && this->Allow.Upgrades[faction_upgrade->ID] != 'R') {
				if (GameEstablishing) {
					AllowUpgradeId(*this, faction_upgrade->ID, 'R');
				} else {
					UpgradeAcquire(*this, faction_upgrade);
				}
			}
		}
		
		const CUpgrade *faction_type_upgrade = wyrmgus::defines::get()->get_faction_type_upgrade(this->get_faction()->get_type());
		if (faction_type_upgrade != nullptr && this->Allow.Upgrades[faction_type_upgrade->ID] != 'R') {
			if (GameEstablishing) {
				AllowUpgradeId(*this, faction_type_upgrade->ID, 'R');
			} else {
				UpgradeAcquire(*this, faction_type_upgrade);
			}
		}
	} else {
		fprintf(stderr, "Invalid faction \"%s\" tried to be set for player %d of civilization \"%s\".\n", faction->get_name().c_str(), this->get_index(), this->get_civilization()->get_identifier().c_str());
	}
	
	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);

		if (unit.Type->BoolFlag[ORGANIC_INDEX].value) {
			if (personal_names_changed && unit.get_character() == nullptr && unit.Type == faction->get_class_unit_type(unit.Type->get_unit_class())) {
				if ((unit.Type->get_civilization() != nullptr && unit.Type->get_civilization()->get_species() == faction->get_civilization()->get_species()) || (unit.Type->get_civilization_group() != nullptr && unit.Type->get_civilization_group()->get_species() == faction->get_civilization()->get_species())) {
					unit.UpdatePersonalName();
				}
			}
		} else {
			if (unit.get_unique() == nullptr) {
				const unit_class *unit_class = unit.Type->get_unit_class();
				if (unit_class != nullptr && (old_faction == nullptr || faction == nullptr || old_faction->get_unit_class_name_generator(unit_class) != faction->get_unit_class_name_generator(unit_class))) {
					unit.UpdatePersonalName();
				}
			}
		}

		unit.UpdateSoldUnits();
		unit.UpdateButtonIcons();
	}
}

void CPlayer::set_faction_async(wyrmgus::faction *faction)
{
	engine_interface::get()->post([this, faction]() {
		SendCommandSetFaction(this, faction);
	});
}

void CPlayer::set_random_faction()
{
	// set random one from the civilization's factions
	std::vector<wyrmgus::faction *> potential_factions = this->get_potential_factions();
	
	if (!potential_factions.empty()) {
		const wyrmgus::faction *chosen_faction = vector::get_random(potential_factions);
		this->SetFaction(chosen_faction);
	} else {
		this->SetFaction(nullptr);
	}
}

void CPlayer::set_dynasty(const wyrmgus::dynasty *dynasty)
{
	if (dynasty == this->get_dynasty()) {
		return;
	}

	const wyrmgus::dynasty *old_dynasty = this->dynasty;
	
	if (old_dynasty != nullptr) {
		if (old_dynasty->get_upgrade() != nullptr && this->Allow.Upgrades[old_dynasty->get_upgrade()->ID] == 'R') {
			UpgradeLost(*this, old_dynasty->get_upgrade()->ID);
		}
	}

	this->dynasty = dynasty;

	if (dynasty == nullptr) {
		return;
	}
	
	if (dynasty->get_upgrade() != nullptr) {
		if (this->Allow.Upgrades[dynasty->get_upgrade()->ID] != 'R') {
			if (GameEstablishing) {
				AllowUpgradeId(*this, dynasty->get_upgrade()->ID, 'R');
			} else {
				UpgradeAcquire(*this, dynasty->get_upgrade());
			}
		}
	}

	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit &unit = this->GetUnit(i);
		unit.UpdateSoldUnits(); //in case conditions changed (e.g. some heroes may require a certain dynasty)
	}
}

interface_style *CPlayer::get_interface_style() const
{
	const wyrmgus::civilization *civilization = this->get_civilization();
	if (civilization != nullptr) {
		return civilization->get_interface_style();
	}

	return nullptr;
}

/**
**	@brief	Check which age fits the player's current situation best, and set it as the player's age
*/
void CPlayer::check_age()
{
	//pick an age which fits the player, giving priority to the first ones (ages are already sorted by priority)
	
	for (wyrmgus::age *potential_age : wyrmgus::age::get_all()) {
		if (!check_conditions(potential_age, this)) {
			continue;
		}
		
		this->set_age(potential_age);
		return;
	}
	
	this->set_age(nullptr);
}

/**
**	@brief	Set the player's age
**
**	@param	age	The age to be set for the player
*/
void CPlayer::set_age(const wyrmgus::age *age)
{
	if (this->age == age) {
		return;
	}
	
	this->age = age;
	
	if (this == CPlayer::GetThisPlayer()) {
		if (this->age != nullptr) {
			if (GameCycle > 0 && !SaveGameLoading) {
				this->Notify(_("The %s has dawned upon us."), _(this->age->get_name().c_str()));
			}
		}
	}
	
	wyrmgus::age::check_current_age();
}

/**
**	@brief	Get the player's currency
**
**	@return	The player's currency
*/
CCurrency *CPlayer::GetCurrency() const
{
	if (this->get_faction() != nullptr) {
		return this->get_faction()->GetCurrency();
	}
	
	if (this->get_civilization() != nullptr) {
		return this->get_civilization()->GetCurrency();
	}
	
	return nullptr;
}

void CPlayer::ShareUpgradeProgress(CPlayer &player, CUnit &unit)
{
	std::vector<const CUpgrade *> upgrade_list = this->GetResearchableUpgrades();
	std::vector<const CUpgrade *> potential_upgrades;

	for (size_t i = 0; i < upgrade_list.size(); ++i) {
		if (this->Allow.Upgrades[upgrade_list[i]->ID] != 'R') {
			continue;
		}
		
		if (upgrade_list[i]->get_upgrade_class() == nullptr) {
			continue;
		}

		if (player.get_faction() == nullptr) {
			continue;
		}
		
		const CUpgrade *upgrade = player.get_faction()->get_class_upgrade(upgrade_list[i]->get_upgrade_class());
		if (upgrade == nullptr) {
			continue;
		}
		
		if (player.Allow.Upgrades[upgrade->ID] != 'A' || !check_conditions(upgrade, &player)) {
			continue;
		}
	
		if (player.UpgradeRemovesExistingUpgrade(upgrade, player.AiEnabled)) {
			continue;
		}
		
		potential_upgrades.push_back(upgrade);
	}
	
	if (potential_upgrades.size() > 0) {
		const CUpgrade *chosen_upgrade = potential_upgrades[SyncRand(potential_upgrades.size())];
		
		if (!chosen_upgrade->get_name().empty()) {
			player.Notify(NotifyGreen, unit.tilePos, unit.MapLayer->ID, _("%s acquired through contact with %s"), chosen_upgrade->get_name().c_str(), this->get_name().c_str());
		}
		if (&player == CPlayer::GetThisPlayer() && player.get_civilization() != nullptr) {
			const wyrmgus::sound *sound = player.get_civilization()->get_research_complete_sound();

			if (sound != nullptr) {
				PlayGameSound(sound, MaxSampleVolume);
			}
		}
		if (player.AiEnabled) {
			AiResearchComplete(unit, chosen_upgrade);
		}
		UpgradeAcquire(player, chosen_upgrade);
	}
}

int CPlayer::get_player_color_usage_count(const wyrmgus::player_color *player_color) const
{
	int count = 0;

	for (int i = 0; i < PlayerMax; ++i) {
		if (this->get_index() != i && CPlayer::Players[i]->get_faction() != nullptr && CPlayer::Players[i]->get_type() != player_type::nobody && CPlayer::Players[i]->get_player_color() == player_color) {
			count++;
		}		
	}

	return count;
}

void CPlayer::update_territory_tiles()
{
	for (const auto &kv_pair : this->get_units_by_type()) {
		const wyrmgus::unit_type *unit_type = kv_pair.first;
		if (!unit_type->BoolFlag[TOWNHALL_INDEX].value) {
			continue;
		}

		for (const CUnit *town_hall : kv_pair.second) {
			town_hall->settlement->get_game_data()->update_territory_tiles();
		}
	}

	//also update the minimap territory of vassals, as they get strokes of the overlord's colors
	for (CPlayer *vassal : this->get_vassals()) {
		vassal->update_territory_tiles();
	}
}

unit_type *CPlayer::get_class_unit_type(const unit_class *unit_class) const
{
	const wyrmgus::faction *faction = this->get_faction();
	if (faction == nullptr) {
		return nullptr;
	}

	return faction->get_class_unit_type(unit_class);
}

bool CPlayer::is_class_unit_type(const unit_type *unit_type) const
{
	const wyrmgus::faction *faction = this->get_faction();
	if (faction == nullptr) {
		return false;
	}

	return faction->is_class_unit_type(unit_type);
}

CUpgrade *CPlayer::get_class_upgrade(const wyrmgus::upgrade_class *upgrade_class) const
{
	const wyrmgus::faction *faction = this->get_faction();
	if (faction == nullptr) {
		return nullptr;
	}

	return faction->get_class_upgrade(upgrade_class);
}

bool CPlayer::has_upgrade(const CUpgrade *upgrade) const
{
	assert_throw(upgrade != nullptr);

	return this->Allow.Upgrades[upgrade->ID] == 'R';
}

bool CPlayer::has_upgrade_class(const wyrmgus::upgrade_class *upgrade_class) const
{
	if (this->get_civilization() == nullptr || upgrade_class == nullptr) {
		return false;
	}
	
	const CUpgrade *upgrade = nullptr;
	
	if (this->get_faction() != nullptr) {
		upgrade = this->get_faction()->get_class_upgrade(upgrade_class);
	} else {
		upgrade = this->get_civilization()->get_class_upgrade(upgrade_class);
	}
	
	if (upgrade != nullptr && this->has_upgrade(upgrade)) {
		return true;
	}

	return false;
}

const unit_class *CPlayer::get_default_population_class(const unit_domain domain) const
{
	switch (domain) {
		case unit_domain::water:
			if (this->get_class_unit_type(defines::get()->get_default_water_population_class()) != nullptr) {
				return defines::get()->get_default_water_population_class();
			}
			break;
		case unit_domain::space:
			if (this->get_class_unit_type(defines::get()->get_default_space_population_class()) != nullptr) {
				return defines::get()->get_default_space_population_class();
			}
			break;
		default:
			break;
	}

	return defines::get()->get_default_population_class();
}

std::vector<CUnit *> CPlayer::get_town_hall_units() const
{
	std::vector<CUnit *> town_hall_units;

	for (const auto &kv_pair : this->get_units_by_type()) {
		const wyrmgus::unit_type *unit_type = kv_pair.first;
		if (!unit_type->BoolFlag[TOWNHALL_INDEX].value) {
			continue;
		}

		FindPlayerUnitsByType(*this, *unit_type, town_hall_units, true);
	}

	return town_hall_units;
}

std::vector<const wyrmgus::site *> CPlayer::get_settlements() const
{
	std::vector<const wyrmgus::site *> settlements;

	const std::vector<CUnit *> town_hall_units = this->get_town_hall_units();
	for (const CUnit *town_hall_unit : town_hall_units) {
		settlements.push_back(town_hall_unit->settlement);
	}

	return settlements;
}

bool CPlayer::has_settlement(const wyrmgus::site *settlement) const
{
	if (settlement == nullptr) {
		return false;
	}

	const wyrmgus::site_game_data *settlement_game_data = settlement->get_game_data();

	if (settlement_game_data->get_site_unit() && settlement_game_data->get_site_unit()->Player == this) {
		return true;
	}

	return false;
}

bool CPlayer::has_coastal_settlement() const
{
	const std::vector<const wyrmgus::site *> settlements = this->get_settlements();

	for (const wyrmgus::site *settlement : settlements) {
		if (settlement->get_game_data()->is_coastal()) {
			return true;
		}
	}

	return false;
}

bool CPlayer::HasSettlementNearWaterZone(const landmass *water_zone) const
{
	const std::vector<CUnit *> settlement_unit_table = this->get_town_hall_units();

	for (const CUnit *settlement_unit : settlement_unit_table) {
		if (!settlement_unit->IsAliveOnMap()) {
			continue;
		}
		
		const landmass *settlement_landmass = CMap::get()->get_tile_landmass(settlement_unit->tilePos, settlement_unit->MapLayer->ID);
		if (settlement_landmass == nullptr || !settlement_landmass->borders_landmass(water_zone)) {
			//settlement's landmass doesn't even border the water zone, continue
			continue;
		}
		
		Vec2i pos(0, 0);
		if (FindTerrainType(tile_flag::none, nullptr, 8, *this, settlement_unit->tilePos, &pos, settlement_unit->MapLayer->ID, water_zone)) {
			return true;
		}
	}

	return false;
}

bool CPlayer::has_settlement_with_resource_source(const wyrmgus::resource *resource) const
{
	const std::vector<const wyrmgus::site *> settlements = this->get_settlements();

	for (const wyrmgus::site *settlement : settlements) {
		if (settlement->get_game_data()->has_resource_source(resource)) {
			return true;
		}
	}

	return false;
}

const wyrmgus::site *CPlayer::GetNearestSettlement(const Vec2i &pos, int z, const Vec2i &size) const
{
	CUnit *best_hall = nullptr;
	int best_distance = -1;
	
	for (CUnit *settlement_unit : CMap::get()->get_settlement_units()) {
		if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->Type->BoolFlag[TOWNHALL_INDEX].value || z != settlement_unit->MapLayer->ID) {
			continue;
		}
		if (!this->has_neutral_faction_type() && this != settlement_unit->Player) {
			continue;
		}
		int distance = MapDistance(size, pos, z, settlement_unit->Type->get_tile_size(), settlement_unit->tilePos, settlement_unit->MapLayer->ID);
		if (!best_hall || distance < best_distance) {
			best_hall = settlement_unit;
			best_distance = distance;
		}
	}
	
	if (best_hall) {
		return best_hall->settlement;
	} else {
		return nullptr;
	}
}

void CPlayer::update_building_settlement_assignment(const wyrmgus::site *old_settlement, const int z) const
{
	for (int i = 0; i < this->GetUnitCount(); ++i) {
		CUnit *unit = &this->GetUnit(i);

		if (unit == nullptr || !unit->IsAliveOnMap()) {
			continue;
		}

		if (!unit->Type->BoolFlag[BUILDING_INDEX].value || unit->Type->BoolFlag[TOWNHALL_INDEX].value || unit->Type == settlement_site_unit_type || unit->MapLayer->ID != z) {
			continue;
		}

		if (old_settlement != nullptr && unit->settlement != old_settlement) {
			continue;
		}

		unit->UpdateSettlement();
	}
}

site_set CPlayer::get_border_settlements() const
{
	//get the settlements bordering this player
	const std::vector<const site *> settlements = this->get_settlements();

	site_set border_settlements;

	for (const site *settlement : settlements) {
		for (const site *border_settlement : settlement->get_game_data()->get_border_settlements()) {
			const CPlayer *border_settlement_owner = border_settlement->get_game_data()->get_owner();
			if (border_settlement_owner == this) {
				continue;
			}

			border_settlements.insert(border_settlement);
		}
	}

	return border_settlements;
}

bool CPlayer::HasUnitBuilder(const wyrmgus::unit_type *type, const wyrmgus::site *settlement) const
{
	const std::vector<const wyrmgus::unit_type *> *builders = nullptr;
	const std::vector<const wyrmgus::unit_class *> *builder_classes = nullptr;

	if (type->BoolFlag[BUILDING_INDEX].value) {
		builders = &AiHelpers.get_builders(type);
		builder_classes = &AiHelpers.get_builder_classes(type->get_unit_class());
	} else {
		builders = &AiHelpers.get_trainers(type);
		builder_classes = &AiHelpers.get_trainer_classes(type->get_unit_class());
	}

	for (const wyrmgus::unit_type *builder : *builders) {
		if (this->GetUnitTypeCount(builder) > 0) {
			return true;
		}
	}

	if (this->get_faction() != nullptr) {
		for (const wyrmgus::unit_class *builder_class : *builder_classes) {
			const wyrmgus::unit_type *builder = this->get_faction()->get_class_unit_type(builder_class);

			if (builder == nullptr) {
				continue;
			}

			if (this->GetUnitTypeCount(builder) > 0) {
				return true;
			}
		}
	}

	const std::vector<const wyrmgus::unit_type *> &unit_type_upgradees = AiHelpers.get_unit_type_upgradees(type);
	const std::vector<const wyrmgus::unit_class *> &unit_class_upgradees = AiHelpers.get_unit_class_upgradees(type->get_unit_class());

	for (const wyrmgus::unit_type *unit_type_upgradee : unit_type_upgradees) {
		if (this->GetUnitTypeCount(unit_type_upgradee) > 0) {
			if (!settlement) {
				return true;
			} else {
				for (int i = 0; i < this->GetUnitCount(); ++i) {
					CUnit &unit = this->GetUnit(i);
					if (unit.Type == unit_type_upgradee && unit.settlement == settlement) {
						return true;
					}
				}
			}
		}
	}

	if (this->get_faction() != nullptr) {
		for (const wyrmgus::unit_class *unit_class_upgradee : unit_class_upgradees) {
			const wyrmgus::unit_type *unit_type_upgradee = this->get_faction()->get_class_unit_type(unit_class_upgradee);

			if (unit_type_upgradee == nullptr) {
				continue;
			}

			if (this->GetUnitTypeCount(unit_type_upgradee) > 0) {
				if (!settlement) {
					return true;
				} else {
					for (int i = 0; i < this->GetUnitCount(); ++i) {
						CUnit &unit = this->GetUnit(i);
						if (unit.Type->get_unit_class() == unit_class_upgradee && unit.settlement == settlement) {
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
	for (const wyrmgus::unit_type *researcher_type : AiHelpers.get_researchers(upgrade)) {
		if (this->GetUnitTypeCount(researcher_type) > 0 || HasUnitBuilder(researcher_type)) {
			return true;
		}
	}

	for (const wyrmgus::unit_class *researcher_class : AiHelpers.get_researcher_classes(upgrade->get_upgrade_class())) {
		const wyrmgus::unit_type *researcher_type = this->get_class_unit_type(researcher_class);
		if (researcher_type != nullptr && (this->GetUnitTypeCount(researcher_type) > 0 || this->HasUnitBuilder(researcher_type))) {
			return true;
		}
	}

	return false;
}

template <bool preconditions_only>
bool CPlayer::can_found_faction(const wyrmgus::faction *faction) const
{
	if (CurrentQuest != nullptr) {
		return false;
	}
	
	const CUpgrade *faction_type_upgrade = defines::get()->get_faction_type_upgrade(faction->get_type());
	if (faction_type_upgrade != nullptr && !this->has_upgrade(faction_type_upgrade) && !check_conditions<preconditions_only>(faction_type_upgrade, this, false)) {
		return false;
	}

	if (faction->get_upgrade() != nullptr) {
		const CUpgrade *faction_upgrade = faction->get_upgrade();
		if (!check_conditions<preconditions_only>(faction_upgrade, this, false)) {
			return false;
		}
	}

	for (const qunique_ptr<CPlayer> &other_player : CPlayer::Players) {
		if (other_player.get() != this && other_player->get_type() != player_type::nobody && other_player->get_faction() == faction) {
			//faction is already in use
			return false;
		}
	}

	if (faction->get_preconditions() != nullptr && !faction->get_preconditions()->check(this)) {
		return false;
	}

	if constexpr (!preconditions_only) {
		//check if the required core settlements are owned by the player
		if (game::get()->get_current_campaign() != nullptr) { //only check for settlements in the Scenario mode
			for (const site *core_settlement : faction->get_core_settlements()) {
				const site_game_data *settlement_game_data = core_settlement->get_game_data();

				if (settlement_game_data->get_site_unit() == nullptr || settlement_game_data->get_site_unit()->Player != this || settlement_game_data->get_site_unit()->CurrentAction() == UnitAction::Built) {
					return false;
				}
			}
		}

		if (faction->get_conditions() != nullptr && !faction->get_conditions()->check(this)) {
			return false;
		}
	}
	
	return true;
}

template bool CPlayer::can_found_faction<false>(const wyrmgus::faction *faction) const;
template bool CPlayer::can_found_faction<true>(const wyrmgus::faction *faction) const;

std::vector<faction *> CPlayer::get_potential_factions() const
{
	std::vector<wyrmgus::faction *> faction_list;

	const civilization *civilization = this->get_civilization();

	if (civilization != nullptr) {
		for (wyrmgus::faction *faction : civilization->get_factions()) {
			if (!faction->is_playable()) {
				continue;
			}

			if (faction->get_type() != faction_type::tribe && faction->get_type() != faction_type::polity) {
				//trading companies, mercenary squads and etc. are not playable
				continue;
			}

			const CUpgrade *polity_upgrade = defines::get()->get_faction_type_upgrade(faction_type::polity);
			const bool can_be_polity = (polity_upgrade != nullptr && check_conditions<false>(polity_upgrade, this, false));

			switch (faction->get_type()) {
				case faction_type::tribe:
					if (can_be_polity) {
						//don't put tribal factions in the list if the player already has access to polity factions
						continue;
					}
					break;
				case faction_type::polity:
					if (!can_be_polity) {
						continue;
					}
					break;
				default:
					break;
			}

			//in multiplayer factions can be chosen multiple times by different (human) players, but otherwise factions can only appear for one given player
			if (!game::get()->is_multiplayer() || this->get_type() != player_type::person) {
				const CPlayer *faction_player = GetFactionPlayer(faction);
				if (faction_player != nullptr && faction_player != this) {
					continue;
				}
			}

			faction_list.push_back(faction);
		}
	}

	return faction_list;
}

template <bool preconditions_only>
bool CPlayer::can_choose_dynasty(const wyrmgus::dynasty *dynasty) const
{
	if (CurrentQuest != nullptr) {
		return false;
	}
	
	if (dynasty->get_upgrade() == nullptr) {
		return false;
	}

	if (!wyrmgus::check_conditions<preconditions_only>(dynasty->get_upgrade(), this, false)) {
		return false;
	}

	return wyrmgus::check_conditions<preconditions_only>(dynasty, this, false);
}

template bool CPlayer::can_choose_dynasty<false>(const wyrmgus::dynasty *dynasty) const;
template bool CPlayer::can_choose_dynasty<true>(const wyrmgus::dynasty *dynasty) const;


bool CPlayer::is_character_available_for_recruitment(const character *character, bool ignore_neutral) const
{
	if (character->is_deity()) {
		return false;
	}
	
	if (!character->get_civilization() || character->get_civilization() != this->get_civilization()) {
		return false;
	}
	
	if (character->get_conditions() != nullptr && !character->get_conditions()->check(this)) {
		return false;
	}
	
	//the conditions for the character's unit type must be fulfilled
	if (!check_conditions(character->get_unit_type(), this, true)) {
		return false;
	}
	
	if (character->Conditions) {
		CclCommand("trigger_player = " + std::to_string(this->get_index()) + ";");
		character->Conditions->pushPreamble();
		character->Conditions->run(1);
		if (character->Conditions->popBoolean() == false) {
			return false;
		}
	}

	if (game::get()->get_current_campaign() != nullptr) {
		//for campaigns, take the character's start and end year into consideration for whether they can be recruited
		const int current_year = game::get()->get_current_year();

		if (character->get_start_year() != 0 && current_year < character->get_start_year()) {
			return false;
		}

		if (character->get_end_year() != 0 && current_year > character->get_end_year()) {
			return false;
		}
	}
	
	if (!character->CanAppear(ignore_neutral)) {
		return false;
	}
	
	return true;
}

std::vector<character *> CPlayer::get_recruitable_heroes_from_list(const std::vector<character *> &heroes)
{
	std::vector<character *> recruitable_heroes;

	for (character *hero : heroes) {
		if (this->is_character_available_for_recruitment(hero)) {
			recruitable_heroes.push_back(hero);
		}
	}

	return recruitable_heroes;
}
/**
**  Check if the upgrade removes an existing upgrade of the player.
**
**  @param upgrade    Upgrade.
*/
bool CPlayer::UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade, bool ignore_lower_priority) const
{
	for (const auto &modifier : upgrade->get_modifiers()) {
		for (size_t j = 0; j < modifier->RemoveUpgrades.size(); ++j) {
			const CUpgrade *removed_upgrade = modifier->RemoveUpgrades[j];
			bool has_upgrade = this->AiEnabled ? AiHasUpgrade(*this->Ai, removed_upgrade, true) : (UpgradeIdAllowed(*this, removed_upgrade->ID) == 'R');
			if (has_upgrade) {
				if (ignore_lower_priority && this->get_faction() != nullptr && this->get_faction()->GetUpgradePriority(removed_upgrade) < this->get_faction()->GetUpgradePriority(upgrade)) {
					continue;
				}
				return true;
			}
		}
	}
	
	return false;
}

std::string CPlayer::get_full_name() const
{
	if (!IsNetworkGame()) {
		const wyrmgus::faction *faction = this->get_faction();

		if (faction != nullptr) {
			const wyrmgus::government_type government_type = this->get_government_type();
			const wyrmgus::faction_tier tier = this->get_faction_tier();
			return faction->get_titled_name(government_type, tier);
		}
	}

	return this->get_name();
}

std::string_view CPlayer::get_faction_title_name() const
{
	if (this->get_civilization() == nullptr || this->get_faction() == nullptr) {
		return wyrmgus::string::empty_str;
	}
	
	const wyrmgus::faction *faction = this->get_faction();
	const wyrmgus::government_type government_type = this->get_government_type();
	const wyrmgus::faction_tier tier = this->get_faction_tier();

	return faction->get_title_name(government_type, tier);
}

std::string_view CPlayer::GetCharacterTitleName(const character_title title_type, const wyrmgus::gender gender) const
{
	if (this->get_faction() == nullptr || title_type == character_title::none || gender == gender::none) {
		return wyrmgus::string::empty_str;
	}
	
	const wyrmgus::faction *faction = this->get_faction();
	const wyrmgus::government_type government_type = this->get_government_type();
	const wyrmgus::faction_tier tier = this->get_faction_tier();

	return faction->get_character_title_name(title_type, government_type, tier, gender);
}

landmass_set CPlayer::get_builder_landmasses(const wyrmgus::unit_type *building) const
{
	landmass_set builder_landmasses;

	for (const wyrmgus::unit_type *builder_type : AiHelpers.get_builders(building)) {
		if (this->GetUnitTypeAiActiveCount(builder_type) > 0) {
			std::vector<CUnit *> builder_table;

			FindPlayerUnitsByType(*this, *builder_type, builder_table, true);

			for (const CUnit *builder : builder_table) {
				const landmass *landmass = CMap::get()->get_tile_landmass(builder->tilePos, builder->MapLayer->ID);

				if (landmass == nullptr) {
					continue;
				}

				builder_landmasses.insert(landmass);
			}
		}
	}

	if (this->get_faction() != nullptr) {
		for (const wyrmgus::unit_class *builder_class : AiHelpers.get_builder_classes(building->get_unit_class())) {
			const wyrmgus::unit_type *builder_type = this->get_faction()->get_class_unit_type(builder_class);

			if (this->GetUnitTypeAiActiveCount(builder_type) > 0) {
				std::vector<CUnit *> builder_table;

				FindPlayerUnitsByType(*this, *builder_type, builder_table, true);

				for (const CUnit *builder : builder_table) {
					const landmass *landmass = CMap::get()->get_tile_landmass(builder->tilePos, builder->MapLayer->ID);

					if (landmass == nullptr) {
						continue;
					}

					builder_landmasses.insert(landmass);
				}
			}
		}
	}

	return builder_landmasses;
}

std::vector<const CUpgrade *> CPlayer::GetResearchableUpgrades()
{
	std::vector<const CUpgrade *> researchable_upgrades;

	for (const auto &kv_pair : this->UnitTypesAiActiveCount) {
		const wyrmgus::unit_type *type = kv_pair.first;

		for (const CUpgrade *upgrade : AiHelpers.get_researched_upgrades(type)) {
			if (!wyrmgus::vector::contains(researchable_upgrades, upgrade)) {
				researchable_upgrades.push_back(upgrade);
			}
		}

		for (const wyrmgus::upgrade_class *upgrade_class : AiHelpers.get_researched_upgrade_classes(type->get_unit_class())) {
			const CUpgrade *upgrade = this->get_class_upgrade(upgrade_class);
			if (upgrade != nullptr && !wyrmgus::vector::contains(researchable_upgrades, upgrade)) {
				researchable_upgrades.push_back(upgrade);
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
	this->name.clear();
	this->set_type(player_type::none);
	this->Race = defines::get()->get_neutral_civilization()->ID;
	this->faction = nullptr;
	this->faction_tier = faction_tier::none;
	this->government_type = government_type::none;
	this->religion = nullptr;
	this->dynasty = nullptr;
	this->age = nullptr;
	this->overlord = nullptr;
	this->vassalage_type = vassalage_type::none;
	this->vassals.clear();
	this->AiName.clear();
	this->set_alive(false);
	this->Team = 0;
	this->enemies.clear();
	this->allies.clear();
	this->shared_vision.clear();
	this->mutual_shared_vision.clear();
	this->StartPos.x = 0;
	this->StartPos.y = 0;
	//Wyrmgus start
	this->StartMapLayer = 0;
	//Wyrmgus end

	this->resources.clear();
	this->stored_resources.clear();
	this->max_resources.clear();
	this->last_resources.clear();
	this->incomes.clear();
	this->revenues.clear();
	//Wyrmgus start
	this->resource_demands.clear();
	this->stored_resource_demands.clear();
	//Wyrmgus end

	this->UnitTypesCount.clear();
	this->UnitTypesUnderConstructionCount.clear();
	this->UnitTypesAiActiveCount.clear();
	//Wyrmgus start
	this->Heroes.clear();
	this->Deities.clear();
	this->units_by_type.clear();
	this->units_by_class.clear();
	this->AiActiveUnitsByType.clear();
	this->last_created_unit = nullptr;

	this->available_quests.clear();
	this->current_quests.clear();
	this->completed_quests.clear();
	this->AutosellResources.clear();
	this->quest_objectives.clear();
	this->Modifiers.clear();
	//Wyrmgus end
	this->AiEnabled = false;
	this->revealed = false;
	this->Ai = 0;
	this->Units.clear();
	this->FreeWorkers.clear();
	//Wyrmgus start
	this->LevelUpUnits.clear();
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
	this->resource_totals.clear();
	this->TotalRazings = 0;
	this->TotalKills = 0;
	//Wyrmgus start
	memset(this->UnitTypeKills, 0, sizeof(this->UnitTypeKills));
	this->LostTownHallTimer = 0;
	this->HeroCooldownTimer = 0;
	//Wyrmgus end
	this->UpgradeTimers.Clear();
	this->resource_harvest_speeds.clear();
	this->resource_return_speeds.clear();

	for (const resource *resource : resource::get_all()) {
		this->set_price(resource, resource->get_base_price());
	}

	this->SpeedBuild = CPlayer::base_speed_factor;
	this->SpeedTrain = CPlayer::base_speed_factor;
	this->SpeedUpgrade = CPlayer::base_speed_factor;
	this->SpeedResearch = CPlayer::base_speed_factor;

	emit diplomatic_stances_changed();
	emit shared_vision_changed();
}


void CPlayer::AddUnit(CUnit &unit)
{
	assert_throw(unit.Player != this);
	assert_throw(unit.PlayerSlot == static_cast<size_t>(-1));
	unit.PlayerSlot = this->Units.size();
	this->Units.push_back(&unit);
	unit.Player = this;
	assert_throw(this->Units[unit.PlayerSlot] == &unit);

	if (this->Units.size() == 1) {
		this->set_alive(true);
	}
}

void CPlayer::RemoveUnit(CUnit &unit)
{
	assert_throw(unit.Player == this);
	//Wyrmgus start
	if (unit.PlayerSlot == -1 || this->Units[unit.PlayerSlot] != &unit) {
		log::log_error("Error in CPlayer::RemoveUnit: the unit's PlayerSlot doesn't match its position in the player's units array; Unit's PlayerSlot: " + std::to_string(unit.PlayerSlot) + ", Unit Type: \"" + (unit.Type ? unit.Type->Ident : "") + "\".");
		return;
	}
	//Wyrmgus end
	assert_throw(this->Units[unit.PlayerSlot] == &unit);

	//	unit.Player = nullptr; // we can remove dying unit...
	CUnit *last = this->Units.back();

	this->Units[unit.PlayerSlot] = last;
	last->PlayerSlot = unit.PlayerSlot;
	this->Units.pop_back();
	unit.PlayerSlot = static_cast<size_t>(-1);
	assert_throw(last == &unit || this->Units[last->PlayerSlot] == last);

	if (this->Units.empty()) {
		this->set_alive(false);
	}
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
			if (unit.CurrentAction() == UnitAction::Still) {
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
		const resource *res = resource::get_all()[this->AutosellResources[i]];
		
		if ((this->get_resource(res) + this->get_stored_resource(res)) >= 100) { //sell 100 per second, as long as there is enough of the resource stored
			market_unit->sell_resource(res, this->get_index());
		}
		
		//increase price due to domestic demand
		this->change_stored_resource_demand(res, this->get_effective_resource_demand(res));
		while (this->get_stored_resource_demand(res) >= 100) {
			this->increase_resource_price(res);
			this->change_stored_resource_demand(res, -100);
		}
	}
	
	for (size_t i = 0; i < LuxuryResources.size(); ++i) {
		const resource *res = resource::get_all()[LuxuryResources[i]];
		
		while ((this->get_resource(res) + this->get_stored_resource(res)) >= 100) {
			market_unit->sell_resource(res, this->get_index());
		}
		
		//increase price due to domestic demand
		this->change_stored_resource_demand(res, this->get_effective_resource_demand(res));
		while (this->get_stored_resource_demand(res) >= 100) {
			this->increase_resource_price(res);
			this->change_stored_resource_demand(res, -100);
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
		const wyrmgus::unit_type &market_type = *AiHelpers.SellMarkets[0][i];

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
		const wyrmgus::unit_type &market_type = *AiHelpers.SellMarkets[0][i];

		if (this->GetUnitTypeCount(&market_type)) {
			std::vector<CUnit *> market_table;
			FindPlayerUnitsByType(*this, market_type, market_table);
			if (market_table.size() > 0) {
				market_unit = market_table[SyncRand(market_table.size())];
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
	this->LevelUpUnits.clear();
	if (this->LevelUpUnits.capacity() != 0) {
		// Just calling LevelUpUnits.clear() is not always appropriate.
		// Certain paths may leave LevelUpUnits in an invalid state, so
		// it's safer to re-initialize.
		std::vector<CUnit*>().swap(this->LevelUpUnits);
	}
	const int nunits = this->GetUnitCount();

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = this->GetUnit(i);
		if (unit.IsAlive() && unit.Variable[LEVELUP_INDEX].Value >= 1) {
			LevelUpUnits.push_back(&unit);
		}
	}
}

void CPlayer::update_quest_pool()
{
	if (wyrmgus::game::get()->get_current_campaign() == nullptr) { // in-game quests only while playing the campaign mode
		return;
	}

	if (this->get_faction() == nullptr) {
		return;
	}
	
	const bool exausted_available_quests = this->available_quests.empty();
	
	this->available_quests.clear();
	
	std::vector<wyrmgus::quest *> potential_quests;
	for (wyrmgus::quest *quest : wyrmgus::quest::get_all()) {
		if (this->can_quest_be_available(quest)) {
			potential_quests.push_back(quest);
		}
	}
	
	for (int i = 0; i < CPlayer::max_quest_pool; ++i) { //fill the quest pool with quests up to the max quantity
		if (potential_quests.empty()) {
			break;
		}

		wyrmgus::quest *quest = wyrmgus::vector::take_random(potential_quests);
		this->available_quests.push_back(quest);
	}

	this->on_available_quests_changed();

	// notify the player when new quests are available (but only if the player has already exausted the quests available to him, so that they aren't bothered if they choose not to engage with the quest system)
	if (this == CPlayer::GetThisPlayer() && GameCycle >= CYCLES_PER_MINUTE && this->available_quests.size() > 0 && exausted_available_quests && this->NumTownHalls > 0 && this->get_current_quests().size() < CPlayer::max_current_quests) {
		CPlayer::GetThisPlayer()->Notify("%s", _("New quests available"));
	}
	
	if (this->AiEnabled && this->NumTownHalls > 0) { // if is an AI player, accept all quests that it can
		int available_quest_quantity = this->available_quests.size();
		for (int i = (available_quest_quantity  - 1); i >= 0; --i) {
			if (this->get_current_quests().size() >= CPlayer::max_current_quests) {
				break;
			}

			if (this->can_accept_quest(this->available_quests[i])) { // something may have changed, so recheck if the player is able to accept the quest
				this->accept_quest(this->available_quests[i]);
			}
		}
	}
}

void CPlayer::on_available_quests_changed()
{
	if (this == CPlayer::GetThisPlayer()) {
		read_only_context script_ctx;
		script_ctx.source_player = this;
		script_ctx.current_player = this;

		text_processing_context text_processing_ctx(script_ctx);
		const text_processor text_processor(std::move(text_processing_ctx));

		for (button *button : button::get_all()) {
			if (button->Action != ButtonCmd::Quest || button->Value >= static_cast<int>(this->available_quests.size())) {
				continue;
			}
			
			const quest *quest = this->available_quests[button->Value];
			button->Hint = "Quest: " + quest->get_name();

			button->Description = text_processor.process_text(quest->get_description(), true);

			button->Description += "\n \nObjectives:";
			for (const auto &objective : quest->get_objectives()) {
				button->Description += "\n- ";
				if (!objective->get_objective_string().empty()) {
					button->Description += objective->get_objective_string();
				} else {
					button->Description += objective->generate_objective_string(this);
				}
			}
			for (const std::string &objective_string : quest->get_objective_strings()) {
				button->Description += "\n" + objective_string;
			}
			const std::string rewards_string = quest->get_rewards_string(this);
			if (!rewards_string.empty()) {
				button->Description += "\n \nRewards:\n" + rewards_string;
			}

			if (!quest->get_hint().empty()) {
				button->Description += "\n \nHint: " + text_processor.process_text(quest->get_hint(), true);
			}

			if (quest->get_highest_completed_difficulty() > difficulty::none) {
				std::string highest_completed_difficulty;
				if (quest->get_highest_completed_difficulty() == difficulty::easy) {
					highest_completed_difficulty = "Easy";
				} else if (quest->get_highest_completed_difficulty() == difficulty::normal) {
					highest_completed_difficulty = "Normal";
				} else if (quest->get_highest_completed_difficulty() == difficulty::hard) {
					highest_completed_difficulty = "Hard";
				} else if (quest->get_highest_completed_difficulty() == difficulty::brutal) {
					highest_completed_difficulty = "Brutal";
				}
				button->Description += "\n \nHighest Completed Difficulty: " + highest_completed_difficulty;
			}
			
		}
		
		if (!Selected.empty() && Selected[0]->Type->BoolFlag[TOWNHALL_INDEX].value) {
			UI.ButtonPanel.Update();
		}
	}
}

void CPlayer::update_current_quests()
{
	for (const auto &objective : this->get_quest_objectives()) {
		objective->update_counter();
	}
	
	for (int i = (this->current_quests.size()  - 1); i >= 0; --i) {
		wyrmgus::quest *quest = this->current_quests[i];
		const std::pair<bool, std::string> failure_result = this->check_quest_failure(quest);
		if (failure_result.first) {
			this->fail_quest(quest, failure_result.second);
		} else if (this->check_quest_completion(quest)) {
			this->complete_quest(quest);
		}
	}
}

void CPlayer::accept_quest(wyrmgus::quest *quest)
{
	if (!quest) {
		return;
	}
	
	vector::remove(this->available_quests, quest);
	this->current_quests.push_back(quest);
	
	for (const auto &quest_objective : quest->get_objectives()) {
		auto objective = std::make_unique<wyrmgus::player_quest_objective>(quest_objective.get(), this);
		this->quest_objectives.push_back(std::move(objective));
	}
	
	CclCommand("trigger_player = " + std::to_string(this->get_index()) + ";");
	
	if (quest->AcceptEffects) {
		quest->AcceptEffects->pushPreamble();
		quest->AcceptEffects->run();
	}

	if (quest->get_accept_effects() != nullptr) {
		wyrmgus::context ctx;
		ctx.current_player = this;
		quest->get_accept_effects()->do_effects(this, ctx);
	}

	this->on_available_quests_changed();
	
	this->update_current_quests();
}

void CPlayer::complete_quest(wyrmgus::quest *quest)
{
	if (wyrmgus::vector::contains(this->completed_quests, quest)) {
		return;
	}
	
	this->remove_current_quest(quest);
	
	this->completed_quests.push_back(quest);
	if (quest->is_competitive()) {
		quest->CurrentCompleted = true;
	}
	
	CclCommand("trigger_player = " + std::to_string(this->get_index()) + ";");
	
	if (quest->CompletionEffects) {
		quest->CompletionEffects->pushPreamble();
		quest->CompletionEffects->run();
	}

	if (quest->get_completion_effects() != nullptr) {
		wyrmgus::context ctx;
		ctx.current_player = this;
		quest->get_completion_effects()->do_effects(this, ctx);
	}
	
	if (this == CPlayer::GetThisPlayer()) {
		if (game::get()->is_persistency_enabled()) {
			const difficulty difficulty = static_cast<wyrmgus::difficulty>(GameSettings.Difficulty);
			quest->on_completed(difficulty);
		}

		const wyrmgus::campaign *current_campaign = wyrmgus::game::get()->get_current_campaign();
		if (current_campaign != nullptr && current_campaign->get_quest() == quest) {
			wyrmgus::context ctx;
			ctx.current_player = this;
			wyrmgus::defines::get()->get_campaign_victory_dialogue()->call(this, ctx);
		}

		std::string rewards_string = quest->get_rewards_string(this);
		if (!rewards_string.empty()) {
			wyrmgus::string::replace(rewards_string, "\n", "\\n");
			wyrmgus::string::replace(rewards_string, "\t", "\\t");
			rewards_string = "Rewards:\\n" + rewards_string;
		}
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Completed\", \"You have completed the " + quest->get_name() + " quest!\\n\\n" + rewards_string + "\", nil, \"" + (quest->get_icon() ? quest->get_icon()->get_identifier() : "") + "\", \"" + (quest->get_player_color() ? quest->get_player_color()->get_identifier() : "") + "\", " + std::to_string(quest->get_icon() ? quest->get_icon()->get_frame() : 0) + ") end;");
	}
}

void CPlayer::fail_quest(wyrmgus::quest *quest, const std::string &fail_reason)
{
	this->remove_current_quest(quest);
	
	CclCommand("trigger_player = " + std::to_string(this->get_index()) + ";");
	
	if (quest->FailEffects) {
		quest->FailEffects->pushPreamble();
		quest->FailEffects->run();
	}
	
	if (quest->get_failure_effects() != nullptr) {
		wyrmgus::context ctx;
		ctx.current_player = this;
		quest->get_failure_effects()->do_effects(this, ctx);
	}

	if (this == CPlayer::GetThisPlayer()) {
		const wyrmgus::campaign *current_campaign = wyrmgus::game::get()->get_current_campaign();
		if (current_campaign != nullptr && current_campaign->get_quest() == quest) {
			wyrmgus::context ctx;
			ctx.current_player = this;
			wyrmgus::defines::get()->get_campaign_defeat_dialogue()->call(this, ctx);
		}

		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Quest Failed\", \"You have failed the " + quest->get_name() + " quest! " + fail_reason + "\", nil, \"" + (quest->get_icon() ? quest->get_icon()->get_identifier() : "") + "\", \"" + (quest->get_player_color() ? quest->get_player_color()->get_identifier() : "") + "\", " + std::to_string(quest->get_icon() ? quest->get_icon()->get_frame() : 0) + ") end;");
	}
}

void CPlayer::remove_current_quest(wyrmgus::quest *quest)
{
	vector::remove(this->current_quests, quest);
	
	for (int i = (this->quest_objectives.size()  - 1); i >= 0; --i) {
		if (this->quest_objectives[i]->get_quest_objective()->get_quest() == quest) {
			vector::remove(this->quest_objectives, this->quest_objectives[i]);
		}
	}
}

bool CPlayer::can_quest_be_available(const wyrmgus::quest *quest) const
{
	if (quest->is_hidden() || quest->is_unobtainable()) {
		return false;
	}

	return this->can_accept_quest(quest);
}

bool CPlayer::can_accept_quest(const wyrmgus::quest *quest) const
{
	if (this->get_faction() == nullptr) {
		return false;
	}

	if (quest->CurrentCompleted) {
		return false;
	}
	
	if (vector::contains(this->current_quests, quest) || vector::contains(this->completed_quests, quest)) {
		return false;
	}

	if (quest->get_conditions() != nullptr && !quest->get_conditions()->check(this)) {
		return false;
	}
	
	int recruit_heroes_quantity = 0;
	for (const auto &objective : quest->get_objectives()) {
		if (!objective->is_quest_acceptance_allowed(this)) {
			return false;
		}

		if (objective->get_objective_type() == wyrmgus::objective_type::recruit_hero) {
			++recruit_heroes_quantity;
		}
	}
	
	if (recruit_heroes_quantity > 0 && (this->Heroes.size() + recruit_heroes_quantity) > CPlayer::max_heroes) {
		return false;
	}

	for (const wyrmgus::quest *current_quest : this->current_quests) {
		if (quest->overlaps_with(current_quest)) {
			return false;
		}
	}
	
	for (const wyrmgus::character *hero : quest->HeroesMustSurvive) {
		if (!this->HasHero(hero)) {
			return false;
		}
	}

	if (quest->Conditions != nullptr) {
		CclCommand("trigger_player = " + std::to_string(this->get_index()) + ";");
		quest->Conditions->pushPreamble();
		quest->Conditions->run(1);
		return quest->Conditions->popBoolean();
	} else {
		return true;
	}
}

bool CPlayer::check_quest_completion(const wyrmgus::quest *quest) const
{
	if (quest->is_uncompleteable()) {
		return false;
	}
	
	for (const auto &objective : this->get_quest_objectives()) {
		const wyrmgus::quest_objective *quest_objective = objective->get_quest_objective();
		if (quest_objective->get_quest() != quest) {
			continue;
		}
		if (quest_objective->get_quantity() != 0 && objective->get_counter() < quest_objective->get_quantity()) {
			return false;
		}
	}
	
	return true;
}

//returns the reason for failure (empty if none)
std::pair<bool, std::string> CPlayer::check_quest_failure(const wyrmgus::quest *quest) const
{
	if (quest->CurrentCompleted) { // quest already completed by someone else
		return std::make_pair(true, "Another faction has completed the quest before you could.");
	}

	for (const auto &objective : this->get_quest_objectives()) {
		const wyrmgus::quest_objective *quest_objective = objective->get_quest_objective();
		if (quest_objective->get_quest() != quest) {
			continue;
		}

		//"unfailable" quests should also fail when a hero which should survive dies
		if (quest->is_unfailable() && quest_objective->get_objective_type() != wyrmgus::objective_type::hero_must_survive) {
			continue;
		}

		if (quest_objective->get_quantity() != 0 && objective->get_counter() >= quest_objective->get_quantity()) {
			//objective already fulfilled
			continue;
		}

		std::pair<bool, std::string> result = quest_objective->check_failure(this);
		if (result.first) {
			return result;
		}
	}

	//unfailable" quests should also fail when a hero which should survive dies
	for (const wyrmgus::character *character : quest->HeroesMustSurvive) {
		if (!this->HasHero(character)) {
			return std::make_pair(true, "A hero necessary for the quest has died.");
		}
	}

	return std::make_pair(false, std::string());
}

bool CPlayer::has_quest(const wyrmgus::quest *quest) const
{
	return vector::contains(this->current_quests, quest);
}

bool CPlayer::is_quest_completed(const wyrmgus::quest *quest) const
{
	return vector::contains(this->completed_quests, quest);
}

void CPlayer::on_unit_built(const CUnit *unit)
{
	bool counter_changed = false;

	for (const auto &objective : this->get_quest_objectives()) {
		const int old_counter = objective->get_counter();
		objective->on_unit_built(unit);
		const int new_counter = objective->get_counter();

		if (old_counter != new_counter) {
			counter_changed = true;
		}
	}

	if (counter_changed) {
		this->update_current_quests();
	}
}

void CPlayer::on_unit_destroyed(const CUnit *unit)
{
	bool counter_changed = false;

	for (const auto &objective : this->get_quest_objectives()) {
		const int old_counter = objective->get_counter();
		objective->on_unit_destroyed(unit);
		const int new_counter = objective->get_counter();

		if (old_counter != new_counter) {
			counter_changed = true;
		}
	}

	if (counter_changed) {
		this->update_current_quests();
	}
}

void CPlayer::on_resource_gathered(const wyrmgus::resource *resource, const int quantity)
{
	bool counter_changed = false;

	for (const auto &objective : this->get_quest_objectives()) {
		const int old_counter = objective->get_counter();
		objective->on_resource_gathered(resource, quantity);
		const int new_counter = objective->get_counter();

		if (old_counter != new_counter) {
			counter_changed = true;
		}
	}

	if (counter_changed) {
		this->update_current_quests();
	}
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

bool CPlayer::at_war() const
{
	for (int i = 0; i < PlayerNumNeutral; ++i) {
		const CPlayer *other_player = CPlayer::Players[i].get();

		if (other_player == this) {
			continue;
		}

		if (!this->HasContactWith(*other_player)) {
			continue;
		}

		if (!other_player->is_alive()) {
			continue;
		}

		if (this->is_enemy_of(*other_player)) {
			return true;
		}
	}
	
	return false;
}
//Wyrmgus end

std::vector<CUnit *>::const_iterator CPlayer::UnitBegin() const
{
	return Units.begin();
}

std::vector<CUnit *>::const_iterator CPlayer::UnitEnd() const
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
int CPlayer::get_resource(const wyrmgus::resource *resource, const resource_storage_type type) const
{
	switch (type) {
		case resource_storage_type::overall:
			return this->get_resource(resource);
		case resource_storage_type::building:
			return this->get_stored_resource(resource);
		case resource_storage_type::both:
			return this->get_resource(resource) + this->get_stored_resource(resource);
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
void CPlayer::change_resource(const wyrmgus::resource *resource, const int value, const bool store)
{
	if (value < 0) {
		const int fromStore = std::min(this->get_stored_resource(resource), abs(value));
		this->change_stored_resource(resource, -fromStore);
		this->change_resource(resource, -(abs(value) - fromStore));
		this->set_resource(resource, std::max(this->get_resource(resource), 0));
	} else {
		if (store && this->get_max_resource(resource) != -1) {
			this->change_stored_resource(resource,  std::min(value, this->get_max_resource(resource) - this->get_stored_resource(resource)));
		} else {
			this->change_resource(resource,  value);
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
void CPlayer::set_resource(const wyrmgus::resource *resource, const int value, const resource_storage_type type)
{
	switch (type) {
		case resource_storage_type::both:
			if (this->get_max_resource(resource) != -1) {
				const int to_res = std::max(0, value - this->get_stored_resource(resource));
				this->set_resource(resource, std::max(0, to_res));
				this->set_stored_resource(resource, std::min(value - to_res, this->get_max_resource(resource)));
			} else {
				this->set_resource(resource, std::max(0, value));
			}
			break;
		case resource_storage_type::building:
			if (this->get_max_resource(resource) != -1) {
				this->set_stored_resource(resource, std::min(value, this->get_max_resource(resource)));
			}
			break;
		case resource_storage_type::overall:
			this->set_resource(resource, std::max(0, value));
			break;
		default:
			break;
	}
}

/**
**  Check, if there enough resources for action.
**
**  @param resource  Resource to change.
**  @param value     How many of this resource.
*/
bool CPlayer::check_resource(const resource *resource, const int value)
{
	int result = this->get_resource(resource);
	if (this->get_max_resource(resource) != -1) {
		result += this->get_stored_resource(resource);
	}
	return result < value ? false : true;
}

void CPlayer::increase_resource_price(const resource *resource)
{
	int price_change = resource->get_base_price() / std::max(this->get_price(resource), 100);
	price_change = std::max(1, price_change);
	this->change_price(resource, price_change);
}

void CPlayer::decrease_resource_price(const resource *resource)
{
	int price_change = this->get_price(resource) / resource->get_base_price();
	price_change = std::max(1, price_change);
	this->change_price(resource, -price_change);
	this->set_price(resource, std::max(1, this->get_price(resource)));
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

		for (const resource *resource : resource::get_all()) {
			if (resource == defines::get()->get_time_resource()) {
				continue;
			}

			if (resource->get_base_price() == 0) {
				continue;
			}

			int convergence_increase = 100;

			if (this->get_price(resource) < player.get_price(resource) && convergences < max_convergences) {
				this->increase_resource_price(resource);
				convergences += convergence_increase;
				converged = true;

				if (this->get_price(resource) < player.get_price(resource) && convergences < max_convergences) {
					//now do the convergence for the other side as well, if possible
					player.decrease_resource_price(resource);
					convergences += convergence_increase;
					converged = true;
				}
			} else if (this->get_price(resource) > player.get_price(resource) && convergences < max_convergences) {
				this->decrease_resource_price(resource);
				convergences += convergence_increase;
				converged = true;

				if (this->get_price(resource) > player.get_price(resource) && convergences < max_convergences) { //do the convergence for the other side as well, if possible
					player.increase_resource_price(resource);
					convergences += convergence_increase;
					converged = true;
				}
			}
		}
	}
	
	return convergences;
}

int CPlayer::get_resource_price(const resource *resource) const
{
	if (resource == defines::get()->get_wealth_resource()) {
		return 100;
	}
	
	return this->get_price(resource);
}

/**
**  Get the effective resource demand for the player, given the current prices
**
**  @param resource  Resource.
*/
int CPlayer::get_effective_resource_demand(const resource *resource) const
{
	int resource_demand = this->get_resource_demand(resource);
	
	if (this->get_price(resource) != 0) {
		resource_demand *= resource->get_base_price();
		resource_demand /= this->get_price(resource);
	}
	
	if (resource->DemandElasticity != 100) {
		resource_demand = this->get_resource_demand(resource) + ((resource_demand - this->get_resource_demand(resource)) * resource->DemandElasticity / 100);
	}
	
	resource_demand = std::max(resource_demand, 0);

	return resource_demand;
}

/**
**  Get the effective sell price of a resource
*/
int CPlayer::get_effective_resource_sell_price(const resource *resource, const int traded_quantity) const
{
	if (resource == defines::get()->get_wealth_resource()) {
		return 100;
	}
	
	int price = traded_quantity * this->get_price(resource) / 100 * (100 - this->TradeCost) / 100;
	price = std::max(1, price);
	return price;
}

/**
**  Get the effective buy quantity of a resource
*/
int CPlayer::get_effective_resource_buy_price(const resource *resource, const int traded_quantity) const
{
	int price = traded_quantity * this->get_price(resource) / 100 * 100 / (100 - this->TradeCost);
	price = std::max(1, price);
	return price;
}

/**
**  Get the total price difference between this player and another one
*/
int CPlayer::GetTotalPriceDifferenceWith(const CPlayer &player) const
{
	int difference = 0;

	for (const resource *resource : resource::get_all()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (resource->get_base_price() == 0) {
			continue;
		}

		difference += abs(this->get_price(resource) - player.get_price(resource));
	}

	return difference;
}

/**
**  Get the trade potential between this player and another one
*/
int CPlayer::GetTradePotentialWith(const CPlayer &player) const
{
	int trade_potential = 0;

	for (const resource *resource : resource::get_all()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (resource->get_base_price() == 0) {
			continue;
		}

		const int price_difference = abs(this->get_price(resource) - player.get_price(resource));
		trade_potential += price_difference * 100;
	}

	trade_potential = std::max(trade_potential, 10);
	
	return trade_potential;
}

void CPlayer::pay_overlord_tax(const wyrmgus::resource *resource, const int taxable_quantity)
{
	if (this->get_overlord() == nullptr) {
		return;
	}

	//if the player has an overlord, give 10% of the resources gathered to them
	const int quantity = taxable_quantity / 10;

	if (quantity == 0) {
		return;
	}

	this->get_overlord()->change_resource(resource, quantity, true);
	this->get_overlord()->change_resource_total(resource, quantity);
	this->change_resource(resource, -quantity, true);

	//make the overlord pay tax to their overlord in turn (if they have one)
	this->get_overlord()->pay_overlord_tax(resource, quantity);
}

int CPlayer::GetUnitTotalCount(const wyrmgus::unit_type &type) const
{
	int count = this->GetUnitTypeCount(&type);
	for (std::vector<CUnit *>::const_iterator it = this->UnitBegin(); it != this->UnitEnd(); ++it) {
		//Wyrmgus start
		if (*it == nullptr) {
			fprintf(stderr, "Error in CPlayer::GetUnitTotalCount: unit of player %d is null.\n", this->get_index());
			continue;
		}
		//Wyrmgus end
		CUnit &unit = **it;

		if (unit.CurrentAction() == UnitAction::UpgradeTo) {
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
int CPlayer::CheckLimits(const wyrmgus::unit_type &type) const
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
//	if (this->Demand + type.Stats[this->get_index()].Variables[DEMAND_INDEX].Value > this->Supply && type.Stats[this->get_index()].Variables[DEMAND_INDEX].Value) {
	if (this->Demand + (type.Stats[this->get_index()].Variables[DEMAND_INDEX].Value * (type.TrainQuantity ? type.TrainQuantity : 1)) > this->Supply && type.Stats[this->get_index()].Variables[DEMAND_INDEX].Value) {
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
int CPlayer::CheckCosts(const resource_map<int> &costs, const bool notify) const
{
	bool sound_played = false;
	int err = 0;

	for (const auto &[resource, cost] : costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		const int resource_index = resource->get_index();

		if (this->get_resource(resource) + this->get_stored_resource(resource) >= cost) {
			continue;
		}

		if (notify) {
			const std::string &name = resource->get_identifier();
			const std::string &action_name = resource->get_action_name();

			Notify(_("Not enough %s... %s more %s."), _(name.c_str()), _(action_name.c_str()), _(name.c_str()));

			if (this == CPlayer::GetThisPlayer() && this->get_civilization() != nullptr && !sound_played) {
				const wyrmgus::sound *sound = this->get_civilization()->get_not_enough_resource_sound(resource);
				if (sound != nullptr) {
					sound_played = true;
					PlayGameSound(sound, MaxSampleVolume);
				}
			}
		}

		err |= 1 << resource_index;
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
int CPlayer::CheckUnitType(const wyrmgus::unit_type &type, bool hire) const
{
	//Wyrmgus start
//	return this->CheckCosts(type.Stats[this->get_index()].Costs);
	const resource_map<int> type_costs = this->GetUnitTypeCosts(&type, hire);
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
		change_resource(wyrmgus::resource::get_all()[i], costs[i], false);
	}
}

/**
**  Add the costs of an unit type to resources
**
**  @param type    Type of unit.
*/
void CPlayer::AddUnitType(const wyrmgus::unit_type &type, bool hire)
{
	//Wyrmgus start
//	AddCosts(type.Stats[this->get_index()].Costs);
	const resource_map<int> type_costs = this->GetUnitTypeCosts(&type, hire);
	AddCostsFactor(type_costs, 100);
	//Wyrmgus end
}

/**
**  Add a factor of costs to the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::AddCostsFactor(const resource_map<int> &costs, const int factor)
{
	if (factor == 0) {
		return;
	}
	
	for (const auto &[resource, cost] : costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		change_resource(resource, cost * factor / 100, true);
	}
}

void CPlayer::subtract_costs(const resource_map<int> &costs)
{
	for (const auto &[resource, cost] : costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		this->change_resource(resource, -cost, true);
	}
}

/**
**  Subtract the costs of new unit from resources
**
**  @param type    Type of unit.
*/
void CPlayer::SubUnitType(const wyrmgus::unit_type &type, bool hire)
{
	//Wyrmgus start
//	this->SubCosts(type.Stats[this->get_index()].Costs);
	const resource_map<int> type_costs = this->GetUnitTypeCosts(&type, hire);
	this->SubCostsFactor(type_costs, 100);
	//Wyrmgus end
}

/**
**  Subtract a factor of costs from the resources
**
**  @param costs   How many costs.
**  @param factor  Factor of the costs to apply.
*/
void CPlayer::SubCostsFactor(const resource_map<int> &costs, const int factor)
{
	for (const auto &[resource, cost] : costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		this->change_resource(resource, -cost * 100 / factor, false);
	}
}

//Wyrmgus start
/**
**  Gives the cost of a unit type for the player
*/
resource_map<int> CPlayer::GetUnitTypeCosts(const unit_type *type, const bool hire, const bool ignore_one) const
{
	resource_map<int> costs;

	if (hire) {
		costs[defines::get()->get_wealth_resource()] = type->Stats[this->get_index()].get_price();
	} else {
		costs = type->Stats[this->get_index()].get_costs();
	}

	for (auto &[resource, cost] : costs) {
		if (type->TrainQuantity != 0) {
			cost *= type->TrainQuantity;
		}

		if (type->CostModifier != 0) {
			int type_count = this->GetUnitTypeCount(type) + this->GetUnitTypeUnderConstructionCount(type);
			if (ignore_one) {
				type_count--;
			}

			for (int j = 0; j < type_count; ++j) {
				cost *= 100 + type->CostModifier;
				cost /= 100;
			}
		}
	}

	return costs;
}

int CPlayer::GetUnitTypeCostsMask(const wyrmgus::unit_type *type, bool hire) const
{
	int costs_mask = 0;
	
	const resource_map<int> type_costs = this->GetUnitTypeCosts(type, hire);
	
	for (const auto &[resource, cost] : type_costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (cost > 0) {
			costs_mask |= 1 << resource->get_index();
		}
	}
	
	return costs_mask;
}

/**
**  Gives the cost of an upgrade for the player
*/
resource_map<int> CPlayer::GetUpgradeCosts(const CUpgrade *upgrade) const
{
	resource_map<int> costs = upgrade->get_costs();

	for (const auto &[resource, scaled_cost] : upgrade->get_scaled_costs()) {
		for (const wyrmgus::unit_type *unit_type : upgrade->get_scaled_cost_unit_types()) {
			costs[resource] += scaled_cost * this->GetUnitTypeCount(unit_type);
		}

		for (const unit_class *unit_class : upgrade->get_scaled_cost_unit_classes()) {
			costs[resource] += scaled_cost * this->get_unit_class_count(unit_class);
		}
	}

	return costs;
}

int CPlayer::GetUpgradeCostsMask(const CUpgrade *upgrade) const
{
	int costs_mask = 0;
	
	const resource_map<int> upgrade_costs = AiPlayer->Player->GetUpgradeCosts(upgrade);
	
	for (const auto &[resource, cost] : upgrade_costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (cost > 0) {
			costs_mask |= 1 << resource->get_index();
		}
	}
	
	return costs_mask;
}

//Wyrmgus end

void CPlayer::SetUnitTypeCount(const wyrmgus::unit_type *type, int quantity)
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

void CPlayer::ChangeUnitTypeCount(const wyrmgus::unit_type *type, int quantity)
{
	this->SetUnitTypeCount(type, this->GetUnitTypeCount(type) + quantity);
}

int CPlayer::GetUnitTypeCount(const wyrmgus::unit_type *type) const
{
	if (type != nullptr && this->UnitTypesCount.find(type) != this->UnitTypesCount.end()) {
		return this->UnitTypesCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::SetUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type, int quantity)
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

void CPlayer::ChangeUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type, int quantity)
{
	this->SetUnitTypeUnderConstructionCount(type, this->GetUnitTypeUnderConstructionCount(type) + quantity);
}

int CPlayer::GetUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type) const
{
	if (type && this->UnitTypesUnderConstructionCount.find(type) != this->UnitTypesUnderConstructionCount.end()) {
		return this->UnitTypesUnderConstructionCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::SetUnitTypeAiActiveCount(const wyrmgus::unit_type *type, int quantity)
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

void CPlayer::ChangeUnitTypeAiActiveCount(const wyrmgus::unit_type *type, int quantity)
{
	this->SetUnitTypeAiActiveCount(type, this->GetUnitTypeAiActiveCount(type) + quantity);
}

int CPlayer::GetUnitTypeAiActiveCount(const wyrmgus::unit_type *type) const
{
	if (type && this->UnitTypesAiActiveCount.find(type) != this->UnitTypesAiActiveCount.end()) {
		return this->UnitTypesAiActiveCount.find(type)->second;
	} else {
		return 0;
	}
}

void CPlayer::IncreaseCountsForUnit(CUnit *unit, const bool type_change)
{
	const wyrmgus::unit_type *type = unit->Type;

	this->ChangeUnitTypeCount(type, 1);
	this->units_by_type[type].push_back(unit);

	if (type->get_unit_class() != nullptr) {
		this->units_by_class[type->get_unit_class()].push_back(unit);
	}
	
	if (unit->Active) {
		this->ChangeUnitTypeAiActiveCount(type, 1);
		this->AiActiveUnitsByType[type].push_back(unit);
	}

	if (type->BoolFlag[TOWNHALL_INDEX].value) {
		this->NumTownHalls++;
	}
	
	for (const auto &[resource, quantity] : type->Stats[this->get_index()].get_resource_demands()) {
		this->change_resource_demand(resource, quantity);
	}
	
	if (this->AiEnabled && type->BoolFlag[COWARD_INDEX].value && !type->BoolFlag[HARVESTER_INDEX].value && !type->CanTransport() && type->Spells.size() == 0 && CMap::get()->Info->IsPointOnMap(unit->tilePos, unit->MapLayer) && unit->CanMove() && unit->Active && unit->GroupId != 0 && unit->Variable[SIGHTRANGE_INDEX].Value > 0) { //assign coward, non-worker, non-transporter, non-spellcaster units to be scouts
		this->Ai->Scouts.push_back(unit);
	}
	
	if (!type_change) {
		if (unit->get_character() != nullptr) {
			this->Heroes.push_back(unit);
		}

		if (!SaveGameLoading) {
			this->last_created_unit = unit;
		}
	}
}

void CPlayer::DecreaseCountsForUnit(CUnit *unit, const bool type_change)
{
	const wyrmgus::unit_type *type = unit->Type;

	this->ChangeUnitTypeCount(type, -1);
	
	vector::remove(this->units_by_type[type], unit);

	if (this->units_by_type[type].empty()) {
		this->units_by_type.erase(type);
	}

	if (type->get_unit_class() != nullptr) {
		vector::remove(this->units_by_class[type->get_unit_class()], unit);

		if (this->units_by_class[type->get_unit_class()].empty()) {
			this->units_by_class.erase(type->get_unit_class());
		}
	}
	
	if (unit->Active) {
		this->ChangeUnitTypeAiActiveCount(type, -1);
		
		vector::remove(this->AiActiveUnitsByType[type], unit);
		
		if (this->AiActiveUnitsByType[type].empty()) {
			this->AiActiveUnitsByType.erase(type);
		}
	}
	
	if (type->BoolFlag[TOWNHALL_INDEX].value) {
		this->NumTownHalls--;
	}
	
	for (const auto &[resource, quantity] : type->Stats[this->get_index()].get_resource_demands()) {
		this->change_resource_demand(resource, -quantity);
	}
	
	if (this->AiEnabled && this->Ai && std::find(this->Ai->Scouts.begin(), this->Ai->Scouts.end(), unit) != this->Ai->Scouts.end()) {
		this->Ai->Scouts.erase(std::remove(this->Ai->Scouts.begin(), this->Ai->Scouts.end(), unit), this->Ai->Scouts.end());
	}
	
	if (!type_change) {
		if (unit->get_character() != nullptr) {
			this->Heroes.erase(std::remove(this->Heroes.begin(), this->Heroes.end(), unit), this->Heroes.end());
		}

		if (unit == this->last_created_unit) {
			this->last_created_unit = nullptr;
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
bool CPlayer::has_unit_type(const wyrmgus::unit_type *unit_type) const
{
	return this->GetUnitTypeCount(unit_type) > 0;
}

int CPlayer::get_population() const
{
	int people_count = 0;

	for (const auto &kv_pair : this->UnitTypesCount) {
		const wyrmgus::unit_type *unit_type = kv_pair.first;
		if (unit_type->BoolFlag[BUILDING_INDEX].value || unit_type->BoolFlag[FAUNA_INDEX].value) {
			continue;
		}

		people_count += kv_pair.second;
	}

	return people_count * defines::get()->get_population_per_unit();
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
		try {
			const qunique_ptr<CPlayer> &p = CPlayer::Players[player];

			if (p->LostTownHallTimer && !p->is_revealed() && p->LostTownHallTimer < ((int) GameCycle) && CPlayer::GetThisPlayer()->HasContactWith(*p)) {
				p->set_revealed(true);
				for (int j = 0; j < NumPlayers; ++j) {
					if (player != j && CPlayer::Players[j]->get_type() != player_type::nobody) {
						CPlayer::Players[j]->Notify(_("%s's units have been revealed!"), p->get_name().c_str());
					} else {
						CPlayer::Players[j]->Notify("%s", _("Your units have been revealed!"));
					}
				}
			}


			for (size_t i = 0; i < p->Modifiers.size(); ++i) { //if already has the modifier, make it have the greater duration of the new or old one
				if ((unsigned long) p->Modifiers[i].second < GameCycle) {
					p->RemoveModifier(p->Modifiers[i].first); //only remove one modifier per cycle, to prevent too many upgrade changes from happening at the same cycle (for performance reasons)
					break;
				}
			}

			if (p->HeroCooldownTimer) {
				p->HeroCooldownTimer--;
			}

			if (p->AiEnabled) {
				AiEachCycle(*p);
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error executing the per cycle actions for player " + std::to_string(player) + "."));
		}
	}
}

/**
**  Handle AI of a player each second.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachSecond(const int playerIdx)
{
	try {
		const qunique_ptr<CPlayer> &player = CPlayer::Players[playerIdx];

		if ((GameCycle / CYCLES_PER_SECOND) % 10 == 0) {
			for (const resource *resource : resource::get_all()) {
				int revenue = player->get_resource(resource) + player->get_stored_resource(resource) - player->get_last_resource(resource);
				//estimate per minute
				revenue *= 6;
				player->set_revenue(resource, revenue);
				player->set_last_resource(resource, player->get_resource(resource) + player->get_stored_resource(resource));
			}
		}

		if (player->AiEnabled) {
			AiEachSecond(*player);
		}

		player->UpdateFreeWorkers();
		//Wyrmgus start
		player->PerformResourceTrade();
		//Wyrmgus end
		player->update_current_quests();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing the per second actions for player " + std::to_string(playerIdx) + "."));
	}
}

/**
**  Handle AI of a player each half minute.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachHalfMinute(const int playerIdx)
{
	try {
		const qunique_ptr<CPlayer> &player = CPlayer::Players[playerIdx];

		if (player->AiEnabled) {
			AiEachHalfMinute(*player);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing the per half minute actions for player " + std::to_string(playerIdx) + "."));
	}
}

/**
**  Handle AI of a player each minute.
**
**  @param playerIdx  the player to update AI
*/
void PlayersEachMinute(const int playerIdx)
{
	try {
		const qunique_ptr<CPlayer> &player = CPlayer::Players[playerIdx];

		if (player->AiEnabled) {
			AiEachMinute(*player);
		}

		player->update_quest_pool();

		player->clear_recent_trade_partners();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing the per minute actions for player " + std::to_string(playerIdx) + "."));
	}
}

/**
**  Setup the player colors for the current palette.
**
**  @todo  FIXME: could be called before PixelsXX is setup.
*/
void SetPlayersPalette()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->get_faction() == nullptr) {
			CPlayer::Players[i]->player_color = vector::get_random(player_color::get_all());
		}
	}

	CPlayer::get_neutral_player()->player_color = defines::get()->get_neutral_player_color();
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
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));
	std::array<char, 128> temp{};
	uint32_t color;
	va_list va;

	// Notify me, and my TEAM members
	if (this != CPlayer::GetThisPlayer() && !IsTeamed(*CPlayer::GetThisPlayer())) {
		return;
	}

	va_start(va, fmt);
	temp[temp.size() - 1] = '\0';
	vsnprintf(temp.data(), temp.size() - 1, fmt, va);
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

	UI.get_minimap()->AddEvent(pos, z, color);

	if (this == CPlayer::GetThisPlayer()) {
		//Wyrmgus start
//		SetMessageEvent(pos, "%s", temp.data());
		SetMessageEvent(pos, z, "%s", temp.data());
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		SetMessageEvent(pos, "(%s): %s", this->get_name().c_str(), temp.data());
		SetMessageEvent(pos, z, "(%s): %s", this->get_name().c_str(), temp.data());
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
	if (this != CPlayer::GetThisPlayer() && !IsTeamed(*CPlayer::GetThisPlayer())) {
		return;
	}
	std::array<char, 128> temp{};
	va_list va;

	va_start(va, fmt);
	temp[temp.size() - 1] = '\0';
	vsnprintf(temp.data(), temp.size() - 1, fmt, va);
	va_end(va);
	if (this == CPlayer::GetThisPlayer()) {
		SetMessage("%s", temp.data());
	} else {
		SetMessage("(%s): %s", this->get_name().c_str(), temp.data());
	}
}

void CPlayer::set_neutral_diplomatic_stance_with(const CPlayer *player)
{
	std::unique_lock<std::shared_mutex> lock(this->mutex);

	this->enemies.erase(player->get_index());
	this->allies.erase(player->get_index());

	emit diplomatic_stances_changed();

	if (GameCycle > 0 && player == CPlayer::GetThisPlayer()) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to neutral"), _(this->get_name().c_str()));
	}
}

void CPlayer::set_neutral_diplomatic_stance_with_async(CPlayer *player)
{
	const int index = this->get_index();
	const int other_index = player->get_index();

	engine_interface::get()->post([index, other_index]() {
		SendCommandDiplomacy(index, diplomacy_state::neutral, other_index);
	});
}

void CPlayer::set_allied_diplomatic_stance_with(const CPlayer *player)
{
	std::unique_lock<std::shared_mutex> lock(this->mutex);

	this->enemies.erase(player->get_index());
	this->allies.insert(player->get_index());

	emit diplomatic_stances_changed();

	if (GameCycle > 0 && player == CPlayer::GetThisPlayer()) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to allied"), _(this->get_name().c_str()));
	}
}

void CPlayer::set_allied_diplomatic_stance_with_async(CPlayer *player)
{
	const int index = this->get_index();
	const int other_index = player->get_index();

	engine_interface::get()->post([index, other_index]() {
		SendCommandDiplomacy(index, diplomacy_state::allied, other_index);
	});
}

void CPlayer::set_enemy_diplomatic_stance_with(CPlayer *player)
{
	{
		std::unique_lock<std::shared_mutex> lock(this->mutex);

		this->enemies.insert(player->get_index());
		this->allies.erase(player->get_index());

		emit diplomatic_stances_changed();
	}

	if (GameCycle > 0) {
		if (player == CPlayer::GetThisPlayer()) {
			CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to enemy"), _(this->get_name().c_str()));
		} else if (this == CPlayer::GetThisPlayer()) {
			CPlayer::GetThisPlayer()->Notify(_("We have changed our diplomatic stance with %s to enemy"), _(player->get_name().c_str()));
		}
	}

	if (this->has_shared_vision_with(player)) {
		CommandSharedVision(this->get_index(), false, player->get_index());
	}

	// if either player is the overlord of another (indirect or otherwise), break the vassalage bond after the declaration of war
	if (this->is_overlord_of(player)) {
		player->set_overlord(nullptr, vassalage_type::none);
	} else if (player->is_overlord_of(this)) {
		this->set_overlord(nullptr, vassalage_type::none);
	}

	//if the other player has an overlord, then we must also go to war with them
	if (player->get_overlord() != nullptr) {
		this->set_enemy_diplomatic_stance_with(player->get_overlord());
	}
}

void CPlayer::set_enemy_diplomatic_stance_with_async(CPlayer *player)
{
	const int index = this->get_index();
	const int other_index = player->get_index();

	engine_interface::get()->post([index, other_index]() {
		SendCommandDiplomacy(index, diplomacy_state::enemy, other_index);
	});
}

void CPlayer::SetDiplomacyCrazyWith(const CPlayer &player)
{
	this->enemies.insert(player.get_index());
	this->allies.insert(player.get_index());
	
	emit diplomatic_stances_changed();

	if (GameCycle > 0 && &player == CPlayer::GetThisPlayer()) {
		CPlayer::GetThisPlayer()->Notify(_("%s changed their diplomatic stance with us to Crazy"), _(this->get_name().c_str()));
	}
}

void CPlayer::set_shared_vision_with(CPlayer *player, const bool shared_vision)
{
	if (shared_vision == this->has_shared_vision_with(player)) {
		return;
	}

	std::unique_lock<std::shared_mutex> lock(this->mutex);

	if (shared_vision) {
		this->shared_vision.insert(player->get_index());

		if (player->has_shared_vision_with(this)) {
			this->mutual_shared_vision.insert(player->get_index());
			player->mutual_shared_vision.insert(this->get_index());
		}

		if (GameCycle > 0 && player == CPlayer::GetThisPlayer()) {
			CPlayer::GetThisPlayer()->Notify(_("%s is now sharing vision with us"), _(this->get_name().c_str()));
		}
	} else {
		this->shared_vision.erase(player->get_index());

		if (player->has_shared_vision_with(this)) {
			this->mutual_shared_vision.erase(player->get_index());
			player->mutual_shared_vision.erase(this->get_index());
		}

		if (GameCycle > 0 && player == CPlayer::GetThisPlayer()) {
			CPlayer::GetThisPlayer()->Notify(_("%s is no longer sharing vision with us"), _(this->get_name().c_str()));
		}
	}

	emit shared_vision_changed();
}

void CPlayer::set_shared_vision_with_async(CPlayer *player, const bool shared_vision)
{
	const int index = this->get_index();
	const int other_index = player->get_index();

	engine_interface::get()->post([index, other_index, shared_vision]() {
		SendCommandSharedVision(index, shared_vision, other_index);
	});
}

void CPlayer::set_overlord(CPlayer *overlord, const wyrmgus::vassalage_type)
{
	if (overlord == this->get_overlord()) {
		return;
	}

	if (overlord != nullptr && overlord->get_overlord() == this) {
		throw std::runtime_error("Cannot set player \"" + overlord->get_name() + "\" as the overlord of \"" + this->get_name() + "\", as the former is a vassal of the latter, and a vassal can't be the overlord of its own overlord.");
	}

	CPlayer *old_overlord = this->get_overlord();
	if (old_overlord != nullptr) {
		vector::remove(old_overlord->vassals, this);

		//remove alliance and shared vision with the old overlord, and any upper overlords
		if (!SaveGameLoading) {
			while (old_overlord != nullptr) {
				this->break_overlordship_alliance(old_overlord);
				old_overlord = old_overlord->get_overlord();
			}
		}
	}

	this->overlord = overlord;
	this->vassalage_type = vassalage_type;

	if (overlord != nullptr) {
		overlord->vassals.push_back(this);

		//establish alliance and shared vision with the new overlord, and any upper overlords
		if (!SaveGameLoading) {
			while (overlord != nullptr) {
				this->establish_overlordship_alliance(overlord);
				overlord = overlord->get_overlord();
			}
		}
	}

	this->update_territory_tiles();
}

void CPlayer::establish_overlordship_alliance(CPlayer *overlord)
{
	this->set_allied_diplomatic_stance_with(overlord);
	overlord->set_allied_diplomatic_stance_with(this);
	CommandSharedVision(this->get_index(), true, overlord->get_index());
	CommandSharedVision(overlord->get_index(), true, this->get_index());

	//vassals should also be allied with overlords higher up in the hierarchy
	for (CPlayer *vassal : this->get_vassals()) {
		vassal->establish_overlordship_alliance(overlord);
	}
}

void CPlayer::break_overlordship_alliance(CPlayer *overlord)
{
	if (this->is_allied_with(*overlord)) {
		this->set_neutral_diplomatic_stance_with(overlord);
		overlord->set_neutral_diplomatic_stance_with(this);
	}
	CommandSharedVision(this->get_index(), false, overlord->get_index());
	CommandSharedVision(overlord->get_index(), false, this->get_index());

	//vassals should also have their alliances with overlords higher up in the hierarchy broken
	for (CPlayer *vassal : this->get_vassals()) {
		vassal->break_overlordship_alliance(overlord);
	}
}

/**
**  Check if the player is an enemy
*/
bool CPlayer::is_enemy_of(const CPlayer &player) const
{
	if (this->get_overlord() != nullptr && this->get_overlord()->is_enemy_of(player)) {
		return true;
	}

	if (&player == this) {
		return false;
	}

	//be hostile to the other player if they are hostile, even if the diplomatic stance hasn't been changed
	return this->has_enemy_stance_with(player.get_index()) || player.has_enemy_stance_with(this->get_index());
}

/**
**  Check if the unit is an enemy
*/
bool CPlayer::is_enemy_of(const CUnit &unit) const
{
	if (
		unit.Player->get_type() == player_type::neutral
		&& (unit.Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value || unit.Type->BoolFlag[PREDATOR_INDEX].value)
		&& this->get_type() != player_type::neutral
	) {
		return true;
	}
	
	if (
		this != unit.Player
		&& this->get_type() != player_type::neutral
		&& unit.CurrentAction() == UnitAction::Attack
		&& unit.CurrentOrder()->has_goal()
		&& unit.CurrentOrder()->get_goal()->Player == this
		&& !unit.CurrentOrder()->get_goal()->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value
	) {
		return true;
	}
	
	if (unit.Player != this && this->get_type() != player_type::neutral && unit.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive() && !this->has_neutral_faction_type()) {
		return true;
	}
	
	return this->is_enemy_of(*unit.Player);
}

/**
**  Check if the player is an ally
*/
bool CPlayer::is_allied_with(const CPlayer &player) const
{
	if (&player == this) {
		return false;
	}

	//only consider yourself to be the ally of another player if they have the allied stance with you as well
	return this->has_allied_stance_with(player.get_index()) && player.has_allied_stance_with(this->get_index());
}

/**
**  Check if the unit is an ally
*/
bool CPlayer::is_allied_with(const CUnit &unit) const
{
	return this->is_allied_with(*unit.Player);
}

bool CPlayer::has_shared_vision_with(const CPlayer *player) const
{
	return this->has_shared_vision_with(player->get_index());
}

bool CPlayer::has_shared_vision_with(const CUnit &unit) const
{
	return this->has_shared_vision_with(unit.Player);
}

bool CPlayer::is_vision_sharing() const
{
	return !this->get_shared_vision().empty();
}

bool CPlayer::has_mutual_shared_vision_with(const CPlayer *player) const
{
	return this->has_mutual_shared_vision_with(player->get_index());
}

bool CPlayer::has_mutual_shared_vision_with(const CUnit &unit) const
{
	return this->has_mutual_shared_vision_with(unit.Player);
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
	return player.StartMapLayer == this->StartMapLayer || (player.StartMapLayer < (int) CMap::get()->MapLayers.size() && this->StartMapLayer < (int) CMap::get()->MapLayers.size() && CMap::get()->MapLayers[player.StartMapLayer]->world == CMap::get()->MapLayers[this->StartMapLayer]->world);
}

/**
**  Check if the player's faction type is a neutral one
*/
bool CPlayer::has_neutral_faction_type() const
{
	if (
		this->get_civilization() != nullptr
		&& this->get_faction() != nullptr
		&& (is_faction_type_neutral(this->get_faction()->get_type()))
	) {
		return true;
	}

	return false;
}

/**
**  Check if the player can use the buildings of another, for neutral building functions (i.e. unit training)
*/
bool CPlayer::has_building_access(const CPlayer *player, const ButtonCmd button_action) const
{
	if (player->is_enemy_of(*this)) {
		return false;
	}

	if (player->get_type() == player_type::neutral) {
		return true;
	}

	if (player->get_faction() == nullptr) {
		return false;
	}

	if (
		player->has_neutral_faction_type()
		&& (player->get_overlord() == nullptr || this->is_any_overlord_of(player) || player->get_overlord()->is_allied_with(*this))
	) {
		if (player->get_faction()->get_type() != wyrmgus::faction_type::holy_order || (button_action != ButtonCmd::Train && button_action != ButtonCmd::TrainClass && button_action != ButtonCmd::Buy) || wyrmgus::vector::contains(this->Deities, player->get_faction()->get_holy_order_deity())) { //if the faction is a holy order, the player must have chosen its respective deity
			return true;
		}
	}

	return false;
}

bool CPlayer::has_building_access(const CPlayer *player) const
{
	return this->has_building_access(player, ButtonCmd::None);
}

bool CPlayer::has_building_access(const CUnit *unit, const ButtonCmd button_action) const
{
	if (unit->is_enemy_of(*this)) {
		return false;
	}

	return this->has_building_access(unit->Player, button_action);
}

bool CPlayer::has_building_access(const CUnit *unit) const
{
	return this->has_building_access(unit, ButtonCmd::None);
}

bool CPlayer::HasHero(const wyrmgus::character *hero) const
{
	if (!hero) {
		return false;
	}
	
	for (const CUnit *hero_unit : this->Heroes) {
		if (hero_unit->get_character() == hero) {
			return true;
		}
	}
	
	return false;
}
//Wyrmgus end
