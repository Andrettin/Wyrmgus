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
/**@name action_train.cpp - The building train action. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "action/action_train.h"

#include "ai.h"
#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map_layer.h"
#include "player.h"
#include "sound.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unitsound.h"
#include "unit/unittype.h"

/// How many resources the player gets back if canceling training
#define CancelTrainingCostsFactor  100

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//Wyrmgus start
// /* static */ COrder *COrder::NewActionTrain(CUnit &trainer, CUnitType &type)
/* static */ COrder *COrder::NewActionTrain(CUnit &trainer, CUnitType &type, int player)
//Wyrmgus end
{
	COrder_Train *order = new COrder_Train;

	order->Type = &type;
	// FIXME: if you give quick an other order, the resources are lost!
	//Wyrmgus start
	order->Player = player;
//	trainer.Player->SubUnitType(type);
	Players[player].SubUnitType(type, trainer.Type->Stats[trainer.Player->Index].GetUnitStock(&type) != 0);
	//Wyrmgus end

	return order;
}

/* virtual */ void COrder_Train::Save(CFile &file, const CUnit &unit) const
{
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

/* virtual */ bool COrder_Train::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "type")) {
		++j;
		this->Type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
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

/* virtual */ bool COrder_Train::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Train::Show(const CViewport &, const PixelPos &lastScreenPos) const
{
	return lastScreenPos;
}


/* virtual */ void COrder_Train::Cancel(CUnit &unit)
{
	DebugPrint("Cancel training\n");
	//Wyrmgus start
//	CPlayer &player = *unit.Player;
	CPlayer &player = *(&Players[this->Player]);
	//Wyrmgus end

	//Wyrmgus start
//	player.AddCostsFactor(this->Type->Stats[player.Index].Costs, CancelTrainingCostsFactor);
	int type_costs[MaxCosts];
	player.GetUnitTypeCosts(this->Type, type_costs);
	player.AddCostsFactor(type_costs, CancelTrainingCostsFactor);
	//Wyrmgus end
}

/* virtual */ void COrder_Train::UpdateUnitVariables(CUnit &unit) const
{
	Assert(unit.CurrentOrder() == this);

	unit.Variable[TRAINING_INDEX].Value = this->Ticks;
	//Wyrmgus start
//	unit.Variable[TRAINING_INDEX].Max = this->Type->Stats[unit.Player->Index].Costs[TimeCost];
	unit.Variable[TRAINING_INDEX].Max = this->Type->Stats[this->Player].Costs[TimeCost];
	//Wyrmgus end
}

void COrder_Train::ConvertUnitType(const CUnit &unit, CUnitType &newType)
{
	//Wyrmgus start
//	const CPlayer &player = *unit.Player;
	const CPlayer &player = *(&Players[this->Player]);
	//Wyrmgus end
	const int oldCost = this->Type->Stats[player.Index].Costs[TimeCost];
	const int newCost = newType.Stats[player.Index].Costs[TimeCost];

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
	if (order->Action == UnitActionResource) {
		//  Check if new unit can harvest.
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value) {
			return false;
		}
		//  Also check if new unit can harvest this specific resource.
		CUnit *goal = order->GetGoal();
		//Wyrmgus start
//		if (goal && !unit.Type->ResInfo[goal->Type->GivesResource]) {
		if (goal && !unit.Type->ResInfo[goal->GivesResource]) {
		//Wyrmgus end
			return false;
		}
		return true;
	}
	//Wyrmgus start
//	if (order->Action == UnitActionAttack && !unit.Type->CanAttack) {
	if (order->Action == UnitActionAttack && !unit.CanAttack(true)) {
	//Wyrmgus end
		return false;
	}
	if (order->Action == UnitActionBoard && unit.Type->UnitType != UnitTypeLand) {
		return false;
	}
	return true;
}

static void AnimateActionTrain(CUnit &unit)
{
	//Wyrmgus start
	/*
	if (unit.Type->Animations->Train) {
		UnitShowAnimation(unit, unit.Type->Animations->Train);
	} else {
		UnitShowAnimation(unit, unit.Type->Animations->Still);
	}
	*/
	if (unit.GetAnimations()->Train) {
		UnitShowAnimation(unit, unit.GetAnimations()->Train);
	} else {
		UnitShowAnimation(unit, unit.GetAnimations()->Still);
	}
	//Wyrmgus end
}

/* virtual */ void COrder_Train::Execute(CUnit &unit)
{
	//Wyrmgus start
	/*
	AnimateActionTrain(unit);
	if (unit.Wait) {
		unit.Wait--;
		return ;
	}
	*/
	if (unit.CriticalOrder != this) {
		AnimateActionTrain(unit);
		if (unit.Wait) {
			unit.Wait--;
			return ;
		}
	}
	//Wyrmgus end
	//Wyrmgus start
//	CPlayer &player = *unit.Player;
	CPlayer &player = *(&Players[this->Player]);
	//Wyrmgus end
	CUnitType &nType = *this->Type;
	const int cost = nType.Stats[player.Index].Costs[TimeCost];
	
	//Wyrmgus start
	// Check if enough supply available.
	const int food = player.CheckLimits(nType);
	if (food < 0) {
		if (food == -3 && player.AiEnabled) {
			AiNeedMoreSupply(player);
		}
		this->Ticks = 0;
		unit.Wait = CYCLES_PER_SECOND / 6;
		return ;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	this->Ticks += std::max(1, player.SpeedTrain / SPEEDUP_FACTOR);
	this->Ticks += std::max(1, (player.SpeedTrain + unit.Variable[TIMEEFFICIENCYBONUS_INDEX].Value) / SPEEDUP_FACTOR);
	
	if (unit.Type->Stats[unit.Player->Index].GetUnitStock(&nType) != 0) { // if the training unit/building has a "stock" of the trained unit, that means it should be created with no time wait
		this->Ticks = cost;
	}
	//Wyrmgus end

	if (this->Ticks < cost) {
		unit.Wait = CYCLES_PER_SECOND / 6;
		return ;
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
		return ;
	}
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	if (nType.BoolFlag[RAIL_INDEX].value && !unit.HasAdjacentRailForUnitType(&nType)) {
		if (&player == ThisPlayer) {
			ThisPlayer->Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID, "%s", _("The unit requires railroads to be placed on"));
			PlayGameSound(GameSounds.PlacementError[ThisPlayer->Race].Sound, MaxSampleVolume);
		}
		unit.Wait = CYCLES_PER_SECOND * 10;
		return;
	}
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
		return ;
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
		delete unit.NewOrder;
		unit.NewOrder = nullptr;
	}

	if (CanHandleOrder(*newUnit, unit.NewOrder) == true) {
		delete newUnit->Orders[0];
		newUnit->Orders[0] = unit.NewOrder->Clone();
	} else {
#if 0
		// Tell the unit to right-click ?
#endif
	}
	*/
	for (int i = 0; i < (this->Type->TrainQuantity ? this->Type->TrainQuantity : 1); ++i) {
		if (unit.Type->Stats[unit.Player->Index].GetUnitStock(&nType) != 0) {
			if (unit.GetUnitStock(&nType) > 0) {
				unit.ChangeUnitStock(&nType, -1);
			} else {
				continue; //don't create the unit if no further stock of it is available
			}
		}
		
		//Wyrmgus start
//		CUnit *newUnit = MakeUnit(nType, &player);
		CUnit *newUnit = MakeUnit(nType, &Players[owner_player]);
		//Wyrmgus end

		if (newUnit == nullptr) { // No more memory :/
			//Wyrmgus start
	//		player.Notify(NotifyYellow, unit.tilePos, _("Unable to train %s"), nType.Name.c_str());
			player.Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID, _("Unable to train %s"), nType.GetDefaultName(player).c_str());
			//Wyrmgus end
			unit.Wait = CYCLES_PER_SECOND / 6;
			return ;
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

		DropOutOnSide(*newUnit, LookingW, &unit);

		//Wyrmgus start
		if (this->Player != unit.Player->Index && unit.Player->Type != PlayerNeutral && Players[this->Player].HasBuildingAccess(*unit.Player)) { //if the player who gave the order is different from the owner of the building, and the latter is non-neutral (i.e. if the owner of the building is a mercenary company), provide the owner of the building with appropriate recompensation
			unit.Player->ChangeResource(CopperCost, newUnit->GetPrice(), true);
		}
		//Wyrmgus end
		
		//we don't need to send the player a message every time a new unit is ready
		//player.Notify(NotifyGreen, newUnit->tilePos, _("New %s ready"), nType.Name.c_str());

		if (&player == ThisPlayer) {
			PlayUnitSound(*newUnit, VoiceReady);
		}
		if (newUnit->Player->AiEnabled) {
			AiTrainingComplete(unit, *newUnit);
		}

		/*
		if (unit.NewOrder && unit.NewOrder->HasGoal()
			&& unit.NewOrder->GetGoal()->Destroyed) {
			delete unit.NewOrder;
			unit.NewOrder = nullptr;
		}

		if (CanHandleOrder(*newUnit, unit.NewOrder) == true) {
			delete newUnit->Orders[0];
			newUnit->Orders[0] = unit.NewOrder->Clone();
		} else {
	#if 0
			// Tell the unit to right-click ?
	#endif
		}
		*/
		
		if (unit.RallyPointPos.x != -1 && unit.RallyPointPos.y != -1 && unit.RallyPointMapLayer && newUnit->CanMove()) {
			bool command_found = false;
			std::vector<CUnit *> table;
			Select(unit.RallyPointPos, unit.RallyPointPos, table, unit.RallyPointMapLayer->ID);
			for (size_t j = 0; j != table.size(); ++j) {
				if (!table[j]->IsAliveOnMap() || table[j]->Type->BoolFlag[DECORATION_INDEX].value) {
					continue;
				}
				if (newUnit->Type->RepairRange && table[j]->Type->RepairHP && table[j]->Variable[HP_INDEX].Value < table[j]->GetModifiedVariable(HP_INDEX, VariableMax) && (table[j]->Player == newUnit->Player || newUnit->IsAllied(*table[j]))) { //see if can repair
					CommandRepair(*newUnit, unit.RallyPointPos, table[j], FlushCommands, unit.RallyPointMapLayer->ID);
					command_found = true;
				} else if (newUnit->CanHarvest(table[j])) { // see if can harvest
					CommandResource(*newUnit, *table[j], FlushCommands);
					command_found = true;
				} else if (newUnit->Type->BoolFlag[HARVESTER_INDEX].value && table[j]->Type->GivesResource && newUnit->Type->ResInfo[table[j]->Type->GivesResource] && !table[j]->Type->BoolFlag[CANHARVEST_INDEX].value && (table[j]->Player == newUnit->Player || table[j]->Player->Index == PlayerNumNeutral)) { // see if can build mine on top of deposit
					for (size_t z = 0; z < UnitTypes.size(); ++z) {
						if (UnitTypes[z] && UnitTypes[z]->GivesResource == table[j]->Type->GivesResource && UnitTypes[z]->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(newUnit, *UnitTypes[z], table[j]->tilePos, 1, false, table[j]->MapLayer->ID)) {
							CommandBuildBuilding(*newUnit, table[j]->tilePos, *UnitTypes[z], FlushCommands, table[j]->MapLayer->ID);
							command_found = true;
							break;
						}
					}
				}
				
				if (command_found) {
					break;
				}
			}
			
			if (!command_found && unit.RallyPointMapLayer->Field(unit.RallyPointPos)->playerInfo.IsTeamExplored(*newUnit->Player)) { // see if can harvest terrain
				for (size_t res = 0; res < CResource::Resources.size(); ++res) {
					if (newUnit->Type->ResInfo[res] && CMap::Map.Field(unit.RallyPointPos, unit.RallyPointMapLayer->ID)->IsTerrainResourceOnMap(res)) {
						CommandResourceLoc(*newUnit, unit.RallyPointPos, FlushCommands, unit.RallyPointMapLayer->ID);
						command_found = true;
						break;
					}
				}
			}
			
			if (!command_found) {
				CommandMove(*newUnit, unit.RallyPointPos, FlushCommands, unit.RallyPointMapLayer->ID);
			}
		}
	}
	//Wyrmgus end
	this->Finished = true;
	if (IsOnlySelected(unit)) {
		UI.ButtonPanel.Update();
	}

}
