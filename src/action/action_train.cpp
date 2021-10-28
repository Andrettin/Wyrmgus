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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "iolib.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "player/player.h"
#include "player/player_type.h"
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
	CPlayer::Players[player]->SubUnitType(type, trainer.Type->Stats[trainer.Player->get_index()].get_unit_stock(&type) != 0);
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
	file.printf("\"type\", \"%s\",", this->Type->Ident.c_str());
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

/**
**  Unit can handle order.
**
**  @param unit   Newly trained unit.
**  @param order  New order for the unit.
**
**  @return  true if the the unit can do it, false otherwise.
*/
static bool CanHandleOrder(const CUnit &unit, COrder *order)
{
	if (order == nullptr) {
		return false;
	}

	if (order->Action == UnitAction::Resource) {
		//  Check if new unit can harvest.
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value) {
			return false;
		}
		//  Also check if new unit can harvest this specific resource.
		CUnit *goal = order->get_goal();
		//Wyrmgus start
//		if (goal && unit.Type->get_resource_info(goal->Type->get_give_n_resource()) == nullptr) {
		if (goal && unit.Type->get_resource_info(goal->get_given_resource()) == nullptr) {
		//Wyrmgus end
			return false;
		}
		return true;
	}

	//Wyrmgus start
//	if (order->Action == UnitAction::Attack && !unit.Type->CanAttack) {
	if (order->Action == UnitAction::Attack && !unit.CanAttack(true)) {
	//Wyrmgus end
		return false;
	}

	if (order->Action == UnitAction::Board && unit.Type->get_domain() != unit_domain::land) {
		return false;
	}

	return true;
}

static void AnimateActionTrain(CUnit &unit)
{
	if (unit.get_animation_set()->Train) {
		UnitShowAnimation(unit, unit.get_animation_set()->Train.get());
	} else {
		UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
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
	const int food = player.CheckLimits(nType);
	if (food < 0) {
		if (food == -3 && player.AiEnabled) {
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
	
	if (unit.Type->Stats[unit.Player->get_index()].get_unit_stock(&nType) != 0) { // if the training unit/building has a "stock" of the trained unit, that means it should be created with no time wait
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
	const int food = player.CheckLimits(nType);
	if (food < 0) {
		if (food == -3 && unit.Player->AiEnabled) {
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
//		player.Notify(NotifyYellow, unit.tilePos, _("Unable to train %s"), nType.Name.c_str());
		player.Notify(NotifyYellow, unit.tilePos, _("Unable to train %s"), nType.GetDefaultName(player).c_str());
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

	DropOutOnSide(*newUnit, LookingW, &unit);
	//Wyrmgus start
	//we don't need to send the player a message every time a new unit is ready
	//player.Notify(NotifyGreen, newUnit->tilePos, _("New %s ready"), nType.Name.c_str());
	//Wyrmgus end
	if (&player == ThisPlayer) {
		PlayUnitSound(*newUnit, VoiceReady);
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
		if (unit.Type->Stats[unit.Player->get_index()].get_unit_stock(&nType) != 0) {
			if (unit.GetUnitStock(&nType) > 0) {
				unit.ChangeUnitStock(&nType, -1);
			} else {
				continue; //don't create the unit if no further stock of it is available
			}
		}
		
		//Wyrmgus start
//		CUnit *newUnit = MakeUnit(nType, &player);
		CUnit *newUnit = MakeUnit(nType, CPlayer::Players[owner_player].get());
		//Wyrmgus end

		if (newUnit == nullptr) { // No more memory :/
			//Wyrmgus start
	//		player.Notify(NotifyYellow, unit.tilePos, _("Unable to train %s"), nType.Name.c_str());
			player.Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID, _("Unable to train %s"), nType.GetDefaultName(&player).c_str());
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
			DropOutNearest(*newUnit, unit.get_rally_point_pos(), &unit);
		} else {
			DropOutOnSide(*newUnit, LookingW, &unit);
		}

		//Wyrmgus start
		if (this->Player != unit.Player->get_index() && unit.Player->get_type() != player_type::neutral && CPlayer::Players[this->Player]->has_building_access(unit.Player)) { //if the player who gave the order is different from the owner of the building, and the latter is non-neutral (i.e. if the owner of the building is a mercenary company), provide the owner of the building with appropriate recompensation
			unit.Player->change_resource(defines::get()->get_wealth_resource(), newUnit->GetPrice(), true);
		}
		//Wyrmgus end
		
		//we don't need to send the player a message every time a new unit is ready
		//player.Notify(NotifyGreen, newUnit->tilePos, _("New %s ready"), nType.Name.c_str());

		if (&player == CPlayer::GetThisPlayer()) {
			PlayUnitSound(*newUnit, wyrmgus::unit_sound_type::ready);
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
				if (newUnit->Type->RepairRange && table[j]->Type->get_repair_hp() != 0 && table[j]->Variable[HP_INDEX].Value < table[j]->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) && (table[j]->Player == newUnit->Player || newUnit->is_allied_with(*table[j]))) { //see if can repair
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
