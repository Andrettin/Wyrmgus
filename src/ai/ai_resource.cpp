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
/**@name ai_resource.cpp - AI resource manager. */
//
//      (c) Copyright 2000-2019 by Lutz Sammer, Antonis Chaniotis
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
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ai_local.h"

#include "action/action_build.h"
#include "action/action_repair.h"
#include "action/action_resource.h"
#include "commands.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "pathfinder.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unittype.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define COLLECT_RESOURCES_INTERVAL 4

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//Wyrmgus start
//static int AiMakeUnit(CUnitType &type, const Vec2i &nearPos);
static int AiMakeUnit(CUnitType &type, const Vec2i &nearPos, int z, int landmass = 0, CSite *settlement = nullptr);
//Wyrmgus end

/**
**  Check if the costs are available for the AI.
**
**  Take reserve and already used resources into account.
**
**  @param costs  Costs for something.
**
**  @return       A bit field of the missing costs.
*/
static int AiCheckCosts(const int *costs)
{
	// FIXME: the used costs shouldn't be calculated here
	int *used = AiPlayer->Used;

	for (int i = 1; i < MaxCosts; ++i) {
		used[i] = 0;
	}

	const int nunits = AiPlayer->Player->GetUnitCount();
	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		for (size_t k = 0; k < unit.Orders.size(); ++k) {
			const COrder &order = *unit.Orders[k];

			if (order.Action == UnitActionBuild) {
				const COrder_Build &orderBuild = static_cast<const COrder_Build &>(order);
				int building_costs[MaxCosts];
				AiPlayer->Player->GetUnitTypeCosts(&orderBuild.GetUnitType(), building_costs);

				for (int j = 1; j < MaxCosts; ++j) {
					used[j] += building_costs[j];
				}
			}
		}
	}

	int err = 0;
	const int *resources = AiPlayer->Player->Resources;
	const int *storedresources = AiPlayer->Player->StoredResources;
	const int *reserve = AiPlayer->Reserve;
	for (int i = 1; i < MaxCosts; ++i) {
		if (resources[i] + storedresources[i] - used[i] < costs[i] - reserve[i]) {
			err |= 1 << i;
		}
	}
	return err;
}

/**
**  Check if the AI player needs food.
**
**  It counts buildings in progress and units in training queues.
**
**  @param pai   AI player.
**  @param type  Unit-type that should be build.
**
**  @return      True if enought, false otherwise.
**
**  @todo  The number of food currently trained can be stored global
**         for faster use.
*/
static int AiCheckSupply(const PlayerAi &pai, const CUnitType &type)
{
	// Count food supplies under construction.
	int remaining = 0;
	for (unsigned int i = 0; i < pai.UnitTypeBuilt.size(); ++i) {
		const AiBuildQueue &queue = pai.UnitTypeBuilt[i];
		if (queue.Type->Stats[pai.Player->Index].Variables[SUPPLY_INDEX].Value) {
			remaining += queue.Made * queue.Type->Stats[pai.Player->Index].Variables[SUPPLY_INDEX].Value;
		}
	}

	// We are already out of food.
	remaining += pai.Player->Supply - pai.Player->Demand - type.Stats[pai.Player->Index].Variables[DEMAND_INDEX].Value;
	if (remaining < 0) {
		return 0;
	}
	// Count what we train.
	for (unsigned int i = 0; i < pai.UnitTypeBuilt.size(); ++i) {
		const AiBuildQueue &queue = pai.UnitTypeBuilt[i];
		
		if (queue.Type->BoolFlag[TOWNHALL_INDEX].value) { //don't count town halls
			continue;
		}

		remaining -= queue.Made * queue.Type->Stats[pai.Player->Index].Variables[DEMAND_INDEX].Value;
		if (remaining < 0) {
			return 0;
		}
	}
	return 1;
}

/**
**  Check if the costs for an unit-type are available for the AI.
**
**  Take reserve and already used resources into account.
**
**  @param type  Unit-type to check the costs for.
**
**  @return      A bit field of the missing costs.
*/
int AiCheckUnitTypeCosts(const CUnitType &type)
{
	int type_costs[MaxCosts];
	AiPlayer->Player->GetUnitTypeCosts(&type, type_costs);
	return AiCheckCosts(type_costs);
}

/**
**  Check if the costs for an upgrade are available for the AI.
**
**  Take reserve and already used resources into account.
**
**  @param upgrade  Upgrade to check the costs for.
**
**  @return      A bit field of the missing costs.
*/
static int AiCheckUpgradeCosts(const CUpgrade &upgrade)
{
	int upgrade_costs[MaxCosts];
	AiPlayer->Player->GetUpgradeCosts(&upgrade, upgrade_costs);
	return AiCheckCosts(upgrade_costs);
}

class IsAEnemyUnitOf
{
public:
	explicit IsAEnemyUnitOf(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsVisibleAsGoal(*player) && unit->IsEnemy(*player);
	}
private:
	const CPlayer *player;
};

class IsAEnemyUnitWhichCanCounterAttackOf
{
public:
	explicit IsAEnemyUnitWhichCanCounterAttackOf(const CPlayer &_player, const CUnitType &_type) :
		player(&_player), type(&_type)
	{}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsVisibleAsGoal(*player)
			   && unit->IsEnemy(*player)
			   && CanTarget(*unit->Type, *type);
	}
private:
	const CPlayer *player;
	const CUnitType *type;
};

/**
**  Enemy units in distance.
**
**  @param player  Find enemies of this player
**  @param type    Optional unit type to check if enemy can target this
**  @param pos     location
**  @param range   Distance range to look.
**
**  @return       Number of enemy units.
*/
int AiEnemyUnitsInDistance(const CPlayer &player,
						   //Wyrmgus start
//						   const CUnitType *type, const Vec2i &pos, unsigned range)
						   const CUnitType *type, const Vec2i &pos, unsigned range, int z)
						   //Wyrmgus end
{
	const Vec2i offset(range, range);
	std::vector<CUnit *> units;

	if (type == nullptr) {
		//Wyrmgus start
//		Select(pos - offset, pos + offset, units, IsAEnemyUnitOf(player));
		Select(pos - offset, pos + offset, units, z, IsAEnemyUnitOf(player));
		//Wyrmgus end
		return static_cast<int>(units.size());
	} else {
		const Vec2i typeSize(type->TileSize - 1);
		const IsAEnemyUnitWhichCanCounterAttackOf pred(player, *type);

		//Wyrmgus start
//		Select(pos - offset, pos + typeSize + offset, units, pred);
		Select(pos - offset, pos + typeSize + offset, units, z, pred);
		//Wyrmgus end
		return static_cast<int>(units.size());
	}
}

/**
**  Enemy units in distance.
**
**  @param unit   Find in distance for this unit.
**  @param range  Distance range to look.
**
**  @return       Number of enemy units.
*/
//Wyrmgus start
//int AiEnemyUnitsInDistance(const CUnit &unit, unsigned range)
int AiEnemyUnitsInDistance(const CUnit &unit, unsigned range, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return AiEnemyUnitsInDistance(*unit.Player, unit.Type, unit.tilePos, range);
	return AiEnemyUnitsInDistance(*unit.Player, unit.Type, unit.tilePos, range, z);
	//Wyrmgus end
}

static bool IsAlreadyWorking(const CUnit &unit)
{
	for (size_t i = 0; i != unit.Orders.size(); ++i) {
		const int action = unit.Orders[i]->Action;

		if (action == UnitActionBuild || action == UnitActionRepair) {
			return true;
		}
		if (action == UnitActionResource) {
			const COrder_Resource &order = *static_cast<const COrder_Resource *>(unit.Orders[i]);

			if (order.IsGatheringStarted()) {
				return true;
			}
		}
	}
	return false;
}


/**
**  Check if we can build the building.
**
**  @param type      Unit that can build the building.
**  @param building  Building to be build.
**
**  @return          True if made, false if can't be made.
**
**  @note            We must check if the dependencies are fulfilled.
*/
//Wyrmgus start
//static int AiBuildBuilding(const CUnitType &type, CUnitType &building, const Vec2i &nearPos)
static int AiBuildBuilding(const CUnitType &type, CUnitType &building, const Vec2i &nearPos, int z, int landmass = 0, CSite *settlement = nullptr)
//Wyrmgus end
{
	std::vector<CUnit *> table;

	FindPlayerUnitsByType(*AiPlayer->Player, type, table, true);

	int num = 0;

	// Remove all workers on the way building something
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		
		//Wyrmgus start
		if (!unit.Active) {
			continue;
		}
		
		if (landmass) {
			int worker_landmass = Map.GetTileLandmass(unit.tilePos, unit.MapLayer->ID);
			if (worker_landmass != landmass && std::find(Map.BorderLandmasses[landmass].begin(), Map.BorderLandmasses[landmass].end(), worker_landmass) == Map.BorderLandmasses[landmass].end()) { //if the landmass is not the same as the worker's, and the worker isn't in an adjacent landmass, then the worker can't build the building at the appropriate location
				continue;
			}
		}
		//Wyrmgus end

		if (IsAlreadyWorking(unit) == false) {
			table[num++] = &unit;
		}
	}
	if (num == 0) {
		// No workers available to build
		return 0;
	}
	
	//Wyrmgus start
	table.resize(num);	
	
	CUnit *near_unit = nullptr;
	if (building.TerrainType || building.BoolFlag[TOWNHALL_INDEX].value) { //terrain type units and town halls have a particular place to be built, so we need to find the worker with a terrain traversal
		TerrainTraversal terrainTraversal;

		terrainTraversal.SetSize(Map.Info.MapWidths[z], Map.Info.MapHeights[z]);
		terrainTraversal.Init();

		terrainTraversal.PushPos(nearPos);

		int maxRange = 15;
		if (building.BoolFlag[TOWNHALL_INDEX].value) { //for settlements, look farther for builders
			maxRange = 9999;
		}
		int movemask = type.MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit);
		if (OnTopDetails(building, nullptr)) { //if the building is built on top of something else, make sure the building it is built on top of doesn't block the movemask
			movemask &= ~(MapFieldBuilding);
		}
		UnitFinder unitFinder(*AiPlayer->Player, table, maxRange, movemask, &near_unit, z);

		terrainTraversal.Run(unitFinder);
		
		if (!near_unit) {
			return 0;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	CUnit &unit = (num == 1) ? *table[0] : *table[SyncRand() % num];
	CUnit &unit = near_unit ? *near_unit : ((num == 1) ? *table[0] : *table[SyncRand() % num]);
	//Wyrmgus end
	
	if (!Map.Info.IsPointOnMap(nearPos, z)) {
		z = unit.MapLayer->ID;
	}
	
	Vec2i pos;
	// Find a place to build.
	//Wyrmgus start
//	if (AiFindBuildingPlace(unit, building, nearPos, &pos)) {
	if (AiFindBuildingPlace(unit, building, nearPos, &pos, true, z, landmass, settlement)) {
	//Wyrmgus end
		//Wyrmgus start
//		CommandBuildBuilding(unit, pos, building, FlushCommands);
		CommandBuildBuilding(unit, pos, building, FlushCommands, z, settlement);
		//Wyrmgus end
		return 1;
	//Wyrmgus start
	/*
	} else {
		//when first worker can't build then rest also won't be able (save CPU)
		if (Map.Info.IsPointOnMap(nearPos, z)) {
			//Crush CPU !!!!!
			for (int i = 0; i < num && table[i] != &unit; ++i) {
				// Find a place to build.
				//Wyrmgus start
//				if (AiFindBuildingPlace(*table[i], building, nearPos, &pos)) {
				if (AiFindBuildingPlace(*table[i], building, nearPos, &pos, true, z, landmass)) {
				//Wyrmgus end
					//Wyrmgus start
//					CommandBuildBuilding(*table[i], pos, building, FlushCommands);
					CommandBuildBuilding(*table[i], pos, building, FlushCommands, z);
					//Wyrmgus end
					return 1;
				}
			}
		}
	*/
	//Wyrmgus end
	}
	
	return 0;
}

bool AiRequestedTypeAllowed(const CPlayer &player, const CUnitType &type, bool allow_can_build_builder, bool include_upgrade)
{
	//Wyrmgus start
	std::vector<std::vector<CUnitType *> > *tablep;
	if (type.BoolFlag[BUILDING_INDEX].value) {
		if (
			include_upgrade
			&& (type.Slot >= (int) AiHelpers.Build.size() || AiHelpers.Build[type.Slot].empty())
			&& type.Slot < (int) AiHelpers.Upgrade.size() && !AiHelpers.Upgrade[type.Slot].empty()
		) {
			tablep = &AiHelpers.Upgrade;
		} else {
			tablep = &AiHelpers.Build;
		}
	} else {
		tablep = &AiHelpers.Train;
	}
	if (type.Slot >= (int) (*tablep).size()) {
		return false;
	}
//	const size_t size = AiHelpers.Build[type.Slot].size();
	const size_t size = (*tablep)[type.Slot].size();
	//Wyrmgus end
	for (size_t i = 0; i != size; ++i) {
		//Wyrmgus start
//		CUnitType &builder = *AiHelpers.Build[type.Slot][i];
		CUnitType &builder = *(*tablep)[type.Slot][i];
		//Wyrmgus end

		if ((player.GetUnitTypeAiActiveCount(&builder) > 0 || (allow_can_build_builder && AiRequestedTypeAllowed(player, builder)))
			&& CheckDependencies(&type, &player)) {
			return true;
		}
	}
	return false;
}

//Wyrmgus start
static bool AiRequestedUpgradeAllowed(const CPlayer &player, const CUpgrade *upgrade, bool allow_can_build_researcher = false)
{
	if (UpgradeIdAllowed(*AiPlayer->Player, upgrade->ID) != 'A') {
		return false;
	}
	if (upgrade->ID >= (int) AiHelpers.Research.size()) {
		return false;
	}
	const size_t size = AiHelpers.Research[upgrade->ID].size();
	for (size_t i = 0; i < size; ++i) {
		CUnitType &researcher = *AiHelpers.Research[upgrade->ID][i];

		if ((player.GetUnitTypeAiActiveCount(&researcher) > 0 || (allow_can_build_researcher && AiRequestedTypeAllowed(player, researcher))) && CheckDependencies(upgrade, &player)) {
			return true;
		}
	}
	return false;
}
//Wyrmgus end

struct cnode {
	int unit_cost;
	int needmask;
	CUnitType *type;
};

static bool cnode_cmp(const cnode &lhs, const cnode &rhs)
{
	return lhs.unit_cost < rhs.unit_cost;
}

int AiGetBuildRequestsCount(const PlayerAi &pai, int (&counter)[UnitTypeMax])
{
	const int size = (int)pai.UnitTypeBuilt.size();
	memset(counter, 0, sizeof(int) * UnitTypeMax);
	for (int i = 0; i < size; ++i) {
		const AiBuildQueue &queue = pai.UnitTypeBuilt[i];

		counter[queue.Type->Slot] += queue.Want;
	}
	return size;
}

//Wyrmgus start
//extern CUnit *FindDepositNearLoc(CPlayer &p, const Vec2i &pos, int range, int resource);
extern CUnit *FindDepositNearLoc(CPlayer &p, const Vec2i &pos, int range, int resource, int z);
//Wyrmgus end

void AiNewDepotRequest(CUnit &worker)
{
#if 0
	DebugPrint("%d: Worker %d report: Resource [%d] too far from depot, returning time [%d].\n"
			   _C_ worker->Player->Index _C_ worker->Slot
			   _C_ worker->CurrentResource
			   _C_ worker->Data.Move.Cycles);
#endif
	Assert(worker.CurrentAction() == UnitActionResource);
	COrder_Resource &order = *static_cast<COrder_Resource *>(worker.CurrentOrder());
	const int resource = order.GetCurrentResource();

	const Vec2i pos = order.GetHarvestLocation();
	//Wyrmgus start
	const int z = order.GetHarvestMapLayer();
	//Wyrmgus end
	const int range = 15;

	if (pos.x != -1 && nullptr != FindDepositNearLoc(*worker.Player, pos, range, resource, z)) {
		/*
		 * New Depot has just be finished and worker just return to old depot
		 * (far away) from new Deopt.
		 */
		return;
	}
	CUnitType *best_type = nullptr;
	int best_cost = 0;
	//int best_mask = 0;
	// Count the already made build requests.
	int counter[UnitTypeMax];

	AiGetBuildRequestsCount(*worker.Player->Ai, counter);

	const int n = AiHelpers.Depots[resource].size();

	for (int i = 0; i < n; ++i) {
		CUnitType &type = *AiHelpers.Depots[resource][i];

		if (counter[type.Slot]) { // Already ordered.
			return;
		}
		if (!AiRequestedTypeAllowed(*worker.Player, type)) {
			continue;
		}

		// Check if resources available.
		//int needmask = AiCheckUnitTypeCosts(type);
		int type_costs[MaxCosts];
		worker.Player->GetUnitTypeCosts(&type, type_costs);
		int cost = 0;
		for (int c = 1; c < MaxCosts; ++c) {
			cost += type_costs[c];
		}

		if (best_type == nullptr || (cost < best_cost)) {
			best_type = &type;
			best_cost = cost;
			//best_mask = needmask;
		}
	}

	if (best_type) {
		//if(!best_mask) {
		AiBuildQueue queue;

		queue.Type = best_type;
		queue.Want = 1;
		queue.Made = 0;
		queue.Pos = pos;
		//Wyrmgus start
		queue.MapLayer = z;
		//Wyrmgus end

		worker.Player->Ai->UnitTypeBuilt.push_back(queue);

		DebugPrint("%d: Worker %d report: Requesting new depot near [%d,%d].\n"
				   _C_ worker.Player->Index _C_ UnitNumber(worker)
				   _C_ queue.Pos.x _C_ queue.Pos.y);
		/*
		} else {
			AiPlayer->NeededMask |= best_mask;
		}
		*/
	}
}

class IsAWorker
{
public:
	explicit IsAWorker() {}
	bool operator()(const CUnit *const unit) const
	{
		return (unit->Type->BoolFlag[HARVESTER_INDEX].value && !unit->Removed);
	}
};

class CompareDepotsByDistance
{
public:
	explicit CompareDepotsByDistance(const CUnit &worker) : worker(worker) {}
	bool operator()(const CUnit *lhs, const CUnit *rhs) const
	{
		return lhs->MapDistanceTo(worker) < rhs->MapDistanceTo(worker);
	}
private:
	const CUnit &worker;
};

/**
**  Get a suitable depot for better resource harvesting.
**
**  @param worker    Worker itself.
**  @param oldDepot  Old assigned depot.
**  @param resUnit   Resource to harvest from, if succeed
**
**  @return          new depot if found, null otherwise.
*/
CUnit *AiGetSuitableDepot(const CUnit &worker, const CUnit &oldDepot, CUnit **resUnit)
{
	Assert(worker.CurrentAction() == UnitActionResource);
	COrder_Resource &order = *static_cast<COrder_Resource *>(worker.CurrentOrder());
	const int resource = order.GetCurrentResource();
	std::vector<CUnit *> depots;
	const Vec2i offset(MaxMapWidth, MaxMapHeight);

	for (std::vector<CUnit *>::iterator it = worker.Player->UnitBegin(); it != worker.Player->UnitEnd(); ++it) {
		CUnit &unit = **it;

		//Wyrmgus start
//		if (unit.Type->CanStore[resource] && !unit.IsUnusable()) {
		if (worker.CanReturnGoodsTo(&unit, resource) && !unit.IsUnusable()) {
		//Wyrmgus end
			depots.push_back(&unit);
		}
	}
	// If there aren't any alternatives, exit
	if (depots.size() < 2) {
		return nullptr;
	}
	std::sort(depots.begin(), depots.end(), CompareDepotsByDistance(worker));

	for (std::vector<CUnit *>::iterator it = depots.begin(); it != depots.end(); ++it) {
		CUnit &unit = **it;

		const unsigned int tooManyWorkers = 15;
		const int range = 15;

		if (&oldDepot == &unit) {
			continue;
		}
		if (unit.Refs > tooManyWorkers) {
			continue;
		}
		if (AiEnemyUnitsInDistance(worker, range, worker.MapLayer->ID)) {
			continue;
		}
		//Wyrmgus start
//		CUnit *res = UnitFindResource(worker, unit, range, resource, unit.Player->AiEnabled);
		CUnit *res = UnitFindResource(worker, unit, range, resource, true, nullptr, true, false, false, false, true);
		//Wyrmgus end
		if (res) {
			*resUnit = res;
			return &unit;
		}
	}
	return nullptr;
}

//Wyrmgus start
void AiTransportCapacityRequest(int capacity_needed, int landmass)
{
	CUnitType *best_type = nullptr;
	int best_cost = 0;

	const int n = AiHelpers.NavalTransporters[0].size();

	for (int i = 0; i < n; ++i) {
		CUnitType &type = *AiHelpers.NavalTransporters[0][i];

		if (!AiRequestedTypeAllowed(*AiPlayer->Player, type, true)) {
			continue;
		}

		int type_costs[MaxCosts];
		AiPlayer->Player->GetUnitTypeCosts(&type, type_costs);
		int cost = 0;
		for (int c = 1; c < MaxCosts; ++c) {
			cost += type_costs[c];
		}
		cost /= type.MaxOnBoard;

		if (best_type == nullptr || (cost < best_cost)) {
			best_type = &type;
			best_cost = cost;
			//best_mask = needmask;
		}
	}
	
	if (best_type) {
		int count_requested = capacity_needed / best_type->MaxOnBoard;
		if (capacity_needed % best_type->MaxOnBoard) {
			count_requested += 1;
		}
		count_requested = std::max(count_requested, 1);

		bool has_builder = false;
		const size_t size = AiHelpers.Train[best_type->Slot].size();
		for (size_t i = 0; i != size; ++i) {
			CUnitType &builder = *AiHelpers.Train[best_type->Slot][i];

			if (AiPlayer->Player->GetUnitTypeAiActiveCount(&builder) > 0) {
				std::vector<CUnit *> builder_table;

				FindPlayerUnitsByType(*AiPlayer->Player, builder, builder_table, true);

				for (size_t j = 0; j != builder_table.size(); ++j) {
					CUnit &builder_unit = *builder_table[j];
					
					if (Map.GetTileLandmass(builder_unit.tilePos, builder_unit.MapLayer->ID) == landmass) {
						has_builder = true;
						break;
					}
				}
				if (has_builder) {
					break;
				}
			}
		}
		
		if (!has_builder) { //if doesn't have an already built builder, see if there's one in the requests already
			for (unsigned int i = 0; i < AiPlayer->UnitTypeBuilt.size(); ++i) { //count transport capacity under construction to see if should request more
				const AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[i];
				if (queue.Landmass == landmass && std::find(AiHelpers.Train[best_type->Slot].begin(), AiHelpers.Train[best_type->Slot].end(), queue.Type) != AiHelpers.Train[best_type->Slot].end()) {
					has_builder = true;
					break;
				}
			}
		}
		
		if (!has_builder) { // if doesn't have a builder, request one
			const size_t size = AiHelpers.Train[best_type->Slot].size();
			for (size_t i = 0; i != size; ++i) {
				CUnitType &builder = *AiHelpers.Train[best_type->Slot][i];
				
				if (!AiRequestedTypeAllowed(*AiPlayer->Player, builder)) {
					continue;
				}

				AiAddUnitTypeRequest(builder, 1, landmass);
			}
		}

		AiAddUnitTypeRequest(*best_type, count_requested, landmass);
	}
}
//Wyrmgus end

/**
**  Build new units to reduce the food shortage.
*/
static bool AiRequestSupply()
{
	// Don't request supply if we're sleeping.  When the script starts it may
	// request a better unit than the one we pick here.  If we only have enough
	// resources for one unit we don't want to build the wrong one.
	if (AiPlayer->SleepCycles != 0) {
		/* we still need supply */
		return true;
	}
	
	if (AiPlayer->Player->NumTownHalls < 1) { //don't request supply if has no town hall yet
		return true;
	}

	// Count the already made build requests.
	int counter[UnitTypeMax];

	AiGetBuildRequestsCount(*AiPlayer, counter);
	struct cnode cache[16];

	memset(cache, 0, sizeof(cache));

	//
	// Check if we can build this?
	//
	int j = 0;
	const int n = AiHelpers.UnitLimit[0].size();

	for (int i = 0; i < n; ++i) {
		CUnitType &type = *AiHelpers.UnitLimit[0][i];
		if (counter[type.Slot]) { // Already ordered.
#if defined(DEBUG) && defined(DebugRequestSupply)
			DebugPrint("%d: AiRequestSupply: Supply already build in %s\n"
					   _C_ AiPlayer->Player->Index _C_ type->Name.c_str());
#endif
			return false;
		}

		if (!AiRequestedTypeAllowed(*AiPlayer->Player, type)) {
			continue;
		}

		//
		// Check if resources available.
		//
		cache[j].needmask = AiCheckUnitTypeCosts(type);

		int type_costs[MaxCosts];
		AiPlayer->Player->GetUnitTypeCosts(&type, type_costs);

		for (int c = 1; c < MaxCosts; ++c) {
			cache[j].unit_cost += type_costs[c];
		}
		cache[j].unit_cost += type.Stats[AiPlayer->Player->Index].Variables[SUPPLY_INDEX].Value - 1;
		cache[j].unit_cost /= type.Stats[AiPlayer->Player->Index].Variables[SUPPLY_INDEX].Value;
		cache[j++].type = &type;
		Assert(j < 16);
	}

	if (j > 1) {
		std::sort(&cache[0], &cache[j], cnode_cmp);
	}
	if (j) {
		if (!cache[0].needmask) {
			CUnitType &type = *cache[0].type;
			Vec2i invalidPos(-1, -1);
			//Wyrmgus start
//			if (AiMakeUnit(type, invalidPos)) {
			if (AiMakeUnit(type, invalidPos, AiPlayer->Player->StartMapLayer)) {
			//Wyrmgus end
				AiBuildQueue newqueue;
				newqueue.Type = &type;
				newqueue.Want = 1;
				newqueue.Made = 1;
				AiPlayer->UnitTypeBuilt.insert(
					AiPlayer->UnitTypeBuilt.begin(), newqueue);
#if defined( DEBUG) && defined( DebugRequestSupply )
				DebugPrint("%d: AiRequestSupply: build Supply in %s\n"
						   _C_ AiPlayer->Player->Index _C_ type->Name.c_str());
#endif
				return false;
			}
		}
		AiPlayer->NeededMask |= cache[0].needmask;
	}


#if defined( DEBUG) && defined( DebugRequestSupply )
	std::string needed("");
	for (int i = 1; i < MaxCosts; ++i) {
		if (cache[0].needmask & (1 << i)) {
			needed += ":";
			switch (i) {
				//Wyrmgus start
//				case GoldCost:
				case CopperCost:
				//Wyrmgus end
					//Wyrmgus start
//					needed += "Gold<";
					needed += "Copper<";
					//Wyrmgus end
					break;
				case WoodCost:
					needed += "Wood<";
					break;
				case OilCost:
					needed += "Oil<";
					break;
				//Wyrmgus start
				case StoneCost:
					needed += "Stone<";
					break;
				//Wyrmgus end
				default:
					needed += "unknown<";
					break;
			}
			needed += '0' + i;
			needed += ">";
		}
	}
	DebugPrint("%d: AiRequestSupply: needed build %s with %s resource\n"
			   _C_ AiPlayer->Player->Index _C_ cache[0].type->Name.c_str() _C_ needed.c_str());
#endif
	return true;
}

/**
**  Check if we can train the unit.
**
**  @param type  Unit that can train the unit.
**  @param what  What to be trained.
**
**  @return      True if made, false if can't be made.
**
**  @note        We must check if the dependencies are fulfilled.
*/
//Wyrmgus start
//static bool AiTrainUnit(const CUnitType &type, CUnitType &what)
static bool AiTrainUnit(const CUnitType &type, CUnitType &what, int landmass = 0, CSite *settlement = nullptr)
//Wyrmgus end
{
	std::vector<CUnit *> table;

	FindPlayerUnitsByType(*AiPlayer->Player, type, table, true);
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		//Wyrmgus start
		if (landmass && Map.GetTileLandmass(unit.tilePos, unit.MapLayer->ID) != landmass) {
			continue;
		}
		
		if (settlement && unit.Settlement != settlement) {
			continue;
		}
		//Wyrmgus end
		
		if (unit.IsIdle()) {
			//Wyrmgus start
//			CommandTrainUnit(unit, what, FlushCommands);
			CommandTrainUnit(unit, what, AiPlayer->Player->Index, FlushCommands);
			//Wyrmgus end
			return true;
		}
	}
	return false;
}

/**
**  Check if we can make a unit-type.
**
**  @param type  Unit-type that must be made.
**
**  @return      True if made, false if can't be made.
**
**  @note        We must check if the dependencies are fulfilled.
*/
//Wyrmgus start
//static int AiMakeUnit(CUnitType &typeToMake, const Vec2i &nearPos)
static int AiMakeUnit(CUnitType &typeToMake, const Vec2i &nearPos, int z, int landmass, CSite *settlement)
//Wyrmgus end
{
	// Find equivalents unittypes.
	int usableTypes[UnitTypeMax + 1];
	const int usableTypesCount = AiFindAvailableUnitTypeEquiv(typeToMake, usableTypes);

	// Iterate them
	for (int currentType = 0; currentType < usableTypesCount; ++currentType) {
		CUnitType &type = *UnitTypes[usableTypes[currentType]];
		int n;
		std::vector<std::vector<CUnitType *> > *tablep;
		//
		// Check if we have a place for building or a unit to build.
		//
		if (type.BoolFlag[BUILDING_INDEX].value) {
			n = AiHelpers.Build.size();
			tablep = &AiHelpers.Build;
		} else {
			n = AiHelpers.Train.size();
			tablep = &AiHelpers.Train;
		}
		if (type.Slot >= n) { // Oops not known.
			DebugPrint("%d: AiMakeUnit I: Nothing known about '%s'\n"
					   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
			continue;
		}
		std::vector<CUnitType *> &table = (*tablep)[type.Slot];
		if (table.empty()) { // Oops not known.
			DebugPrint("%d: AiMakeUnit II: Nothing known about '%s'\n"
					   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
			continue;
		}

		for (unsigned int i = 0; i < table.size(); ++i) {
			//
			// The type for builder/trainer is available
			//
			if (AiPlayer->Player->GetUnitTypeAiActiveCount(table[i])) {
				if (type.BoolFlag[BUILDING_INDEX].value) {
					//Wyrmgus start
//					if (AiBuildBuilding(*table[i], type, nearPos)) {
					if (AiBuildBuilding(*table[i], type, nearPos, z, landmass, settlement)) {
					//Wyrmgus end
						return 1;
					}
				} else {
					//Wyrmgus start
//					if (AiTrainUnit(*table[i], type)) {
					if (AiTrainUnit(*table[i], type, landmass, settlement)) {
					//Wyrmgus end
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

/**
**  Check if we can research the upgrade.
**
**  @param type  Unit that can research the upgrade.
**  @param what  What should be researched.
**
**  @return      True if made, false if can't be made.
**
**  @note        We must check if the dependencies are fulfilled.
*/
static bool AiResearchUpgrade(const CUnitType &type, CUpgrade &what)
{
	std::vector<CUnit *> table;

	FindPlayerUnitsByType(*AiPlayer->Player, type, table, true);
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (unit.IsIdle()) {
			//Wyrmgus start
//			CommandResearch(unit, what, FlushCommands);
			CommandResearch(unit, what, AiPlayer->Player->Index, FlushCommands);
			//Wyrmgus end
			return true;
		}
	}
	return false;
}

/**
**  Check if the research can be done.
**
**  @param upgrade  Upgrade to research
*/
void AiAddResearchRequest(CUpgrade *upgrade)
{
	// Check if resources are available.
	//Wyrmgus start
//	const int costNeeded = AiCheckCosts(upgrade->Costs);
	const int costNeeded = AiCheckUpgradeCosts(*upgrade);
	//Wyrmgus end

	if (costNeeded) {
		AiPlayer->NeededMask |= costNeeded;
		return;
	}
	//
	// Check if we have a place for the upgrade to research
	//
	const int n = AiHelpers.Research.size();
	std::vector<std::vector<CUnitType *> > &tablep = AiHelpers.Research;

	if (upgrade->ID >= n) { // Oops not known.
		DebugPrint("%d: AiAddResearchRequest I: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ upgrade->Ident.c_str());
		return;
	}
	std::vector<CUnitType *> &table = tablep[upgrade->ID];
	if (table.empty()) { // Oops not known.
		DebugPrint("%d: AiAddResearchRequest II: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ upgrade->Ident.c_str());
		return;
	}

	for (unsigned int i = 0; i < table.size(); ++i) {
		// The type is available
		if (AiPlayer->Player->GetUnitTypeAiActiveCount(table[i])
			&& AiResearchUpgrade(*table[i], *upgrade)) {
			return;
		}
	}
}

/**
**  Check if we can upgrade to unit-type.
**
**  @param type  Unit that can upgrade to unit-type
**  @param what  To what should be upgraded.
**
**  @return      True if made, false if can't be made.
**
**  @note        We must check if the dependencies are fulfilled.
*/
static bool AiUpgradeTo(const CUnitType &type, CUnitType &what)
{
	std::vector<CUnit *> table;

	// Remove all units already doing something.
	FindPlayerUnitsByType(*AiPlayer->Player, type, table, true);
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (unit.IsIdle()) {
			CommandUpgradeTo(unit, what, FlushCommands);
			return true;
		}
	}
	return false;
}

/**
**  Check if the upgrade-to can be done.
**
**  @param type  FIXME: docu
*/
void AiAddUpgradeToRequest(CUnitType &type)
{
	// Check if resources are available.
	const int resourceNeeded = AiCheckUnitTypeCosts(type);
	if (resourceNeeded) {
		AiPlayer->NeededMask |= resourceNeeded;
		return;
	}
	if (AiPlayer->Player->CheckLimits(type) < 0) {
		return;
	}
	//
	// Check if we have a place for the upgrade to.
	//
	const int n = AiHelpers.Upgrade.size();
	std::vector<std::vector<CUnitType *> > &tablep = AiHelpers.Upgrade;

	if (type.Slot >= n) { // Oops not known.
		DebugPrint("%d: AiAddUpgradeToRequest I: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
		return;
	}
	std::vector<CUnitType *> &table = tablep[type.Slot];
	if (table.empty()) { // Oops not known.
		DebugPrint("%d: AiAddUpgradeToRequest II: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
		return;
	}

	for (unsigned int i = 0; i < table.size(); ++i) {
		//
		// The type is available
		//
		if (AiPlayer->Player->GetUnitTypeAiActiveCount(table[i])) {
			if (AiUpgradeTo(*table[i], type)) {
				return;
			}
		}
	}
}

/**
**  Check what must be built / trained.
*/
static void AiCheckingWork()
{
	// Supply has the highest priority
	if (AiPlayer->NeedSupply) {
		if (AiPlayer->UnitTypeBuilt.empty() || AiPlayer->UnitTypeBuilt[0].Type->Stats[AiPlayer->Player->Index].Variables[SUPPLY_INDEX].Value == 0) {
			AiPlayer->NeedSupply = false;
			AiRequestSupply();
		}
	}
	// Look to the build requests, what can be done.
	const int sz = AiPlayer->UnitTypeBuilt.size();
	for (int i = 0; i < sz; ++i) {
		AiBuildQueue *queuep = &AiPlayer->UnitTypeBuilt[AiPlayer->UnitTypeBuilt.size() - sz + i];
		CUnitType &type = *queuep->Type;
		
		if ( //if has a build request specific to a settlement, but the player doesn't own the settlement, remove the order
			queuep->Settlement
			&& (
				(!type.BoolFlag[TOWNHALL_INDEX].value && queuep->Settlement->SiteUnit->Player != AiPlayer->Player)
				|| (type.BoolFlag[TOWNHALL_INDEX].value && queuep->Settlement->SiteUnit->Player->Index != PlayerNumNeutral)
			)
		) {
			AiPlayer->UnitTypeBuilt.erase(AiPlayer->UnitTypeBuilt.begin() + (AiPlayer->UnitTypeBuilt.size() - sz + i));
			continue;
		}

		// FIXME: must check if requirements are fulfilled.
		// Buildings can be destroyed.

		// Check if we have enough food.
		if (type.Stats[AiPlayer->Player->Index].Variables[DEMAND_INDEX].Value && !AiCheckSupply(*AiPlayer, type)) {
			AiPlayer->NeedSupply = true;
			AiRequestSupply();
			// AiRequestSupply can change UnitTypeBuilt so recalculate queuep
			queuep = &AiPlayer->UnitTypeBuilt[AiPlayer->UnitTypeBuilt.size() - sz + i];
		}
		// Check limits, AI should be broken if reached.
		if (queuep->Want > queuep->Made && AiPlayer->Player->CheckLimits(type) < 0) {
			continue;
		}
		// Check if resources available.
		const int c = AiCheckUnitTypeCosts(type);
		if (c) {
			AiPlayer->NeededMask |= c;
			// NOTE: we can continue and build things with lesser
			//  resource or other resource need!
			continue;
		} else if (queuep->Want > queuep->Made && queuep->Wait <= GameCycle) {
			//Wyrmgus start
//			if (AiMakeUnit(type, queuep->Pos)) {
			if (AiMakeUnit(type, queuep->Pos, queuep->MapLayer, queuep->Landmass, queuep->Settlement)) {
			//Wyrmgus end
				++queuep->Made;
				queuep->Wait = 0;
			} else if (queuep->Type->BoolFlag[BUILDING_INDEX].value) {
				// Finding a building place is costly, don't try again for a while
				if (queuep->Wait == 0) {
					queuep->Wait = GameCycle + 150;
				} else {
					queuep->Wait = GameCycle + 450;
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------
--  WORKERS/RESOURCES
----------------------------------------------------------------------------*/

/**
**  Assign worker to gather a certain resource from terrain.
**
**  @param unit      pointer to the unit.
**  @param resource  resource identification.
**
**  @return          1 if the worker was assigned, 0 otherwise.
*/
//Wyrmgus start
//static int AiAssignHarvesterFromTerrain(CUnit &unit, int resource)
static int AiAssignHarvesterFromTerrain(CUnit &unit, int resource, int resource_range)
//Wyrmgus end
{
	// TODO : hardcoded forest
	Vec2i forestPos;
	//Wyrmgus start
	Vec2i rockPos;
	//Wyrmgus end

	// Code for terrain harvesters. Search for piece of terrain to mine.
	if (FindTerrainType(unit.Type->MovementMask, resource, resource_range, *unit.Player, unit.tilePos, &forestPos, unit.MapLayer->ID)) {
		CommandResourceLoc(unit, forestPos, FlushCommands, unit.MapLayer->ID);
		return 1;
	}
	// Ask the AI to explore...
	//Wyrmgus start
//	AiExplore(unit.tilePos, MapFieldLandUnit);
	//Wyrmgus end

	// Failed.
	return 0;
}

/**
**  Assign worker to gather a certain resource from Unit.
**
**  @param unit      pointer to the unit.
**  @param resource  resource identification.
**
**  @return          1 if the worker was assigned, 0 otherwise.
*/
//Wyrmgus start
//static int AiAssignHarvesterFromUnit(CUnit &unit, int resource)
static int AiAssignHarvesterFromUnit(CUnit &unit, int resource, int resource_range)
//Wyrmgus end
{
	// Try to find the nearest depot first.
	CUnit *depot = FindDeposit(unit, 1000, resource);
	
	// Find a resource to harvest from.
	//Wyrmgus start
//	CUnit *mine = UnitFindResource(unit, depot ? *depot : unit, 1000, resource, true);
	CUnit *mine = UnitFindResource(unit, depot ? *depot : unit, resource_range, resource, true, nullptr, false, false, false, resource == CopperCost && AiPlayer->Player->HasMarketUnit());
	//Wyrmgus end

	if (mine) {
		//Wyrmgus start
//		CommandResource(unit, *mine, FlushCommands);
//		return 1;
		if (mine->Type->BoolFlag[CANHARVEST_INDEX].value) {
			CommandResource(unit, *mine, FlushCommands);
			return 1;
		} else { // if the resource isn't readily harvestable (but is a deposit), build a mine there
			const int n = AiHelpers.Mines[mine->GivesResource].size();

			for (int i = 0; i < n; ++i) {
				CUnitType &type = *AiHelpers.Mines[mine->GivesResource][i];

				if (
					type.Slot < (int) AiHelpers.Build.size()
					&& std::find(AiHelpers.Build[type.Slot].begin(), AiHelpers.Build[type.Slot].end(), unit.Type) != AiHelpers.Build[type.Slot].end()
					&& CanBuildUnitType(&unit, type, mine->tilePos, 1, true, mine->MapLayer->ID)
				) {
					CommandBuildBuilding(unit, mine->tilePos, type, FlushCommands, mine->MapLayer->ID);
					return 1;
				}
			}
		}
		//Wyrmgus end
	}
	
	//Wyrmgus start
	/*
	int exploremask = 0;

	for (size_t i = 0; i != UnitTypes.size(); ++i) {
		const CUnitType *type = UnitTypes[i];

		if (type && type->GivesResource == resource) {
			switch (type->UnitType) {
				case UnitTypeLand:
					exploremask |= MapFieldLandUnit;
					break;
				case UnitTypeFly:
					exploremask |= MapFieldAirUnit;
					break;
				//Wyrmgus start
				case UnitTypeFlyLow:
					exploremask |= MapFieldLandUnit;
					break;
				//Wyrmgus end
				case UnitTypeNaval:
					exploremask |= MapFieldSeaUnit;
					break;
				default:
					Assert(0);
			}
		}
	}
	// Ask the AI to explore
	AiExplore(unit.tilePos, exploremask);
	*/
	//Wyrmgus end
	// Failed.
	return 0;
}
/**
**  Assign worker to gather a certain resource.
**
**  @param unit      pointer to the unit.
**  @param resource  resource identification.
**
**  @return          1 if the worker was assigned, 0 otherwise.
*/
static int AiAssignHarvester(CUnit &unit, int resource)
{
	// It can't.
	//Wyrmgus start
//	if (unit.Removed) {
	if (unit.Removed || unit.CurrentAction() == UnitActionBuild) { //prevent units building from outside to being assigned to gather a resource, and then leaving the construction unbuilt forever and ever
	//Wyrmgus end
		return 0;
	}
	
	if (std::find(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), &unit) != AiPlayer->Scouts.end()) { //if a scouting unit was assigned to harvest, remove it from the scouts vector
		AiPlayer->Scouts.erase(std::remove(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), &unit), AiPlayer->Scouts.end());
	}

	const ResourceInfo &resinfo = *unit.Type->ResInfo[resource];
	Assert(&resinfo);

	//Wyrmgus start
	/*
	if (resinfo.TerrainHarvester) {
		return AiAssignHarvesterFromTerrain(unit, resource);
	} else {
		return AiAssignHarvesterFromUnit(unit, resource);
	}
	*/
	int ret = 0;
	int resource_range = 0;
	for (int i = 0; i < 3; ++i) { //search for resources first in a 16 tile radius, then in a 32 tile radius, and then in the whole map
		resource_range += 16;
		if (i == 2) {
			resource_range = 1000;
		}
		
		ret = AiAssignHarvesterFromUnit(unit, resource, resource_range);
		
		if (ret == 0) {
			ret = AiAssignHarvesterFromTerrain(unit, resource, resource_range);
		}
		
		if (ret != 0) {
			return ret;
		}
	}
	
	return ret;
	//Wyrmgus end
}

static bool CmpWorkers(const CUnit *lhs, const CUnit *rhs)
{
	return lhs->ResourcesHeld < rhs->ResourcesHeld;
}

//Wyrmgus start
static bool AiCanSellResource(int resource)
{
	if ((AiPlayer->Player->Resources[resource] + AiPlayer->Player->StoredResources[resource]) <= (AiPlayer->Collect[resource] * 100)) {
		return false;
	}
	
	if ((AiPlayer->NeededMask & ((long long int) 1 << resource))) {
		return false;
	}
	
	return true;
}

static void AiProduceResources()
{
	CUnit *market_unit = AiPlayer->Player->GetMarketUnit();
	
	const int n = AiPlayer->Player->GetUnitCount();
	for (int i = 0; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);
		if (unit.Type->Slot >= ((int) AiHelpers.ProducedResources.size()) || AiHelpers.ProducedResources[unit.Type->Slot].size() == 0 || !unit.Active) {
			continue;
		}
		
		if (!unit.IsIdle()) {
			continue;
		}

		int chosen_resource = 0;
		int best_value = 0;
		for (size_t j = 0; j != AiHelpers.ProducedResources[unit.Type->Slot].size(); ++j) {
			int resource = AiHelpers.ProducedResources[unit.Type->Slot][j];
			
			if (!CResource::Resources[resource]->LuxuryResource && AiCanSellResource(resource)) {
				continue;
			}
			
			if (CResource::Resources[resource]->LuxuryResource && !market_unit) {
				continue;
			}
			
			int input_resource = CResource::Resources[resource]->InputResource;

			if (input_resource && !AiCanSellResource(input_resource) && !(input_resource == CopperCost && CResource::Resources[resource]->LuxuryResource)) { //if the resource is a luxury resource and the input is copper skip this check, the AI should produce it as long as its price is greater than that of copper
				continue;
			}
			
			int resource_value = AiPlayer->Player->GetEffectiveResourceSellPrice(resource);
			if (input_resource) {
				resource_value -= AiPlayer->Player->GetEffectiveResourceSellPrice(input_resource);
			}

			if (resource_value > best_value) {
				chosen_resource = resource;
				best_value = resource_value;
			}
		}

		if (!chosen_resource && unit.Type->GivesResource) { // don't toggle off resource production if a building should always have a resource produced
			continue;
		}

		CommandProduceResource(unit, chosen_resource);
	}
}
//Wyrmgus end

/**
**  Assign workers to collect resources.
**
**  If we have a shortage of a resource, let many workers collecting this.
**  If no shortage, split workers to all resources.
*/
static void AiCollectResources()
{
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}
	
	std::vector<CUnit *> units_assigned[MaxCosts]; // Worker assigned to resource
	std::vector<CUnit *> units_unassigned[MaxCosts]; // Unassigned workers
	int num_units_with_resource[MaxCosts];
	int num_units_assigned[MaxCosts];
	int num_units_unassigned[MaxCosts];
	int percent[MaxCosts];
	int priority_resource[MaxCosts];
	int priority_needed[MaxCosts];
	int wanted[MaxCosts];
	int total_harvester = 0;

	memset(num_units_with_resource, 0, sizeof(num_units_with_resource));
	memset(num_units_unassigned, 0, sizeof(num_units_unassigned));
	memset(num_units_assigned, 0, sizeof(num_units_assigned));

	// Collect statistics about the current assignment
	const int n = AiPlayer->Player->GetUnitCount();
	for (int i = 0; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);
		//Wyrmgus start
//		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value) {
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value || !unit.Active) {
		//Wyrmgus end
			continue;
		}

		// See if it's assigned already
		if (unit.Orders.size() == 1 &&
			unit.CurrentAction() == UnitActionResource) {
			const COrder_Resource &order = *static_cast<COrder_Resource *>(unit.CurrentOrder());
			//Wyrmgus start
//			const int c = order.GetCurrentResource();
			int c = CResource::Resources[order.GetCurrentResource()]->FinalResource;
			if (CResource::Resources[c]->LuxuryResource) {
				num_units_assigned[c]++;
				c = CopperCost;
			}
			//Wyrmgus end
			units_assigned[c].push_back(&unit);
			num_units_assigned[c]++;
			total_harvester++;
			continue;
		}

		// Ignore busy units. ( building, fighting, ... )
		if (!unit.IsIdle()) {
			continue;
		}
		
		//Wyrmgus start
		if (unit.GroupId != 0) { //don't gather/trade with units that are parts of forces
			continue;
		}
		//Wyrmgus end

		// Send workers with resources back home.
		if (unit.ResourcesHeld) {
			const int c = unit.CurrentResource;

			num_units_with_resource[c]++;
			CommandReturnGoods(unit, 0, FlushCommands);
			total_harvester++;
			continue;
		}

		// Look what the unit can do
		for (int c = 1; c < MaxCosts; ++c) {
			if (unit.Type->ResInfo[c]) {
				units_unassigned[c].push_back(&unit);
				num_units_unassigned[c]++;
			}
		}
		++total_harvester;
	}

	if (!total_harvester) {
		return;
	}

	memset(wanted, 0, sizeof(wanted));

	int percent_total = 100;
	for (int c = 1; c < MaxCosts; ++c) {
		percent[c] = AiPlayer->Collect[c];
		if ((AiPlayer->NeededMask & ((long long int) 1 << c))) { // Double percent if needed
			percent_total += percent[c];
			percent[c] <<= 1;
		}
	}

	// Turn percent values into harvester numbers.
	for (int c = 1; c < MaxCosts; ++c) {
		if (percent[c]) {
			// Wanted needs to be representative.
			if (total_harvester < 5) {
				wanted[c] = 1 + (percent[c] * 5) / percent_total;
			} else {
				wanted[c] = 1 + (percent[c] * total_harvester) / percent_total;
			}
		}
	}

	// Initialise priority & mapping
	for (size_t c = 0; c < CResource::Resources.size(); ++c) {
		priority_resource[c] = c;
		priority_needed[c] = wanted[c] - num_units_assigned[c] - num_units_with_resource[c];

		if (c && num_units_assigned[c] > 1) {
			//first should go workers with lower ResourcesHeld value
			std::sort(units_assigned[c].begin(), units_assigned[c].end(), CmpWorkers);
		}
	}
	CUnit *unit;
	// sort resources by priority
	for (size_t i = 0; i < CResource::Resources.size(); ++i) {
		for (size_t j = i + 1; j < CResource::Resources.size(); ++j) {
			if (priority_needed[j] > priority_needed[i]) {
				std::swap(priority_needed[i], priority_needed[j]);
				std::swap(priority_resource[i], priority_resource[j]);
			}
		}
	}
	unit = nullptr;

	// Try to complete each resource in the priority order
	for (size_t i = 0; i < CResource::Resources.size(); ++i) {
		int c = priority_resource[i];
			
		//Wyrmgus start
		if (!wanted[c]) {
			continue;
		}
		//Wyrmgus end

		// If there is a free worker for c, take it.
		if (num_units_unassigned[c]) {
			CUnit *unassigned_unit = units_unassigned[c][SyncRand(units_unassigned[c].size())]; //only try to assign one unit (randomly, so it isn't always the first unit) per time, to lessen the strain on performance
			// Take the unit.
			if (AiAssignHarvester(*unassigned_unit, c)) {
				// unit is assigned
				unit = unassigned_unit;
					
				// remove it from other ressources
				for (size_t j = 0; j < CResource::Resources.size(); ++j) {
					if (j == c || !unit->Type->ResInfo[j]) {
						continue;
					}
					for (int k = 0; k < num_units_unassigned[j]; ++k) {
						if (units_unassigned[j][k] == unit) {
							units_unassigned[j][k] = units_unassigned[j][--num_units_unassigned[j]];
							units_unassigned[j].pop_back();
							break;
						}
					}
				}
			}
		}

		// Else : Take from already assigned worker with lower priority.
		if (!unit) {
			// Take from lower priority only (i+1).
			for (size_t j = i + 1; j < CResource::Resources.size() && !unit; ++j) {
				// Try to move worker from src_c to c
				const int src_c = priority_resource[j];

				//Wyrmgus start
				//don't reassign workers from one resource to another, that is too expensive performance-wise (this could be re-implemented if the AI is altered to keep track of found resource spots
				break;
				//Wyrmgus end
					
				//Wyrmgus start
				// don't reassign if the src_c resource has no workers, or if the new resource has 0 "wanted"
				if (num_units_assigned[src_c] == 0 || !wanted[c]) {
					continue;
				}
				//Wyrmgus end

				// Don't complete with lower priority ones...
				//Wyrmgus start
				/*
				if (wanted[src_c] > wanted[c]
					|| (wanted[src_c] == wanted[c]
						&& num_units_assigned[src_c] <= num_units_assigned[c] + 1)) {
				*/
				if (wanted[src_c] && ((num_units_assigned[src_c] + 1) * 100 / wanted[src_c]) <= (num_units_assigned[c] * 100 / wanted[c])) { // what matters is the percent of "wanted" fulfilled, not the absolute quantity of needed workers for that resource; add one worker to num_units_assigned[src_c] so that you won't get one worker being shuffled back and forth
					//Wyrmgus end
					//Wyrmgus start
//					continue;
					if (num_units_assigned[c] != 0) { // if there are no workers for that resource, allow reshuffling regardless of proportion calculation, so that if the game starts with few workers (like two), the AI isn't stuck gathering only the first resource it finds
						continue;
					}
					//Wyrmgus end
				}
					
				//Wyrmgus start
				if (num_units_assigned[src_c] <= wanted[src_c]) { // don't reassign if the src_c resource already has less workers than desired
					if (num_units_assigned[c] != 0) {
						continue;
					}
				}
				//Wyrmgus end
					
				//Wyrmgus start
//				for (int k = num_units_assigned[src_c] - 1; k >= 0 && !unit; --k) {
				for (int k = num_units_assigned[src_c] - 1; k >= 0; --k) { // unit may be null now, continue instead of breaking loop if so
				//Wyrmgus end
					unit = units_assigned[src_c][k];
					//Wyrmgus start
					if (!unit) {
						continue;
					}
					//Wyrmgus end

					Assert(unit->CurrentAction() == UnitActionResource);
					COrder_Resource &order = *static_cast<COrder_Resource *>(unit->CurrentOrder());

					if (order.IsGatheringFinished()) {
						//worker returning with resource
						continue;
					}

					//Wyrmgus start
					if (unit->Removed || unit->CurrentAction() == UnitActionBuild) { //if unit is removed or is currently building something, it can't be told to harvest (do this here so that AiAssignHarvester returning false later on is only because of unit type not being able to harvest something)
						unit = nullptr;
						continue;
					}
					//Wyrmgus end
					
					// unit can't harvest : next one
					if (!unit->Type->ResInfo[c] || !AiAssignHarvester(*unit, c)) {
						unit = nullptr;
						continue;
					}

					// Remove from src_c
					units_assigned[src_c][k] = units_assigned[src_c][--num_units_assigned[src_c]];
					units_assigned[src_c].pop_back();

					// j need one more
					priority_needed[j]++;
					
					//Wyrmgus start
					break; //only reassign one worker per time
					//Wyrmgus end
				}
			}
		}
	}
	
	//Wyrmgus start
	//buy or sell resources
	for (int c = 1; c < MaxCosts; ++c) {
		if (c == CopperCost) {
			continue;
		}
		
		//buy resource
		if (
			percent[c] > 0 //don't buy a resource if the AI isn't instructed to collect that resource
			&& num_units_assigned[c] == 0 //don't buy a resource if there are already workers assigned to harvesting it
			&& AiCanSellResource(CopperCost)
			&& !AiCanSellResource(c) //if there's enough of the resource stored to sell, then there's no need to buy it
		) {
			if ((c - 1) >= ((int) AiHelpers.BuyMarkets.size())) {
				continue;
			}

			const int n_m = AiHelpers.BuyMarkets[c - 1].size();

			for (int i = 0; i < n_m; ++i) {
				CUnitType &market_type = *AiHelpers.BuyMarkets[c - 1][i];

				if (AiPlayer->Player->GetUnitTypeAiActiveCount(&market_type)) {
					std::vector<CUnit *> market_table;
					FindPlayerUnitsByType(*AiPlayer->Player, market_type, market_table, true);
					
					if (market_table.size() > 0) {
						CUnit &market_unit = *market_table[SyncRand() % market_table.size()];
						CommandBuyResource(market_unit, c, AiPlayer->Player->Index);
						break;
					}
				}
			}
		//sell resource
		} else if (
			(percent[c] == 0 || num_units_assigned[c] > 0) //only sell the resource if either the AI isn't instructed to collect it, or if there are harvesters assigned to it
			&& num_units_assigned[CopperCost] == 0 //don't sell a resource if there are already workers assigned to obtaining copper
			&& !AiCanSellResource(CopperCost)
			&& AiCanSellResource(c)
		) {
			bool is_luxury_input = false;
			for (int i = 1; i < MaxCosts; ++i) {
				if (CResource::Resources[i]->LuxuryResource && CResource::Resources[i]->InputResource == c && num_units_assigned[i] > 0) {
					is_luxury_input = true;
					break;
				}
			}
			if (is_luxury_input) { //if the resource is a luxury resource input, and there are workers producing that luxury, don't sell it directly, as it makes more sense to transform it into the luxury resource
				continue;
			}
			
			if ((c - 1) >= ((int) AiHelpers.SellMarkets.size())) {
				continue;
			}

			const int n_m = AiHelpers.SellMarkets[c - 1].size();

			for (int i = 0; i < n_m; ++i) {
				CUnitType &market_type = *AiHelpers.SellMarkets[c - 1][i];

				if (AiPlayer->Player->GetUnitTypeAiActiveCount(&market_type)) {
					std::vector<CUnit *> market_table;
					FindPlayerUnitsByType(*AiPlayer->Player, market_type, market_table, true);

					if (market_table.size() > 0) {
						CUnit &market_unit = *market_table[SyncRand() % market_table.size()];
						CommandSellResource(market_unit, c, AiPlayer->Player->Index);
						break;
					}
				}
			}
		}
	}
	
	//explore with the workers that are still idle (as that means they haven't gotten something to harvest)
	for (int i = 0; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value || !unit.Active) {
			continue;
		}

		if (!unit.IsIdle()) {
			continue;
		}
		
		unit.Scout();
		break; //only do this with one at a time to not strain performance too much
	}
	//Wyrmgus end

	// Unassigned units there can't be assigned ( ie : they can't move to ressource )
	// IDEA : use transporter here.
}

/*----------------------------------------------------------------------------
--  WORKERS/REPAIR
----------------------------------------------------------------------------*/

static bool IsReadyToRepair(const CUnit &unit)
{
	if (unit.IsIdle()) {
		return true;
	} else if (unit.Orders.size() == 1 && unit.CurrentAction() == UnitActionResource) {
		COrder_Resource &order = *static_cast<COrder_Resource *>(unit.CurrentOrder());

		if (order.IsGatheringStarted() == false) {
			return true;
		}
	}
	return false;
}

/**
**  Check if we can repair the building.
**
**  @param type      Unit that can repair the building.
**  @param building  Building to be repaired.
**
**  @return          True if can repair, false if can't repair..
*/
//Wyrmgus start
//static bool AiRepairBuilding(const CPlayer &player, const CUnitType &type, CUnit &building)
static bool AiRepairBuilding(const CPlayer &player, const CUnitType &type, CUnit &building)
//Wyrmgus end
{
	if (type.RepairRange == 0) {
		return false;
	}

	// We need to send all nearby free workers to repair that building
	// AI shouldn't send workers that are far away from repair point
	// Selection of mining workers.
	std::vector<CUnit *> table;
	FindPlayerUnitsByType(*AiPlayer->Player, type, table, true);
	int num = 0;
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (IsReadyToRepair(unit)) {
			table[num++] = &unit;
		}
	}
	table.resize(num);

	if (table.empty()) {
		return false;
	}
	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(building.MapLayer->GetWidth(), building.MapLayer->GetHeight());
	terrainTraversal.Init();

	terrainTraversal.PushUnitPosAndNeighbor(building);

	const int maxRange = 15;
	const int movemask = type.MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit);
	CUnit *unit = nullptr;
	UnitFinder unitFinder(player, table, maxRange, movemask, &unit, building.MapLayer->ID);

	if (terrainTraversal.Run(unitFinder) && unit != nullptr) {
		const Vec2i invalidPos(-1, -1);
		CommandRepair(*unit, invalidPos, &building, FlushCommands, building.MapLayer->ID);
		return true;
	}
	return false;
}

/**
**  Check if we can repair this unit.
**
**  @param unit  Unit that must be repaired.
**
**  @return      True if made, false if can't be made.
*/
static int AiRepairUnit(CUnit &unit)
{
	int n = AiHelpers.Repair.size();
	std::vector<std::vector<CUnitType *> > &tablep = AiHelpers.Repair;
	const CUnitType &type = *unit.Type;
	if (type.Slot >= n) { // Oops not known.
		DebugPrint("%d: AiRepairUnit I: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
		return 0;
	}
	std::vector<CUnitType *> &table = tablep[type.Slot];
	if (table.empty()) { // Oops not known.
		DebugPrint("%d: AiRepairUnit II: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ type.Ident.c_str());
		return 0;
	}

	for (unsigned int i = 0; i < table.size(); ++i) {
		//
		// The type is available
		//
		if (AiPlayer->Player->GetUnitTypeAiActiveCount(table[i])) {
			if (AiRepairBuilding(*AiPlayer->Player, *table[i], unit)) {
				return 1;
			}
		}
	}
	return 0;
}

/**
**  Check if there's a unit that should be repaired.
*/
static void AiCheckRepair()
{
	const int n = AiPlayer->Player->GetUnitCount();
	int k = 0;

	// Selector for next unit
	//Wyrmgus start
	/*
	for (int i = n - 1; i >= 0; --i) {
		const CUnit &unit = AiPlayer->Player->GetUnit(i);
		if (UnitNumber(unit) == AiPlayer->LastRepairBuilding) {
			k = i + 1;
		}
	}
	*/
	if (AiPlayer->LastRepairBuilding) {
		for (int i = n - 1; i >= 0; --i) {
			const CUnit &unit = AiPlayer->Player->GetUnit(i);
			if (UnitNumber(unit) == AiPlayer->LastRepairBuilding) {
				k = i + 1;
			}
		}
	}
	//Wyrmgus end

	for (int i = k; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);
		bool repair_flag = true;

		if (!unit.IsAliveOnMap()) {
			continue;
		}

		int type_costs[MaxCosts];
		AiPlayer->Player->GetUnitTypeCosts(unit.Type, type_costs);
			
		// Unit damaged?
		// Don't repair attacked unit (wait 5 sec before repairing)
		if (unit.Type->RepairHP
			//Wyrmgus start
//			&& unit.CurrentAction() != UnitActionBuilt
			&& (unit.CurrentAction() != UnitActionBuilt || unit.Type->BoolFlag[BUILDEROUTSIDE_INDEX].value)
			//Wyrmgus end
			&& unit.CurrentAction() != UnitActionUpgradeTo
			//Wyrmgus start
//			&& unit.Variable[HP_INDEX].Value < unit.Variable[HP_INDEX].Max
			&& unit.Variable[HP_INDEX].Value < unit.GetModifiedVariable(HP_INDEX, VariableMax)
//			&& unit.Attacked + 5 * CYCLES_PER_SECOND < GameCycle) {
			) {
			//Wyrmgus end
			//
			// FIXME: Repair only units under control
			//
			if (AiEnemyUnitsInDistance(unit, unit.Variable[SIGHTRANGE_INDEX].Max, unit.MapLayer->ID)) {
				continue;
			}
			//
			// Must check, if there are enough resources
			//
			for (int j = 1; j < MaxCosts; ++j) {
				if (type_costs[j]
					&& (AiPlayer->Player->Resources[j] + AiPlayer->Player->StoredResources[j])  < 99) {
					repair_flag = false;
					break;
				}
			}

			//
			// Find a free worker, who can build this building can repair it?
			//
			if (repair_flag) {
				AiRepairUnit(unit);
				AiPlayer->LastRepairBuilding = UnitNumber(unit);
				return;
			}
		}
		// Building under construction but no worker
		if (unit.CurrentAction() == UnitActionBuilt) {
			int j;
			for (j = 0; j < AiPlayer->Player->GetUnitCount(); ++j) {
				COrder *order = AiPlayer->Player->GetUnit(j).CurrentOrder();
				if (order->Action == UnitActionRepair) {
					COrder_Repair &orderRepair = *static_cast<COrder_Repair *>(order);

					if (orderRepair.GetReparableTarget() == &unit) {
						break;
					}
				}
			}
			if (j == AiPlayer->Player->GetUnitCount()) {
				// Make sure we have enough resources first
				for (j = 0; j < MaxCosts; ++j) {
					// FIXME: the resources don't necessarily have to be in storage
					if (AiPlayer->Player->Resources[j] + AiPlayer->Player->StoredResources[j] < type_costs[j]) {
						break;
					}
				}
				if (j == MaxCosts) {
					AiRepairUnit(unit);
					AiPlayer->LastRepairBuilding = UnitNumber(unit);
					return;
				}
			}
		}
	}
	AiPlayer->LastRepairBuilding = 0;
}

//Wyrmgus start
/**
**  Check if there's a building that should have pathways around it, but doesn't.
*/
static void AiCheckPathwayConstruction()
{
	if (AiPlayer->Player->NumTownHalls < 1) { //don't build pathways if has no town hall yet
		return;
	}

	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	std::vector<CUnitType *> pathway_types;
	
	for (size_t i = 0; i != UnitTypes.size(); ++i) { //assumes the pathways are listed in order of speed bonus
		CUnitType *type = UnitTypes[i];

		if (!type || !type->TerrainType || !AiRequestedTypeAllowed(*AiPlayer->Player, *type)) {
			continue;
		}
		
		const int resourceNeeded = AiCheckUnitTypeCosts(*type);
		if (AiPlayer->NeededMask & AiPlayer->Player->GetUnitTypeCostsMask(type)) { //don't request the pathway type if it is going to use up a resource that is currently needed
			continue;
		}
		
		if ((type->TerrainType->Flags & MapFieldRoad) || (type->TerrainType->Flags & MapFieldRailroad)) {
			pathway_types.push_back(type);
		}
	}
	
	if (pathway_types.size() == 0) {
		return;
	}
	
	int n_t = AiHelpers.Build.size();
	std::vector<std::vector<CUnitType *> > &tablep = AiHelpers.Build;
	for (size_t i = 0; i != pathway_types.size(); ++i) {
		if (pathway_types[i]->Slot >= n_t) { // Oops not known.
			DebugPrint("%d: AiCheckPathwayConstruction I: Nothing known about '%s'\n"
					   _C_ AiPlayer->Player->Index _C_ pathway_types[i]->Ident.c_str());
			return;
		}
		
		std::vector<CUnitType *> &table = tablep[pathway_types[i]->Slot];
		if (table.empty()) { // Oops not known.
			DebugPrint("%d: AiCheckPathwayConstruction II: Nothing known about '%s'\n"
					   _C_ AiPlayer->Player->Index _C_ pathway_types[i]->Ident.c_str());
			return;
		}
	}
	
	const int n = AiPlayer->Player->GetUnitCount();
	int k = 0;
	
	// Selector for next unit
	if (AiPlayer->LastPathwayConstructionBuilding) {
		for (int i = n - 1; i >= 0; --i) {
			const CUnit &unit = AiPlayer->Player->GetUnit(i);
			if (UnitNumber(unit) == AiPlayer->LastPathwayConstructionBuilding) {
				k = i + 1;
				break;
			}
		}
	}

	//build roads around buildings
	int checked_buildings = 0;
	for (int i = k; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (!unit.IsAliveOnMap()) {
			continue;
		}

		bool built_pathway_for_building = false;
					
		// Building should have pathways but doesn't?
		if (
			unit.Type->BoolFlag[BUILDING_INDEX].value
			&& unit.CurrentAction() != UnitActionBuilt //only build pathways for buildings that have already been constructed
		) {
			//
			// FIXME: Construct pathways only for buildings under control
			//
			if (AiEnemyUnitsInDistance(unit, 8, unit.MapLayer->ID)) {
				continue;
			}
			
			std::vector<Vec2i> pathway_tiles;
			for (int x = unit.tilePos.x - 1; x < unit.tilePos.x + unit.Type->TileSize.x + 1; ++x) {
				for (int y = unit.tilePos.y - 1; y < unit.tilePos.y + unit.Type->TileSize.y + 1; ++y) {
					pathway_tiles.push_back(Vec2i(x, y));
				}
			}

			if (unit.Type->GivesResource) { //if is a mine, build pathways to the depot as well
				const CUnit *depot = FindDepositNearLoc(*unit.Player, unit.tilePos + Vec2i((unit.Type->TileSize - 1) / 2), 32, unit.GivesResource, unit.MapLayer->ID);
				if (depot) {
					//create a worker to test the path; the worker can't be a rail one, or the path construction won't work
					int worker_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, GetUnitTypeClassIndexByName("worker"));
					if (worker_type_id != -1) {
						CUnitType *test_worker_type = UnitTypes[worker_type_id];
						
						UnmarkUnitFieldFlags(unit);
						UnmarkUnitFieldFlags(*depot);
						
						CUnit *test_worker = MakeUnitAndPlace(unit.tilePos + Vec2i((unit.Type->TileSize - 1) / 2), *test_worker_type, &Players[PlayerNumNeutral], unit.MapLayer->ID);
						char worker_path[64];
						
						//make the first path
						int worker_path_length = AStarFindPath(test_worker->tilePos, depot->tilePos, depot->Type->TileSize.x, depot->Type->TileSize.y, test_worker->Type->TileSize.x, test_worker->Type->TileSize.y, 0, 1, worker_path, 64, *test_worker, 0, unit.MapLayer->ID, false);
						Vec2i worker_path_pos(test_worker->tilePos);
						std::vector<Vec2i> first_path_tiles;
						while (worker_path_length > 0 && worker_path_length <= 64) {
							Vec2i pos_change(0, 0);
							pos_change.x = Heading2X[(int)worker_path[worker_path_length - 1]];
							pos_change.y = Heading2Y[(int)worker_path[worker_path_length - 1]];
							worker_path_pos += pos_change;
							pathway_tiles.push_back(worker_path_pos);
							if (worker_path_length > 1) {
								first_path_tiles.push_back(worker_path_pos);
							}
							worker_path_length -= 1;
						}
						
						// mark the tiles of the first path (that aren't the first and last tile) as unpassable, so that the second path has to follow a different way
						for (size_t z = 1; z < first_path_tiles.size(); ++z) {
							if (!(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldForest) && !(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldRocks) && !(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldWall)) {
								unit.MapLayer->Field(first_path_tiles[z])->Flags |= MapFieldUnpassable;
							}
						}
						
						//make the second path
						worker_path_length = AStarFindPath(test_worker->tilePos, depot->tilePos, depot->Type->TileSize.x, depot->Type->TileSize.y, test_worker->Type->TileSize.x, test_worker->Type->TileSize.y, 0, 1, worker_path, 64, *test_worker, 0, unit.MapLayer->ID, false);
						worker_path_pos = test_worker->tilePos;
						while (worker_path_length > 0 && worker_path_length <= 64) {
							Vec2i pos_change(0, 0);
							pos_change.x = Heading2X[(int)worker_path[worker_path_length - 1]];
							pos_change.y = Heading2Y[(int)worker_path[worker_path_length - 1]];
							worker_path_pos += pos_change;
							pathway_tiles.push_back(worker_path_pos);
							worker_path_length -= 1;
						}
						
						//unmark the tiles of the first path
						for (size_t z = 1; z < first_path_tiles.size(); ++z) {
							if (!(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldForest) && !(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldRocks) && !(unit.MapLayer->Field(first_path_tiles[z])->Flags & MapFieldWall)) {
								unit.MapLayer->Field(first_path_tiles[z])->Flags &= ~(MapFieldUnpassable);
							}
						}
						
						MarkUnitFieldFlags(unit);
						MarkUnitFieldFlags(*depot);
						
						test_worker->Remove(nullptr);
						LetUnitDie(*test_worker);
					}
				}
			}
			
			for (size_t z = 0; z != pathway_tiles.size(); ++z) {
				Vec2i pathway_pos(pathway_tiles[z].x, pathway_tiles[z].y);
				if (!Map.Info.IsPointOnMap(pathway_pos, unit.MapLayer)) {
					continue;
				}
				CMapField &mf = *unit.MapLayer->Field(pathway_pos);
				if (mf.Flags & MapFieldBuilding) { //this is a tile where the building itself is located, continue
					continue;
				}
					
				if (pathway_types.size() == 0) {
					AiPlayer->LastPathwayConstructionBuilding = UnitNumber(unit);
					return;
				}

				bool built_pathway = false;
					
				for (int p = (pathway_types.size()  - 1); p >= 0; --p) {
					if ((pathway_types[p]->TerrainType->Flags & MapFieldRailroad) && (unit.GivesResource == -1 || !CResource::Resources[unit.GivesResource]->IsMineResource())) { //build roads around buildings, not railroads (except for mines)
						continue;
					}
						
					if (
						(
							!(mf.Flags & MapFieldRailroad)
							&& (pathway_types[p]->TerrainType->Flags & MapFieldRailroad)
						)
						|| (
							!(mf.Flags & MapFieldRoad)
							&& !(mf.Flags & MapFieldRailroad)
							&& (pathway_types[p]->TerrainType->Flags & MapFieldRoad)
						)
					) {
						if (!UnitTypeCanBeAt(*pathway_types[p], pathway_pos, unit.MapLayer->ID) || !CanBuildHere(nullptr, *pathway_types[p], pathway_pos, unit.MapLayer->ID)) {
							continue;
						}
							
						const int resourceNeeded = AiCheckUnitTypeCosts(*pathway_types[p]);
						if (resourceNeeded) { //if no longer has the resource, or if the resource is already needed, don't build this pathway type
							pathway_types.erase(std::remove(pathway_types.begin(), pathway_types.end(), pathway_types[p]), pathway_types.end());
							continue;
						}
						//
						// Find a free worker, who can build pathways for this building
						//
						for (unsigned int j = 0; j < tablep[pathway_types[p]->Slot].size(); ++j) {
							//
							// The type is available
							//
							if (AiPlayer->Player->GetUnitTypeAiActiveCount(tablep[pathway_types[p]->Slot][j])) {
								if (AiBuildBuilding(*tablep[pathway_types[p]->Slot][j], *pathway_types[p], pathway_pos, unit.MapLayer->ID)) {
									built_pathway = true;
									built_pathway_for_building = true;
									break;
								}
							}
						}
					}
					if (built_pathway) {
						break;
					}
				}
			}

			checked_buildings += 1;
			if (built_pathway_for_building || checked_buildings >= 2) { //don't check too many buildings at once, for performance reasons; and stop if succeeded in building a pathway for a building, so that we can check again the next time if that building still needs pathways
				AiPlayer->LastPathwayConstructionBuilding = UnitNumber(unit);
				return;
			}
		}
	}
		
	AiPlayer->LastPathwayConstructionBuilding = 0;
}

/**
**  Check if the AI can build a settlement.
*/
void AiCheckSettlementConstruction()
{
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	int town_hall_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, GetUnitTypeClassIndexByName("town-hall"));			
	if (town_hall_type_id == -1) {
		return;
	}
	
	CUnitType *town_hall_type = UnitTypes[town_hall_type_id];
	
	if (!CheckDependencies(town_hall_type, AiPlayer->Player)) {
		return;
	}

	int n_t = AiHelpers.Build.size();
	std::vector<std::vector<CUnitType *> > &tablep = AiHelpers.Build;
	if (town_hall_type->Slot >= n_t) { // Oops not known.
		DebugPrint("%d: AiCheckSettlementConstruction I: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ town_hall_type->Ident.c_str());
		return;
	}
		
	std::vector<CUnitType *> &table = tablep[town_hall_type->Slot];
	if (table.empty()) { // Oops not known.
		DebugPrint("%d: AiCheckSettlementConstruction II: Nothing known about '%s'\n"
				   _C_ AiPlayer->Player->Index _C_ town_hall_type->Ident.c_str());
		return;
	}

	//check in which landmasses this player has workers
	std::vector<int> worker_landmasses;
	AiPlayer->Player->GetWorkerLandmasses(worker_landmasses, town_hall_type);
						
	//check settlement units to see if can build in one
	for (size_t i = 0; i < Map.SiteUnits.size(); ++i) {
		CUnit *settlement_unit = Map.SiteUnits[i];
		
		if (!settlement_unit->IsAliveOnMap()) {
			continue;
		}
		
		if (settlement_unit->Player->Index != PlayerNumNeutral) {
			continue;
		}
		
		if (!settlement_unit->IsVisibleAsGoal(*AiPlayer->Player)) {
			continue;
		}
		
		int settlement_landmass = Map.GetTileLandmass(settlement_unit->tilePos, settlement_unit->MapLayer->ID);
		if (std::find(worker_landmasses.begin(), worker_landmasses.end(), settlement_landmass) == worker_landmasses.end()) {
			continue;
		}
		
		if (AiGetUnitTypeRequestedCount(*AiPlayer, town_hall_type, 0, settlement_unit->Settlement) > 0) { //already requested
			continue;
		}
		
		if (!CanBuildHere(nullptr, *town_hall_type, settlement_unit->tilePos, settlement_unit->MapLayer->ID)) {
			continue;
		}
		
		bool requested_settlement = false;
		//
		// Find a free worker who can build a settlement on this site
		//
		for (unsigned int j = 0; j < tablep[town_hall_type->Slot].size(); ++j) {
			//
			// The type is available
			//
			if (AiPlayer->Player->GetUnitTypeAiActiveCount(tablep[town_hall_type->Slot][j])) {
				AiAddUnitTypeRequest(*town_hall_type, 1, 0, settlement_unit->Settlement, settlement_unit->tilePos, settlement_unit->MapLayer->ID);
				requested_settlement = true;
				break;
			}
		}
		
		if (requested_settlement) {
			break;
		}
	}
}

/**
**  Check if the AI should build a dock in a specific "landmass" (water zone).
*/
void AiCheckDockConstruction()
{
	if (AiPlayer->Player->NumTownHalls < 1) { //don't build docks if has no town hall yet
		return;
	}
	
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	int dock_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, GetUnitTypeClassIndexByName("dock"));			
	if (dock_type_id == -1) {
		return;
	}
	
	CUnitType *dock_type = UnitTypes[dock_type_id];
	
	if (!AiRequestedTypeAllowed(*AiPlayer->Player, *dock_type)) {
		return;
	}

	//check in which landmasses this player has workers
	std::vector<int> worker_landmasses;
	AiPlayer->Player->GetWorkerLandmasses(worker_landmasses, dock_type);

	std::vector<int> neighbor_water_landmasses; //water "landmasses" neighboring the landmasses where the player has workers
	for (size_t i = 0; i < worker_landmasses.size(); ++i) {
		int worker_landmass = worker_landmasses[i];
		for (size_t j = 0; j < Map.BorderLandmasses[worker_landmass].size(); ++j) {
			int border_landmass = Map.BorderLandmasses[worker_landmass][j];
			if (std::find(neighbor_water_landmasses.begin(), neighbor_water_landmasses.end(), border_landmass) == neighbor_water_landmasses.end()) {
				neighbor_water_landmasses.push_back(border_landmass);
			}
		}
	}
	
	std::vector<CUnit *> dock_table;
	FindPlayerUnitsByType(*AiPlayer->Player, *dock_type, dock_table, true);
	
	for (size_t i = 0; i < neighbor_water_landmasses.size(); ++i) {
		int water_landmass = neighbor_water_landmasses[i];
		if (Map.BorderLandmasses[water_landmass].size() < 2) { //if the water "landmass" only borders one landmass, then there is no need to build a dock on it, as it can lead to no other landmasses
			continue;
		}
		
		if (!AiPlayer->Player->HasSettlementNearWaterZone(water_landmass)) { //must have a settlement near the water "landmass" before building a dock on it; we don't want workers to go far away from the player's settlements to build a dock
			continue;
		}
		
		//check if already has a dock in the water "landmass"
		bool has_dock = false;
		for (size_t j = 0; j < dock_table.size(); ++j) {
			CUnit &dock_unit = *dock_table[j];
					
			if (Map.GetTileLandmass(dock_unit.tilePos, dock_unit.MapLayer->ID) == water_landmass) {
				has_dock = true;
				break;
			}
		}
		
		if (!has_dock) { //if doesn't have an already built dock, see if there's one in the requests already
			for (size_t j = 0; j < AiPlayer->UnitTypeBuilt.size(); ++j) {
				const AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[j];
				if (queue.Landmass == water_landmass && queue.Type == dock_type) {
					has_dock = true;
					break;
				}
			}
		}
		
		if (!has_dock) { // if doesn't have a dock, request one
			AiAddUnitTypeRequest(*dock_type, 1, water_landmass);
		} else {
			int transport_capacity = AiGetTransportCapacity(water_landmass) + AiGetRequestedTransportCapacity(water_landmass);
			if (transport_capacity == 0) { //if the AI has no transporters in the given landmass, build one (for scouting)
				AiTransportCapacityRequest(1, water_landmass);
			}
		}
	}
}

void AiCheckUpgrades()
{
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	std::vector<CUpgrade *> potential_upgrades = AiPlayer->Player->GetResearchableUpgrades();
	
	for (size_t i = 0; i < potential_upgrades.size(); ++i) {
		CUpgrade *upgrade = potential_upgrades[i];
		
		if (!AiRequestedUpgradeAllowed(*AiPlayer->Player, upgrade)) {
			continue;
		}
		
		if (AiHasUpgrade(*AiPlayer, upgrade, true)) {
			continue;
		}
		
		if (AiPlayer->Player->UpgradeRemovesExistingUpgrade(upgrade, true)) {
			continue;
		}
		
		if (AiPlayer->NeededMask & AiPlayer->Player->GetUpgradeCostsMask(upgrade)) { //don't request the upgrade if it is going to use up a resource that is currently needed
			continue;
		}
		
		//remove any removed upgrades from the requests, to prevent mutually-incompatible upgrades from being researched back and forth
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			for (size_t j = 0; j < upgrade->UpgradeModifiers[z]->RemoveUpgrades.size(); ++j) {
				CUpgrade *removed_upgrade = upgrade->UpgradeModifiers[z]->RemoveUpgrades[j];
				if (std::find(AiPlayer->ResearchRequests.begin(), AiPlayer->ResearchRequests.end(), removed_upgrade) != AiPlayer->ResearchRequests.end()) {
					AiPlayer->ResearchRequests.erase(std::remove(AiPlayer->ResearchRequests.begin(), AiPlayer->ResearchRequests.end(), removed_upgrade), AiPlayer->ResearchRequests.end());
				}
			}
		}

		AiPlayer->ResearchRequests.push_back(upgrade);
	}
}

void AiCheckBuildings()
{
	if (AiPlayer->Player->Race == -1 || AiPlayer->Player->Faction == -1) {
		return;
	}

	if (AiPlayer->Player->NumTownHalls < 1) { //don't build structures if has no town hall yet
		return;
	}

	if (AiPlayer->Player->AiName == "passive") {
		return;
	}

	std::vector<CAiBuildingTemplate *> building_templates = PlayerRaces.Factions[AiPlayer->Player->Faction]->GetAiBuildingTemplates();
	std::vector<CAiBuildingTemplate *> potential_building_templates;
	
	int priority = 0;
	std::vector<int> want_counter;
	std::vector<int> have_counter;
	std::vector<int> have_with_requests_counter;
	for (size_t i = 0; i < UnitTypeClasses.size(); ++i) {
		want_counter.push_back(0);
		have_counter.push_back(-1);
		have_with_requests_counter.push_back(-1);
	}
	for (size_t i = 0; i < building_templates.size(); ++i) {
		if (building_templates[i]->Priority < priority) {
			break; //building templates are ordered by priority, so there is no need to go further
		}
		
		int unit_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, building_templates[i]->UnitClass);
		CUnitType *type = nullptr;
		if (unit_type_id != -1) {
			type = UnitTypes[unit_type_id];
		}
		if (!type || !AiRequestedTypeAllowed(*AiPlayer->Player, *type, false, true)) {
			continue;
		}
		
		if (AiPlayer->NeededMask & AiPlayer->Player->GetUnitTypeCostsMask(type)) { //don't request the building if it is going to use up a resource that is currently needed
			continue;
		}
		
		want_counter[building_templates[i]->UnitClass]++;
			
		if (have_counter[building_templates[i]->UnitClass] == -1 || have_with_requests_counter[building_templates[i]->UnitClass] == -1) { //initialize values
			have_counter[building_templates[i]->UnitClass] = AiGetUnitTypeCount(*AiPlayer, type, 0, false, true);
			have_with_requests_counter[building_templates[i]->UnitClass] = AiGetUnitTypeCount(*AiPlayer, type, 0, true, true);
		}
			
		if (have_with_requests_counter[building_templates[i]->UnitClass] >= want_counter[building_templates[i]->UnitClass]) {
			if (have_counter[building_templates[i]->UnitClass] < want_counter[building_templates[i]->UnitClass]) {
				priority = building_templates[i]->Priority; //requested but not built, don't build anything of lower priority while this isn't done
			}
			continue; //already requested/built, continue
		}
			
		if (building_templates[i]->Priority > priority) {
			priority = building_templates[i]->Priority;
			potential_building_templates.clear();
		}
		potential_building_templates.push_back(building_templates[i]);
	}
	
	if (potential_building_templates.empty()) {
		return;
	}
	
	CAiBuildingTemplate *building_template = potential_building_templates[SyncRand(potential_building_templates.size())];
	
	int unit_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, building_template->UnitClass);
	CUnitType *type = UnitTypes[unit_type_id];
	
	if (type->Slot < (int) AiHelpers.Build.size() && !AiHelpers.Build[type->Slot].empty()) { //constructed by worker
		AiAddUnitTypeRequest(*type, 1);
	} else if (type->Slot < (int) AiHelpers.Upgrade.size() && !AiHelpers.Upgrade[type->Slot].empty()) { //upgraded to from another building
		AiAddUpgradeToRequest(*type);
	} else {
		fprintf(stderr, "Unit type \"%s\" is in an AiBuildingTemplate, but it cannot be built by any worker, and no unit type can upgrade to it.\n", type->Ident.c_str());
	}
}

static void AiCheckMinecartConstruction()
{
	int minecart_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, GetUnitTypeClassIndexByName("minecart"));
	if (minecart_type_id == -1) {
		return;
	}
	
	CUnitType *minecart_type = UnitTypes[minecart_type_id];
		
	if (!AiRequestedTypeAllowed(*AiPlayer->Player, *minecart_type, false, false)) {
		return;
	}
	
	int max_minecarts = 5;
	int minecart_count = AiGetUnitTypeCount(*AiPlayer, minecart_type, 0, true, false);
	
	if (minecart_count >= max_minecarts) {
		return;
	}
	
	std::vector<CSite *> potential_settlements;
		
	for (size_t res = 0; res < CResource::Resources.size(); ++res) {
		if (res >= (int) AiHelpers.Mines.size()) {
			break;
		}
		if (!minecart_type->ResInfo[res]) {
			continue;
		}
				
		for (size_t i = 0; i < AiHelpers.Mines[res].size(); ++i) {
			CUnitType &mine_type = *AiHelpers.Mines[res][i];
					
			std::vector<CUnit *> mine_table;
			FindPlayerUnitsByType(*AiPlayer->Player, mine_type, mine_table, true);
					
			for (size_t j = 0; j < mine_table.size(); ++j) {
				const CUnit *mine_unit = mine_table[j];
				CSite *mine_settlement = mine_unit->Settlement;
						
				if (!mine_settlement) {
					continue;
				}
						
				const CUnit *town_hall_unit = mine_settlement->SiteUnit;
				
				if (!town_hall_unit) {
					continue;
				}
				
				std::vector<CUnit *> settlement_minecart_table;
				SelectAroundUnit(*mine_unit, 8, settlement_minecart_table, MakeAndPredicate(HasSamePlayerAs(*AiPlayer->Player), HasSameTypeAs(*minecart_type)));
				int settlement_minecart_count = settlement_minecart_table.size();
				settlement_minecart_count += mine_unit->GetTotalInsideCount(AiPlayer->Player, true, false, minecart_type);
				settlement_minecart_count += town_hall_unit->GetTotalInsideCount(AiPlayer->Player, true, false, minecart_type);
				settlement_minecart_count += AiGetUnitTypeRequestedCount(*AiPlayer, minecart_type, 0, mine_settlement);
				
				if (settlement_minecart_count > 0) { //only build one minecart per mine, to prevent pathway blockage
					continue;
				}
				
				if (CheckPathwayConnection(*mine_unit, *town_hall_unit, MapFieldRailroad)) {
					potential_settlements.push_back(mine_settlement);
				}
			}
		}
	}
	
	if (potential_settlements.size() > 0) {
		AiAddUnitTypeRequest(*minecart_type, 1, 0, potential_settlements[SyncRand(potential_settlements.size())]);
	}
}

static void AiCheckMinecartSalvaging()
{
	int minecart_type_id = PlayerRaces.GetFactionClassUnitType(AiPlayer->Player->Faction, GetUnitTypeClassIndexByName("minecart"));
	if (minecart_type_id == -1) {
		return;
	}
	
	CUnitType *minecart_type = UnitTypes[minecart_type_id];
	std::vector<CUnit *> minecart_table;
	FindPlayerUnitsByType(*AiPlayer->Player, *minecart_type, minecart_table, true);
	
	for (size_t i = 0; i < minecart_table.size(); ++i) {
		CUnit *minecart_unit = minecart_table[i];
		
		if (!minecart_unit->IsIdle()) {
			continue;
		}
		
		if (minecart_unit->Container) {
			continue;
		}
		
		bool has_accessible_mine = false;
		
		for (size_t res = 0; res < CResource::Resources.size(); ++res) {
			if (!minecart_type->ResInfo[res]) {
				continue;
			}
			
			if (UnitFindResource(*minecart_unit, *minecart_unit, 1000, res, false, nullptr, true, true, false, false, true, false) != nullptr) {
				has_accessible_mine = true;
				break;
			}
		}
		
		//salvage minecarts that have no accessible mines
		if (!has_accessible_mine) {
			CommandDismiss(*minecart_unit, true);
		}
	}
}
	
void AiCheckWorkers()
{
	if (AiPlayer->Player->Race == -1 || AiPlayer->Player->Faction == -1) {
		return;
	}

	if (AiPlayer->Player->AiName == "passive") {
		return;
	}
	
	AiCheckMinecartSalvaging();
	
	if (AiPlayer->Player->NumTownHalls < 1) { //don't train workers if has no town hall yet
		return;
	}

	AiCheckMinecartConstruction();
}
//Wyrmgus end

/**
**  Add unit-type request to resource manager.
**
**  @param type   Unit type requested.
**  @param count  How many units.
**
**  @todo         FIXME: should store the end of list and not search it.
*/
//Wyrmgus start
//void AiAddUnitTypeRequest(CUnitType &type, int count)
void AiAddUnitTypeRequest(CUnitType &type, const int count, const int landmass, CSite *settlement, const Vec2i pos, int z)
//Wyrmgus end
{
	AiBuildQueue queue;

	queue.Type = &type;
	queue.Want = count;
	queue.Made = 0;
	queue.Landmass = landmass;
	queue.Settlement = settlement;
	queue.Pos = pos;
	queue.MapLayer = z;
	AiPlayer->UnitTypeBuilt.push_back(queue);
}

/**
**  Mark that a zone is requiring exploration.
**
**  @param pos   Pos of the zone
**  @param mask  Mask to explore ( land/sea/air )
*/
void AiExplore(const Vec2i &pos, int mask)
{
	if (!Preference.AiExplores) {
		return;
	}
	AiExplorationRequest req(pos, mask);

	// Link into the exploration requests list
	AiPlayer->FirstExplorationRequest.insert(
		AiPlayer->FirstExplorationRequest.begin(), req);
}

/**
**  Entry point of resource manager, periodically called.
*/
void AiResourceManager()
{
	// Check if something needs to be build / trained.
	AiCheckingWork();

	// Look if we can build a farm in advance.
	//Wyrmgus start
//	if (!AiPlayer->NeedSupply && AiPlayer->Player->Supply == AiPlayer->Player->Demand) {
	if (!AiPlayer->NeedSupply && AiPlayer->Player->Supply <= (AiPlayer->Player->Demand + 4)) { //try to keep surplus supply (of 4 in this case)
	//Wyrmgus end
		AiRequestSupply();
	}

	// Collect resources.
	if ((GameCycle / CYCLES_PER_SECOND) % COLLECT_RESOURCES_INTERVAL ==
		(unsigned long)AiPlayer->Player->Index % COLLECT_RESOURCES_INTERVAL) {
		//Wyrmgus start
		AiProduceResources(); //handle building resource production choice
		//Wyrmgus end
		AiCollectResources();
	}

	// Check repair.
	AiCheckRepair();
	
	//Wyrmgus start
	AiCheckPathwayConstruction();
	//Wyrmgus end
}

//@}
