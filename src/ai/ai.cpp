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
//      (c) Copyright 2000-2022 by Lutz Sammer, Ludovic Pollet,
//      Jimmy Salmon and Andrettin
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

/**
** @page AiModule Module - AI
**
** @section aibasics What is it?
**
** Stratagus uses a very simple scripted AI. There are no optimizations
** yet. The complete AI was written on one weekend.
** Until no AI specialist joins, I keep this AI.
**
** @subsection aiscripted What is scripted AI?
**
** The AI script tells the engine build 4 workers, than build 3 footman,
** than attack the player, than sleep 100 frames.
**
** @section API The AI API
**
** @subsection aimanage Management calls
**
** Manage the inititialse and cleanup of the AI players.
**
** ::InitAiModule()
**
** Initialise all global varaibles and structures.
** Called before AiInit, or before game loading.
**
** ::AiInit(::Player)
**
** Called for each player, to setup the AI structures
** Player::Aiin the player structure. It can use Player::AiName to
** select different AI's.
**
** ::CleanAi()
**
** Called to release all the memory for all AI structures.
** Must handle self which players contains AI structures.
**
** ::SaveAi(::FILE *)
**
** Save the AI structures of all players to file.
** Must handle self which players contains AI structures.
**
**
** @subsection aipcall Periodic calls
**
** This functions are called regular for all AI players.
**
** ::AiEachCycle(::Player)
**
** Called each game cycle, to handle quick checks, which needs
** less CPU.
**
** ::AiEachSecond(::Player)
**
** Called each second, to handle more CPU intensive things.
**
**
** @subsection aiecall Event call-backs
**
** This functions are called, when some special events happens.
**
** ::AiHelpMe()
**
** Called if a unit owned by the AI is attacked.
**
** ::AiUnitKilled()
**
** Called if a unit owned by the AI is killed.
**
** ::AiNeedMoreSupply()
**
** Called if an trained unit is ready, but not enough food is
** available for it.
**
** ::AiWorkComplete()
**
** Called if a unit has completed its work.
**
** ::AiCanNotBuild()
**
** Called if the AI unit can't build the requested unit-type.
**
** ::AiCanNotReach()
**
** Called if the AI unit can't reach the building place.
**
** ::AiTrainingComplete()
**
** Called if AI unit has completed training a new unit.
**
** ::AiUpgradeToComplete()
**
** Called if AI unit has completed upgrade to new unit-type.
**
** ::AiResearchComplete()
**
** Called if AI unit has completed research of an upgrade or spell.
*/

#include "stratagus.h"

#include "ai.h"
#include "ai_local.h"

#include "actions.h"
#include "action/action_attack.h"
#include "commands.h"
#include "database/defines.h"
//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "game/game.h"
#include "grand_strategy.h"
#include "iolib.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/landmass.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/tile.h"
//Wyrmgus start
#include "network/network.h"
//Wyrmgus end
#include "pathfinder/pathfinder.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_type.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "quest/quest.h"
#include "script.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/enum_util.h"
#include "util/log_util.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

int AiSleepCycles;              /// Ai sleeps # cycles

std::vector<std::unique_ptr<CAiType>> AiTypes; /// List of all AI types.
AiHelper AiHelpers;             /// AI helper variables

PlayerAi *AiPlayer;             /// Current AI player

void PlayerAi::check_factions()
{
	//check if any factions can be founded, and if so, pick one randomly
	if (this->Player->get_faction() != nullptr && this->Player->NumTownHalls > 0) {
		std::vector<const faction *> potential_factions;
		for (const faction *possible_faction : this->Player->get_potentially_foundable_factions()) {
			if (!this->Player->can_found_faction(possible_faction)) {
				continue;
			}

			potential_factions.push_back(possible_faction);
		}

		if (!potential_factions.empty()) {
			this->Player->set_faction(vector::get_random(potential_factions));
		}

		if (this->Player->get_dynasty() == nullptr) { //if the AI player has no dynasty, pick one if available
			std::vector<const dynasty *> potential_dynasties;
			for (const dynasty *dynasty : this->Player->get_faction()->get_dynasties()) {
				if (!this->Player->can_choose_dynasty(dynasty)) {
					continue;
				}

				potential_dynasties.push_back(dynasty);
			}

			if (!potential_dynasties.empty()) {
				this->Player->set_dynasty(vector::get_random(potential_dynasties));
			}
		}
	}
}

void PlayerAi::evaluate_diplomacy()
{
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	if (game::get()->get_current_campaign() == nullptr) {
		//only evaluate diplomacy in the campaign mode
		return;
	}

	//evaluate whether the AI's diplomacy should change
	CPlayer *player = this->Player;

	const bool territorial_player = player->NumTownHalls > 0 || player->is_revealed();

	//if we have aren't a territorial player, declare war on the owner of our start tile, if anyone owns it
	if (!territorial_player) {
		const tile *start_tile = CMap::get()->Field(player->StartPos, player->StartMapLayer);
		CPlayer *tile_owner = start_tile->get_owner();

		if (tile_owner != nullptr && GameCycle > PlayerAi::enforced_peace_cycle_count) {
			assert_throw(tile_owner != player);

			if (!player->has_enemy_stance_with(tile_owner) && player->can_declare_war_on(tile_owner)) {
				player->set_enemy_diplomatic_stance_with(tile_owner);
			}
		}

		return;
	}

	if (player->NumTownHalls > 0) {
		//check if we should declare war against another player
		if (!player->at_war() && GameCycle > PlayerAi::enforced_peace_cycle_count) {
			if (player->is_independent()) {
				std::vector<CPlayer *> border_players = container::to_vector(player->get_border_players());
				vector::shuffle(border_players);

				for (CPlayer *border_player : border_players) {
					if (!player->is_player_capital_explored(border_player)) {
						//don't declare war on players for which we don't have their capital location explored, as they might be unreachable (e.g. in a world or landmass we have no way to get to)
						continue;
					}

					if (!player->can_declare_war_on(border_player)) {
						continue;
					}

					if (player->get_military_score_percent_advantage_over(border_player) >= PlayerAi::military_score_advantage_threshold) {
						player->set_enemy_diplomatic_stance_with(border_player);
						break;
					}
				}
			} else {
				CPlayer *overlord = player->get_overlord();

				assert_throw(overlord != nullptr);

				if (player->can_declare_war_on(overlord) && player->has_military_advantage_over(overlord)) {
					player->set_enemy_diplomatic_stance_with(overlord);
				}
			}
		}

		//declare war on revealed players with units in our territory
		for (const int revealed_player_index : CPlayer::get_revealed_player_indexes()) {
			CPlayer *revealed_player = CPlayer::Players[revealed_player_index].get();

			if (player->has_enemy_stance_with(revealed_player)) {
				//already an enemy
				continue;
			}

			if (!player->can_declare_war_on(revealed_player) || !player->has_military_advantage_over(revealed_player)) {
				continue;
			}

			bool unit_in_territory = false;
			for (const CUnit *revealed_player_unit : revealed_player->get_units()) {
				const CPlayer *unit_tile_owner = revealed_player_unit->get_center_tile_owner();

				if (unit_tile_owner == player) {
					unit_in_territory = true;
					break;
				}
			}

			if (unit_in_territory) {
				player->set_enemy_diplomatic_stance_with(revealed_player);
			}
		}
	}

	//check if any players we have a hostile stance to do not have a hostile stance with us, and if so, whether we should accept peace

	//copy the set, as we may modify the original one during the loop
	const player_index_set enemies = player->get_enemies();

	for (const int enemy_player_index : enemies) {
		const CPlayer *enemy_player = CPlayer::Players[enemy_player_index].get();

		if (enemy_player->has_enemy_stance_with(player)) {
			continue;
		}

		if (enemy_player->has_military_advantage_over(player)) {
			player->set_neutral_diplomatic_stance_with(enemy_player);
		}
	}
}

void PlayerAi::check_quest_objectives()
{
	//iterate through the quest objectives, and affect the AI accordingly

	for (const auto &objective : this->Player->get_quest_objectives()) {
		objective->check_ai();
	}
}

/**
**  Execute the AI Script.
*/
static void AiExecuteScript()
{
	if (AiPlayer->Script.empty()) {
		return;
	}
	lua_getglobal(Lua, "_ai_scripts_");
	lua_pushstring(Lua, AiPlayer->Script.c_str());
	lua_rawget(Lua, -2);
	LuaCall(0, 1);
	lua_pop(Lua, 1);
}

/**
**  Check if everything is fine, send new requests to resource manager.
*/
static void AiCheckUnits()
{
	//  Count the already made build requests.
	int counter[UnitTypeMax];
	AiGetBuildRequestsCount(*AiPlayer, counter);

	//  Look if some unit-types are missing.
	int n = AiPlayer->UnitTypeRequests.size();
	for (int i = 0; i < n; ++i) {
		const unsigned int t = AiPlayer->UnitTypeRequests[i].Type->Slot;
		const int x = AiPlayer->UnitTypeRequests[i].Count;
		const wyrmgus::unit_class *unit_class = AiPlayer->UnitTypeRequests[i].Type->get_unit_class();

		// Add equivalent units
		int e = AiPlayer->Player->GetUnitTypeAiActiveCount(AiPlayer->UnitTypeRequests[i].Type);
		if (t < AiHelpers.Equiv.size()) {
			for (unsigned int j = 0; j < AiHelpers.Equiv[t].size(); ++j) {
				e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][j]);
			}
		}
		if (unit_class != nullptr) {
			for (const wyrmgus::unit_type *class_unit_type : unit_class->get_unit_types()) {
				if (class_unit_type != AiPlayer->UnitTypeRequests[i].Type) {
					e += AiPlayer->Player->GetUnitTypeAiActiveCount(class_unit_type);
				}
			}
		}
		const int requested = x - e - counter[t];
		if (requested > 0) {  // Request it.
			//Wyrmgus start
//			AiAddUnitTypeRequest(*AiPlayer->UnitTypeRequests[i].Type, requested);
			AiAddUnitTypeRequest(*AiPlayer->UnitTypeRequests[i].Type, requested, AiPlayer->UnitTypeRequests[i].landmass);
			//Wyrmgus end
			counter[t] += requested;
		}
		counter[t] -= x;
	}

	AiPlayer->Force.CheckUnits(counter);

	//  Look if some upgrade-to are missing.
	n = AiPlayer->UpgradeToRequests.size();
	for (int i = 0; i < n; ++i) {
		const unsigned int t = AiPlayer->UpgradeToRequests[i]->Slot;
		const int x = 1;

		//  Add equivalent units
		int e = AiPlayer->Player->GetUnitTypeAiActiveCount(AiPlayer->UpgradeToRequests[i]);
		if (t < AiHelpers.Equiv.size()) {
			for (unsigned int j = 0; j < AiHelpers.Equiv[t].size(); ++j) {
				e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][j]);
			}
		}

		const int requested = x - e - counter[t];
		if (requested > 0) {  // Request it.
			AiAddUpgradeToRequest(*AiPlayer->UpgradeToRequests[i]);
			counter[t] += requested;
		}
		counter[t] -= x;
	}

	//  Look if some researches are missing.
	for (const CUpgrade *requested_upgrade : AiPlayer->get_research_requests()) {
		if (UpgradeIdAllowed(*AiPlayer->Player, requested_upgrade->ID) == 'A') {
			AiAddResearchRequest(requested_upgrade);
		}
	}
	
	//Wyrmgus start
	if (AiPlayer->Player->NumTownHalls > 0 && !AiPlayer->Player->has_neutral_faction_type()) {
		//check if can hire any heroes
		if (AiPlayer->Player->Heroes.size() < CPlayer::max_heroes && AiPlayer->Player->HeroCooldownTimer == 0 && !IsNetworkGame() && CurrentQuest == nullptr) {
			for (int i = 0; i < AiPlayer->Player->GetUnitCount(); ++i) {
				CUnit *hero_recruiter = &AiPlayer->Player->GetUnit(i);
				if (!hero_recruiter || !hero_recruiter->IsAliveOnMap() || !hero_recruiter->Type->BoolFlag[RECRUITHEROES_INDEX].value || hero_recruiter->CurrentAction() == UnitAction::Built) {
					continue;
				}
				
				for (CUnit *hero : hero_recruiter->SoldUnits) {
					if (!hero_recruiter->Player->is_character_available_for_recruitment(hero->get_character(), true)) {
						continue;
					}

					resource_map<int> buy_costs;
					buy_costs[defines::get()->get_wealth_resource()] = hero->GetPrice();

					if (!AiPlayer->Player->CheckCosts(buy_costs) && AiPlayer->Player->check_limits<false>(*hero->Type, hero_recruiter) == check_limits_result::success) {
						CommandBuy(*hero_recruiter, hero, AiPlayer->Player->get_index());
						break;
					}
				}
			}
		}
		
		//check if can hire any mercenaries
		for (int i = 0; i < PlayerMax; ++i) {
			if (i == AiPlayer->Player->get_index()) {
				continue;
			}

			const CPlayer *other_player = CPlayer::Players[i].get();

			if (other_player->get_type() != player_type::computer || !AiPlayer->Player->has_building_access(other_player)) {
				continue;
			}
			for (int j = 0; j < other_player->GetUnitCount(); ++j) {
				CUnit *mercenary_building = &other_player->GetUnit(j);
				if (!mercenary_building || !mercenary_building->IsAliveOnMap() || !mercenary_building->Type->BoolFlag[BUILDING_INDEX].value || !mercenary_building->IsVisible(*AiPlayer->Player)) {
					continue;
				}

				if (AiPlayer->Player->Heroes.size() < CPlayer::max_heroes && AiPlayer->Player->HeroCooldownTimer == 0 && mercenary_building->Type->BoolFlag[RECRUITHEROES_INDEX].value && !IsNetworkGame() && CurrentQuest == nullptr) { //check if can hire any heroes at the mercenary camp
					for (CUnit *mercenary_hero : mercenary_building->SoldUnits) {
						if (!mercenary_building->Player->is_character_available_for_recruitment(mercenary_hero->get_character(), true)) {
							continue;
						}

						resource_map<int> buy_costs;
						buy_costs[defines::get()->get_wealth_resource()] = mercenary_hero->GetPrice();

						if (!AiPlayer->Player->CheckCosts(buy_costs) && AiPlayer->Player->check_limits<false>(*mercenary_hero->Type, mercenary_building) == check_limits_result::success) {
							CommandBuy(*mercenary_building, mercenary_hero, AiPlayer->Player->get_index());
							break;
						}
					}
				}

				bool mercenary_recruited = false;
				for (const auto &kv_pair : mercenary_building->UnitStock) {
					const unit_type *mercenary_type = unit_type::get_all()[kv_pair.first];
					const int unit_stock = kv_pair.second;
					if (
						unit_stock > 0
						&& !mercenary_type->BoolFlag[ITEM_INDEX].value
						&& check_conditions(mercenary_type, other_player)
						&& AiPlayer->Player->check_limits<false>(*mercenary_type, mercenary_building) == check_limits_result::success
						&& !AiPlayer->Player->CheckUnitType(*mercenary_type, true)
						&& (mercenary_type->get_unit_class() == nullptr || other_player->is_class_unit_type(mercenary_type))
					) {
						//see if there are any unit type requests for units of the same class as the mercenary
						for (size_t k = 0; k < AiPlayer->UnitTypeBuilt.size(); ++k) {
							AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[k];
							if (
								mercenary_type->get_unit_class() == queue.Type->get_unit_class()
								&& queue.Want > queue.Made
								&& (!queue.landmass || queue.landmass == CMap::get()->get_tile_landmass(mercenary_building->tilePos, mercenary_building->MapLayer->ID))
								&& (!queue.settlement || queue.settlement == mercenary_building->get_settlement())
							) {
								queue.Made++;
								CommandTrainUnit(*mercenary_building, *mercenary_type, AiPlayer->Player->get_index(), FlushCommands);
								mercenary_recruited = true;
								break;
							}
						}
					}
					if (mercenary_recruited) {
						break; // only hire one unit per mercenary camp per second
					}
				}
			}
		}
	}
	//Wyrmgus end
}

/**
**  Save state of player AI.
**
**  @param file   Output file.
**  @param plynr  Player number.
**  @param ai     Player AI.
*/
static void SaveAiPlayer(CFile &file, int plynr, const PlayerAi &ai)
{
	file.printf("DefineAiPlayer(%d,\n", plynr);
	file.printf("  \"ai-type\", \"%s\",\n", ai.AiType->Name.c_str());

	file.printf("  \"script\", \"%s\",\n", ai.Script.c_str());
	file.printf("  \"script-debug\", %s,\n", ai.ScriptDebug ? "true" : "false");
	file.printf("  \"sleep-cycles\", %lu,\n", ai.SleepCycles);

	//Wyrmgus start
	if (ai.Scouting) {
		file.printf("  \"scouting\",\n");
	}
	//Wyrmgus end
	
	//  All forces
	for (size_t i = 0; i < ai.Force.Size(); ++i) {
		file.printf("  \"force\", {%d, %s%s%s", (int) i,
					ai.Force[i].Completed ? "\"complete\"," : "\"recruit\",",
					ai.Force[i].Attacking ? " \"attack\"," : "",
					ai.Force[i].Defending ? " \"defend\"," : "");

		file.printf(" \"role\", ");
		switch (ai.Force[i].Role) {
			case AiForceRole::Attack:
				file.printf("\"attack\",");
				break;
			case AiForceRole::Defend:
				file.printf("\"defend\",");
				break;
			default:
				file.printf("\"unknown-%d\",", static_cast<int>(ai.Force[i].Role));
				break;
		}

		file.printf("\n    \"types\", { ");
		const size_t unitTypesCounst = ai.Force[i].UnitTypes.size();
		for (size_t j = 0; j != unitTypesCounst; ++j) {
			const AiUnitType &aut = ai.Force[i].UnitTypes[j];
			file.printf("%d, \"%s\", ", aut.Want, aut.Type->get_identifier().c_str());
		}
		file.printf("},\n    \"units\", {");
		for (const std::shared_ptr<wyrmgus::unit_ref> &ai_unit_ref : ai.Force[i].get_units()) {
			const CUnit *ai_unit = ai_unit_ref->get();
			file.printf(" %d, \"%s\",", UnitNumber(*ai_unit), ai_unit->Type->get_identifier().c_str());
		}
		file.printf("},\n    \"state\", %d, \"goalx\", %d, \"goaly\", %d,",
					static_cast<int>(ai.Force[i].State), ai.Force[i].GoalPos.x, ai.Force[i].GoalPos.y);
		file.printf("},\n");
	}

	file.printf("  \"reserve\", {");
	for (const auto &[resource, quantity] : ai.get_reserve()) {
		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}
	file.printf("},\n");

	file.printf("  \"used\", {");
	for (const auto &[resource, quantity] : ai.Used) {
		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}
	file.printf("},\n");

	file.printf("  \"needed\", {");
	for (const auto &[resource, quantity] : ai.Needed) {
		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}
	file.printf("},\n");

	file.printf("  \"collect\", {");
	for (const auto &[resource, quantity] : ai.get_collect()) {
		file.printf("\"%s\", %d, ", resource->get_identifier().c_str(), quantity);
	}
	file.printf("},\n");

	file.printf("  \"need-mask\", {");
	for (size_t i = 0; i < wyrmgus::resource::get_all().size(); ++i) {
		if (ai.NeededMask & ((long long int) 1 << i)) {
			file.printf("\"%s\", ", DefaultResourceNames[i].c_str());
		}
	}
	file.printf("},\n");
	if (ai.NeedSupply) {
		file.printf("  \"need-supply\",\n");
	}

	//  Requests
	if (!ai.FirstExplorationRequest.empty()) {
		file.printf("  \"exploration\", {");
		const size_t FirstExplorationRequestCount = ai.FirstExplorationRequest.size();
		for (size_t i = 0; i != FirstExplorationRequestCount; ++i) {
			const AiExplorationRequest &ptr = ai.FirstExplorationRequest[i];
			file.printf("{%d, %d, %" PRIu32 "}, ", ptr.pos.x, ptr.pos.y, enumeration::to_underlying(ptr.Mask));
		}
		file.printf("},\n");
	}
	file.printf("  \"last-exploration-cycle\", %lu,\n", ai.LastExplorationGameCycle);
	file.printf("  \"last-can-not-move-cycle\", %lu,\n", ai.LastCanNotMoveGameCycle);
	file.printf("  \"unit-type\", {");
	const size_t unitTypeRequestsCount = ai.UnitTypeRequests.size();
	for (size_t i = 0; i != unitTypeRequestsCount; ++i) {
		file.printf("\"%s\", ", ai.UnitTypeRequests[i].Type->get_identifier().c_str());
		file.printf("%d, ", ai.UnitTypeRequests[i].Count);

		const int landmass_index = ai.UnitTypeRequests[i].landmass ? static_cast<int>(ai.UnitTypeRequests[i].landmass->get_index()) : -1;
		file.printf("%d, ", landmass_index);
	}
	file.printf("},\n");

	file.printf("  \"upgrade\", {");
	const size_t upgradeToRequestsCount = ai.UpgradeToRequests.size();
	for (size_t i = 0; i != upgradeToRequestsCount; ++i) {
		file.printf("\"%s\", ", ai.UpgradeToRequests[i]->get_identifier().c_str());
	}
	file.printf("},\n");

	file.printf("  \"research\", {");
	for (const CUpgrade *requested_upgrade : ai.get_research_requests()) {
		file.printf("\"%s\", ", requested_upgrade->get_identifier().c_str());
	}
	file.printf("},\n");

	//
	//  Building queue
	//
	file.printf("  \"building\", {");
	const size_t UnitTypeBuiltCount = ai.UnitTypeBuilt.size();
	for (size_t i = 0; i != UnitTypeBuiltCount; ++i) {
		const AiBuildQueue &queue = ai.UnitTypeBuilt[i];
		/* rb - for backward compatibility of save format we have to put it first */
		if (queue.Pos.x != -1) {
			file.printf("\"onpos\", %d, %d, ", queue.Pos.x, queue.Pos.y);
		}
		//Wyrmgus start
		if (queue.MapLayer != -1) {
			file.printf("\"map-layer\", %d, ", queue.MapLayer);
		}
		
		if (queue.landmass != nullptr) {
			file.printf("\"landmass\", %zu, ", queue.landmass->get_index());
		}
		
		if (queue.settlement != nullptr) {
			file.printf("\"settlement\", \"%s\", ", queue.settlement->get_identifier().c_str());
		}
		//Wyrmgus end

		file.printf("\"%s\", %d, %d", queue.Type->get_identifier().c_str(), queue.Made, queue.Want);
		if (i < UnitTypeBuiltCount - 1) {
			file.printf(",\n");
		}
	}
	file.printf("},\n");

	//Wyrmgus start
	if (!ai.Scouts.empty()) {
		file.printf("  \"scouts\", {");
		for (size_t i = 0; i != ai.Scouts.size(); ++i) {
			const CUnit &aiunit = *ai.Scouts[i];
			file.printf(" %d, \"%s\",", UnitNumber(aiunit), aiunit.Type->get_identifier().c_str());
		}
		file.printf("},\n");
	}
	//Wyrmgus end

	if (!ai.get_transporters().empty()) {
		file.printf("  \"transporters\", {");
		for (const auto &kv_pair : ai.get_transporters()) {
			const landmass *landmass = kv_pair.first;
			for (const CUnit *ai_unit : kv_pair.second) {
				file.printf(" %zu, %d,", landmass ? landmass->get_index() : -1, UnitNumber(*ai_unit));
			}
		}
		file.printf("},\n");
	}
	
	if (!ai.get_site_transport_units().empty()) {
		file.printf("  \"site-transport-units\", {");
		for (const auto &[site, transport_units] : ai.get_site_transport_units()) {
			for (const std::shared_ptr<unit_ref> &ai_unit : transport_units) {
				file.printf(" \"%s\", %d,", site->get_identifier().c_str(), UnitNumber(**ai_unit));
			}
		}
		file.printf("},\n");
	}
	
	//Wyrmgus start
	file.printf("  \"pathway-construction-building\", %u,\n", ai.LastPathwayConstructionBuilding);
	//Wyrmgus end

	file.printf("  \"repair-building\", %u\n", ai.LastRepairBuilding);
	
	file.printf(")\n\n");
}

/**
**  Save state of AI to file.
**
**  @param file  Output file.
*/
void SaveAi(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");

	for (int i = 0; i < PlayerMax; ++i) {
		if (CPlayer::Players[i]->Ai) {
			SaveAiPlayer(file, i, *CPlayer::Players[i]->Ai);
		}
	}

	DebugPrint("FIXME: Saving lua function definition isn't supported\n");
}

/**
**  Setup all at start.
**
**  @param player  The player structure pointer.
*/
void AiInit(CPlayer &player)
{
	auto pai = std::make_unique<PlayerAi>();

	if (!pai) {
		throw std::runtime_error("Out of memory.");
	}

	pai->Player = &player;

	DebugPrint("%d - looking for class %s\n" _C_ player.get_index() _C_ player.AiName.c_str());
	//MAPTODO print the player name (player->Name) instead of the pointer

	//  Search correct AI type.
	if (AiTypes.empty()) {
		throw std::runtime_error("AI: Got no scripts at all! You need at least one dummy fallback script. See the DefineAi() documentation.");
	}

	CAiType *ait = nullptr;

	for (const std::unique_ptr<CAiType> &ai_type : AiTypes) {
		if (!ai_type->Race.empty() && ai_type->Race != wyrmgus::civilization::get_all()[player.Race]->get_identifier()) {
			continue;
		}
		if (!player.AiName.empty() && ai_type->Name != player.AiName) {
			continue;
		}
		ait = ai_type.get();
		break;
	}
	if (ait == nullptr) {
		throw std::runtime_error("AI: Found no matching ai scripts at all!");
	}
	if (player.AiName.empty()) {
		DebugPrint("AI: not found!!!!!!!!!!\n");
		DebugPrint("AI: Using fallback:\n");
	}
	DebugPrint("AI: %s:%s with %s:%s\n" _C_ wyrmgus::civilization::get_all()[player.Race]->get_identifier().c_str() _C_
			   !ait->Race.empty() ? ait->Race.c_str() : "All" _C_ player.AiName.c_str() _C_ ait->Class.c_str());

	pai->AiType = ait;
	pai->Script = ait->Script;

	//Wyrmgus start
//	pai->Collect[GoldCost] = 50;
//	pai->Collect[WoodCost] = 50;
	pai->set_collect(defines::get()->get_wealth_resource(), 45);
	pai->set_collect(resource::get_all()[WoodCost], 45);
	//Wyrmgus end
	pai->set_collect(resource::get_all()[OilCost], 0);
	//Wyrmgus start
	pai->set_collect(resource::get_all()[StoneCost], 10);
	//Wyrmgus end

	player.Ai = std::move(pai);
}

/**
**  Initialize global structures of the AI
*/
void InitAiModule()
{
	AiResetUnitTypeEquiv();
}

/**
**  Cleanup the AI in order to enable to restart a game.
*/
void CleanAi()
{
	for (int p = 0; p < PlayerMax; ++p) {
		CPlayer::Players[p]->Ai.reset();
	}
}

/**
**  Free all AI resources.
*/
void FreeAi()
{
	CleanAi();

	//  Free AiTypes.
	AiTypes.clear();

	//  Free AiHelpers.
	AiHelpers.clear();

	AiResetUnitTypeEquiv();
}

/*----------------------------------------------------------------------------
-- Support functions
----------------------------------------------------------------------------*/

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True, if unit-type was found in list.
*/
static int AiRemoveFromBuilt2(PlayerAi *pai, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement)
{
	std::vector<AiBuildQueue>::iterator i;

	for (i = pai->UnitTypeBuilt.begin(); i != pai->UnitTypeBuilt.end(); ++i) {
		assert_throw((*i).Want > 0);
		//Wyrmgus start
//		if (&type == (*i).Type && (*i).Made) {
		if (
			&type == (*i).Type
			&& (*i).Made
			&& (!(*i).landmass || !landmass || (*i).landmass == landmass)
			&& (!(*i).settlement || !settlement || (*i).settlement == settlement)
		) {
		//Wyrmgus end
			--(*i).Made;
			if (!--(*i).Want) {
				pai->UnitTypeBuilt.erase(i);
			}
			return 1;
		}
	}
	return 0;
}

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
static void AiRemoveFromBuilt(PlayerAi *pai, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement)
{
	//Wyrmgus start
	if (
		(type.get_given_resource() != nullptr && type.BoolFlag[CANHARVEST_INDEX].value && type.get_given_resource()->get_index() != TradeCost) //don't reduce refineries from the build request, they should be built dynamically via the resource gathering code without being requested
		|| type.get_terrain_type() // tile units are built without requests
		|| type.BoolFlag[TOWNHALL_INDEX].value // town halls are built without requests
	) {
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	if (AiRemoveFromBuilt2(pai, type)) {
	if (AiRemoveFromBuilt2(pai, type, landmass, settlement)) {
	//Wyrmgus end
		return;
	}

	//  This could happen if an upgrade is ready, look for equivalent units.
	int equivalents[UnitTypeMax + 1];
	const int equivalentsCount = AiFindUnitTypeEquiv(type, equivalents);
	for (int i = 0; i < equivalentsCount; ++i) {
		if (AiRemoveFromBuilt2(pai, *wyrmgus::unit_type::get_all()[equivalents[i]], landmass, settlement)) {
			return;
		}
	}
	if (pai->Player == CPlayer::GetThisPlayer()) {
		DebugPrint("My guess is that you built something under ai me. naughty boy!\n");
		return;
	}
	log::log_error("Can't remove \"" + type.get_identifier() + "\" from build list.");
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True if the unit-type could be reduced.
*/
static bool AiReduceMadeInBuilt2(PlayerAi &pai, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement)
{
	std::vector<AiBuildQueue>::iterator i;

	for (i = pai.UnitTypeBuilt.begin(); i != pai.UnitTypeBuilt.end(); ++i) {
		//Wyrmgus start
//		if (&type == (*i).Type && (*i).Made) {
		if (
			&type == (*i).Type
			&& (*i).Made
			&& ((*i).landmass == nullptr || landmass == nullptr || (*i).landmass == landmass)
			&& (!(*i).settlement || !settlement || (*i).settlement == settlement)
		) {
		//Wyrmgus end
			(*i).Made--;
			return true;
		}
	}
	return false;
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
void AiReduceMadeInBuilt(PlayerAi &pai, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement)
{
	//Wyrmgus start
	if (
		(type.get_given_resource() != nullptr && type.BoolFlag[CANHARVEST_INDEX].value && type.get_given_resource()->get_index() != TradeCost) //don't reduce refineries from the build request, they should be built dynamically via the resource gathering code without being requested
		|| type.get_terrain_type() // tile units are built without requests
		|| type.BoolFlag[TOWNHALL_INDEX].value // town halls are built without requests
	) {
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	if (AiReduceMadeInBuilt2(pai, type)) {
	if (AiReduceMadeInBuilt2(pai, type, landmass, settlement)) {
	//Wyrmgus end
		return;
	}
	//  This could happen if an upgrade is ready, look for equivalent units.
	int equivs[UnitTypeMax + 1];
	const unsigned int equivnb = AiFindUnitTypeEquiv(type, equivs);

	for (unsigned int i = 0; i < equivnb; ++i) {
		if (AiReduceMadeInBuilt2(pai, *wyrmgus::unit_type::get_all()[equivs[i]], landmass, settlement)) {
			return;
		}
	}
	if (pai.Player == CPlayer::GetThisPlayer()) {
		DebugPrint("My guess is that you built something under ai me. naughty boy!\n");
		return;
	}

	log::log_error("Can't reduce made for \"" + type.get_identifier() + "\" from build list.");
}

/*----------------------------------------------------------------------------
-- Callback Functions
----------------------------------------------------------------------------*/

/**
**  Called if a Unit is Attacked
**
**  @param attacker  Pointer to attacker unit.
**  @param defender  Pointer to unit that is being attacked.
*/
void AiHelpMe(CUnit *attacker, CUnit &defender)
{
	/* Friendly Fire - typical splash */
	if (!attacker || attacker->Player == defender.Player) {
		//FIXME - try react somehow
		return;
	}

	DebugPrint("%d: %d(%s) attacked at %d,%d\n" _C_
			   defender.Player->get_index() _C_ UnitNumber(defender) _C_
			   defender.Type->get_identifier().c_str() _C_ defender.tilePos.x _C_ defender.tilePos.y);

	//  Don't send help to scouts (zeppelin,eye of vision).
	if (!defender.CanAttack() && defender.Type->get_domain() == unit_domain::air) {
		return;
	}
	// Summoned unit, don't help
	if (defender.Summoned) {
		return;
	}

	PlayerAi &pai = *defender.Player->Ai;
	AiPlayer = &pai;
	
	//Wyrmgus start
	pai.Scouting = false;
	//Wyrmgus end

	//  If unit belongs to an attacking force, check if force members can help.
	//Wyrmgus start
	/*
	if (defender.GroupId) {
		AiForce &aiForce = pai.Force[defender.GroupId - 1];

		//  Unit belongs to an force, check if brothers in arms can help
		for (unsigned int i = 0; i < aiForce.Units.size(); ++i) {
			CUnit &aiunit = *aiForce.Units[i];

			if (&defender == &aiunit) {
				continue;
			}
			
			//Wyrmgus start
			if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger your cargo/passengers
				continue;
			}
			//Wyrmgus end

			// if brother is idle or attack no-agressive target and
			// can attack our attacker then ask for help
			// FIXME ad support for help from Coward type units
			if (aiunit.IsAgressive() && aiunit.Type->can_target(attacker)
				&& aiunit.CurrentOrder()->GetGoal() != attacker) {
				bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

				if (aiunit.CurrentAction() == UnitAction::Attack) {
					const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
					const CUnit *oldGoal = orderAttack.GetGoal();

					if (oldGoal == nullptr || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
											//Wyrmgus start
//											&& aiunit.MapDistanceTo(defender) <= aiunit.Stats->Variables[ATTACKRANGE_INDEX].Max)) {
											&& aiunit.MapDistanceTo(defender) <= aiunit.get_best_attack_range())) {
											//Wyrmgus end
						shouldAttack = true;
					}
				}

				if (shouldAttack) {
					//Wyrmgus start
					const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.

					aiunit.Wait += delay;
					//Wyrmgus end
					
					//Wyrmgus start
//					CommandAttack(aiunit, attacker->tilePos, attacker, FlushCommands);
//					std::unique_ptr<COrder> saved_order = COrder::NewActionAttack(aiunit, attacker->tilePos);
					CommandAttack(aiunit, attacker->tilePos, attacker, FlushCommands, attacker->MapLayer);
					//Wyrmgus end

					//Wyrmgus start
//					if (aiunit.CanStoreOrder(saved_order.get()) == false) {
//						saved_order.reset();
//					} else {
//						aiunit.SavedOrder = std::move(saved_order);
//					}
					//Wyrmgus end
				}
			}
		}
		if (!aiForce.Defending && aiForce.State > 0) {
			DebugPrint("%d: %d(%s) belong to attacking force, don't defend it\n" _C_
					   defender.Player->get_index() _C_ UnitNumber(defender) _C_ defender.Type->get_identifier().c_str());
			// unit belongs to an attacking force,
			// so don't send others force in such case.
			// FIXME: there may be other attacking the same place force who can help
			return;
		}
	}
	*/
	//Wyrmgus end

	//Wyrmgus start
	/*
	// Send defending forces, also send attacking forces if they are home/traning.
	// This is still basic model where we suspect only one base ;(
	const Vec2i &pos = attacker->tilePos;

	for (unsigned int i = 0; i < pai.Force.Size(); ++i) {
		AiForce &aiForce = pai.Force[i];

		if (aiForce.Size() > 0
			&& ((aiForce.Role == AiForceRole::Defend && !aiForce.Attacking)
				//Wyrmgus start
//				|| (aiForce.Role == AiForceRole::Attack && !aiForce.Attacking && !aiForce.State))) {  // none attacking
				|| (aiForce.Role == AiForceRole::Attack && !aiForce.Attacking && !aiForce.State)
				|| (defender.GroupId && i == (defender.GroupId - 1)))) {  // none attacking
				//Wyrmgus end
			//Wyrmgus start
//			aiForce.Defending = true;
			if (!aiForce.Attacking) {
				aiForce.Defending = true;
			}
//			aiForce.Attack(pos);

			//instead of making the force attack the position, make its units that can target the attacker attack it (so that we don't have, for instance, a melee land force try to attack back ships...
			for (unsigned int j = 0; j < aiForce.Units.size(); ++j) {
				CUnit &aiunit = *aiForce.Units[j];

				if (&defender == &aiunit) {
					continue;
				}
				
				if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger your cargo/passengers
					continue;
				}

				// if unit is idle or attacking a non-agressive target and
				// can attack our attacker then ask for help
				// FIXME ad support for help from Coward type units
				if (aiunit.IsAgressive() && aiunit.Type->can_target(attacker)
					&& aiunit.CurrentOrder()->GetGoal() != attacker) {
					bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

					if (aiunit.CurrentAction() == UnitAction::Attack) {
						const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
						const CUnit *oldGoal = orderAttack.GetGoal();

						if (oldGoal == nullptr || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
												//Wyrmgus start
	//											&& aiunit.MapDistanceTo(defender) <= aiunit.Stats->Variables[ATTACKRANGE_INDEX].Max)) {
												&& aiunit.MapDistanceTo(defender) <= aiunit.get_best_attack_range())) {
												//Wyrmgus end
							shouldAttack = true;
						}
					}

					if (shouldAttack) {
						const int delay = j; // To avoid lot of CPU consuption, send them with a small time difference.

						aiunit.Wait += delay;
						
						CommandAttack(aiunit, attacker->tilePos, attacker, FlushCommands, attacker->MapLayer);
					}
				}
			}
			if (!aiForce.Defending && aiForce.State > 0) {
				DebugPrint("%d: %d(%s) belong to attacking force, don't defend it\n" _C_
						   defender.Player->get_index() _C_ UnitNumber(defender) _C_ defender.Type->get_identifier().c_str());
				// unit belongs to an attacking force,
				// so don't send others force in such case.
				// FIXME: there may be other attacking the same place force who can help
				continue;
			}
			//Wyrmgus end
		}
	}
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	//check if there are any nearby units with active AI, and send them to help
	for (int max_dist = 16; max_dist <= MaxMapWidth; max_dist *= 2) { //search for units in increasingly greater distances, until a helper is found
		bool has_helper = false;
		std::vector<CUnit *> helper_table;
		SelectAroundUnit(defender, max_dist, helper_table, HasSamePlayerAs(*defender.Player));
		for (size_t i = 0; i < helper_table.size(); ++i) {
			CUnit &aiunit = *helper_table[i];

			if (&defender == &aiunit) {
				continue;
			}
			
			if (!aiunit.IsAliveOnMap()) {
				continue;
			}

			if (!aiunit.CanMove()) {
				continue;
			}

			if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger the cargo/passengers
				continue;
			}

			if (aiunit.Type->BoolFlag[HARVESTER_INDEX].value) { //harvesters shouldn't go to help units being attacked
				continue;
			}
			
			// if brother is idle or attack no-agressive target and
			// can attack our attacker then ask for help
			// FIXME ad support for help from Coward type units
			if (aiunit.Active && aiunit.IsAgressive() && aiunit.Type->can_target(attacker)
				&& aiunit.CurrentOrder()->get_goal() != attacker) {
				bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

				if (aiunit.CurrentAction() == UnitAction::Attack) {
					const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
					const CUnit *oldGoal = orderAttack.get_goal();

					if (oldGoal == nullptr || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
											&& aiunit.MapDistanceTo(defender) <= aiunit.get_best_attack_range())) {
						shouldAttack = true;
					}
				}

				if (shouldAttack) {
					CommandAttack(aiunit, attacker->tilePos, attacker, FlushCommands, attacker->MapLayer->ID);
					has_helper = true;
				}
			}
		}
		if (has_helper) {
			break;
		}
	}
	//Wyrmgus end
}

/**
**  Called if a unit is killed.
**
**  @param unit  Pointer to unit.
*/
void AiUnitKilled(CUnit &unit)
{
	DebugPrint("%d: %d(%s) killed\n" _C_
			   unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str());

	assert_throw(unit.Player->get_type() != player_type::person);

	if (unit.GroupId) {
		AiForce &force = unit.Player->Ai->Force[unit.GroupId - 1];

		force.Remove(&unit);
		if (force.Size() == 0) {
			force.Attacking = false;
			if (!force.Defending && force.State > AiForceAttackingState::Waiting) {
				DebugPrint("%d: Attack force #%lu was destroyed, giving up\n"
						   _C_ unit.Player->get_index() _C_(long unsigned int)(&force  - & (unit.Player->Ai->Force[0])));
				force.Reset(true);
			}
		}
	}

	unit.CurrentOrder()->AiUnitKilled(unit);
}

/**
**  Called if work complete (Buildings).
**
**  @param unit  Pointer to unit that builds the building.
**  @param what  Pointer to unit building that was built.
*/
void AiWorkComplete(CUnit *unit, CUnit &what)
{
	if (unit) {
		DebugPrint("%d: %d(%s) build %s at %d,%d completed\n" _C_
				   what.Player->get_index() _C_ UnitNumber(*unit) _C_ unit->Type->get_identifier().c_str() _C_
				   what.Type->get_identifier().c_str() _C_ unit->tilePos.x _C_ unit->tilePos.y);
	} else {
		DebugPrint("%d: building %s at %d,%d completed\n" _C_
				   what.Player->get_index() _C_ what.Type->get_identifier().c_str() _C_ what.tilePos.x _C_ what.tilePos.y);
	}

	assert_throw(what.Player->get_type() != player_type::person);
	//Wyrmgus start
//	AiRemoveFromBuilt(what.Player->Ai, *what.Type);
	AiRemoveFromBuilt(what.Player->Ai.get(), *what.Type, CMap::get()->get_tile_landmass(what.tilePos, what.MapLayer->ID), what.get_settlement());
	//Wyrmgus end
}

/**
**  Called if building can't be build.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
void AiCanNotBuild(const CUnit &unit, const wyrmgus::unit_type &what, const landmass *landmass, const wyrmgus::site *settlement)
{
	DebugPrint("%d: %d(%s) Can't build %s at %d,%d\n" _C_
			   unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str() _C_
			   what.get_identifier().c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	assert_throw(unit.Player->get_type() != player_type::person);
	//Wyrmgus start
//	AiReduceMadeInBuilt(*unit.Player->Ai, what);
	AiReduceMadeInBuilt(*unit.Player->Ai, what, landmass, settlement);
	//Wyrmgus end
}

/**
**  Called if building place can't be reached.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
void AiCanNotReach(CUnit &unit, const wyrmgus::unit_type &what, const landmass *landmass, const wyrmgus::site *settlement)
{
	assert_throw(unit.Player->get_type() != player_type::person);
	//Wyrmgus start
//	AiReduceMadeInBuilt(*unit.Player->Ai, what);
	AiReduceMadeInBuilt(*unit.Player->Ai, what, landmass, settlement);
	//Wyrmgus end
}

/**
**  Try to move a unit that's in the way
*/
static void AiMoveUnitInTheWay(CUnit &unit)
{
	static constexpr std::array<Vec2i, 8> dirs = {Vec2i(-1, -1), Vec2i(-1, 0), Vec2i(-1, 1), Vec2i(0, 1), Vec2i(1, 1), Vec2i(1, 0), Vec2i(1, -1), Vec2i(0, -1)};

	std::array<CUnit *, 16> movableunits{};
	std::array<Vec2i, 16> movablepos;
	int movablenb;

	AiPlayer = unit.Player->Ai.get();

	// No more than 1 move per 10 cycle ( avoid stressing the pathfinder )
	if (GameCycle <= AiPlayer->LastCanNotMoveGameCycle + 10) {
		return;
	}

	const wyrmgus::unit_type &unittype = *unit.Type;
	const Vec2i u0 = unit.tilePos;
	const Vec2i u1(u0 + unittype.get_tile_size() - QSize(1, 1));

	movablenb = 0;

	// Try to make some unit moves around it
	//Wyrmgus start
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, 1, table);
//	for (auto it = wyrmgus::unit_manager::get()->get_units().begin(); it != wyrmgus::unit_manager::get()->get_units().end(); ++it) {
	for (size_t i = 0; i != table.size(); ++i) {
	//Wyrmgus end
		//Wyrmgus start
//		CUnit &blocker = **it;
		CUnit &blocker = *table[i];
		//Wyrmgus end

		if (blocker.IsUnusable()) {
			continue;
		}
		//Wyrmgus start
		if (!blocker.IsIdle()) { // don't move units that aren't idle, as that will stop their current orders
			continue;
		}
		//Wyrmgus end
		if (!blocker.CanMove() || blocker.Moving) {
			continue;
		}
		if (blocker.Player != unit.Player && blocker.Player->is_allied_with(*unit.Player) == false) {
			continue;
		}
		//Wyrmgus start
		if (!blocker.Player->AiEnabled) {
			continue;
		}
		
		int blocker_force = blocker.Player->Ai->Force.GetForce(blocker);
		if (blocker_force != -1 && blocker.Player->Ai->Force[blocker_force].Attacking) { //don't try to move the blocker if it is part of a force that is attacking
			if (unit.CurrentAction() != UnitAction::Board) { //unless the unit is trying to board a ship, in which case the blocker should be moved
				continue;
			}
		}
		//Wyrmgus end
		const wyrmgus::unit_type &blockertype = *blocker.Type;

		if (blockertype.get_domain() != unittype.get_domain()) {
			continue;
		}

		const Vec2i b0 = blocker.tilePos;
		const Vec2i b1(b0 + blockertype.get_tile_size() - QSize(1, 1));

		if (&unit == &blocker) {
			continue;
		}
		// Check for collision
		if (unit.MapDistanceTo(blocker) >= unit.Type->get_tile_width() + 1) {
			continue;
		}

		// Move blocker in a rand dir
		int r = SyncRand(8);
		int trycount = 8;
		while (trycount > 0) {
			r = (r + 1) & 7;
			--trycount;

			const Vec2i pos = blocker.tilePos + Vec2i(blocker.Type->get_tile_size()) * dirs[r];

			// Out of the map => no !
			if (!CMap::get()->Info->IsPointOnMap(pos, unit.MapLayer)) {
				continue;
			}
			// move to blocker ? => no !
			if (pos == u0) {
				continue;
			}
			if (unit.MapLayer->Field(pos)->UnitCache.size() > 0) {
				continue;
			}

			movableunits[movablenb] = &blocker;
			movablepos[movablenb] = pos;

			++movablenb;
			trycount = 0;
		}
		if (movablenb >= 16) {
			break;
		}
	}

	// Don't move more than 1 unit.
	if (movablenb) {
		const int index = SyncRand(movablenb);
		std::unique_ptr<COrder> saved_order;
		if (movableunits[index]->IsIdle() == false) {
			if (unit.CanStoreOrder(unit.CurrentOrder())) {
				saved_order = unit.CurrentOrder()->Clone();
			}
		}
		CommandMove(*movableunits[index], movablepos[index], FlushCommands, movableunits[index]->MapLayer->ID);
		if (saved_order != nullptr) {
			unit.SavedOrder = std::move(saved_order);
		}
		AiPlayer->LastCanNotMoveGameCycle = GameCycle;
	}
}

/**
**  Called if a unit can't move. Try to move unit in the way
**
**  @param unit  Pointer to unit what builds the building.
*/
void AiCanNotMove(CUnit &unit)
{
	const Vec2i &goal_pos = unit.pathFinderData->input.GetGoalPos();
	const Vec2i &goal_size = unit.pathFinderData->input.GetGoalSize();

	AiPlayer = unit.Player->Ai.get();
	if (PlaceReachable(unit, goal_pos, goal_size, 0, MaxMapWidth - 1, 0, unit.MapLayer->ID)) {
		// Path probably closed by unit here
		AiMoveUnitInTheWay(unit);
		//Wyrmgus start
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value) {
			unit.Wait = CYCLES_PER_SECOND * 10; // wait a bit before trying to move the unit again; this is so when units are attacking an enemy they won't clog performance if one blocks the other; not for workers since otherwise they will spend too much time doing nothing if blocked briefly by another worker during gathering
		}
		//Wyrmgus end
	}
}

/**
**  Called if the AI needs more farms.
**
**  @param player  player which need supply.
*/
void AiNeedMoreSupply(const CPlayer &player)
{
	assert_throw(player.get_type() != player_type::person);
	player.Ai->NeedSupply = true;
}

/**
**  Called if training of a unit is completed.
**
**  @param unit  Pointer to unit making.
**  @param what  Pointer to new ready trained unit.
*/
void AiTrainingComplete(CUnit &unit, CUnit &what)
{
	DebugPrint("%d: %d(%s) training %s at %d,%d completed\n" _C_
			   //Wyrmgus start
//			   unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str() _C_
			   what.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str() _C_
			   //Wyrmgus end
			   what.Type->get_identifier().c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	assert_throw(what.Player->get_type() != player_type::person);

	//Wyrmgus start
//	AiRemoveFromBuilt(unit.Player->Ai, *what.Type);
	if (unit.Player == what.Player) {
		AiRemoveFromBuilt(what.Player->Ai.get(), *what.Type, CMap::get()->get_tile_landmass(what.tilePos, what.MapLayer->ID), what.get_settlement());
	} else { //remove the request of the unit the mercenary is substituting
		const unit_type *requested_unit_type = what.Player->get_faction()->get_class_unit_type(what.Type->get_unit_class());
		if (requested_unit_type != nullptr) {
			AiRemoveFromBuilt(what.Player->Ai.get(), *requested_unit_type, CMap::get()->get_tile_landmass(what.tilePos, what.MapLayer->ID), what.get_settlement());
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	what.Player->Ai->Force.remove_dead_units();
	what.Player->Ai->Force.Assign(what, -1);
	
	if (what.Player->Ai->Force.GetForce(what) == -1) { // if the unit hasn't been assigned to a force, see if it is a transporter, and assign it accordingly
		if (what.Type->CanTransport() && what.CanMove() && (what.Type->get_domain() == unit_domain::water || what.Type->get_domain() == unit_domain::air || what.Type->get_domain() == unit_domain::air_low || what.Type->get_domain() == unit_domain::space)) {
			const landmass *landmass = CMap::get()->get_tile_landmass(what.tilePos, what.MapLayer->ID);
			
			if (landmass != nullptr) {
				what.Player->Ai->add_transporter(&what, landmass);
			}
		}
	}
	//Wyrmgus end
}

/**
**  Called if upgrading of an unit is completed.
**
**  @param unit Pointer to unit working.
**  @param what Pointer to the new unit-type.
*/
void AiUpgradeToComplete(CUnit &unit, const wyrmgus::unit_type &what)
{
	DebugPrint("%d: %d(%s) upgrade-to %s at %d,%d completed\n" _C_
			   unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str() _C_
			   what.get_identifier().c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	assert_throw(unit.Player->get_type() != player_type::person);
}

/**
**  Called if reseaching of an unit is completed.
**
**  @param unit  Pointer to unit working.
**  @param what  Pointer to the new upgrade.
*/
void AiResearchComplete(CUnit &unit, const CUpgrade *what)
{
	DebugPrint("%d: %d(%s) research %s at %d,%d completed\n" _C_
			   unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.Type->get_identifier().c_str() _C_
			   what->get_identifier().c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	assert_throw(unit.Player->get_type() != player_type::person);

	// FIXME: upgrading knights -> paladins, must rebuild lists!
}

/**
**  This is called for each player, each game cycle.
**
**  @param player  The player structure pointer.
*/
void AiEachCycle(CPlayer &player)
{
	AiPlayer = player.Ai.get();
}

/**
**  This is called for each player each second.
**
**  @param player  The player structure pointer.
*/
void AiEachSecond(CPlayer &player)
{
	AiPlayer = player.Ai.get();

	assert_log(AiPlayer != nullptr);

	if (AiPlayer == nullptr) {
		return;
	}

	//if the AI player doesn't have a faction, set a random one for it
	if (player.get_faction() == nullptr) {
		player.set_random_faction();
	}

	//  Advance script
	AiExecuteScript();
	
	if (!player.is_alive()) {
		return;
	}
	
	AiPlayer->NeededMask = 0;

	//  Look if everything is fine.
	AiCheckUnits();

	AiPlayer->check_factions();

	//  Handle the resource manager.
	AiResourceManager();

	//  Handle the force manager.
	AiForceManager();

	AiPlayer->check_site_transport_units();

	//  Check for magic actions.
	AiCheckMagic();

	// At most 1 explorer each 5 seconds
	if (GameCycle > AiPlayer->LastExplorationGameCycle + 5 * CYCLES_PER_SECOND) {
		AiSendExplorers();
	}

	AiPlayer->evaluate_diplomacy();
}

/**
**  This is called for each player each half minute.
**
**  @param player  The player structure pointer.
*/
void AiEachHalfMinute(CPlayer &player)
{
	AiPlayer = player.Ai.get();

	assert_log(AiPlayer != nullptr);

	if (AiPlayer == nullptr) {
		return;
	}

	if (AiPlayer->Scouting) { //check periodically if has found new enemies
		AiPlayer->Scouting = false;
	}
	
	AiCheckWorkers();
	AiCheckUpgrades();
	AiCheckBuildings();
	
	AiForceManagerEachHalfMinute();
}

/**
**  This is called for each player each minute.
**
**  @param player  The player structure pointer.
*/
void AiEachMinute(CPlayer &player)
{
	AiPlayer = player.Ai.get();

	assert_log(AiPlayer != nullptr);

	if (AiPlayer == nullptr) {
		return;
	}

	AiPlayer->check_settlement_construction();
	AiPlayer->check_transporters();
	AiCheckDockConstruction();
	
	AiForceManagerEachMinute();
}

int AiGetUnitTypeCount(const PlayerAi &pai, const wyrmgus::unit_type *type, const landmass *landmass, const bool include_requests, const bool include_upgrades)
{
	int count = 0;
	
	//check for units in the landmass if it has been designated; otherwise look for all units of the type
	if (landmass) {
		std::vector<CUnit *> table;
		FindPlayerUnitsByType(*pai.Player, *type, table, true);
		
		for (size_t i = 0; i < table.size(); ++i) {
			CUnit &unit = *table[i];
					
			if (CMap::get()->get_tile_landmass(unit.tilePos, unit.MapLayer->ID) == landmass) {
				count++;
			}
		}
	} else {
		count += pai.Player->GetUnitTypeAiActiveCount(type);
	}
		
	if (include_requests) {
		count += AiGetUnitTypeRequestedCount(pai, type, landmass);
	}
	
	if (include_upgrades) {
		const std::vector<const wyrmgus::unit_type *> &unit_type_upgrades = AiHelpers.get_unit_type_upgrades(type);

		for (const wyrmgus::unit_type *unit_type_upgrade : unit_type_upgrades) {
			count += AiGetUnitTypeCount(pai, unit_type_upgrade, landmass, include_requests, include_upgrades);
		}

		if (type->get_unit_class() != nullptr && pai.Player->get_faction() != nullptr) {
			const std::vector<const wyrmgus::unit_class *> &unit_class_upgrades = AiHelpers.get_unit_class_upgrades(type->get_unit_class());

			for (const wyrmgus::unit_class *unit_class_upgrade : unit_class_upgrades) {
				const wyrmgus::unit_type *unit_type_upgrade = pai.Player->get_faction()->get_class_unit_type(unit_class_upgrade);

				if (unit_type_upgrade == nullptr) {
					continue;
				}

				count += AiGetUnitTypeCount(pai, unit_type_upgrade, landmass, include_requests, include_upgrades);
			}
		}
	}
	
	return count;
}

int AiGetUnitTypeRequestedCount(const PlayerAi &pai, const wyrmgus::unit_type *type, const landmass *landmass, const wyrmgus::site *settlement)
{
	int count = 0;
	
	for (size_t i = 0; i < pai.UnitTypeBuilt.size(); ++i) {
		const AiBuildQueue &queue = pai.UnitTypeBuilt[i];
		if (
			queue.Type == type
			&& (landmass == nullptr || queue.landmass == landmass)
			&& (settlement == nullptr || queue.settlement == settlement)
		) {
			count += queue.Want;
		}
	}
		
	return count;
}

bool AiHasUpgrade(const PlayerAi &pai, const CUpgrade *upgrade, bool include_requests)
{
	if (UpgradeIdAllowed(*pai.Player, upgrade->ID) == 'R') {
		return true;
	}

	if (include_requests) {
		if (pai.Player->UpgradeTimers.Upgrades[upgrade->ID] > 0) {
			//already researching
			return true;
		}
		
		if (vector::contains(pai.get_research_requests(), upgrade)) {
			return true;
		}
	}
	
	return false;
}
