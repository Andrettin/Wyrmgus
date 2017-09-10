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
/**@name unit_find.cpp - The find/select for units. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include <limits.h>

#include "stratagus.h"

#include "unit_find.h"

#include "actions.h"
#include "map.h"
#include "missile.h"
#include "pathfinder.h"
#include "player.h"
#include "spells.h"
#include "tileset.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"

/*----------------------------------------------------------------------------
  -- Finding units
  ----------------------------------------------------------------------------*/

//Wyrmgus start
//void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units)
void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, bool circle)
//Wyrmgus end
{
	//Wyrmgus start
//	Select(ltPos, rbPos, units, NoFilter());
	Select(ltPos, rbPos, units, z, NoFilter());
	//Wyrmgus end
}

//Wyrmgus start
//void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units)
void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, bool circle)
//Wyrmgus end
{
	//Wyrmgus start
//	Select(ltPos, rbPos, units, NoFilter());
	Select(ltPos, rbPos, units, z, NoFilter());
	//Wyrmgus end
}

//Wyrmgus start
//void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around)
void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around, bool circle)
//Wyrmgus end
{
	SelectAroundUnit(unit, range, around, NoFilter());
}

CUnit *UnitFinder::FindUnitAtPos(const Vec2i &pos) const
{
	//Wyrmgus start
//	CUnitCache &cache = Map.Field(pos)->UnitCache;
	CUnitCache &cache = Map.Field(pos, z)->UnitCache;
	//Wyrmgus end

	for (CUnitCache::iterator it = cache.begin(); it != cache.end(); ++it) {
		CUnit *unit = *it;

		if (std::find(units.begin(), units.end(), unit) != units.end()) {
			return unit;
		}
	}
	return NULL;
}

VisitResult UnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
//	if (!player.AiEnabled && !Map.Field(pos)->playerInfo.IsExplored(player)) {
	if (!Map.Field(pos, z)->playerInfo.IsTeamExplored(player)) {
	//Wyrmgus end
		return VisitResult_DeadEnd;
	}
	// Look if found what was required.
	CUnit *unit = FindUnitAtPos(pos);
	if (unit) {
		*unitP = unit;
		return VisitResult_Finished;
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult_Ok;
		} else {
			return VisitResult_DeadEnd;
		}
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

class TerrainFinder
{
public:
	//Wyrmgus start
//	TerrainFinder(const CPlayer &player, int maxDist, int movemask, int resmask, Vec2i *resPos) :
//		player(player), maxDist(maxDist), movemask(movemask), resmask(resmask), resPos(resPos) {}
	TerrainFinder(const CPlayer &player, int maxDist, int movemask, int resource, Vec2i *resPos, int z) :
		player(player), maxDist(maxDist), movemask(movemask), resource(resource), resPos(resPos), z(z) {}
	//Wyrmgus end
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CPlayer &player;
	int maxDist;
	int movemask;
	//Wyrmgus start
//	int resmask;
	int resource;
	int z;
	//Wyrmgus end
	Vec2i *resPos;
};

VisitResult TerrainFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
//	if (!player.AiEnabled && !Map.Field(pos)->playerInfo.IsExplored(player)) {
	if (!Map.Field(pos, z)->playerInfo.IsTeamExplored(player)) {
	//Wyrmgus end
		return VisitResult_DeadEnd;
	}
	
	//Wyrmgus start
	if (Map.Field(pos, z)->Owner != -1 && Map.Field(pos, z)->Owner != player.Index && !Players[Map.Field(pos, z)->Owner].HasNeutralFactionType() && !player.HasNeutralFactionType()) {
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	
	// Look if found what was required.
	//Wyrmgus start
//	if (Map.Field(pos)->CheckMask(resmask)) {
	if (Map.Field(pos, z)->GetResource() == resource) {
	//Wyrmgus end
		if (resPos) {
			*resPos = pos;
		}
		return VisitResult_Finished;
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult_Ok;
		} else {
			return VisitResult_DeadEnd;
		}
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

/**
**  Find the closest piece of terrain with the given flags.
**
**  @param movemask    The movement mask to reach that location.
**  @param resmask     Result tile mask.
**  @param range       Maximum distance for the search.
**  @param player      Only search fields explored by player
**  @param startPos    Map start position for the search.
**
**  @param terrainPos  OUT: Map position of tile.
**
**  @note Movement mask can be 0xFFFFFFFF to have no effect
**  Range is not circular, but square.
**  Player is ignored if nil(search the entire map)
**
**  @return            True if wood was found.
*/
//Wyrmgus start
//bool FindTerrainType(int movemask, int resmask, int range,
bool FindTerrainType(int movemask, int resource, int range,
//Wyrmgus end
					 //Wyrmgus start
//					 const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos)
					 const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos, int z)
					 //Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
	terrainTraversal.SetSize(Map.Info.LayersSizes[z].x, Map.Info.LayersSizes[z].y);
	//Wyrmgus end
	terrainTraversal.Init();

	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	TerrainFinder terrainFinder(player, range, movemask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit), resmask, terrainPos);
	TerrainFinder terrainFinder(player, range, movemask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit), resource, terrainPos, z);
	//Wyrmgus end

	return terrainTraversal.Run(terrainFinder);
}


template <const bool NEARLOCATION>
class BestDepotFinder
{
	inline void operator()(CUnit *const dest)
	{
		/* Only resource depots */
		if (dest->Type->CanStore[resource]
			//Wyrmgus start
			&& (NEARLOCATION || u_near.worker->CanReturnGoodsTo(dest, resource))
			//Wyrmgus end
			&& dest->IsAliveOnMap()
			&& dest->CurrentAction() != UnitActionBuilt) {
			// Unit in range?

			if (NEARLOCATION) {
				//Wyrmgus start
//				int d = dest->MapDistanceTo(u_near.loc);
				int d = dest->MapDistanceTo(u_near.loc, u_near.layer);
				//Wyrmgus end

				//
				// Take this depot?
				//
				if (d <= range && d < best_dist) {
					best_depot = dest;
					best_dist = d;
				}
			} else {
				int d;
				const CUnit *worker = u_near.worker;
				if (!worker->Container) {
					d = worker->MapDistanceTo(*dest);
				} else {
					d = worker->Container->MapDistanceTo(*dest);
				}

				// Use Circle, not square :)
				if (d > range) {
					return;
				}

				if (best_dist == INT_MAX) {
					best_depot = dest;
				}
				
				// calck real travel distance
				if (worker->Container) {
					UnmarkUnitFieldFlags(*worker->Container);
				}
				d = UnitReachable(*worker, *dest, 1);
				if (worker->Container) {
					MarkUnitFieldFlags(*worker->Container);
				}
				//
				// Take this depot?
				//
				if (d && d < best_dist) {
					best_depot = dest;
					best_dist = d;
				}
			}
		}
	}

public:
	BestDepotFinder(const CUnit &w, int res, int ran) :
		resource(res), range(ran),
		best_dist(INT_MAX), best_depot(0)
	{
		u_near.worker = &w;
	}

	//Wyrmgus start
//	BestDepotFinder(const Vec2i &pos, int res, int ran) :
	BestDepotFinder(const Vec2i &pos, int res, int ran, int z) :
	//Wyrmgus end
		resource(res), range(ran),
		best_dist(INT_MAX), best_depot(0)
	{
		u_near.loc = pos;
		//Wyrmgus start
		u_near.layer = z;
		//Wyrmgus end
	}

	template <typename ITERATOR>
	CUnit *Find(ITERATOR begin, ITERATOR end)
	{
		for (ITERATOR it = begin; it != end; ++it) {
			this->operator()(*it);
		}
		return best_depot;
	}

	CUnit *Find(CUnitCache &cache)
	{
		cache.for_each(*this);
		return best_depot;
	}
private:
	struct {
		const CUnit *worker;
		Vec2i loc;
		//Wyrmgus start
		int layer;
		//Wyrmgus end
	} u_near;
	const int resource;
	const int range;
	int best_dist;
public:
	CUnit *best_depot;
};

//Wyrmgus start
template <const bool NEARLOCATION>
class BestHomeMarketFinder
{
	inline void operator()(CUnit *const dest)
	{
		/* Only markets */
		if (
			dest->Type->BoolFlag[MARKET_INDEX].value
			&& dest->IsAliveOnMap()
			&& dest->CurrentAction() != UnitActionBuilt) {
			// Unit in range?

			if (NEARLOCATION) {
				int d = dest->MapDistanceTo(u_near.loc, u_near.layer);

				//
				// Take this market?
				//
				if (d <= range && d < best_dist) {
					best_market = dest;
					best_dist = d;
				}
			} else {
				int d;
				const CUnit *worker = u_near.worker;
				if (!worker->Container) {
					d = worker->MapDistanceTo(*dest);
				} else {
					d = worker->Container->MapDistanceTo(*dest);
				}

				// Use Circle, not square :)
				if (d > range) {
					return;
				}

				if (best_dist == INT_MAX) {
					best_market = dest;
				}
				
				// calck real travel distance
				if (worker->Container) {
					UnmarkUnitFieldFlags(*worker->Container);
				}
				d = UnitReachable(*worker, *dest, 1);
				if (worker->Container) {
					MarkUnitFieldFlags(*worker->Container);
				}
				//
				// Take this market?
				//
				if (d && d < best_dist) {
					best_market = dest;
					best_dist = d;
				}
			}
		}
	}

public:
	BestHomeMarketFinder(const CUnit &w, int ran) :
		range(ran),
		best_dist(INT_MAX), best_market(0)
	{
		u_near.worker = &w;
	}

	BestHomeMarketFinder(const Vec2i &pos, int ran, int z) :
		range(ran),
		best_dist(INT_MAX), best_market(0)
	{
		u_near.loc = pos;
		u_near.layer = z;
	}

	template <typename ITERATOR>
	CUnit *Find(ITERATOR begin, ITERATOR end)
	{
		for (ITERATOR it = begin; it != end; ++it) {
			this->operator()(*it);
		}
		return best_market;
	}

	CUnit *Find(CUnitCache &cache)
	{
		cache.for_each(*this);
		return best_market;
	}
private:
	struct {
		const CUnit *worker;
		Vec2i loc;
		int layer;
	} u_near;
	const int range;
	int best_dist;
public:
	CUnit *best_market;
};
//Wyrmgus end

//Wyrmgus start
//CUnit *FindDepositNearLoc(CPlayer &p, const Vec2i &pos, int range, int resource)
CUnit *FindDepositNearLoc(CPlayer &p, const Vec2i &pos, int range, int resource, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	BestDepotFinder<true> finder(pos, resource, range);
	BestDepotFinder<true> finder(pos, resource, range, z);
	//Wyrmgus end
	std::vector<CUnit *> table;
	for (std::vector<CUnit *>::iterator it = p.UnitBegin(); it != p.UnitEnd(); ++it) {
		table.push_back(*it);
	}
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].IsAllied(p) && p.IsAllied(Players[i])) {
			for (std::vector<CUnit *>::iterator it = Players[i].UnitBegin(); it != Players[i].UnitEnd(); ++it) {
				table.push_back(*it);
			}
		}
	}
	return finder.Find(table.begin(), table.end());
}

class CResourceFinder
{
public:
	//Wyrmgus start
//	CResourceFinder(int r, bool on_top) : resource(r), mine_on_top(on_top) {}
	CResourceFinder(int r, bool harvestable, bool luxury, bool same) : resource(r), only_harvestable(harvestable), include_luxury(luxury), only_same(same) {}
	//Wyrmgus end
	bool operator()(const CUnit *const unit) const
	{
		const CUnitType &type = *unit->Type;
		//Wyrmgus start
//		return (type.GivesResource == resource
		return ((unit->GivesResource == resource || (!only_same && unit->GivesResource != TradeCost && DefaultResourceFinalResources[unit->GivesResource] == resource) || (include_luxury && LuxuryResources[unit->GivesResource]))
		//Wyrmgus end
				&& unit->ResourcesHeld != 0
				//Wyrmgus start
//				&& (mine_on_top ? type.BoolFlag[CANHARVEST_INDEX].value : !type.BoolFlag[CANHARVEST_INDEX].value)
				&& (type.BoolFlag[CANHARVEST_INDEX].value || !only_harvestable)
				//Wyrmgus end
				&& !unit->IsUnusable(true) //allow mines under construction
			   );
	}
private:
	const int resource;
	//Wyrmgus start
//	const bool mine_on_top;
	const bool only_harvestable;
	const bool include_luxury;
	const bool only_same;
	//Wyrmgus end
};

class ResourceUnitFinder
{
public:
	//Wyrmgus start
//	ResourceUnitFinder(const CUnit &worker, const CUnit *deposit, int resource, int maxRange, bool check_usage, CUnit **resultMine) :
	ResourceUnitFinder(const CUnit &worker, const CUnit *deposit, int resource, int maxRange, bool check_usage, CUnit **resultMine, bool only_harvestable, bool ignore_exploration, bool only_unsettled_area, bool include_luxury, bool only_same) :
	//Wyrmgus end
		worker(worker),
		resinfo(*worker.Type->ResInfo[resource]),
		deposit(deposit),
		movemask(worker.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		maxRange(maxRange),
		check_usage(check_usage),
		//Wyrmgus start
		only_harvestable(only_harvestable),
		ignore_exploration(ignore_exploration),
		only_unsettled_area(only_unsettled_area),
		include_luxury(include_luxury),
		only_same(only_same),
//		res_finder(resource, 1),
		res_finder(resource, only_harvestable, include_luxury, only_same),
		//Wyrmgus end
		resultMine(resultMine)
	{
		bestCost.SetToMax();
		*resultMine = NULL;
	}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	bool MineIsUsable(const CUnit &mine) const;

	struct ResourceUnitFinder_Cost {
	public:
		//Wyrmgus start
//		void SetFrom(const CUnit &mine, const CUnit *deposit, bool check_usage);
		void SetFrom(const CUnit &mine, const CUnit *deposit, const CUnit &worker, bool check_usage);
		//Wyrmgus end
		bool operator < (const ResourceUnitFinder_Cost &rhs) const
		{
			if (waiting != rhs.waiting) {
				return waiting < rhs.waiting;
			} else if (distance != rhs.distance) {
				return distance < rhs.distance;
			} else {
				return assigned < rhs.assigned;
			}
		}
		void SetToMax() { assigned = waiting = distance = UINT_MAX; }
		bool IsMin() const { return assigned == 0 && waiting == 0 && distance == 0; }

	public:
		unsigned int assigned;
		unsigned int waiting;
		unsigned int distance;
	};

private:
	const CUnit &worker;
	const ResourceInfo &resinfo;
	const CUnit *deposit;
	unsigned int movemask;
	int maxRange;
	bool check_usage;
	//Wyrmgus start
	bool only_harvestable;
	bool ignore_exploration;
	bool only_unsettled_area;
	bool include_luxury;
	bool only_same;
	//Wyrmgus end
	CResourceFinder res_finder;
	ResourceUnitFinder_Cost bestCost;
	CUnit **resultMine;
};

bool ResourceUnitFinder::MineIsUsable(const CUnit &mine) const
{
	//Wyrmgus start
	if (only_unsettled_area) {
		std::vector<CUnit *> table;
		SelectAroundUnit(mine, 8, table, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
		if (table.size() > 0) {
			return false;
		}
	}
	
//	return mine.Type->BoolFlag[CANHARVEST_INDEX].value && mine.ResourcesHeld
	return mine.ResourcesHeld
	//Wyrmgus end
			//Wyrmgus start
		   && !mine.IsUnusable(false)
//		   && (resinfo.HarvestFromOutside
//			   || mine.Player->Index == PlayerMax - 1
//			   || mine.Player == worker.Player
//			   || (worker.IsAllied(mine) && mine.IsAllied(worker)));
		   && worker.CanHarvest(&mine, only_harvestable);
			//Wyrmgus end
}

//Wyrmgus start
//void ResourceUnitFinder::ResourceUnitFinder_Cost::SetFrom(const CUnit &mine, const CUnit *deposit, bool check_usage)
void ResourceUnitFinder::ResourceUnitFinder_Cost::SetFrom(const CUnit &mine, const CUnit *deposit, const CUnit &worker, bool check_usage)
//Wyrmgus end
{
	distance = deposit ? mine.MapDistanceTo(*deposit) : 0;
	//Wyrmgus start
	distance = distance * 100 / DefaultResourceFinalResourceConversionRates[mine.GivesResource]; // alter the distance score by the conversion rate, so that the unit will prefer resources with better conversion rates, but without going for ones that are too far away
	if (LuxuryResources[mine.GivesResource]) {
		int price_modifier = worker.Player->Prices[mine.GivesResource];
		if (DefaultResourceInputResources[mine.GivesResource]) {
			price_modifier -= worker.Player->Prices[DefaultResourceInputResources[mine.GivesResource]];
		}
		price_modifier = std::max(price_modifier, 1);
		distance = distance * 100 / price_modifier;
	}
	if (!mine.Type->BoolFlag[CANHARVEST_INDEX].value) { // if it is a deposit rather than a readily-harvestable resource, multiply the distance score
		distance *= 8;
	}
	//Wyrmgus end
	if (check_usage) {
		assigned = mine.Resource.Assigned - mine.Type->MaxOnBoard;
		waiting = GetNumWaitingWorkers(mine);
	} else {
		assigned = 0;
		waiting = 0;
	}
}

VisitResult ResourceUnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
//	if (!worker.Player->AiEnabled && !Map.Field(pos)->playerInfo.IsExplored(*worker.Player)) {
	if (!Map.Field(pos, worker.MapLayer)->playerInfo.IsTeamExplored(*worker.Player) && !ignore_exploration) {
	//Wyrmgus end
		return VisitResult_DeadEnd;
	}

	//Wyrmgus start
	if (Map.Field(pos, worker.MapLayer)->Owner != -1 && Map.Field(pos, worker.MapLayer)->Owner != worker.Player->Index && !Players[Map.Field(pos, worker.MapLayer)->Owner].HasNeutralFactionType() && !worker.Player->HasNeutralFactionType()) {
		return VisitResult_DeadEnd;
	}
	
//	CUnit *mine = Map.Field(pos)->UnitCache.find(res_finder);
	CUnit *mine = Map.Field(pos, worker.MapLayer)->UnitCache.find(res_finder);
	//Wyrmgus end

	//Wyrmgus start
//	if (mine && mine != *resultMine && MineIsUsable(*mine)) {
	if (
		mine && mine != *resultMine && MineIsUsable(*mine)
		&& (mine->Type->BoolFlag[CANHARVEST_INDEX].value || Map.Field(pos, worker.MapLayer)->Owner == -1 || Map.Field(pos, worker.MapLayer)->Owner == worker.Player->Index) //this is needed to prevent neutral factions from trying to build mines in others' territory
	) {
	//Wyrmgus end
		ResourceUnitFinder::ResourceUnitFinder_Cost cost;

		//Wyrmgus start
//		cost.SetFrom(*mine, deposit, check_usage);
		cost.SetFrom(*mine, deposit, worker, check_usage);
		if (worker.Container) {
			UnmarkUnitFieldFlags(*worker.Container);
		}
		//Wyrmgus end
		if (cost < bestCost && UnitReachable(worker, *mine, 1)) {
			*resultMine = mine;

			if (cost.IsMin()) {
				//Wyrmgus start
				if (worker.Container) {
					MarkUnitFieldFlags(*worker.Container);
				}
				//Wyrmgus end
				return VisitResult_Finished;
			}
			bestCost = cost;
		}
		//Wyrmgus start
		if (worker.Container) {
			MarkUnitFieldFlags(*worker.Container);
		}
		//Wyrmgus end
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, worker.MapLayer)) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) < maxRange) {
			return VisitResult_Ok;
		} else {
			return VisitResult_DeadEnd;
		}
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

/**
**  Find Resource.
**
**  @param unit        The unit that wants to find a resource.
**  @param startUnit   Find closest unit from this location
**  @param range       Maximum distance to the resource.
**  @param resource    The resource id.
**
**  @note This will return an usable resource building that doesn't
**  belong to the player or one of his allies.
**
**  @return            NULL or resource unit
*/
CUnit *UnitFindResource(const CUnit &unit, const CUnit &startUnit, int range, int resource,
						//Wyrmgus start
//						bool check_usage, const CUnit *deposit)
						bool check_usage, const CUnit *deposit, bool only_harvestable, bool ignore_exploration, bool only_unsettled_area, bool include_luxury, bool only_same)
						//Wyrmgus end
{
	if (!deposit) { // Find the nearest depot
		//Wyrmgus start
//		deposit = FindDepositNearLoc(*unit.Player, startUnit.tilePos, range, resource);
		deposit = FindDepositNearLoc(*unit.Player, startUnit.tilePos, range, resource, unit.MapLayer);
		//Wyrmgus end
	}

	TerrainTraversal terrainTraversal;

	//Wyrmgus start
	terrainTraversal.SetSize(Map.Info.LayersSizes[unit.MapLayer].x, Map.Info.LayersSizes[unit.MapLayer].y);
	//Wyrmgus end
	terrainTraversal.Init();

	terrainTraversal.PushUnitPosAndNeighboor(startUnit);

	CUnit *resultMine = NULL;

	//Wyrmgus start
//	ResourceUnitFinder resourceUnitFinder(unit, deposit, resource, range, check_usage, &resultMine);
	ResourceUnitFinder resourceUnitFinder(unit, deposit, resource, range, check_usage, &resultMine, only_harvestable, ignore_exploration, only_unsettled_area, include_luxury, only_same);
	//Wyrmgus end

	terrainTraversal.Run(resourceUnitFinder);
	return resultMine;
}

/**
**  Find "deposit". This will find a depot for a resource
**
**  @param unit        The unit that wants to find a resource.
**  @param range       Maximum distance to the depot.
**  @param resource    Resource to find deposit from.
**
**  @note This will return a reachable allied depot.
**
**  @return            NULL or deposit unit
*/
CUnit *FindDeposit(const CUnit &unit, int range, int resource)
{
	BestDepotFinder<false> finder(unit, resource, range);
	std::vector<CUnit *> table;
	for (std::vector<CUnit *>::iterator it = unit.Player->UnitBegin(); it != unit.Player->UnitEnd(); ++it) {
		table.push_back(*it);
	}
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].IsAllied(*unit.Player) && unit.Player->IsAllied(Players[i])) {
			for (std::vector<CUnit *>::iterator it = Players[i].UnitBegin(); it != Players[i].UnitEnd(); ++it) {
				table.push_back(*it);
			}
		}
	}
	return finder.Find(table.begin(), table.end());
}

//Wyrmgus start
/**
**  Find home market. This will find a home market
**
**  @param unit        The unit that wants to find a resource.
**  @param range       Maximum distance to the market.
**
**  @note This will return a reachable self-owned market.
**
**  @return            NULL or market unit
*/
CUnit *FindHomeMarket(const CUnit &unit, int range)
{
	BestHomeMarketFinder<false> finder(unit, range);
	std::vector<CUnit *> table;
	for (std::vector<CUnit *>::iterator it = unit.Player->UnitBegin(); it != unit.Player->UnitEnd(); ++it) {
		table.push_back(*it);
	}
	return finder.Find(table.begin(), table.end());
}
//Wyrmgus end

/**
**  Find the next idle worker
**
**  @param player    Player's units to search through
**  @param last      Previous idle worker selected
**
**  @return NULL or next idle worker
*/
CUnit *FindIdleWorker(const CPlayer &player, const CUnit *last)
{
	CUnit *FirstUnitFound = NULL;
	int SelectNextUnit = (last == NULL) ? 1 : 0;
	const int nunits = player.GetUnitCount();

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = player.GetUnit(i);
		if (unit.Type->BoolFlag[HARVESTER_INDEX].value && !unit.Removed) {
			if (unit.CurrentAction() == UnitActionStill) {
				if (SelectNextUnit && !IsOnlySelected(unit)) {
					return &unit;
				}
				if (FirstUnitFound == NULL) {
					FirstUnitFound = &unit;
				}
			}
		}
		if (&unit == last) {
			SelectNextUnit = 1;
		}
	}
	if (FirstUnitFound != NULL && !IsOnlySelected(*FirstUnitFound)) {
		return FirstUnitFound;
	}
	return NULL;
}

/**
**  Find all units of type.
**
**  @param type       type of unit requested
**  @param units      array in which we have to store the units
**  @param everybody  if true, include all units
*/
void FindUnitsByType(const CUnitType &type, std::vector<CUnit *> &units, bool everybody)
{
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;

		//Wyrmgus start
//		if (unit.Type == &type && !unit.IsUnusable(everybody)) {
		if (unit.Type == &type && (!unit.IsUnusable(everybody) || (everybody && unit.IsAlive()))) {
		//Wyrmgus end
			units.push_back(&unit);
		}
	}
}

/**
**  Find all units of type.
**
**  @param player  we're looking for the units of this player
**  @param type    type of unit requested
**  @param table   table in which we have to store the units
*/
void FindPlayerUnitsByType(const CPlayer &player, const CUnitType &type, std::vector<CUnit *> &table, bool ai_active)
{
	const int nunits = player.GetUnitCount();
	int typecount = player.UnitTypesCount[type.Slot];

	if (ai_active) {
		typecount = player.UnitTypesAiActiveCount[type.Slot];
	}
	
	if (typecount < 0) { // if unit type count is negative, something wrong happened
		fprintf(stderr, "Player %d has a negative %s unit type count of %d.\n", player.Index, type.Ident.c_str(), typecount);
	}

	if (typecount == 0) {
		return;
	}

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = player.GetUnit(i);

		if (unit.Type != &type) {
			continue;
		}
		if (!unit.IsUnusable()) {
			table.push_back(&unit);
		}
		--typecount;
		if (typecount == 0) {
			return ;
		}
	}
}

/**
**  Unit on map tile.
**
**  @param index flat index position on map, tile-based.
**  @param type  UnitTypeType, (unsigned)-1 for any type.
**
**  @return      Returns first found unit on tile.
*/
//Wyrmgus start
//CUnit *UnitOnMapTile(const unsigned int index, unsigned int type)
CUnit *UnitOnMapTile(const unsigned int index, unsigned int type, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return Map.Field(index)->UnitCache.find(CUnitTypeFinder((UnitTypeType)type));
	return Map.Field(index, z)->UnitCache.find(CUnitTypeFinder((UnitTypeType)type));
	//Wyrmgus end
}

/**
**  Unit on map tile.
**
**  @param pos   position on map, tile-based.
**  @param type  UnitTypeType, (unsigned)-1 for any type.
**
**  @return      Returns first found unit on tile.
*/
//Wyrmgus start
//CUnit *UnitOnMapTile(const Vec2i &pos, unsigned int type)
CUnit *UnitOnMapTile(const Vec2i &pos, unsigned int type, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return UnitOnMapTile(Map.getIndex(pos), type);
	return UnitOnMapTile(Map.getIndex(pos, z), type, z);
	//Wyrmgus end
}

/**
**  Choose target on map area.
**
**  @param source  Unit which want to attack.
**  @param pos1    position on map, tile-based.
**  @param pos2    position on map, tile-based.
**
**  @return        Returns ideal target on map tile.
*/
CUnit *TargetOnMap(const CUnit &source, const Vec2i &pos1, const Vec2i &pos2)
{
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(pos1, pos2, table);
	Select(pos1, pos2, table, source.MapLayer);
	//Wyrmgus end
	CUnit *best = NULL;
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!unit.IsVisibleAsGoal(*source.Player)) {
			continue;
		}
		if (!CanTarget(*source.Type, *unit.Type)) {
			continue;
		}

		// Choose the best target.
		if (!best || best->Variable[PRIORITY_INDEX].Value < unit.Variable[PRIORITY_INDEX].Value) {
			best = &unit;
		}
	}
	return best;
}

/*----------------------------------------------------------------------------
--  Finding special units
----------------------------------------------------------------------------*/

/**
**  Resource on map tile
**
**  @param pos       position on map, tile-based.
**  @param resource  resource type.
**  @param mine_on_top  return mine or mining area.
**
**  @return          Returns the deposit if found, or NULL.
*/
//Wyrmgus start
//CUnit *ResourceOnMap(const Vec2i &pos, int resource, bool mine_on_top)
CUnit *ResourceOnMap(const Vec2i &pos, int resource, int z, bool only_harvestable, bool only_same)
//Wyrmgus end
{
	//Wyrmgus start
//	return Map.Field(pos)->UnitCache.find(CResourceFinder(resource, mine_on_top));
	return Map.Field(pos, z)->UnitCache.find(CResourceFinder(resource, only_harvestable, false, only_same));
	//Wyrmgus end
}

class IsADepositForResource
{
public:
	explicit IsADepositForResource(const int r) : resource(r) {}
	bool operator()(const CUnit *const unit) const
	{
		return (unit->Type->CanStore[resource] && !unit->IsUnusable());
	}
private:
	const int resource;
};

/**
**  Resource deposit on map tile
**
**  @param pos       position on map, tile-based.
**  @param resource  resource type.
**
**  @return          Returns the deposit if found, or NULL.
*/
//Wyrmgus start
//CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource)
CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return Map.Field(pos)->UnitCache.find(IsADepositForResource(resource));
	return Map.Field(pos, z)->UnitCache.find(IsADepositForResource(resource));
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
--  Finding units for attack
----------------------------------------------------------------------------*/

class BestTargetFinder
{
public:
	//Wyrmgus start
//	BestTargetFinder(const CUnit &a) :
	BestTargetFinder(const CUnit &a, bool i_n) :
	//Wyrmgus end
		//Wyrmgus start
//		attacker(&a)
		attacker(&a), include_neutral(i_n)
		//Wyrmgus end
	{}

	CUnit *Find(const std::vector<CUnit *> &table) const
	{
		return Find(table.begin(), table.end());
	}

	CUnit *Find(CUnitCache &cache) const
	{
		return Find(cache.begin(), cache.end());
	}

private:
	template <typename Iterator>
	CUnit *Find(Iterator begin, Iterator end) const
	{
		CUnit *enemy = NULL;
		int best_cost = INT_MAX;

		for (Iterator it = begin; it != end; ++it) {
			const int cost = ComputeCost(*it);

			if (cost < best_cost) {
				enemy = *it;
				best_cost = cost;
			}
		}
		return enemy;
	}

	int ComputeCost(CUnit *const dest) const
	{
		const CPlayer &player = *attacker->Player;
		const CUnitType &type = *attacker->Type;
		const CUnitType &dtype = *dest->Type;
		//Wyrmgus start
//		const int attackrange = attacker->Stats->Variables[ATTACKRANGE_INDEX].Max;
		int attackrange = attacker->GetModifiedVariable(ATTACKRANGE_INDEX);
		//Wyrmgus end

		//Wyrmgus start
//		if (!player.IsEnemy(*dest) // a friend or neutral
		if (
			(
				!attacker->IsEnemy(*dest) // a friend or neutral
				&& (!include_neutral || attacker->IsAllied(*dest) || dest->Player->Type == PlayerNeutral || attacker->Player->Index == dest->Player->Index)
			)
		//Wyrmgus end
			|| !dest->IsVisibleAsGoal(player)
			|| !CanTarget(type, dtype)) {
			return INT_MAX;
		}
		// Don't attack invulnerable units
		if (dtype.BoolFlag[INDESTRUCTIBLE_INDEX].value || dest->Variable[UNHOLYARMOR_INDEX].Value) {
			return INT_MAX;
		}

		//Wyrmgus start
		if (Map.IsLayerUnderground(attacker->MapLayer) && attackrange > 1 && !CheckObstaclesBetweenTiles(attacker->tilePos, dest->tilePos, MapFieldAirUnpassable, attacker->MapLayer)) {
			return INT_MAX;
		}
		//Wyrmgus end
		
		// Unit in range ?
		const int d = attacker->MapDistanceTo(*dest);

		if (d > attackrange && !UnitReachable(*attacker, *dest, attackrange, attacker->GetReactionRange() * 8)) {
			return INT_MAX;
		}

		// Attack walls only if we are stuck in them
		if (dtype.BoolFlag[WALL_INDEX].value && d > 1) {
			return INT_MAX;
		}

		if (dtype.UnitType == UnitTypeFly && dest->IsAgressive() == false) {
			return INT_MAX / 2;
		}

		// Calculate the costs to attack the unit.
		// Unit with the smallest attack costs will be taken.
		int cost = 0;

		// Priority 0-255
		//Wyrmgus start
//		cost -= dtype.DefaultStat.Variables[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
		cost -= dest->Variable[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
		//Wyrmgus end
		// Remaining HP (Health) 0-65535
		//Wyrmgus start
//		cost += dest->Variable[HP_INDEX].Value * 100 / dest->Variable[HP_INDEX].Max * HEALTH_FACTOR;
		cost += dest->Variable[HP_INDEX].Value * 100 / dest->GetModifiedVariable(HP_INDEX, VariableMax) * HEALTH_FACTOR;
		//Wyrmgus end

		if (d <= attackrange && d >= type.MinAttackRange) {
			cost += d * INRANGE_FACTOR;
			cost -= INRANGE_BONUS;
		} else {
			cost += d * DISTANCE_FACTOR;
		}

		for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
			if (type.BoolFlag[i].AiPriorityTarget != CONDITION_TRUE) {
				if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_ONLY) &
					(dtype.BoolFlag[i].value)) {
					cost -= AIPRIORITY_BONUS;
				}
				if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_FALSE) &
					(dtype.BoolFlag[i].value)) {
					cost += AIPRIORITY_BONUS;
				}
			}
		}

		// Unit can attack back.
		if (CanTarget(dtype, type)) {
			cost -= CANATTACK_BONUS;
		}
		return cost;
	}

private:
	const CUnit *attacker;
	//Wyrmgus start
	const bool include_neutral;
	//Wyrmgus end
};

/**
**  Attack units in distance, with large missile
**
**  Choose the best target, that can be attacked. It takes into
**  account allied unit which could be hit by the missile
**
**
**  @note This could be improved, for better performance / better trade.
**  @note Will be moved to unit_ai.c soon.
*/
class BestRangeTargetFinder
{
public:
	/**
	**  @param a      Find in distance for this unit.
	**  @param range  Distance range to look.
	**
	*/
	//Wyrmgus start
//	BestRangeTargetFinder(const CUnit &a, const int r) : attacker(&a), range(r),
	BestRangeTargetFinder(const CUnit &a, const int r, const bool i_n) : attacker(&a), range(r), include_neutral(i_n),
	//Wyrmgus end
		//Wyrmgus start
//		best_unit(0), best_cost(INT_MIN), size((a.Type->Missile.Missile->Range + r) * 2)
		best_unit(0), best_cost(INT_MIN), size((a.GetMissile().Missile->Range + r) * 2)
		//Wyrmgus end
	{
		good = new std::vector<int>(size * size, 0);
		bad = new std::vector<int>(size * size, 0);
	};
	
	~BestRangeTargetFinder()
	{
		delete good;
		delete bad;
	};

	class FillBadGood
	{
	public:
		//Wyrmgus start
//		FillBadGood(const CUnit &a, int r, std::vector<int> *g, std::vector<int> *b, int s):
		FillBadGood(const CUnit &a, int r, std::vector<int> *g, std::vector<int> *b, int s, bool i_n):
		//Wyrmgus end
			attacker(&a), range(r), size(s),
			//Wyrmgus start
			include_neutral(i_n),
			//Wyrmgus end
			enemy_count(0), good(g), bad(b)
		{
		}

		int Fill(CUnitCache &cache)
		{
			return Fill(cache.begin(), cache.end());
		}

		template <typename Iterator>
		int Fill(Iterator begin, Iterator end)
		{
			for (Iterator it = begin; it != end; ++it) {
				Compute(*it);
			}
			return enemy_count;
		}
	private:

		void Compute(CUnit *const dest)
		{
			const CPlayer &player = *attacker->Player;

			if (!dest->IsVisibleAsGoal(player)) {
				dest->CacheLock = 1;
				return;
			}

			const CUnitType &type =  *attacker->Type;
			const CUnitType &dtype = *dest->Type;
			// won't be a target...
			if (!CanTarget(type, dtype)) { // can't be attacked.
				dest->CacheLock = 1;
				return;
			}
			// Don't attack invulnerable units
			if (dtype.BoolFlag[INDESTRUCTIBLE_INDEX].value || dest->Variable[UNHOLYARMOR_INDEX].Value) {
				dest->CacheLock = 1;
				return;
			}

			//  Calculate the costs to attack the unit.
			//  Unit with the smallest attack costs will be taken.

			int cost = 0;
			int hp_damage_evaluate;
			if (Damage) {
				hp_damage_evaluate = CalculateDamage(*attacker, *dest, Damage);
			} else {
				hp_damage_evaluate = attacker->Stats->Variables[BASICDAMAGE_INDEX].Value
									 + attacker->Stats->Variables[PIERCINGDAMAGE_INDEX].Value;
			}
			//Wyrmgus start
//			if (!player.IsEnemy(*dest)) { // a friend or neutral
			if (
				!attacker->IsEnemy(*dest) // a friend or neutral
				&& (!include_neutral || attacker->IsAllied(*dest) || dest->Player->Type == PlayerNeutral || attacker->Player->Index == dest->Player->Index)
			) {
			//Wyrmgus end
				dest->CacheLock = 1;

				// Calc a negative cost
				// The gost is more important when the unit would be killed
				// by our fire.

				// It costs (is positive) if hp_damage_evaluate>dest->HP ...)
				// FIXME : assume that PRIORITY_FACTOR>HEALTH_FACTOR
				cost = HEALTH_FACTOR * (2 * hp_damage_evaluate -
										dest->Variable[HP_INDEX].Value) /
					   (dtype.TileSize.x * dtype.TileSize.y);
				cost = std::max(cost, 1);
				cost = -cost;
			} else {
				//  Priority 0-255
				//Wyrmgus start
//				cost += dtype.DefaultStat.Variables[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
				cost += dest->Variable[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
				//Wyrmgus end

				for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
					if (type.BoolFlag[i].AiPriorityTarget != CONDITION_TRUE) {
						if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_ONLY) &
							(dtype.BoolFlag[i].value)) {
							cost -= AIPRIORITY_BONUS;
						} else if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_FALSE) &
								   (dtype.BoolFlag[i].value)) {
							cost += AIPRIORITY_BONUS;
						}
					}
				}

				//  Remaining HP (Health) 0-65535
				// Give a boost to unit we can kill in one shoot only

				// calculate HP which will remain in the enemy unit, after hit
				int effective_hp = (dest->Variable[HP_INDEX].Value - 2 * hp_damage_evaluate);

				// Unit we won't kill are evaluated the same
				// Unit we are sure to kill are all evaluated the same (except PRIORITY)
				clamp(&effective_hp, -hp_damage_evaluate, 0);

				// Here, effective_hp vary from -hp_damage_evaluate (unit will be killed) to 0 (unit can't be killed)
				// => we prefer killing rather than only hitting...
				cost += -effective_hp * HEALTH_FACTOR;

				//  Unit can attack back.
				if (CanTarget(dtype, type)) {
					cost += CANATTACK_BONUS;
				}

				// the cost may be divided across multiple cells
				cost = cost / (dtype.TileSize.x * dtype.TileSize.y);
				cost = std::max(cost, 1);

				// Removed Unit's are in bunkers
				int d;
				if (attacker->Removed) {
					d = attacker->Container->MapDistanceTo(*dest);
				} else {
					d = attacker->MapDistanceTo(*dest);
				}

				//Wyrmgus start
//				int attackrange = attacker->Stats->Variables[ATTACKRANGE_INDEX].Max;
				int attackrange = attacker->GetModifiedVariable(ATTACKRANGE_INDEX);
				//Wyrmgus end
				
				//Wyrmgus start
//				if (d <= attackrange ||
//					(d <= range && UnitReachable(*attacker, *dest, attackrange))) {
				if ((d <= attackrange ||
					(d <= range && UnitReachable(*attacker, *dest, attackrange, attacker->GetReactionRange() * 8))) && (!Map.IsLayerUnderground(attacker->MapLayer) || attackrange <= 1 || CheckObstaclesBetweenTiles(attacker->tilePos, dest->tilePos, MapFieldAirUnpassable, attacker->MapLayer))) {
				//Wyrmgus end
					++enemy_count;
				} else {
					dest->CacheLock = 1;
				}
				// Attack walls only if we are stuck in them
				if (dtype.BoolFlag[WALL_INDEX].value && d > 1) {
					dest->CacheLock = 1;
				}
			}

			//Wyrmgus start
//			const int x = dest->tilePos.x - attacker->tilePos.x + (size / 2);
//			const int y = dest->tilePos.y - attacker->tilePos.y + (size / 2);
			const int x = abs(dest->tilePos.x - attacker->tilePos.x) + (size / 2);
			const int y = abs(dest->tilePos.y - attacker->tilePos.y) + (size / 2);
			//Wyrmgus end
			Assert(x >= 0 && y >= 0);

			// Mark the good/bad array...
			for (int yy = 0; yy < dtype.TileSize.y; ++yy) {
				for (int xx = 0; xx < dtype.TileSize.x; ++xx) {
					int pos = (y + yy) * (size / 2) + (x + xx);
					if (pos >= good->size()) {
						printf("BUG: RangeTargetFinder::FillBadGood.Compute out of range. "\
							"size: %d, pos: %d, " \
							"x: %d, xx: %d, y: %d, yy: %d",
							size, pos, x, xx, y, yy);
						break;
					}
					if (cost < 0) {
						good->at(pos) -= cost;
					} else {
						bad->at(pos) += cost;
					}
				}
			}
		}


	private:
		const CUnit *attacker;
		const int range;
		int enemy_count;
		std::vector<int> *good;
		std::vector<int> *bad;
		const int size;
		//Wyrmgus start
		const bool include_neutral;
		//Wyrmgus end
	};

	CUnit *Find(std::vector<CUnit *> &table)
	{
		//Wyrmgus start
//		FillBadGood(*attacker, range, good, bad, size).Fill(table.begin(), table.end());
		FillBadGood(*attacker, range, good, bad, size, include_neutral).Fill(table.begin(), table.end());
		//Wyrmgus end
		return Find(table.begin(), table.end());

	}

	CUnit *Find(CUnitCache &cache)
	{
		//Wyrmgus start
//		FillBadGood(*attacker, range, good, bad, size).Fill(cache);
		FillBadGood(*attacker, range, good, bad, size, include_neutral).Fill(cache);
		//Wyrmgus end
		return Find(cache.begin(), cache.end());
	}

private:
	template <typename Iterator>
	CUnit *Find(Iterator begin, Iterator end)
	{
		for (Iterator it = begin; it != end; ++it) {
			Compute(*it);
		}
		return best_unit;
	}

	void Compute(CUnit *const dest)
	{
		if (dest->CacheLock) {
			dest->CacheLock = 0;
			return;
		}
		const CUnitType &type = *attacker->Type;
		const CUnitType &dtype = *dest->Type;
		//Wyrmgus start
//		const int missile_range = type.Missile.Missile->Range + range - 1;
		const int missile_range = attacker->GetMissile().Missile->Range + range - 1;
		//Wyrmgus end
		int x = attacker->tilePos.x;
		int y = attacker->tilePos.y;

		// put in x-y the real point which will be hit...
		// (only meaningful when dtype->TileWidth > 1)
		clamp<int,int>(&x, dest->tilePos.x, dest->tilePos.x + dtype.TileSize.x - 1);
		clamp<int,int>(&y, dest->tilePos.y, dest->tilePos.y + dtype.TileSize.y - 1);

		int sbad = 0;
		int sgood = 0;
		
		// cost map is relative to attacker position
		//Wyrmgus start
//		x = dest->tilePos.x - attacker->tilePos.x + (size / 2);
//		y = dest->tilePos.y - attacker->tilePos.y + (size / 2);
		x = abs(dest->tilePos.x - attacker->tilePos.x) + (size / 2);
		y = abs(dest->tilePos.y - attacker->tilePos.y) + (size / 2);
		//Wyrmgus end
		Assert(x >= 0 && y >= 0);
		
		// calculate the costs:
		// costs are the full costs at the target and the splash-factor
		// adjusted costs of the tiles immediately around the target
		//Wyrmgus start
//		int splashFactor = type.Missile.Missile->SplashFactor;
		int splashFactor = attacker->GetMissile().Missile->SplashFactor;
		//Wyrmgus end
		for (int yy = -1; yy <= 1; ++yy) {
			for (int xx = -1; xx <= 1; ++xx) {
				int pos = (y + yy) * (size / 2) + (x + xx);
				int localFactor = (!xx && !yy) ? 1 : splashFactor;
				if (pos >= good->size()) {
					printf("BUG: RangeTargetFinder.Compute out of range. " \
						"size: %d, pos: %d, "	\
						"x: %d, xx: %d, y: %d, yy: %d",
						size, pos, x, xx, y, yy);
					break;
				}
				sbad += bad->at(pos) / localFactor;
				sgood += good->at(pos) / localFactor;
			}
		}

		if (sgood > 0 && attacker->Type->BoolFlag[NOFRIENDLYFIRE_INDEX].value) {
			return;
		}
		
		// don't consider small friendly-fire damages...
		sgood = std::max(sgood, 20);
		int cost = sbad / sgood;
		if (cost > best_cost) {
			best_cost = cost;
			best_unit = dest;
		}
	}

private:
	const CUnit *attacker;
	const int range;
	CUnit *best_unit;
	int best_cost;
	std::vector<int> *good;
	std::vector<int> *bad;
	const int size;
	//Wyrmgus start
	const bool include_neutral;
	//Wyrmgus end
};

struct CompareUnitDistance {
	const CUnit *referenceunit;
	CompareUnitDistance(const CUnit &unit): referenceunit(&unit) {}
	bool operator()(const CUnit *c1, const CUnit *c2)
	{
		int d1 = c1->MapDistanceTo(*referenceunit);
		int d2 = c2->MapDistanceTo(*referenceunit);
		if (d1 == d2) {
			return UnitNumber(*c1) < UnitNumber(*c2);
		} else {
			return d1 < d2;
		}
	}
};

/**
**  Check map for obstacles in a line between 2 tiles
**
**  This function uses Bresenham's line algorithm
**
**  @param unit     First tile
**  @param goal     Second tile
**  @param flags    Terrain type to check
**
**  @return         true, if an obstacle was found, false otherwise
*/
//Wyrmgus start
//bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, unsigned short flags, int *distance)
bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, unsigned long flags, int z, int max_difference, int *distance, int player)
//Wyrmgus end
{
	const Vec2i delta(abs(goalPos.x - unitPos.x), abs(goalPos.y - unitPos.y));
	const Vec2i sign(unitPos.x < goalPos.x ? 1 : -1, unitPos.y < goalPos.y ? 1 : -1);
	int error = delta.x - delta.y;
	Vec2i pos(unitPos), oldPos(unitPos);

	while (pos.x != goalPos.x || pos.y != goalPos.y) {
		const int error2 = error * 2;

		if (error2 > -delta.y) {
			error -= delta.y;
			pos.x += sign.x;
		}
		if (error2 < delta.x) {
			error += delta.x;
			pos.y += sign.y;
		}

		//Wyrmgus start
//		if (Map.Info.IsPointOnMap(pos) == false) {
		if (Map.Info.IsPointOnMap(pos, z) == false) {
		//Wyrmgus end
			DebugPrint("outside of map\n");
		//Wyrmgus start
//		} else if (Map.Field(pos)->Flags & flags) {
		} else if (
			((Map.Field(pos, z)->Flags & flags) || (player != -1 && Map.Field(pos, z)->Owner != player && Map.Field(pos, z)->Owner != -1))
			&& pos != goalPos
			&& (abs(pos.x - goalPos.x) > max_difference || abs(pos.y - goalPos.y) > max_difference)
		) { // the goal's tile itself shouldn't be checked for an obstacle
		//Wyrmgus end
			if (distance) {
				//Wyrmgus start
//				*distance = Distance(unitPos, pos);
				*distance = std::min(*distance, Distance(unitPos, pos));
				//Wyrmgus end
			}
			return false;
		}
		oldPos = pos;
	}
	return true;
}

/**
**  Attack units in distance.
**
**  If the unit can attack must be handled by caller.
**  Choose the best target, that can be attacked.
**
**  @param unit           Find in distance for this unit.
**  @param range          Distance range to look.
**  @param onlyBuildings  Search only buildings (useful when attacking with AI force)
**
**  @return       Unit to be attacked.
*/
//Wyrmgus start
//CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred)
CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred, bool circle, bool include_neutral)
//Wyrmgus end
{
	// if necessary, take possible damage on allied units into account...
	//Wyrmgus start
//	if (unit.Type->Missile.Missile->Range > 1
//		&& (range + unit.Type->Missile.Missile->Range < 15)) {
	if (unit.GetMissile().Missile->Range > 1
		&& (range + unit.GetMissile().Missile->Range < 15)) {
	//Wyrmgus end
		//  If catapult, count units near the target...
		//   FIXME : make it configurable

		//Wyrmgus start
//		int missile_range = unit.Type->Missile.Missile->Range + range - 1;
		int missile_range = unit.GetMissile().Missile->Range + range - 1;
		//Wyrmgus end

		Assert(2 * missile_range + 1 < 32);

		// If unit is removed, use containers x and y
		const CUnit *firstContainer = unit.Container ? unit.Container : &unit;
		std::vector<CUnit *> table;
		SelectAroundUnit(*firstContainer, missile_range, table,
			//Wyrmgus start
//			MakeAndPredicate(HasNotSamePlayerAs(Players[PlayerNumNeutral]), pred));
			pred, circle);
			//Wyrmgus end

		if (table.empty() == false) {
			//Wyrmgus start
//			return BestRangeTargetFinder(unit, range).Find(table);
			return BestRangeTargetFinder(unit, range, include_neutral).Find(table);
			//Wyrmgus end
		}
		return NULL;
	} else {
		// If unit is removed, use containers x and y
		const CUnit *firstContainer = unit.Container ? unit.Container : &unit;
		std::vector<CUnit *> table;

		SelectAroundUnit(*firstContainer, range, table,
			//Wyrmgus start
//			MakeAndPredicate(HasNotSamePlayerAs(Players[PlayerNumNeutral]), pred));
			pred, circle);
			//Wyrmgus end

		const int n = static_cast<int>(table.size());
		if (range > 25 && table.size() > 9) {
			std::sort(table.begin(), table.begin() + n, CompareUnitDistance(unit));
		}

		// Find the best unit to attack
		//Wyrmgus start
//		return BestTargetFinder(unit).Find(table);
		return BestTargetFinder(unit, include_neutral).Find(table);
		//Wyrmgus end
	}
}

//Wyrmgus start
//CUnit *AttackUnitsInDistance(const CUnit &unit, int range)
CUnit *AttackUnitsInDistance(const CUnit &unit, int range, bool circle, bool include_neutral)
//Wyrmgus end
{
	//Wyrmgus start
//	return AttackUnitsInDistance(unit, range, NoFilter());
	return AttackUnitsInDistance(unit, range, NoFilter(), circle, include_neutral);
	//Wyrmgus end
}

/**
**  Attack units in attack range.
**
**  @param unit  Find unit in attack range for this unit.
**
**  @return      Pointer to unit which should be attacked.
*/
CUnit *AttackUnitsInRange(const CUnit &unit, CUnitFilter pred)
{
	//Wyrmgus start
//	Assert(unit.Type->CanAttack);
	Assert(unit.CanAttack());
//	return AttackUnitsInDistance(unit, unit.Stats->Variables[ATTACKRANGE_INDEX].Max, pred);
	return AttackUnitsInDistance(unit, unit.GetModifiedVariable(ATTACKRANGE_INDEX), pred);
	//Wyrmgus end
}

CUnit *AttackUnitsInRange(const CUnit &unit)
{
	return AttackUnitsInRange(unit, NoFilter());
}

/**
**  Attack units in reaction range.
**
**  @param unit  Find unit in reaction range for this unit.
**
**  @return      Pointer to unit which should be attacked.
*/
//Wyrmgus start
//CUnit *AttackUnitsInReactRange(const CUnit &unit, CUnitFilter pred)
CUnit *AttackUnitsInReactRange(const CUnit &unit, CUnitFilter pred, bool include_neutral)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(unit.Type->CanAttack);
	Assert(unit.CanAttack());
//	const int range = unit.Player->Type == PlayerPerson ? unit.Type->ReactRangePerson : unit.Type->ReactRangeComputer;
//	return AttackUnitsInDistance(unit, range, pred);
	const int range = unit.GetReactionRange();
	return AttackUnitsInDistance(unit, range, pred, true, include_neutral);
	//Wyrmgus end
}

//Wyrmgus start
//CUnit *AttackUnitsInReactRange(const CUnit &unit)
CUnit *AttackUnitsInReactRange(const CUnit &unit, bool include_neutral)
//Wyrmgus end
{
	//Wyrmgus start
//	return AttackUnitsInReactRange(unit, NoFilter());
	return AttackUnitsInReactRange(unit, NoFilter(), include_neutral);
	//Wyrmgus end
}

//@}
