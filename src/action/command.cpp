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
/**@name command.cpp - Give units a command. */
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

#include "actions.h"
#include "action/action_built.h"
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
#include "commands.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder.h"
#include "player.h"
#include "spells.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "unit/unittype.h"
#include "upgrade/upgrade.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Release all orders of a unit.
**
**  @param unit  Pointer to unit.
*/
static void ReleaseOrders(CUnit &unit)
{
	Assert(unit.Orders.empty() == false);

	// Order 0 must be stopped in the action loop.
	for (size_t i = 0; i != unit.Orders.size(); ++i) {
		if (unit.Orders[i]->Action == UnitActionBuilt) {
			(dynamic_cast<COrder_Built *>(unit.Orders[i]))->Cancel(unit);
		} else if (unit.Orders[i]->Action == UnitActionResearch) {
			(dynamic_cast<COrder_Research *>(unit.Orders[i]))->Cancel(unit);
		} else if (unit.Orders[i]->Action == UnitActionTrain) {
			(dynamic_cast<COrder_Train *>(unit.Orders[i]))->Cancel(unit);
		} else if (unit.Orders[i]->Action == UnitActionUpgradeTo) {
			(dynamic_cast<COrder_UpgradeTo *>(unit.Orders[i]))->Cancel(unit);
		}
		if (i > 0) {
			delete unit.Orders[i];
		}
	}
	unit.Orders.resize(1);
	//Wyrmgus start
//	unit.Orders[0]->Finished = true;
	if (unit.Variable[STUN_INDEX].Value == 0 || unit.Orders[0]->Action != UnitActionStill) { //if the unit is stunned, don't end its current "still" order
		unit.Orders[0]->Finished = true;
	}
	//Wyrmgus end
}

/**
**  Get next free order slot.
**
**  @param unit   pointer to unit.
**  @param flush  if true, flush order queue.
**
**  @return       Pointer to next free order slot.
*/
static COrderPtr *GetNextOrder(CUnit &unit, int flush)
{
	//Wyrmgus start
//	if (flush) {
	if (flush && unit.CurrentAction() != UnitActionUpgradeTo && unit.CurrentAction() != UnitActionTrain && unit.CurrentAction() != UnitActionResearch) { //training, researching and upgrading must be canceled manually
	//Wyrmgus end
		// empty command queue
		ReleaseOrders(unit);
	}
	// FIXME : Remove Hardcoded value.
	const unsigned int maxOrderCount = 0x7F;

	if (unit.Orders.size() == maxOrderCount) {
		return nullptr;
	}
	unit.Orders.push_back(nullptr);
	return &unit.Orders.back();
}

/**
**  Remove an order from the list of orders pending
**
**  @param unit   pointer to unit
**  @param order  number of the order to remove
*/
static void RemoveOrder(CUnit &unit, unsigned int order)
{
	Assert(order < unit.Orders.size());

	delete unit.Orders[order];
	unit.Orders.erase(unit.Orders.begin() + order);
	if (unit.Orders.empty()) {
		unit.Orders.push_back(COrder::NewActionStill());
	}
}

static void ClearNewAction(CUnit &unit)
{
	delete unit.NewOrder;
	unit.NewOrder = nullptr;
}

/**
**  Clear the saved action.
**
**  @param unit  Unit pointer, that get the saved action cleared.
**
**  @note        If we make a new order, we must clear any saved actions.
**  @note        Internal functions, must protect it, if needed.
*/
static void ClearSavedAction(CUnit &unit)
{
	delete unit.SavedOrder;
	unit.SavedOrder = nullptr;
}

static bool IsUnitValidForNetwork(const CUnit &unit)
{
	return !unit.Removed && unit.CurrentAction() != UnitActionDie;
}

//Wyrmgus start
static void StopRaft(CUnit &unit)
{
	CMapField &mf = *unit.MapLayer->Field(unit.tilePos);
	if ((mf.Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) { 
		std::vector<CUnit *> table;
		Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
		for (size_t i = 0; i != table.size(); ++i) {
			if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
				CommandStopUnit(*table[i]); //always stop the raft if a new command is issued
			}
		}
	}
}

static std::vector<CUnit *> GetLayerConnectorPath(CUnit &unit, int old_z, int new_z, std::vector<CUnit *> &checked_connectors)
{
	for (size_t i = 0; i != CMap::Map.MapLayers[old_z]->LayerConnectors.size(); ++i) {
		CUnit *connector = CMap::Map.MapLayers[old_z]->LayerConnectors[i];
		CUnit *connector_destination = connector->ConnectingDestination;
		std::vector<CUnit *> connector_path;
		if (std::find(checked_connectors.begin(), checked_connectors.end(), connector) == checked_connectors.end() && unit.CanUseItem(connector) && connector->IsVisibleAsGoal(*unit.Player)) {
			connector_path.push_back(connector);
			checked_connectors.push_back(connector);
			checked_connectors.push_back(connector_destination);
			if (connector_destination->MapLayer->ID == new_z) {
				return connector_path;
			} else {
				std::vector<CUnit *> next_connector_path = GetLayerConnectorPath(unit, connector_destination->MapLayer->ID, new_z, checked_connectors);
				if (next_connector_path.size() > 0) {
					for (size_t j = 0; j != next_connector_path.size(); ++j) {
						connector_path.push_back(next_connector_path[j]);
					}
					return connector_path;
				}
			}
		}
	}
	
	std::vector<CUnit *> empty_connector_path;
	return empty_connector_path;
}

static void ReachGoalLayer(CUnit &unit, int new_z, int &flush)
{
	if (unit.MapLayer->ID == new_z) { // already on the correct layer
		return;
	}
	
	std::vector<CUnit *> checked_connectors;
	std::vector<CUnit *> connector_path = GetLayerConnectorPath(unit, unit.MapLayer->ID, new_z, checked_connectors);
	if (connector_path.size() > 0) {
		for (size_t i = 0; i < connector_path.size(); ++i) {
			CommandUse(unit, *connector_path[i], flush, false);
			flush = 0;
		}
	}
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Commands
----------------------------------------------------------------------------*/

/**
**  Stop unit.
**
**  @param unit  pointer to unit.
*/
void CommandStopUnit(CUnit &unit)
{
	// Ignore that the unit could be removed.
	COrderPtr *order = GetNextOrder(unit, FlushCommands); // Flush them.
	Assert(order);
	Assert(*order == nullptr);
	*order = COrder::NewActionStill();

	ClearSavedAction(unit);
	ClearNewAction(unit);
}

/**
**  Stand ground.
**
**  @param unit   pointer to unit.
**  @param flush  if true, flush command queue.
*/
void CommandStandGround(CUnit &unit, int flush)
{
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionStandGround();
	ClearSavedAction(unit);
}

/**
**  Follow unit and defend it
**
**  @param unit   pointer to unit.
**  @param dest   unit to follow
**  @param flush  if true, flush command queue.
*/
void CommandDefend(CUnit &unit, CUnit &dest, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionDefend(dest);
	ClearSavedAction(unit);
}

/**
**  Follow unit to new position
**
**  @param unit   pointer to unit.
**  @param dest   unit to be followed
**  @param flush  if true, flush command queue.
*/
void CommandFollow(CUnit &unit, CUnit &dest, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionFollow(dest);
	ClearSavedAction(unit);
}

/**
**  Move unit to new position
**
**  @param unit   pointer to unit.
**  @param pos    map position to move to.
**  @param flush  if true, flush command queue.
*/
void CommandMove(CUnit &unit, const Vec2i &pos, int flush, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	CMapField &mf = *unit.MapLayer->Field(unit.tilePos);
	CMapField &new_mf = *CMap::Map.Field(pos, z);
	//if the unit is a land unit over a raft, move the raft instead of the unit
	if ((mf.Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) { 
		std::vector<CUnit *> table;
		Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
		for (size_t i = 0; i != table.size(); ++i) {
			if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
				CommandStopUnit(*table[i]); //always stop the raft if a new command is issued
				if ((new_mf.Flags & MapFieldWaterAllowed) || (new_mf.Flags & MapFieldCoastAllowed) || (mf.Flags & MapFieldWaterAllowed)) { // if is standing on water, tell the raft to go to the nearest coast, even if the ultimate goal is on land
					CommandStopUnit(unit);
					CommandMove(*table[i], pos, flush, z);
					return;
				}
			}
		}
	}
	
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	//Wyrmgus start
//	*order = COrder::NewActionMove(pos);
	*order = COrder::NewActionMove(pos, z);
	//Wyrmgus end
	ClearSavedAction(unit);
}

//Wyrmgus start
/**
**  Set new rally point for unit
**
**  @param unit   pointer to unit.
**  @param pos    new rally point map position.
*/
void CommandRallyPoint(CUnit &unit, const Vec2i &pos, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));
	
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.RallyPointPos = pos;
	unit.RallyPointMapLayer = CMap::Map.MapLayers[z];
}

/**
**  Pick up item
**
**  @param unit   pointer to unit.
**  @param dest   item to be picked up
**  @param flush  if true, flush command queue.
*/
void CommandPickUp(CUnit &unit, CUnit &dest, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionPickUp(dest);
	ClearSavedAction(unit);
}

/**
**  Accept new quest for unit's player
**
**  @param unit   pointer to unit.
**  @param quest  quest.
*/
void CommandQuest(CUnit &unit, CQuest *quest)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.Player->AcceptQuest(quest);
}

/**
**  Buy an item
**
**  @param unit   pointer to unit to buy from.
**  @param sold_unit  pointer to bought unit.
*/
void CommandBuy(CUnit &unit, CUnit *sold_unit, int player)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.SellUnit(sold_unit, player);
}

/**
**  Produce a resource
**
**  @param unit   pointer to unit.
**  @param resource  index of the resource.
*/
void CommandProduceResource(CUnit &unit, int resource)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.ProduceResource(resource);
}

/**
**  Sell a resource for copper
**
**  @param unit   pointer to unit.
**  @param resource  index of the resource.
*/
void CommandSellResource(CUnit &unit, int resource, int player)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.SellResource(resource, player);
}

/**
**  Buy a resource with copper
**
**  @param unit   pointer to unit.
**  @param resource  index of the resource.
*/
void CommandBuyResource(CUnit &unit, int resource, int player)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.BuyResource(resource, player);
}
//Wyrmgus end

/**
**  Repair unit
**
**  @param unit   pointer to unit.
**  @param pos    map position to repair.
**  @param dest   or unit to be repaired. FIXME: not supported
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush)
void CommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	if (dest) {
		*order = COrder::NewActionRepair(unit, *dest);
	} else {
		//Wyrmgus start
//		*order = COrder::NewActionRepair(pos);
		*order = COrder::NewActionRepair(pos, z);
		//Wyrmgus end
	}
	ClearSavedAction(unit);
}

/**
**  Auto repair.
**
**  @param unit     pointer to unit.
**  @param on       1 for auto repair on, 0 for off.
*/
void CommandAutoRepair(CUnit &unit, int on)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.AutoRepair = on;
}

/**
**  Attack with unit at new position
**
**  @param unit    pointer to unit.
**  @param pos     map position to attack.
**  @param target  or unit to be attacked.
**  @param flush   if true, flush command queue.
*/
void CommandAttack(CUnit &unit, const Vec2i &pos, CUnit *target, int flush, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	CMapField &mf = *unit.MapLayer->Field(unit.tilePos);
	CMapField &new_mf = *CMap::Map.Field(pos, z);
	if ((mf.Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) { 
		std::vector<CUnit *> table;
		Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
		for (size_t i = 0; i != table.size(); ++i) {
			if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
				CommandStopUnit(*table[i]); //always stop the raft if a new command is issued
			}
		}
	}

	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end

	COrderPtr *order;

	//Wyrmgus start
//	if (!unit.Type->CanAttack) {
	if (!unit.CanAttack(true)) {
	//Wyrmgus end
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	if (target && target->IsAlive()) {
		*order = COrder::NewActionAttack(unit, *target);
	} else {
		//Wyrmgus start
//		*order = COrder::NewActionAttack(unit, pos);
		*order = COrder::NewActionAttack(unit, pos, z);
		//Wyrmgus end
	}
	ClearSavedAction(unit);
}

/**
**  Attack ground with unit.
**
**  @param unit   pointer to unit.
**  @param pos    map position to fire on.
**  @param flush  if true, flush command queue.
*/
void CommandAttackGround(CUnit &unit, const Vec2i &pos, int flush, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	//Wyrmgus start
//	if (!unit.Type->CanAttack) {
	if (!unit.CanAttack(true)) {
	//Wyrmgus end
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	//Wyrmgus start
//	*order = COrder::NewActionAttackGround(unit, pos);
	*order = COrder::NewActionAttackGround(unit, pos, z);
	//Wyrmgus end
	ClearSavedAction(unit);
}

//Wyrmgus start
/**
**  Use unit
**
**  @param unit   pointer to unit.
**  @param dest   unit to be used
**  @param flush  if true, flush command queue.
*/
void CommandUse(CUnit &unit, CUnit &dest, int flush, bool reach_layer)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	if (reach_layer) {
		ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	}
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionUse(dest);
	ClearSavedAction(unit);
}

/**
**  Trade with unit
**
**  @param unit   pointer to unit.
**  @param dest   unit to be traded with
**  @param flush  if true, flush command queue.
*/
void CommandTrade(CUnit &unit, CUnit &dest, int flush, bool reach_layer)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}

	CUnit *home_market = FindHomeMarket(unit, 1000);

	if (!home_market) {
		return; // no home market, cannot trade
	}
	
	StopRaft(unit);
	if (reach_layer) {
		ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	}
	
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	
	*order = COrder::NewActionTrade(dest, *home_market);
	ClearSavedAction(unit);
}
//Wyrmgus end

/**
**  Let a unit patrol from current to new position
**
**  FIXME: want to support patroling between units.
**
**  @param unit   pointer to unit.
**  @param pos    map position to patrol between.
**  @param flush  if true, flush command queue.
*/
void CommandPatrolUnit(CUnit &unit, const Vec2i &pos, int flush, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (!unit.CanMove()) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	//Wyrmgus start
//	*order = COrder::NewActionPatrol(unit.tilePos, pos);
	*order = COrder::NewActionPatrol(unit.tilePos, pos, unit.MapLayer->ID, z);
	//Wyrmgus end

	ClearSavedAction(unit);
}

/**
**  Board a transporter with unit.
**
**  @param unit   pointer to unit.
**  @param dest   unit to be boarded.
**  @param flush  if true, flush command queue.
*/
void CommandBoard(CUnit &unit, CUnit &dest, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	if (dest.Destroyed) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionBoard(dest);
	ClearSavedAction(unit);
}

/**
**  Unload a transporter.
**
**  @param unit   pointer to unit.
**  @param pos    map position to unload.
**  @param what   unit to be unloaded, null for all.
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush)
void CommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush, int z, int landmass)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	
	//Wyrmgus start
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	
	COrderPtr *order = GetNextOrder(unit, flush);

	if (order == nullptr) {
		return;
	}
	//Wyrmgus start
//	*order = COrder::NewActionUnload(pos, what);
	*order = COrder::NewActionUnload(pos, what, z, landmass);
	//Wyrmgus end
	ClearSavedAction(unit);
}

/**
**  Send a unit building
**
**  @param unit   pointer to unit.
**  @param pos    map position to build.
**  @param what   Unit type to build.
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandBuildBuilding(CUnit &unit, const Vec2i &pos, CUnitType &what, int flush)
void CommandBuildBuilding(CUnit &unit, const Vec2i &pos, CUnitType &what, int flush, int z, CSite *settlement)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	//Wyrmgus start
//	if (unit.Type->BoolFlag[BUILDING_INDEX].value && !what.BoolFlag[BUILDEROUTSIDE_INDEX].value && unit.MapDistanceTo(pos) > unit.Type->RepairRange) {
	if (unit.Type->BoolFlag[BUILDING_INDEX].value && !what.BoolFlag[BUILDEROUTSIDE_INDEX].value && unit.MapDistanceTo(pos, z) > unit.Type->RepairRange) {
	//Wyrmgus end
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	//Wyrmgus start
//	*order = COrder::NewActionBuild(unit, pos, what);
	*order = COrder::NewActionBuild(unit, pos, what, z, settlement);
	//Wyrmgus end
	ClearSavedAction(unit);
}

/**
**  Cancel the building construction, or kill a unit.
**
**  @param unit  pointer to unit.
*/
void CommandDismiss(CUnit &unit, bool salvage)
{
	// Check if building is still under construction? (NETWORK!)
	if (unit.CurrentAction() == UnitActionBuilt) {
		unit.CurrentOrder()->Cancel(unit);
	} else {
		if (salvage) {
			std::vector<CUnit *> table;
			SelectAroundUnit(unit, 16, table, IsEnemyWith(*unit.Player));
			for (size_t i = 0; i != table.size(); ++i) {
				if (
					(table[i]->CurrentAction() == UnitActionAttack || table[i]->CurrentAction() == UnitActionSpellCast)
					&& table[i]->CurrentOrder()->HasGoal()
					&& table[i]->CurrentOrder()->GetGoal() == &unit
				) {
					if (unit.Player->Index == ThisPlayer->Index) {
						ThisPlayer->Notify(NotifyRed, unit.tilePos, unit.MapLayer->ID, "%s", _("Cannot salvage if enemies are attacking it."));
					}
					return;
				}
			}
			int type_costs[MaxCosts];
			unit.Player->GetUnitTypeCosts(unit.Type, type_costs, false, true);
			unit.Player->AddCostsFactor(type_costs, unit.Variable[SALVAGEFACTOR_INDEX].Value * unit.Variable[HP_INDEX].Value / unit.GetModifiedVariable(HP_INDEX, VariableMax));
		}
		DebugPrint("Suicide unit ... \n");
		LetUnitDie(unit, true);
	}
	ClearSavedAction(unit);
}

/**
**  Send unit harvest a location
**
**  @param unit   pointer to unit.
**  @param pos    map position for harvest.
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush)
void CommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush, int z)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	if (!unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.Type->BoolFlag[HARVESTER_INDEX].value) {
		ClearSavedAction(unit);
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	//Wyrmgus start
//	*order = COrder::NewActionResource(unit, pos);
	*order = COrder::NewActionResource(unit, pos, z);
	//Wyrmgus end
	ClearSavedAction(unit);
}

/**
**  Send unit to harvest resources
**
**  @param unit   pointer to unit.
**  @param dest   destination unit.
**  @param flush  if true, flush command queue.
*/
void CommandResource(CUnit &unit, CUnit &dest, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	if (dest.Destroyed) {
		return ;
	}
	if (!unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.Type->BoolFlag[HARVESTER_INDEX].value) {
		ClearSavedAction(unit);
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, dest.MapLayer->ID, flush);
	//Wyrmgus end
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionResource(unit, dest);
	ClearSavedAction(unit);
}

/**
**  Let unit returning goods.
**
**  @param unit   pointer to unit.
**  @param depot  bring goods to this depot.
**  @param flush  if true, flush command queue.
*/
void CommandReturnGoods(CUnit &unit, CUnit *depot, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	if ((unit.Type->BoolFlag[HARVESTER_INDEX].value && unit.ResourcesHeld == 0)
		|| (!unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.Type->BoolFlag[HARVESTER_INDEX].value)) {
		ClearSavedAction(unit);
		return ;
	}
	
	//Wyrmgus start
	if (depot) {
		ReachGoalLayer(unit, depot->MapLayer->ID, flush);
	}
	//Wyrmgus end
	
	COrderPtr *order;

	if (unit.Type->BoolFlag[BUILDING_INDEX].value) {
		ClearNewAction(unit);
		order = &unit.NewOrder;
	} else {
		order = GetNextOrder(unit, flush);
		if (order == nullptr) {
			return;
		}
	}
	*order = COrder::NewActionReturnGoods(unit, depot);
	ClearSavedAction(unit);
}

/**
**  Building starts training an unit.
**
**  @param unit   pointer to unit.
**  @param type   unit type to train.
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandTrainUnit(CUnit &unit, CUnitType &type, int)
void CommandTrainUnit(CUnit &unit, CUnitType &type, int player, int)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	// Check if enough resources remains? (NETWORK!)
	// FIXME: wrong if append to message queue!!!
	//Wyrmgus start
//	if (unit.Player->CheckLimits(type) < 0
//		|| unit.Player->CheckUnitType(type)) {
	if (Players[player].CheckLimits(type) < 0
		|| Players[player].CheckUnitType(type, unit.Type->Stats[unit.Player->Index].GetUnitStock(&type) != 0)) {
	//Wyrmgus end
		return;
	}
	//Wyrmgus start
	if (unit.Type->Stats[unit.Player->Index].GetUnitStock(&type) != 0 && unit.GetUnitStock(&type) <= 0) {
		if (player == ThisPlayer->Index) {
			ThisPlayer->Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID, "%s", _("The stock is empty, wait until it is replenished."));
		}
		return;
	}
	
	if (unit.Player->Index != player) { //if the player "training" the unit isn't the same one that owns the trainer building, then make the former share some technological progress with the latter
		Players[player].ShareUpgradeProgress(*unit.Player, unit);
	}

	if (unit.Type->Stats[unit.Player->Index].GetUnitStock(&type) != 0) { //if the trainer unit/building has a stock of the unit type to be trained, do this as a critical order
		if (unit.CriticalOrder && unit.CriticalOrder->Action == UnitActionTrain) {
			return;
		}
		Assert(unit.CriticalOrder == nullptr);
		
		unit.CriticalOrder = COrder::NewActionTrain(unit, type, player);
		return;
	}
	//Wyrmgus end

	// Not already training?
	if (!EnableTrainingQueue && unit.CurrentAction() == UnitActionTrain) {
		DebugPrint("Unit queue disabled!\n");
		return;
	}
	
	const int noFlushCommands = 0;
	COrderPtr *order = GetNextOrder(unit, noFlushCommands);

	if (order == nullptr) {
		return;
	}
	//Wyrmgus start
//	*order = COrder::NewActionTrain(unit, type);
	*order = COrder::NewActionTrain(unit, type, player);
	//Wyrmgus end
	ClearSavedAction(unit);
}

/**
**  Cancel the training of an unit.
**
**  @param unit  pointer to unit.
**  @param slot  slot number to cancel.
**  @param type  Unit-type to cancel.
*/
void CommandCancelTraining(CUnit &unit, int slot, const CUnitType *type)
{
	DebugPrint("Cancel %d type: %s\n" _C_ slot _C_
			   type ? type->Ident.c_str() : "-any-");

	ClearSavedAction(unit);

	// Check if unit is still training 'slot'? (NETWORK!)
	if (slot == -1) {
		// Cancel All training
		while (unit.CurrentAction() == UnitActionTrain) {
			unit.CurrentOrder()->Cancel(unit);
			RemoveOrder(unit, 0);
		}
		if (unit.Player == ThisPlayer && unit.Selected) {
			SelectedUnitChanged();
		}
	} else if (unit.Orders.size() <= static_cast<size_t>(slot)) {
		// Order has moved
		return;
	} else if (unit.Orders[slot]->Action != UnitActionTrain) {
		// Order has moved, we are not training
		return;
	} else if (unit.Orders[slot]->Action == UnitActionTrain) {
		COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[slot]);
		// Still training this order, same unit?
		if (type && &order.GetUnitType() != type) {
			// Different unit being trained
			return;
		}
		order.Cancel(unit);
		RemoveOrder(unit, slot);

		// Update interface.
		if (unit.Player == ThisPlayer && unit.Selected) {
			SelectedUnitChanged();
		}
	}
}

/**
**  Building starts upgrading to.
**
**  @param unit   pointer to unit.
**  @param type   upgrade to type
**  @param flush  if true, flush command queue.
*/
void CommandUpgradeTo(CUnit &unit, CUnitType &type, int flush)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}

	// Check if enough resources remains? (NETWORK!)
	if (unit.Player->CheckUnitType(type)) {
		return;
	}

	COrderPtr *order = GetNextOrder(unit, flush);

	if (order == nullptr) {
		return;
	}
	*order = COrder::NewActionUpgradeTo(unit, type);
	ClearSavedAction(unit);
}

/**
**  Immediate transforming unit into type.
**
**  @param unit   pointer to unit.
**  @param type   upgrade to type
*/
void CommandTransformIntoType(CUnit &unit, CUnitType &type)
{
	if (unit.CriticalOrder && unit.CriticalOrder->Action == UnitActionTransformInto) {
		return;
	}
	Assert(unit.CriticalOrder == nullptr);

	unit.CriticalOrder = COrder::NewActionTransformInto(type);
}

/**
**  Cancel building upgrading to.
**
**  @param unit  pointer to unit.
*/
void CommandCancelUpgradeTo(CUnit &unit)
{
	// Check if unit is still upgrading? (NETWORK!)
	if (unit.CurrentAction() == UnitActionUpgradeTo) {
		unit.CurrentOrder()->Cancel(unit);
		RemoveOrder(unit, 0);
		if (!Selected.empty()) {
			SelectedUnitChanged();
		}
	}
	ClearSavedAction(unit);
}

/**
**  Building starts researching.
**
**  @param unit   pointer to unit.
**  @param what   what to research.
**  @param flush  if true, flush command queue.
*/
//Wyrmgus start
//void CommandResearch(CUnit &unit, CUpgrade &what, int flush)
void CommandResearch(CUnit &unit, CUpgrade &what, int player, int flush)
//Wyrmgus end
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	
	//Wyrmgus start
	if (what.Ability) {
		AbilityAcquire(unit, &what);
		return;
	}
	//Wyrmgus end
	
	// Check if enough resources remains? (NETWORK!)
	//Wyrmgus start
//	if (unit.Player->CheckCosts(what.Costs)) {
	int upgrade_costs[MaxCosts];
	Players[player].GetUpgradeCosts(&what, upgrade_costs);
	if (Players[player].CheckCosts(upgrade_costs)) {
	//Wyrmgus end
		return;
	}
	COrderPtr *order = GetNextOrder(unit, flush);
	if (order == nullptr) {
		return;
	}
	//Wyrmgus start
//	*order = COrder::NewActionResearch(unit, what);
	*order = COrder::NewActionResearch(unit, what, player);
	//Wyrmgus end
	ClearSavedAction(unit);
}

/**
**  Cancel Building researching.
**
**  @param unit  Pointer to unit.
*/
void CommandCancelResearch(CUnit &unit)
{
	// Check if unit is still researching? (NETWORK!)
	if (unit.CurrentAction() == UnitActionResearch) {
		unit.CurrentOrder()->Cancel(unit);
		RemoveOrder(unit, 0);
		if (!Selected.empty()) {
			SelectedUnitChanged();
		}
	}
	ClearSavedAction(unit);
}

//Wyrmgus start
/**
**  Unit starts learning an ability.
**
**  @param unit   pointer to unit.
**  @param what   what to learn.
**  @param flush  if true, flush command queue.
*/
void CommandLearnAbility(CUnit &unit, CUpgrade &what)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	
	if (what.Ability) {
		AbilityAcquire(unit, &what);
	} else { //an individual upgrade of some other kind (i.e. a deity choice)
		IndividualUpgradeAcquire(unit, &what);
	}
}
//Wyrmgus end

/**
**  Cast a spell at position or unit.
**
**  @param unit   Pointer to unit.
**  @param pos    map position to spell cast on.
**  @param dest   Spell cast on unit (if exist).
**  @param spell  Spell type pointer.
**  @param flush  If true, flush command queue.
*/
void CommandSpellCast(CUnit &unit, const Vec2i &pos, CUnit *dest, const CSpell &spell, int flush, int z, bool isAutocast)
{
	DebugPrint(": %d casts %s at %d %d on %d\n" _C_
			   UnitNumber(unit) _C_ spell.Ident.c_str() _C_ pos.x _C_ pos.y _C_ dest ? UnitNumber(*dest) : 0);
	Assert(std::find(unit.Type->Spells.begin(), unit.Type->Spells.end(), &spell) != unit.Type->Spells.end());
	
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	//Wyrmgus start
	StopRaft(unit);
	ReachGoalLayer(unit, z, flush);
	//Wyrmgus end
	
	COrderPtr *order = GetNextOrder(unit, flush);

	if (order == nullptr) {
		return;
	}

	*order = COrder::NewActionSpellCast(spell, pos, dest, z, true);
	ClearSavedAction(unit);
}

/**
**  Auto spell cast.
**
**  @param unit     pointer to unit.
**  @param spellid  Spell id.
**  @param on       1 for auto cast on, 0 for off.
*/
void CommandAutoSpellCast(CUnit &unit, int spellid, int on)
{
	if (IsUnitValidForNetwork(unit) == false) {
		return ;
	}
	unit.AutoCastSpell[spellid] = on;
}

/**
**  Diplomacy changed.
**
**  @param player    Player which changes his state.
**  @param state     New diplomacy state.
**  @param opponent  Opponent.
*/
void CommandDiplomacy(int player, int state, int opponent)
{
	switch (state) {
		case DiplomacyNeutral:
			Players[player].SetDiplomacyNeutralWith(Players[opponent]);
			break;
		case DiplomacyAllied:
			Players[player].SetDiplomacyAlliedWith(Players[opponent]);
			break;
		case DiplomacyEnemy:
			Players[player].SetDiplomacyEnemyWith(Players[opponent]);
			break;
		//Wyrmgus start
		case DiplomacyOverlord:
			Players[opponent].SetOverlord(&Players[player]);
			break;
		case DiplomacyVassal:
			Players[player].SetOverlord(&Players[opponent]);
			break;
		//Wyrmgus end
		case DiplomacyCrazy:
			Players[player].SetDiplomacyCrazyWith(Players[opponent]);
			break;
	}
}

/**
**  Shared vision changed.
**
**  @param player    Player which changes his state.
**  @param state     New shared vision state.
**  @param opponent  Opponent.
*/
void CommandSharedVision(int player, bool state, int opponent)
{
	// Do a real hardcore seen recount. First we unmark EVERYTHING.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (!unit.Destroyed) {
			MapUnmarkUnitSight(unit);
		}
	}

	// Compute Before and after.
	const int before = Players[player].IsBothSharedVision(Players[opponent]);
	if (state == false) {
		Players[player].UnshareVisionWith(Players[opponent]);
	} else {
		Players[player].ShareVisionWith(Players[opponent]);
	}
	const int after = Players[player].IsBothSharedVision(Players[opponent]);

	if (before && !after) {
		// Don't share vision anymore. Give each other explored terrain for good-bye.

		//Wyrmgus start
		/*
		for (int i = 0; i != CMap::Map.Info.MapWidth * CMap::Map.Info.MapHeight; ++i) {
			CMapField &mf = *CMap::Map.Field(i);
			CMapFieldPlayerInfo &mfp = mf.playerInfo;

			//Wyrmgus start
//			if (mfp.Visible[player] && !mfp.Visible[opponent]) {
			if (mfp.Visible[player] && !mfp.Visible[opponent] && !Players[player].Revealed) {
			//Wyrmgus end
				mfp.Visible[opponent] = 1;
				if (opponent == ThisPlayer->Index) {
					CMap::Map.MarkSeenTile(mf);
				}
			}
			//Wyrmgus start
//			if (mfp.Visible[opponent] && !mfp.Visible[player]) {
			if (mfp.Visible[opponent] && !mfp.Visible[player] && !Players[opponent].Revealed) {
			//Wyrmgus end
				mfp.Visible[player] = 1;
				if (player == ThisPlayer->Index) {
					CMap::Map.MarkSeenTile(mf);
				}
			}
		}
		*/
		for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
			for (int i = 0; i != CMap::Map.Info.MapWidths[z] * CMap::Map.Info.MapHeights[z]; ++i) {
				CMapField &mf = *CMap::Map.Field(i, z);
				CMapFieldPlayerInfo &mfp = mf.playerInfo;

				if (mfp.Visible[player] && !mfp.Visible[opponent] && !Players[player].Revealed) {
					mfp.Visible[opponent] = 1;
					if (opponent == ThisPlayer->Index) {
						CMap::Map.MarkSeenTile(mf, z);
					}
				}
				if (mfp.Visible[opponent] && !mfp.Visible[player] && !Players[opponent].Revealed) {
					mfp.Visible[player] = 1;
					if (player == ThisPlayer->Index) {
						CMap::Map.MarkSeenTile(mf, z);
					}
				}
			}
		}
		//Wyrmgus end
	}

	// Do a real hardcore seen recount. Now we remark EVERYTHING
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (!unit.Destroyed) {
			MapMarkUnitSight(unit);
		}
	}
}

/**
**  Player quit.
**
**  @param player  Player number that quit.
*/
void CommandQuit(int player)
{
	// Set player to neutral, remove allied/enemy/shared vision status
	// If the player doesn't have any units then this is pointless?
	Players[player].Type = PlayerNeutral;
	for (int i = 0; i < NumPlayers; ++i) {
		if (i != player && Players[i].Team != Players[player].Team) {
			Players[i].SetDiplomacyNeutralWith(Players[player]);
			Players[player].SetDiplomacyNeutralWith(Players[i]);
			//  We clear Shared vision by sending fake shared vision commands.
			//  We do this because Shared vision is a bit complex.
			CommandSharedVision(i, 0, player);
			CommandSharedVision(player, 0, i);
			// Remove Selection from Quit Player
			std::vector<CUnit *> empty;
			ChangeTeamSelectedUnits(Players[player], empty);
		}
	}

	if (Players[player].GetUnitCount() != 0) {
		SetMessage(_("Player \"%s\" has left the game"), Players[player].Name.c_str());
	} else {
		SetMessage(_("Player \"%s\" has been killed"), Players[player].Name.c_str());
	}
}
