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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_train.h"

#include "ai.h"
#include "animation/animation.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "iolib.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "player/player.h"
#include "player/player_type.h"
#include "population/population_unit_key.h"
#include "script.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "sound/unit_sound_type.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "util/assert_util.h"

/// How many resources the player gets back if canceling training
static constexpr int CancelTrainingCostsFactor = 100;

std::unique_ptr<COrder> COrder::NewActionTrain(CUnit &trainer, const wyrmgus::unit_type &type, int player)
{
	auto order = std::make_unique<COrder_Train>();

	order->Type = &type;
	// FIXME: if you give quick an other order, the resources are lost!
	//Wyrmgus start
	order->Player = player;
//	trainer.Player->SubUnitType(type);

	const bool is_hired = trainer.Type->Stats[trainer.Player->get_index()].has_hired_unit(&type);

	CPlayer::Players[player]->SubUnitType(type, is_hired);
	//Wyrmgus end

	return order;
}

void COrder_Train::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-train\",");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf("\"type\", \"%s\",", this->Type->get_identifier().c_str());
	//Wyrmgus start
	file.printf("\"player\", %d,", this->Player);
	//Wyrmgus end
	file.printf("\"ticks\", %d", this->Ticks);
	file.printf("}");
}

bool COrder_Train::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "type")) {
		++j;
		this->Type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
	//Wyrmgus start
	} else if (!strcmp(value, "player")) {
		++j;
		this->Player = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else if (!strcmp(value, "ticks")) {
		++j;
		this->Ticks = LuaToNumber(l, -1, j + 1);
	} else {
		return false;
	}
	return true;
}

bool COrder_Train::IsValid() const
{
	return true;
}

PixelPos COrder_Train::Show(const CViewport &, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	Q_UNUSED(render_commands)

	return lastScreenPos;
}

void COrder_Train::Cancel(CUnit &unit)
{
	Q_UNUSED(unit)

	DebugPrint("Cancel training\n");
	//Wyrmgus start
//	CPlayer &player = *unit.Player;
	CPlayer &player = *CPlayer::Players[this->Player];
	//Wyrmgus end

	//Wyrmgus start
//	player.AddCostsFactor(this->Type->Stats[player.get_index()].Costs, CancelTrainingCostsFactor);
	const resource_map<int> type_costs = player.GetUnitTypeCosts(this->Type);
	player.AddCostsFactor(type_costs, CancelTrainingCostsFactor);
	//Wyrmgus end
}

void COrder_Train::UpdateUnitVariables(CUnit &unit) const
{
	assert_throw(unit.CurrentOrder() == this);

	unit.Variable[TRAINING_INDEX].Value = this->Ticks;
	//Wyrmgus start
//	unit.Variable[TRAINING_INDEX].Max = this->Type->Stats[unit.Player->get_index()].get_time_cost();
	unit.Variable[TRAINING_INDEX].Max = this->Type->Stats[this->Player].get_time_cost();
	//Wyrmgus end
}

void COrder_Train::ConvertUnitType(const CUnit &unit, wyrmgus::unit_type &newType)
{
	Q_UNUSED(unit)

	//Wyrmgus start
//	const CPlayer &player = *unit.Player;
	const CPlayer &player = *CPlayer::Players[this->Player];
	//Wyrmgus end
	const int oldCost = this->Type->Stats[player.get_index()].get_time_cost();
	const int newCost = newType.Stats[player.get_index()].get_time_cost();

	// Must Adjust Ticks to the fraction that was trained
	this->Ticks = this->Ticks * newCost / oldCost;
	this->Type = &newType;
}

static void AnimateActionTrain(CUnit &unit)
{
	if (unit.get_animation_set()->Train) {
		UnitShowAnimation(unit, unit.get_animation_set()->Train);
	} else {
		UnitShowAnimation(unit, unit.get_animation_set()->Still);
	}
}

void COrder_Train::Execute(CUnit &unit)
{
	//Wyrmgus start
	/*
	AnimateActionTrain(unit);
	if (unit.Wait) {
		unit.Wait--;
		return;
	}
	*/

	if (unit.CriticalOrder.get() != this) {
		AnimateActionTrain(unit);
		if (unit.Wait) {
			unit.Wait--;
			return;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	CPlayer &player = *unit.Player;
	CPlayer &player = *CPlayer::Players[this->Player];
	//Wyrmgus end
	const wyrmgus::unit_type &nType = *this->Type;
	const int cost = nType.Stats[player.get_index()].get_time_cost();
	
	//Wyrmgus start
	// Check if enough supply available.
	const check_limits_result result = player.check_limits<true>(nType, &unit);
	if (result != check_limits_result::success) {
		if (result == check_limits_result::not_enough_food && player.AiEnabled) {
			AiNeedMoreSupply(player);
		}
		this->Ticks = 0;
		unit.Wait = CYCLES_PER_SECOND / 6;
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	this->Ticks += std::max(1, player.SpeedTrain / CPlayer::base_speed_factor);
	this->Ticks += std::max(1, (player.SpeedTrain + unit.Variable[TIMEEFFICIENCYBONUS_INDEX].Value) / CPlayer::base_speed_factor);
	
	const bool is_hired = unit.Type->Stats[unit.Player->get_index()].has_hired_unit(&nType);

	if (is_hired) {
		//if the training unit/building has a "stock" of the trained unit, that means it should be created with no time wait
		this->Ticks = cost;
	}
	//Wyrmgus end

	if (this->Ticks < cost) {
		unit.Wait = CYCLES_PER_SECOND / 6;
		return;
	}
	this->Ticks = std::min(this->Ticks, cost);

	//Wyrmgus start
	// food check should be before changing the ticks
	/*
	// Check if enough supply available.
	const check_limits_result result = player.check_limits<true>(nType, &unit);
	if (result != check_limits_result::success) {
		if (result == check_limits_result::not_enough_food && player.AiEnabled) {
			AiNeedMoreSupply(*unit.Player);
		}
		unit.Wait = CYCLES_PER_SECOND / 6;
		return;
	}
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	int owner_player = this->Player;
	if (this->Type->BoolFlag[ITEM_INDEX].value || this->Type->BoolFlag[POWERUP_INDEX].value) { //items and power-ups should always be owned by the neutral player
		owner_player = PlayerNumNeutral;
	}

	/*
	CUnit *newUnit = MakeUnit(nType, &player);

	if (newUnit == nullptr) { // No more memory :/
		//Wyrmgus start
//		player.Notify(notification_type::yellow, unit.tilePos, _("Unable to train %s"), nType.Name.c_str());
		player.Notify(notification_type::yellow, unit.tilePos, _("Unable to train %s"), nType.GetDefaultName(player).c_str());
		//Wyrmgus end
		unit.Wait = CYCLES_PER_SECOND / 6;
		return;
	}

	// New unit might supply food
	UpdateForNewUnit(*newUnit, 0);

	// Set life span
	if (unit.Type->DecayRate) {
		newUnit->TTL = GameCycle + unit.Type->DecayRate * 6 * CYCLES_PER_SECOND;
	}
	*/
	
	/* Auto Group Add */
	/*
	if (!unit.Player->AiEnabled && unit.GroupId) {
		int num = 0;
		while (!(unit.GroupId & (1 << num))) {
			++num;
		}
		AddToGroup(&newUnit, 1, num);
	}

	newUnit->drop_out_on_side(LookingW, &unit);
	if (&player == ThisPlayer) {
		PlayUnitSound(newUnit, VoiceReady);
	}
	if (unit.Player->AiEnabled) {
		AiTrainingComplete(unit, *newUnit);
	}

	if (unit.NewOrder && unit.NewOrder->HasGoal()
		&& unit.NewOrder->GetGoal()->Destroyed) {
		unit.NewOrder.reset();
	}

	if (CanHandleOrder(*newUnit, unit.NewOrder) == true) {
		newUnit->Orders[0] = unit.NewOrder->Clone();
	} else {
#if 0
		// Tell the unit to right-click ?
#endif
	}
	*/
	for (int i = 0; i < (this->Type->TrainQuantity ? this->Type->TrainQuantity : 1); ++i) {
		if (is_hired) {
			if (unit.get_unit_stock(&nType) > 0) {
				unit.change_unit_stock(&nType, -1);
			} else if (nType.get_unit_class() != nullptr && unit.get_unit_class_stock(nType.get_unit_class()) > 0) {
				unit.change_unit_class_stock(nType.get_unit_class(), -1);
			} else {
				//don't create the unit if no further stock of it is available
				break;
			}
		}

		//Wyrmgus start
//		CUnit *newUnit = MakeUnit(nType, &player);
		CUnit *newUnit = MakeUnit(nType, CPlayer::Players[owner_player].get());
		//Wyrmgus end

		if (newUnit == nullptr) { // No more memory :/
			//Wyrmgus start
	//		player.Notify(notification_type::yellow, unit.tilePos, _("Unable to train %s"), nType.Name.c_str());
			player.Notify(notification_type::yellow, unit.tilePos, unit.MapLayer->ID, _("Unable to train %s"), nType.GetDefaultName(&player).c_str());
			//Wyrmgus end
			unit.Wait = CYCLES_PER_SECOND / 6;
			return;
		}

		// New unit might supply food
		UpdateForNewUnit(*newUnit, 0);

		// Set life span
		if (unit.Type->DecayRate) {
			newUnit->TTL = GameCycle + unit.Type->DecayRate * 6 * CYCLES_PER_SECOND;
		}
		
		/* Auto Group Add */
		/*
		if (!unit.Player->AiEnabled && unit.GroupId) {
			int num = 0;
			while (!(unit.GroupId & (1 << num))) {
				++num;
			}
			AddToGroup(&newUnit, 1, num);
		}
		*/

		if (unit.has_rally_point() && unit.get_rally_point_map_layer() == unit.MapLayer) {
			newUnit->drop_out_nearest(unit.get_rally_point_pos(), &unit);
		} else {
			newUnit->drop_out_on_side(LookingW, &unit);
		}

		//Wyrmgus start
		if (this->Player != unit.Player->get_index() && unit.Player->get_type() != player_type::neutral && CPlayer::Players[this->Player]->has_building_access(unit.Player)) { //if the player who gave the order is different from the owner of the building, and the latter is non-neutral (i.e. if the owner of the building is a mercenary company), provide the owner of the building with appropriate recompensation
			unit.Player->change_resource(defines::get()->get_wealth_resource(), newUnit->GetPrice(), true);
		}
		//Wyrmgus end

		//subtract population cost
		if (defines::get()->is_population_enabled()) {
			if (unit.Player == newUnit->Player && unit.get_settlement() != nullptr && !is_hired && nType.get_population_cost() > 0) {
				site_game_data *settlement_game_data = unit.get_settlement()->get_game_data();

				const population_type *population_type = settlement_game_data->get_class_population_type(nType.get_population_class());

				assert_log(population_type != nullptr);

				if (population_type != nullptr) {
					settlement_game_data->change_population_type_population(population_type, -nType.get_population_cost());
				}

				newUnit->set_home_settlement(unit.get_settlement());
			}
		}
		
		if (&player == CPlayer::GetThisPlayer()) {
			PlayUnitSound(newUnit, unit_sound_type::ready);
		}
		if (newUnit->Player->AiEnabled) {
			AiTrainingComplete(unit, *newUnit);
		}

		/*
		if (unit.NewOrder && unit.NewOrder->HasGoal()
			&& unit.NewOrder->GetGoal()->Destroyed) {
			unit.NewOrder.reset();
		}

		if (CanHandleOrder(*newUnit, unit.NewOrder) == true) {
			newUnit->Orders[0] = unit.NewOrder->Clone();
		} else {
	#if 0
			// Tell the unit to right-click ?
	#endif
		}
		*/
		
		if (unit.has_rally_point() && newUnit->CanMove()) {
			bool command_found = false;
			std::vector<CUnit *> table;
			Select(unit.get_rally_point_pos(), unit.get_rally_point_pos(), table, unit.get_rally_point_map_layer()->ID);
			for (size_t j = 0; j != table.size(); ++j) {
				if (!table[j]->IsAliveOnMap() || table[j]->Type->BoolFlag[DECORATION_INDEX].value) {
					continue;
				}
				if (newUnit->can_repair() && table[j]->Type->get_repair_hp() != 0 && table[j]->Variable[HP_INDEX].Value < table[j]->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) && (table[j]->Player == newUnit->Player || newUnit->is_allied_with(*table[j]))) { //see if can repair
					CommandRepair(*newUnit, unit.get_rally_point_pos(), table[j], FlushCommands, unit.get_rally_point_map_layer()->ID);
					command_found = true;
				} else if (newUnit->can_harvest(table[j])) { // see if can harvest
					CommandResource(*newUnit, *table[j], FlushCommands);
					command_found = true;
				} else if (newUnit->Type->BoolFlag[HARVESTER_INDEX].value && table[j]->Type->get_given_resource() != nullptr && newUnit->Type->get_resource_info(table[j]->Type->get_given_resource()) != nullptr && !table[j]->Type->BoolFlag[CANHARVEST_INDEX].value && (table[j]->Player == newUnit->Player || table[j]->Player->get_index() == PlayerNumNeutral)) { // see if can build mine on top of deposit
					for (const wyrmgus::unit_type *other_unit_type : wyrmgus::unit_type::get_all()) {
						if (other_unit_type->is_template()) {
							continue;
						}

						if (other_unit_type->get_given_resource() == table[j]->Type->get_given_resource() && other_unit_type->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(newUnit, *other_unit_type, table[j]->tilePos, 1, false, table[j]->MapLayer->ID)) {
							CommandBuildBuilding(*newUnit, table[j]->tilePos, *other_unit_type, FlushCommands, table[j]->MapLayer->ID);
							command_found = true;
							break;
						}
					}
				}
				
				if (command_found) {
					break;
				}
			}
			
			if (!command_found && unit.get_rally_point_map_layer()->Field(unit.get_rally_point_pos())->player_info->IsTeamExplored(*newUnit->Player)) { // see if can harvest terrain
				for (const wyrmgus::resource *resource : wyrmgus::resource::get_all()) {
					if (newUnit->Type->get_resource_info(resource) != nullptr && CMap::get()->Field(unit.get_rally_point_pos(), unit.get_rally_point_map_layer()->ID)->get_resource() == resource) {
						CommandResourceLoc(*newUnit, unit.get_rally_point_pos(), FlushCommands, unit.get_rally_point_map_layer()->ID);
						command_found = true;
						break;
					}
				}
			}
			
			if (!command_found) {
				CommandMove(*newUnit, unit.get_rally_point_pos(), FlushCommands, unit.get_rally_point_map_layer()->ID);
			}
		}
	}
	//Wyrmgus end
	this->Finished = true;
	if (IsOnlySelected(unit)) {
		UI.ButtonPanel.Update();
	}
}
