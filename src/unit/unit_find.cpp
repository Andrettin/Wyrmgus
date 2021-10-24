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

#include "unit/unit_find.h"

#include "actions.h"
#include "economy/resource.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "pathfinder.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_domain_finder.h"
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"

/*----------------------------------------------------------------------------
  -- Finding units
  ----------------------------------------------------------------------------*/

bool IsBuiltUnit::operator()(const CUnit *unit) const
{
	return unit->CurrentAction() != UnitAction::Built;
}

CUnit *UnitFinder::FindUnitAtPos(const Vec2i &pos) const
{
	CUnitCache &cache = CMap::get()->Field(pos, z)->UnitCache;

	for (CUnitCache::iterator it = cache.begin(); it != cache.end(); ++it) {
		CUnit *unit = *it;

		if (std::find(units.begin(), units.end(), unit) != units.end()) {
			return unit;
		}
	}
	return nullptr;
}

VisitResult UnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(from)

	if (!CMap::get()->Field(pos, z)->player_info->IsTeamExplored(player)) {
		return VisitResult::DeadEnd;
	}
	// Look if found what was required.
	CUnit *unit = FindUnitAtPos(pos);
	if (unit) {
		*unitP = unit;
		return VisitResult::Finished;
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult::Ok;
		} else {
			return VisitResult::DeadEnd;
		}
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

class TerrainFinder final
{
public:
	//Wyrmgus start
//	TerrainFinder(const CPlayer &player, int maxDist, int movemask, int resmask, Vec2i *resPos) :
//		player(player), maxDist(maxDist), movemask(movemask), resmask(resmask), resPos(resPos) {}
	explicit TerrainFinder(const CPlayer &player, const int maxDist, const tile_flag movemask, const wyrmgus::resource *resource, Vec2i *resPos, const int z, const landmass *landmass) :
		player(player), maxDist(maxDist), movemask(movemask), resource(resource), resPos(resPos), z(z), landmass(landmass)
	{
	}
	//Wyrmgus end
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CPlayer &player;
	int maxDist;
	tile_flag movemask;
	//Wyrmgus start
//	int resmask;
	const wyrmgus::resource *resource = nullptr;
	int z;
	const wyrmgus::landmass *landmass = nullptr;
	//Wyrmgus end
	Vec2i *resPos;
};

VisitResult TerrainFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(from)

	if (!CMap::get()->Field(pos, z)->player_info->IsTeamExplored(player)) {
		return VisitResult::DeadEnd;
	}
	
	//Wyrmgus start
	if (CMap::get()->Field(pos, z)->get_owner() != nullptr && CMap::get()->Field(pos, z)->get_owner() != &player && !CMap::get()->Field(pos, z)->get_owner()->has_neutral_faction_type() && !player.has_neutral_faction_type()) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	
	// Look if found what was required.
	//Wyrmgus start
//	if (Map.Field(pos)->CheckMask(resmask)) {
	if ((!this->landmass || CMap::get()->get_tile_landmass(pos, z) == this->landmass) && (this->resource == nullptr || CMap::get()->Field(pos, z)->get_resource() == resource)) {
	//Wyrmgus end
		if (resPos) {
			*resPos = pos;
		}
		return VisitResult::Finished;
	}

	if (CanMoveToMask(pos, movemask, z)) { // reachable
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult::Ok;
		} else {
			return VisitResult::DeadEnd;
		}
	} else { // unreachable
		return VisitResult::DeadEnd;
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
bool FindTerrainType(const tile_flag movemask, const wyrmgus::resource *resource, int range,
					 //Wyrmgus start
//					 const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos)
					 const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos, int z, const landmass *landmass)
					 //Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	terrainTraversal.Init();

	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	TerrainFinder terrainFinder(player, range, movemask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit), resmask, terrainPos);
	TerrainFinder terrainFinder(player, range, movemask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit), resource, terrainPos, z, landmass);
	//Wyrmgus end

	return terrainTraversal.Run(terrainFinder);
}


template <const bool NEARLOCATION>
class BestDepotFinder final
{
	void operator()(CUnit *const dest)
	{
		/* Only resource depots */
		if (dest->Type->can_store(resource)
			//Wyrmgus start
			&& (NEARLOCATION || u_near.worker->can_return_goods_to(dest, resource))
			//Wyrmgus end
			&& dest->IsAliveOnMap()
			&& dest->CurrentAction() != UnitAction::Built) {
			// Unit in range?

			if constexpr (NEARLOCATION) {
				const int d = dest->MapDistanceTo(u_near.loc, u_near.layer);

				//
				// Take this depot?
				//
				if (d <= this->range && d < this->best_dist) {
					this->best_depot = dest;
					this->best_dist = d;
				}
			} else {
				const CUnit *worker = this->u_near.worker;
				const CUnit *first_container = worker->GetFirstContainer();

				//simple distance
				const int distance = first_container->MapDistanceTo(*dest);

				// Use Circle, not square :)
				if (distance > this->range) {
					return;
				}

				if (this->best_dist == INT_MAX) {
					this->best_depot = dest;
				}

				if (distance >= this->best_dist) {
					//if the depot's simple distance is greater or equal to the real travel distance of the currently-chosen depot, then it can never be closer than it, and we have no reason to actually calculate its real travel distance 
					return;
				}
				
				//calculate real travel distance
				const int travel_distance = UnitReachable(*worker, *dest, 1, 0, true);

				//
				// Take this depot?
				//
				if (travel_distance && travel_distance < this->best_dist) {
					this->best_depot = dest;
					this->best_dist = travel_distance;
				}
			}
		}
	}

public:
	explicit BestDepotFinder(const CUnit &w, const resource *res, const int ran)
		: resource(res), range(ran)
	{
		u_near.worker = &w;
	}

	//Wyrmgus start
//	explicit BestDepotFinder(const Vec2i &pos, const resource *res, const int ran)
	explicit BestDepotFinder(const Vec2i &pos, const resource *res, const int ran, const int z)
	//Wyrmgus end
		: resource(res), range(ran)
	{
		u_near.loc = pos;
		u_near.layer = z;
	}

	CUnit *find(std::vector<CUnit *> &units)
	{
		if constexpr (!NEARLOCATION) {
			//sort by distance as a performance improvement, so that we don't call UnitReachable() unnecessarily
			const CUnit *first_container = this->u_near.worker->GetFirstContainer();

			std::sort(units.begin(), units.end(), [first_container](const CUnit *lhs, const CUnit *rhs) {
				return first_container->MapDistanceTo(*lhs) < first_container->MapDistanceTo(*rhs);
			});
		}

		for (CUnit *unit : units) {
			this->operator()(unit);
		}

		return this->best_depot;
	}

	CUnit *Find(CUnitCache &cache)
	{
		cache.for_each(*this);
		return this->best_depot;
	}

private:
	struct {
		const CUnit *worker;
		Vec2i loc;
		int layer;
	} u_near;

	const wyrmgus::resource *resource = nullptr;
	const int range;
	int best_dist = INT_MAX;
public:
	CUnit *best_depot = nullptr;
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
			&& dest->CurrentAction() != UnitAction::Built) {
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

CUnit *FindDepositNearLoc(CPlayer &p, const Vec2i &pos, const int range, const resource *resource, const int z)
{
	BestDepotFinder<true> finder(pos, resource, range, z);

	std::vector<CUnit *> table;

	for (const auto &kv_pair : p.get_units_by_type()) {
		const unit_type *unit_type = kv_pair.first;
		if (unit_type->can_store(resource)) {
			vector::merge(table, kv_pair.second);
		}
	}

	for (int i = 0; i < PlayerMax - 1; ++i) {
		const CPlayer *other_player = CPlayer::Players[i].get();
		if (other_player->is_allied_with(p) && p.is_allied_with(*other_player)) {
			for (const auto &kv_pair : other_player->get_units_by_type()) {
				const wyrmgus::unit_type *unit_type = kv_pair.first;
				if (unit_type->can_store(resource)) {
					vector::merge(table, kv_pair.second);
				}
			}
		}
	}

	return finder.find(table);
}

class CResourceFinder final
{
public:
	//Wyrmgus start
//	CResourceFinder(int r, bool on_top) : resource(r), mine_on_top(on_top) {}
	explicit CResourceFinder(const resource *r, const bool harvestable, const bool luxury, const bool same)
		: resource(r), only_harvestable(harvestable), include_luxury(luxury), only_same(same)
	{
	}
	//Wyrmgus end

	bool operator()(const CUnit *const unit) const
	{
		const wyrmgus::unit_type &type = *unit->Type;
		//Wyrmgus start
//		return (type.GivesResource == resource
		return ((unit->get_given_resource() == resource || (!only_same && unit->GivesResource != TradeCost && wyrmgus::resource::get_all()[unit->GivesResource]->get_final_resource() == resource) || (include_luxury && wyrmgus::resource::get_all()[unit->GivesResource]->LuxuryResource))
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
	const wyrmgus::resource *resource = nullptr;
	//Wyrmgus start
//	const bool mine_on_top;
	const bool only_harvestable;
	const bool include_luxury;
	const bool only_same;
	//Wyrmgus end
};

class ResourceUnitFinder final
{
public:
	//Wyrmgus start
//	explicit ResourceUnitFinder(const CUnit &worker, const CUnit *deposit, const resource *resource, int maxRange, bool check_usage, CUnit **resultMine) :
	explicit ResourceUnitFinder(const CUnit &worker, const CUnit *deposit, const resource *resource, int maxRange, bool check_usage, CUnit **resultMine, bool only_harvestable, bool ignore_exploration, bool only_unsettled_area, bool include_luxury, bool only_same) :
	//Wyrmgus end
		worker(worker),
		res_info(worker.Type->get_resource_info(resource)),
		deposit(deposit),
		movemask(worker.Type->MovementMask),
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
		*resultMine = nullptr;
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
		unsigned int assigned = 0;
		unsigned int waiting = 0;
		unsigned int distance = 0;
	};

private:
	const CUnit &worker;
	const resource_info *res_info = nullptr;
	const CUnit *deposit;
	tile_flag movemask;
	int maxRange;
	bool check_usage;
	//Wyrmgus start
	bool only_harvestable;
	bool ignore_exploration;
	bool only_unsettled_area;
	bool include_luxury;
	bool only_same;
	bool check_reachable;
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
		SelectAroundUnit(mine, 8, table, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
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
//			   || mine.Player->get_index() == PlayerMax - 1
//			   || mine.Player == worker.Player
//			   || (worker.is_allied_with(mine) && mine.is_allied_with(worker)));
		   && worker.can_harvest(&mine, only_harvestable);
			//Wyrmgus end
}

//Wyrmgus start
//void ResourceUnitFinder::ResourceUnitFinder_Cost::SetFrom(const CUnit &mine, const CUnit *deposit, bool check_usage)
void ResourceUnitFinder::ResourceUnitFinder_Cost::SetFrom(const CUnit &mine, const CUnit *deposit, const CUnit &worker, bool check_usage)
//Wyrmgus end
{
	const wyrmgus::resource *resource = wyrmgus::resource::get_all()[mine.GivesResource];

	distance = deposit ? mine.MapDistanceTo(*deposit) : 0;
	//Wyrmgus start
	distance = distance * 100 / resource->get_final_resource_conversion_rate();
	
	//alter the distance score by the conversion rate, so that the unit will prefer resources with better conversion rates, but without going for ones that are too far away
	int price_modifier = worker.Player->get_resource_price(resource->get_final_resource()) * resource->get_final_resource_conversion_rate() / 100;
	if (resource->get_input_resource() != nullptr) {
		price_modifier -= worker.Player->get_resource_price(resource->get_input_resource());
	}
	price_modifier = std::max(price_modifier, 1);
	distance = distance * 100 / price_modifier;

	if (!mine.Type->BoolFlag[CANHARVEST_INDEX].value) { // if it is a deposit rather than a readily-harvestable resource, multiply the distance score
		distance *= 8;
	}
	//Wyrmgus end

	if (check_usage) {
		assigned = static_cast<int>(mine.Resource.Workers.size()) - mine.Type->MaxOnBoard;
		waiting = GetNumWaitingWorkers(mine);
	} else {
		assigned = 0;
		waiting = 0;
	}
}

VisitResult ResourceUnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(from)

	const wyrmgus::tile *tile = worker.MapLayer->Field(pos);

	//Wyrmgus start
//	if (!worker.Player->AiEnabled && !tile->player_info->is_explored(*worker.Player)) {
	if (!tile->player_info->IsTeamExplored(*worker.Player) && !ignore_exploration) {
	//Wyrmgus end
		return VisitResult::DeadEnd;
	}

	CUnit *mine = tile->UnitCache.find(res_finder);

	//Wyrmgus start
	const CPlayer *tile_owner = tile->get_owner();
	
	if (tile_owner != nullptr && tile_owner != worker.Player && !tile_owner->has_neutral_faction_type() && !worker.Player->has_neutral_faction_type() && (mine == nullptr || mine->Type->get_given_resource() == nullptr || mine->Type->get_given_resource()->get_index() != TradeCost)) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end

	//Wyrmgus start
//	if (mine && mine != *resultMine && MineIsUsable(*mine)) {
	if (
		mine && mine != *resultMine && MineIsUsable(*mine)
		&& (mine->Type->BoolFlag[CANHARVEST_INDEX].value || tile_owner == nullptr || tile_owner == worker.Player) //this is needed to prevent neutral factions from trying to build mines in others' territory
	) {
	//Wyrmgus end
		ResourceUnitFinder::ResourceUnitFinder_Cost cost;

		//Wyrmgus start
//		cost.SetFrom(*mine, deposit, check_usage);
		cost.SetFrom(*mine, deposit, worker, check_usage);
		
		if (cost < bestCost) {
			*resultMine = mine;

			if (cost.IsMin()) {
				return VisitResult::Finished;
			}
			bestCost = cost;
		}
	}

	if (CanMoveToMask(pos, movemask, worker.MapLayer->ID)) { // reachable
		if (terrainTraversal.Get(pos) < maxRange) {
			return VisitResult::Ok;
		} else {
			return VisitResult::DeadEnd;
		}
	} else { // unreachable
		return VisitResult::DeadEnd;
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
**  @return            null or resource unit
*/
CUnit *UnitFindResource(const CUnit &unit, const CUnit &start_unit, const int range, const resource *resource,
						//Wyrmgus start
//						bool check_usage, const CUnit *depot)
						const bool check_usage, const CUnit *depot, const bool only_harvestable, const bool ignore_exploration, const bool only_unsettled_area, const bool include_luxury, const bool only_same)
						//Wyrmgus end
{
	if (!depot) { // Find the nearest depot
		depot = FindDepositNearLoc(*unit.Player, start_unit.tilePos, range, resource, start_unit.MapLayer->ID);
	}

	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(Map.Info.MapWidth, Map.Info.MapHeight);
	terrainTraversal.SetSize(start_unit.MapLayer->get_width(), start_unit.MapLayer->get_height());
	//Wyrmgus end
	terrainTraversal.Init();

	if (&unit != &start_unit || start_unit.Container != nullptr) {
		terrainTraversal.push_unit_pos_and_neighbor_if_passable(start_unit, unit.Type->MovementMask);
	} else {
		terrainTraversal.PushUnitPosAndNeighbor(start_unit);
	}

	CUnit *resultMine = nullptr;

	//Wyrmgus start
//	ResourceUnitFinder resourceUnitFinder(unit, depot, resource, range, check_usage, &resultMine);
	ResourceUnitFinder resourceUnitFinder(unit, depot, resource, range, check_usage, &resultMine, only_harvestable, ignore_exploration, only_unsettled_area, include_luxury, only_same);
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
**  @return            null or deposit unit
*/
CUnit *FindDeposit(const CUnit &unit, const int range, const resource *resource)
{
	BestDepotFinder<false> finder(unit, resource, range);

	std::vector<CUnit *> table;

	for (const auto &kv_pair : unit.Player->get_units_by_type()) {
		const wyrmgus::unit_type *unit_type = kv_pair.first;
		if (unit_type->can_store(resource)) {
			vector::merge(table, kv_pair.second);
		}
	}

	for (int i = 0; i < PlayerMax - 1; ++i) {
		const CPlayer *other_player = CPlayer::Players[i].get();

		if (unit.Player->is_allied_with(*other_player)) {
			for (const auto &kv_pair : other_player->get_units_by_type()) {
				const wyrmgus::unit_type *unit_type = kv_pair.first;
				if (unit_type->can_store(resource)) {
					vector::merge(table, kv_pair.second);
				}
			}
		}
	}

	return finder.find(table);
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
**  @return            null or market unit
*/
CUnit *FindHomeMarket(const CUnit &unit, int range)
{
	BestHomeMarketFinder<false> finder(unit, range);
	std::vector<CUnit *> table;
	for (std::vector<CUnit *>::const_iterator it = unit.Player->UnitBegin(); it != unit.Player->UnitEnd(); ++it) {
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
**  @return null or next idle worker
*/
CUnit *FindIdleWorker(const CPlayer &player, const CUnit *last)
{
	CUnit *FirstUnitFound = nullptr;
	int SelectNextUnit = (last == nullptr) ? 1 : 0;
	const int nunits = player.GetUnitCount();

	for (int i = 0; i < nunits; ++i) {
		CUnit &unit = player.GetUnit(i);
		if (unit.Type->BoolFlag[HARVESTER_INDEX].value && !unit.Removed) {
			if (unit.CurrentAction() == UnitAction::Still) {
				if (SelectNextUnit && !IsOnlySelected(unit)) {
					return &unit;
				}
				if (FirstUnitFound == nullptr) {
					FirstUnitFound = &unit;
				}
			}
		}
		if (&unit == last) {
			SelectNextUnit = 1;
		}
	}
	if (FirstUnitFound != nullptr && !IsOnlySelected(*FirstUnitFound)) {
		return FirstUnitFound;
	}
	return nullptr;
}

/**
**  Find all units of type.
**
**  @param type       type of unit requested
**  @param units      array in which we have to store the units
**  @param everybody  if true, include all units
*/
void FindUnitsByType(const wyrmgus::unit_type &type, std::vector<CUnit *> &units, const bool everybody)
{
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		//Wyrmgus start
//		if (unit->Type == &type && !unit->IsUnusable(everybody)) {
		if (unit->Type == &type && (!unit->IsUnusable(everybody) || (everybody && unit->IsAlive()))) {
		//Wyrmgus end
			units.push_back(unit);
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
void FindPlayerUnitsByType(const CPlayer &player, const wyrmgus::unit_type &type, std::vector<CUnit *> &table, const bool ai_active_only)
{
	const std::vector<CUnit *> *type_units = nullptr;

	if (ai_active_only) {
		auto find_iterator = player.AiActiveUnitsByType.find(&type);
		if (find_iterator != player.AiActiveUnitsByType.end()) {
			type_units = &find_iterator->second;
		}
	} else {
		auto find_iterator = player.get_units_by_type().find(&type);
		if (find_iterator != player.get_units_by_type().end()) {
			type_units = &find_iterator->second;
		}
	}

	if (type_units == nullptr) {
		return;
	}
	
	for (CUnit *unit : *type_units) {
		if (!unit->IsUnusable()) {
			table.push_back(unit);
		}
	}
}

/**
**  Unit on map tile.
**
**  @param index flat index position on map, tile-based.
**  @param type  unit_domain, (unsigned)-1 for any type.
**
**  @return      Returns first found unit on tile.
*/
static CUnit *UnitOnMapTile(const unsigned int index, const unit_domain domain, const int z)
{
	return CMap::get()->Field(index, z)->UnitCache.find(unit_domain_finder(domain));
}

/**
**  Unit on map tile.
**
**  @param pos   position on map, tile-based.
**  @param type  unit_domain, (unsigned)-1 for any type.
**
**  @return      Returns first found unit on tile.
*/
CUnit *UnitOnMapTile(const Vec2i &pos, const unit_domain domain, const int z)
{
	return UnitOnMapTile(CMap::get()->get_pos_index(pos, z), domain, z);
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
CUnit *TargetOnMap(const CUnit &source, const Vec2i &pos1, const Vec2i &pos2, int z)
{
	std::vector<CUnit *> table;

	Select(pos1, pos2, table, z);

	CUnit *best = nullptr;
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
**  @return          Returns the deposit if found, or null.
*/
//Wyrmgus start
//CUnit *ResourceOnMap(const Vec2i &pos, const resource *resource, const bool mine_on_top)
CUnit *ResourceOnMap(const Vec2i &pos, const resource *resource, const int z, const bool only_harvestable, const bool only_same)
//Wyrmgus end
{
	//Wyrmgus start
//	return CMap::get()->Field(pos)->UnitCache.find(CResourceFinder(resource, mine_on_top));
	return CMap::get()->Field(pos, z)->UnitCache.find(CResourceFinder(resource, only_harvestable, false, only_same));
	//Wyrmgus end
}

class IsADepositForResource
{
public:
	explicit IsADepositForResource(const resource *r) : resource(r) 
	{
	}

	bool operator()(const CUnit *const unit) const
	{
		return (unit->Type->can_store(this->resource) && !unit->IsUnusable());
	}
private:
	const wyrmgus::resource *resource = nullptr;
};

/**
**  Resource deposit on map tile
**
**  @param pos       position on map, tile-based.
**  @param resource  resource type.
**
**  @return          Returns the deposit if found, or null.
*/
//Wyrmgus start
//CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource)
CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return CMap::get()->Field(pos)->UnitCache.find(IsADepositForResource(resource));
	return CMap::get()->Field(pos, z)->UnitCache.find(IsADepositForResource(resource::get_all()[resource]));
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
	explicit BestTargetFinder(const CUnit &a, bool i_n) :
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
		CUnit *enemy = nullptr;
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
		const wyrmgus::unit_type &type = *attacker->Type;
		const wyrmgus::unit_type &dtype = *dest->Type;
		int attackrange = attacker->get_best_attack_range();

		//Wyrmgus start
//		if (!player.is_enemy_of(*dest) // a friend or neutral
		if (
			(
				!attacker->is_enemy_of(*dest) // a friend or neutral
				&& (!include_neutral || attacker->is_allied_with(*dest) || dest->Player->get_type() == player_type::neutral || attacker->Player == dest->Player)
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
		if (attackrange > 1 && !CheckObstaclesBetweenTiles(attacker->tilePos, dest->tilePos, tile_flag::air_impassable, attacker->MapLayer->ID)) {
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

		if (dtype.get_domain() == unit_domain::air && dest->IsAgressive() == false) {
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
		cost += dest->Variable[HP_INDEX].Value * 100 / dest->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) * HEALTH_FACTOR;
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
	explicit BestRangeTargetFinder(const CUnit &a, const int r, const bool i_n) : attacker(&a), range(r), include_neutral(i_n),
	//Wyrmgus end
		size((a.GetMissile().Missile->get_range() + r) * 2)
	{
		this->good = std::vector<int>(size * size, 0);
		this->bad = std::vector<int>(size * size, 0);
	}
	
	class FillBadGood
	{
	public:
		//Wyrmgus start
//		FillBadGood(const CUnit &a, int r, std::vector<int> *g, std::vector<int> *b, int s):
		explicit FillBadGood(const CUnit &a, int r, std::vector<int> &g, std::vector<int> &b, const int s, const bool i_n):
		//Wyrmgus end
			attacker(&a), range(r), size(s),
			//Wyrmgus start
			include_neutral(i_n),
			//Wyrmgus end
			good(g), bad(b)
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

			const wyrmgus::unit_type &type =  *attacker->Type;
			const wyrmgus::unit_type &dtype = *dest->Type;
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
				hp_damage_evaluate = CalculateDamage(*attacker, *dest, Damage.get());
			} else {
				hp_damage_evaluate = attacker->Stats->Variables[BASICDAMAGE_INDEX].Value
									 + attacker->Stats->Variables[PIERCINGDAMAGE_INDEX].Value;
			}
			//Wyrmgus start
//			if (!player.is_enemy_of(*dest)) { // a friend or neutral
			if (
				!attacker->is_enemy_of(*dest) // a friend or neutral
				&& (!include_neutral || attacker->is_allied_with(*dest) || dest->Player->get_type() == player_type::neutral || attacker->Player == dest->Player)
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
					   (dtype.get_tile_width() * dtype.get_tile_width());
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
				effective_hp = std::clamp(effective_hp, -hp_damage_evaluate, 0);

				// Here, effective_hp vary from -hp_damage_evaluate (unit will be killed) to 0 (unit can't be killed)
				// => we prefer killing rather than only hitting...
				cost += -effective_hp * HEALTH_FACTOR;

				//  Unit can attack back.
				if (CanTarget(dtype, type)) {
					cost += CANATTACK_BONUS;
				}

				// the cost may be divided across multiple cells
				cost = cost / (dtype.get_tile_width() * dtype.get_tile_width());
				cost = std::max(cost, 1);

				// Removed Unit's are in bunkers
				int d;
				if (attacker->Removed) {
					d = attacker->Container->MapDistanceTo(*dest);
				} else {
					d = attacker->MapDistanceTo(*dest);
				}

				int attackrange = attacker->get_best_attack_range();
				
				//Wyrmgus start
//				if (d <= attackrange ||
//					(d <= range && UnitReachable(*attacker, *dest, attackrange))) {
				if ((d <= attackrange ||
					(d <= range && UnitReachable(*attacker, *dest, attackrange, attacker->GetReactionRange() * 8))) && (attackrange <= 1 || CheckObstaclesBetweenTiles(attacker->tilePos, dest->tilePos, tile_flag::air_impassable, attacker->MapLayer->ID))) {
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
			assert_throw(x >= 0 && y >= 0);

			// Mark the good/bad array...
			for (int yy = 0; yy < dtype.get_tile_height(); ++yy) {
				for (int xx = 0; xx < dtype.get_tile_width(); ++xx) {
					int pos = (y + yy) * (size / 2) + (x + xx);
					if (pos >= (int) good.size()) {
						wyrmgus::log::log_error("Error: RangeTargetFinder::FillBadGood. Compute out of range. Size: " + std::to_string(size) + ", pos: " + std::to_string(pos) + ", x: " + std::to_string(x) + ", xx: " + std::to_string(xx) + ", y: " + std::to_string(y) + ", yy: " + std::to_string(yy) + ".");
						break;
					}
					if (cost < 0) {
						good.at(pos) -= cost;
					} else {
						bad.at(pos) += cost;
					}
				}
			}
		}


	private:
		const CUnit *attacker;
		const int range;
		int enemy_count = 0;
		std::vector<int> &good;
		std::vector<int> &bad;
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
		const wyrmgus::unit_type &type = *attacker->Type;
		const wyrmgus::unit_type &dtype = *dest->Type;
		const int missile_range = attacker->GetMissile().Missile->get_range() + range - 1;
		int x = attacker->tilePos.x;
		int y = attacker->tilePos.y;

		// put in x-y the real point which will be hit...
		// (only meaningful when dtype->get_tile_width() > 1)
		x = std::clamp<int>(x, dest->tilePos.x, dest->tilePos.x + dtype.get_tile_width() - 1);
		y = std::clamp<int>(y, dest->tilePos.y, dest->tilePos.y + dtype.get_tile_height() - 1);

		int sbad = 0;
		int sgood = 0;
		
		// cost map is relative to attacker position
		//Wyrmgus start
//		x = dest->tilePos.x - attacker->tilePos.x + (size / 2);
//		y = dest->tilePos.y - attacker->tilePos.y + (size / 2);
		x = abs(dest->tilePos.x - attacker->tilePos.x) + (size / 2);
		y = abs(dest->tilePos.y - attacker->tilePos.y) + (size / 2);
		//Wyrmgus end
		assert_throw(x >= 0 && y >= 0);
		
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
				if (pos >= (int) good.size()) {
					wyrmgus::log::log_error("Error: RangeTargetFinder. Compute out of range. Size: " + std::to_string(size) + ", pos: " + std::to_string(pos) + ", x: " + std::to_string(x) + ", xx: " + std::to_string(xx) + ", y: " + std::to_string(y) + ", yy: " + std::to_string(yy) + ".");
					break;
				}
				sbad += bad.at(pos) / localFactor;
				sgood += good.at(pos) / localFactor;
			}
		}

		if (sgood > 0 && type.BoolFlag[NOFRIENDLYFIRE_INDEX].value) {
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
	const CUnit *attacker = nullptr;
	const int range = 0;
	CUnit *best_unit = nullptr;
	int best_cost = INT_MIN;
	std::vector<int> good;
	std::vector<int> bad;
	const int size = 0;
	//Wyrmgus start
	const bool include_neutral = false;
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
**  @return         False if an obstacle was found, or true otherwise
*/
//Wyrmgus start
//bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, const tile_flag flags, int *distance)
bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, const tile_flag flags, const int z, int max_difference, int *distance)
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

		if (CMap::get()->Info->IsPointOnMap(pos, z) == false) {
			DebugPrint("outside of map\n");
		//Wyrmgus start
//		} else if (Map.Field(pos)->Flags & flags) {
		} else if (
			(CMap::get()->Field(pos, z)->get_flags() & flags) != tile_flag::none
			&& pos != goalPos
			&& (abs(pos.x - goalPos.x) > max_difference || abs(pos.y - goalPos.y) > max_difference)
		) { // the goal's tile itself shouldn't be checked for an obstacle
		//Wyrmgus end
			if (distance) {
				//Wyrmgus start
//				*distance = Distance(unitPos, pos);
				*distance = std::min(*distance, point::distance_to(unitPos, pos));
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
template <bool circle>
//CUnit *AttackUnitsInDistance(const CUnit &unit, const int range, CUnitFilter pred)
CUnit *AttackUnitsInDistance(const CUnit &unit, const int range, CUnitFilter pred, const bool include_neutral)
//Wyrmgus end
{
	// if necessary, take possible damage on allied units into account...
	if (unit.GetMissile().Missile->get_range() > 1
		&& (range + unit.GetMissile().Missile->get_range() < 15)) {
		//  If catapult, count units near the target...
		//   FIXME : make it configurable

		const int missile_range = unit.GetMissile().Missile->get_range() + range - 1;

		assert_throw(2 * missile_range + 1 < 32);

		// If unit is removed, use containers x and y
		const CUnit *firstContainer = unit.GetFirstContainer();
		std::vector<CUnit *> table;
		SelectAroundUnit<circle>(*firstContainer, missile_range, table,
			//Wyrmgus start
//			MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::get_neutral_player()), pred));
			pred);
			//Wyrmgus end

		if (table.empty() == false) {
			//Wyrmgus start
//			return BestRangeTargetFinder(unit, range).Find(table);
			return BestRangeTargetFinder(unit, range, include_neutral).Find(table);
			//Wyrmgus end
		}
		return nullptr;
	} else {
		// If unit is removed, use containers x and y
		const CUnit *firstContainer = unit.GetFirstContainer();
		std::vector<CUnit *> table;

		SelectAroundUnit<circle>(*firstContainer, range, table,
			//Wyrmgus start
//			MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::get_neutral_player()), pred));
			pred);
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

template CUnit *AttackUnitsInDistance<false>(const CUnit &unit, int range, CUnitFilter pred, bool include_neutral = false);
template CUnit *AttackUnitsInDistance<true>(const CUnit &unit, int range, CUnitFilter pred, bool include_neutral = false);

template <bool circle>
CUnit *AttackUnitsInDistance(const CUnit &unit, const int range, const bool include_neutral)
{
	//Wyrmgus start
//	return AttackUnitsInDistance(unit, range, NoFilter());
	return AttackUnitsInDistance<circle>(unit, range, NoFilter(), include_neutral);
	//Wyrmgus end
}

template CUnit *AttackUnitsInDistance<false>(const CUnit &unit, const int range, const bool include_neutral);
template CUnit *AttackUnitsInDistance<true>(const CUnit &unit, const int range, const bool include_neutral);

/**
**  Attack units in attack range.
**
**  @param unit  Find unit in attack range for this unit.
**
**  @return      Pointer to unit which should be attacked.
*/
CUnit *AttackUnitsInRange(const CUnit &unit, CUnitFilter pred)
{
	assert_throw(unit.CanAttack());
	return AttackUnitsInDistance(unit, unit.get_best_attack_range(), pred);
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
CUnit *AttackUnitsInReactRange(const CUnit &unit, CUnitFilter pred, const bool include_neutral)
//Wyrmgus end
{
	assert_throw(unit.CanAttack());
	const int range = unit.GetReactionRange();
	//Wyrmgus start
//	return AttackUnitsInDistance<true>(unit, range, pred);
	return AttackUnitsInDistance<true>(unit, range, pred, include_neutral);
	//Wyrmgus end
}

CUnit *AttackUnitsInReactRange(const CUnit &unit, bool include_neutral)
{
	//Wyrmgus start
//	return AttackUnitsInReactRange(unit, NoFilter());
	return AttackUnitsInReactRange(unit, NoFilter(), include_neutral);
	//Wyrmgus end
}

class PathwayConnectionFinder final
{
public:
	explicit PathwayConnectionFinder(const CUnit &src_unit, const CUnit &dst_unit, const tile_flag flags, bool *result) :
	//Wyrmgus end
		src_unit(src_unit),
		dst_unit(dst_unit),
		flags(flags),
		result(result)
	{
	}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &src_unit;
	const CUnit &dst_unit;
	tile_flag flags;
	bool *result;
};

VisitResult PathwayConnectionFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	if (pos.x >= dst_unit.tilePos.x && pos.x <= (dst_unit.tilePos.x + dst_unit.Type->get_tile_width() - 1) && pos.y >= dst_unit.tilePos.y && pos.y <= (dst_unit.tilePos.y + dst_unit.Type->get_tile_height() - 1)) {
		*result = true;
		return VisitResult::Finished;
	}
	
	if (!src_unit.MapLayer->Field(pos)->CheckMask(flags)) {
		return VisitResult::DeadEnd;
	}
	
	return VisitResult::Ok;
}

bool CheckPathwayConnection(const CUnit &src_unit, const CUnit &dst_unit, const tile_flag flags)
{
	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(src_unit.MapLayer->get_width(), src_unit.MapLayer->get_height());
	terrainTraversal.Init();

	terrainTraversal.PushUnitPosAndNeighbor(src_unit);

	bool result = false;

	PathwayConnectionFinder pathwayConnectionUnitFinder(src_unit, dst_unit, flags, &result);

	terrainTraversal.Run(pathwayConnectionUnitFinder);
	
	return result;
}
