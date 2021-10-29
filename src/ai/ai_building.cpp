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
/**@name ai_building.cpp - AI building functions. */
//
//      (c) Copyright 2001-2021 by Lutz Sammer and Andrettin
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

#include "ai_local.h"

#include "database/defines.h"
#include "economy/resource.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"

static bool IsPosFree(const Vec2i &pos, const CUnit &exceptionUnit, int z)
{
	if (CMap::get()->Info->IsPointOnMap(pos, z) == false) {
		return false;
	}
	const wyrmgus::tile &mf = *CMap::get()->Field(pos, z);
	const CUnitCache &unitCache = mf.UnitCache;
	if (std::find(unitCache.begin(), unitCache.end(), &exceptionUnit) != unitCache.end()) {
		return true;
	}
	const tile_flag blocked_flag = (tile_flag::impassable | tile_flag::wall | tile_flag::rock | tile_flag::tree | tile_flag::building | tile_flag::space_cliff | tile_flag::air_building);
	if ((mf.get_flags() & blocked_flag) != tile_flag::none) {
		return false;
	}
	const tile_flag passable_flag = (tile_flag::water_allowed | tile_flag::coast_allowed | tile_flag::land_allowed | tile_flag::ford | tile_flag::space);
	return ((mf.get_flags() & passable_flag) != tile_flag::none);
}

/**
**  Check if the surrounding are free. Depending on the value of flag, it will check :
**  0: the building will not block any way
**  1: all surrounding is free
**
**  @param worker    Worker to build.
**  @param type      Type of building.
**  @param pos       map tile position for the building.
**  @param backupok  Location can be used as a backup
**
**  @return          True if the surrounding is free, false otherwise.
**
**  @note            Can be faster written.
*/
static bool AiCheckSurrounding(const CUnit &worker, const wyrmgus::unit_type &type, const Vec2i &pos, bool &backupok, int z)
{
	const int surroundRange = type.AiAdjacentRange != -1 ? type.AiAdjacentRange : 1;
	const Vec2i pos_topLeft(pos.x - surroundRange, pos.y - surroundRange);
	const Vec2i pos_bottomRight(pos + type.get_tile_size() + QSize(surroundRange, surroundRange) - QSize(1, 1));
	Vec2i it = pos_topLeft;
	//Wyrmgus start
//	const bool firstVal = IsPosFree(it, worker);
	const bool firstVal = IsPosFree(it, worker, z);
	//Wyrmgus end
	bool lastval = firstVal;
	int obstacleCount = 0;

	for (++it.x; it.x < pos_bottomRight.x; ++it.x) {
		//Wyrmgus start
//		const bool isfree = IsPosFree(it, worker);
		const bool isfree = IsPosFree(it, worker, z);
		//Wyrmgus end

		if (isfree && !lastval) {
			++obstacleCount;
		}
		lastval = isfree;
	}
	for (; it.y < pos_bottomRight.y; ++it.y) {
		//Wyrmgus start
//		const bool isfree = IsPosFree(it, worker);
		const bool isfree = IsPosFree(it, worker, z);
		//Wyrmugs end

		if (isfree && !lastval) {
			++obstacleCount;
		}
		lastval = isfree;
	}
	for (; pos_topLeft.x < it.x; --it.x) {
		//Wyrmgus start
//		const bool isfree = IsPosFree(it, worker);
		const bool isfree = IsPosFree(it, worker, z);
		//Wyrmgus end

		if (isfree && !lastval) {
			++obstacleCount;
		}
		lastval = isfree;
	}
	for (; pos_topLeft.y < it.y; --it.y) {
		//Wyrmgus start
//		const bool isfree = IsPosFree(it, worker);
		const bool isfree = IsPosFree(it, worker, z);
		//Wyrmgus end

		if (isfree && !lastval) {
			++obstacleCount;
		}
		lastval = isfree;
	}
	if (firstVal && !lastval) {
		++obstacleCount;
	}

	if (!type.BoolFlag[SHOREBUILDING_INDEX].value) {
		backupok = obstacleCount < 5;
	} else {
		// Shore building have at least 2 obstacles : sea->ground & ground->sea
		backupok = obstacleCount < 3;
	}
	return obstacleCount == 0;
}

class BuildingPlaceFinder final
{
public:
	explicit BuildingPlaceFinder(const CUnit &worker, const wyrmgus::unit_type &type, bool checkSurround, Vec2i *resultPos, bool ignore_exploration, int z, const landmass *landmass, const wyrmgus::site *settlement) :
		worker(worker), type(type),
			movemask(worker.Type->MovementMask 
			& ~((type.BoolFlag[SHOREBUILDING_INDEX].value ? (tile_flag::coast_allowed | tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)
			:  (tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)))),
		checkSurround(checkSurround),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos),
		z(z),
		landmass(landmass),
		settlement(settlement),
		IgnoreExploration(ignore_exploration)
		//Wyrmgus start
	{
		resultPos->x = -1;
		resultPos->y = -1;
	}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &worker;
	const wyrmgus::unit_type &type;
	tile_flag movemask;
	bool checkSurround;
	Vec2i *resultPos;
	//Wyrmgus start
	int z;
	const wyrmgus::landmass *landmass = nullptr;
	const wyrmgus::site *settlement;
	bool IgnoreExploration;
	//Wyrmgus end
};

VisitResult BuildingPlaceFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !Map.IsFieldExplored(player, pos)) {
		return VisitResult::DeadEnd;
	}
#endif
	*/

	const wyrmgus::tile *tile = CMap::get()->Field(pos, z);

	if (!IgnoreExploration && !tile->player_info->IsTeamExplored(*worker.Player)) {
		return VisitResult::DeadEnd;
	}
	
	if (tile->get_owner() != nullptr && tile->get_owner() != worker.Player && !tile->get_owner()->has_neutral_faction_type() && !worker.Player->has_neutral_faction_type()) { //buildings cannot be built on other players' land; we return dead end instead of ok because we don't want units to go over another player's territory to build structures elsewhere, resulting in a lot of exclaves; the exception are neutral factions, which should be composed largely of enclaves and exclaves
		return VisitResult::DeadEnd;
	}
	
//	if (CanBuildUnitType(&worker, type, pos, 1)
	if (
		(!landmass || CMap::get()->get_tile_landmass(pos, z) == landmass)
		&& CanBuildUnitType(&worker, type, pos, 1, IgnoreExploration, z)
		&& !AiEnemyUnitsInDistance(*worker.Player, nullptr, pos, 8, z)
		&& (!this->settlement || this->settlement == tile->get_settlement())
	) {
		//Wyrmgus end
		bool backupok;
		//Wyrmgus start
//		if (AiCheckSurrounding(worker, type, pos, backupok) && checkSurround) {
		if (AiCheckSurrounding(worker, type, pos, backupok, z) && checkSurround) {
		//Wyrmgus end
			*resultPos = pos;
			return VisitResult::Finished;
		} else if (backupok && resultPos->x == -1) {
			*resultPos = pos;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

/**
**  Find free building place. (flood fill version)
**
**  @param worker   Worker to build building.
**  @param type     Type of building.
**  @param startPos Original position to try building
**  @param checkSurround Check if the perimeter of the building is free
**  @param resultPos     OUT: Pointer for position returned.
**
**  @return  True if place found, false if no found.
*/
static bool AiFindBuildingPlace2(const CUnit &worker, const wyrmgus::unit_type &type, const Vec2i &startPos, const CUnit *startUnit, bool checkSurround, Vec2i *resultPos, bool ignore_exploration, int z, const landmass *landmass = nullptr, const wyrmgus::site *settlement = nullptr)
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::get()->Info->MapWidth, CMap::get()->Info->MapHeight);
	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	if (startUnit != nullptr) {
		terrainTraversal.PushUnitPosAndNeighbor(*startUnit);
	} else {
		assert_throw(CMap::get()->Info->IsPointOnMap(startPos, z));
		terrainTraversal.PushPos(startPos);
	}

	//Wyrmgus start
//	BuildingPlaceFinder buildingPlaceFinder(worker, type, checkSurround, resultPos);
	BuildingPlaceFinder buildingPlaceFinder(worker, type, checkSurround, resultPos, ignore_exploration, z, landmass, settlement);
	//Wyrmgus end

	terrainTraversal.Run(buildingPlaceFinder);
	return CMap::get()->Info->IsPointOnMap(*resultPos, z);
}

class HallPlaceFinder final
{
public:
	explicit HallPlaceFinder(const CUnit &worker, const wyrmgus::unit_type &type, const resource *resource, Vec2i *resultPos, const bool ignore_exploration, const int z)
		: worker(worker), type(type),
		movemask(worker.Type->MovementMask
			& ~((type.BoolFlag[SHOREBUILDING_INDEX].value ? (tile_flag::coast_allowed | tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)
			:  (tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)))),
		resource(resource),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos),
		IgnoreExploration(ignore_exploration),
		z(z)
		//Wyrmgus end
	{
	}

	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);

private:
	bool IsAUsableMine(const CUnit &mine) const;

private:
	const CUnit &worker;
	const wyrmgus::unit_type &type;
	const tile_flag movemask;
	const wyrmgus::resource *resource = nullptr;
	Vec2i *resultPos;
	//Wyrmgus start
	bool IgnoreExploration;
	int z;
	//Wyrmgus end
};

bool HallPlaceFinder::IsAUsableMine(const CUnit &mine) const
{
	//Wyrmgus start
	if (!mine.Type->BoolFlag[BUILDING_INDEX].value || mine.Type->get_given_resource() == nullptr) {
		return false; //if isn't a building, then it isn't a mine, but a metal rock/resource pile, which is not attractive enough to build a hall nearby; if its type doesn't give a resource, then it isn't a mine but an industrial building, which should already be in a settlement
	}
	//Wyrmgus end
	
	// Check units around mine
	const Vec2i offset(5, 5);
	const Vec2i minpos = mine.tilePos - offset;
	const Vec2i typeSize(mine.Type->get_tile_size() - QSize(1, 1));
	const Vec2i maxpos = mine.tilePos + typeSize + offset;
	std::vector<CUnit *> units;

	Select(minpos, maxpos, units, mine.MapLayer->ID);

	const size_t nunits = units.size();
	int buildings = 0;

	for (size_t j = 0; j < nunits; ++j) {
		const CUnit &unit = *units[j];
		// Enemy near mine
		//Wyrmgus start
//		if (AiPlayer->Player->is_enemy_of(*unit.Player)) {
		if (worker.Player->is_enemy_of(*unit.Player)) {
		//Wyrmgus end
			return false;
		}
		// Town hall near mine
		//Wyrmgus start
//		if (unit.Type->CanStore[resource]) {
		if (unit.Type->can_store(mine.get_given_resource())) {
		//Wyrmgus end
			return false;
		}
		// Town hall may not be near but we may be using it, check
		// for 2 buildings near it and assume it's been used
		//Wyrmgus start
//		if (unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.Type->GivesResource) {
		if (unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.GivesResource) {
		//Wyrmgus end
			++buildings;
			if (buildings == 2) {
				return false;
			}
		}
	}
	return true;
}

VisitResult HallPlaceFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !Map.IsFieldExplored(player, pos)) {
		return VisitResult::DeadEnd;
	}
#endif
	*/
	//Wyrmgus start
//	if (!IgnoreExploration && !CMap::get()->Field(pos)->player_info->IsTeamExplored(*worker.Player)) {
	if (!IgnoreExploration && !CMap::get()->Field(pos, z)->player_info->IsTeamExplored(*worker.Player)) {
	//Wyrmgus end
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	//Wyrmgus start
	if (CMap::get()->Field(pos, z)->get_owner() != nullptr && CMap::get()->Field(pos, z)->get_owner() != worker.Player && !CMap::get()->Field(pos, z)->get_owner()->has_neutral_faction_type() && !worker.Player->has_neutral_faction_type()) {
		return VisitResult::DeadEnd;
	}
	
//	CUnit *mine = ResourceOnMap(pos, resource);
	CUnit *mine = nullptr;
	if (this->resource != nullptr) {
		mine = ResourceOnMap(pos, this->resource, z, false);
	} else {
		for (const wyrmgus::resource *resource : resource::get_all()) {
			if (resource == defines::get()->get_time_resource()) {
				continue;
			}

			if (type.can_store(resource)) {
				mine = ResourceOnMap(pos, resource, z, false);
				if (mine) {
					break;
				}
			}
		}
	}
	//Wyrmgus end
	if (mine && IsAUsableMine(*mine)) {
		//Wyrmgus start
//		if (AiFindBuildingPlace2(worker, type, pos, mine, true, resultPos)) {
		if (AiFindBuildingPlace2(worker, type, pos, mine, true, resultPos, IgnoreExploration, z)) {
		//Wyrmgus end
			return VisitResult::Finished;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

/**
**  Find building place for hall. (flood fill version)
**
**  The best place:
**  1) near to resource.
**  !2) near to wood.
**  !3) near to worker and must be reachable.
**  4) no enemy near it.
**  5) no hall already near
**  !6) enough gold in mine
**
**  @param worker    Worker to build building.
**  @param type      Type of building.
**  @param startPos  Start search position (if == -1 then unit X pos used).
**  @param resource  resource to be near.
**  @param resultPos OUT: Pointer for position returned.
**
**  @return        True if place found, false if not found.
**
**  @todo          FIXME: This is slow really slow, using
**                 two flood fills, is not a perfect solution.
*/
static bool AiFindHallPlace(const CUnit &worker,
							const unit_type &type,
							const Vec2i &startPos,
							const resource *resource,
							//Wyrmgus start
//							Vec2i *resultPos)
							Vec2i *resultPos, bool ignore_exploration, int z)
							//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::get()->Info->MapWidth, CMap::get()->Info->MapHeight);
	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	assert_throw(CMap::get()->Info->IsPointOnMap(startPos, z));
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	HallPlaceFinder hallPlaceFinder(worker, type, resource, resultPos);
	HallPlaceFinder hallPlaceFinder(worker, type, resource, resultPos, ignore_exploration, z);
	//Wyrmgus end

	if (terrainTraversal.Run(hallPlaceFinder)) {
		return true;
	}
	//Wyrmgus start
//	return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos);
	return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos, ignore_exploration, z);
	//Wyrmgus end
}

class LumberMillPlaceFinder
{
public:
	LumberMillPlaceFinder(const CUnit &worker, const wyrmgus::unit_type &type, const int resource, Vec2i *resultPos, bool ignore_exploration, const int z, const wyrmgus::site *settlement) :
		worker(worker), type(type),
		movemask(worker.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)),
		resource(resource),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos),
		IgnoreExploration(ignore_exploration),
		z(z),
		settlement(settlement)
		//Wyrmgus end
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &worker;
	const wyrmgus::unit_type &type;
	tile_flag movemask;
	int resource;
	Vec2i *resultPos;
	//Wyrmgus start
	bool IgnoreExploration;
	int z;
	//Wyrmgus end
	const wyrmgus::site *settlement = nullptr;
};

VisitResult LumberMillPlaceFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)

	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !Map.IsFieldExplored(player, pos)) {
		return VisitResult::DeadEnd;
	}
#endif
	*/

	const wyrmgus::tile *tile = CMap::get()->Field(pos, z);
	if (!IgnoreExploration && !tile->player_info->IsTeamExplored(*worker.Player)) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	//Wyrmgus start


	if (tile->get_owner() != nullptr && tile->get_owner() != worker.Player && !tile->get_owner()->has_neutral_faction_type() && !worker.Player->has_neutral_faction_type()) {
		return VisitResult::DeadEnd;
	}

//	if (CMap::get()->Field(pos)->get_resource() == wyrmgus::resource::get_all()[resource]) {
	if (tile->get_resource() == wyrmgus::resource::get_all()[resource]) {
	//Wyrmgus end
		//Wyrmgus start
//		if (AiFindBuildingPlace2(worker, type, from, nullptr, true, resultPos)) {
		if (AiFindBuildingPlace2(worker, type, from, nullptr, true, resultPos, IgnoreExploration, z, 0, this->settlement)) {
		//Wyrmgus end
			return VisitResult::Finished;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)
	if (CanMoveToMask(pos, movemask, z)
	//Wyrmgus end
		|| (worker.Type->RepairRange == InfiniteRepairRange && type.BoolFlag[BUILDEROUTSIDE_INDEX].value)) { // reachable, or unit can build from outside and anywhere
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

/**
**  Find free building place for lumber mill. (flood fill version)
**
**  @param worker    Worker to build building.
**  @param type      Type of building.
**  @param resource  resource terrain to be near.
**  @param startPos  Start search X position (if == -1 then unit X pos used).
**  @param resultPos OUT: Pointer for position returned.
**
**  @return        True if place found, false if not found.
**
**  @todo          FIXME: This is slow really slow, using two flood fills, is not a perfect solution.
*/
static bool AiFindLumberMillPlace(const CUnit &worker, const wyrmgus::unit_type &type, const Vec2i &startPos, int resource, Vec2i *resultPos, bool ignore_exploration, int z, const wyrmgus::site *settlement)
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::get()->Info->MapWidth, CMap::get()->Info->MapHeight);
	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	assert_throw(CMap::get()->Info->IsPointOnMap(startPos, z));
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	LumberMillPlaceFinder lumberMillPlaceFinder(worker, type, resource, resultPos);
	LumberMillPlaceFinder lumberMillPlaceFinder(worker, type, resource, resultPos, ignore_exploration, z, settlement);
	//Wyrmgus end

	return terrainTraversal.Run(lumberMillPlaceFinder);
}

static bool AiFindMiningPlace(const CUnit &worker,
							  const wyrmgus::unit_type &type,
							  const Vec2i &startPos,
							  int resource,
							  //Wyrmgus start
//							  Vec2i *resultPos)
							  Vec2i *resultPos, bool ignore_exploration, int z)
							  //Wyrmgus end
{
	// look near (mine = ResourceOnMap(pos, resource, false) ?
	//Wyrmgus start
//	return AiFindBuildingPlace2(worker, type, startPos, nullptr, false, resultPos);
	return AiFindBuildingPlace2(worker, type, startPos, nullptr, false, resultPos, ignore_exploration, z);
	//Wyrmgus end
}

/**
**  Find free building place.
**
**  @param worker     Worker to build building.
**  @param type       Type of building.
**  @param nearPos    Start search near nearPos position (or worker->X if nearPos is invalid).
**  @param resultPos  Pointer for position returned.
**
**  @return        True if place found, false if no found.
**
**  @todo          Better and faster way to find building place of oil
**                 platforms Special routines for special buildings.
*/
bool AiFindBuildingPlace(const CUnit &worker, const wyrmgus::unit_type &type, const Vec2i &nearPos, Vec2i *resultPos, bool ignore_exploration, int z, const landmass *landmass, const wyrmgus::site *settlement)
{
	// Find a good place for a new hall
	//Wyrmgus start
//	DebugPrint("%d: Want to build a %s(%s)\n" _C_ AiPlayer->Player->get_index()
	DebugPrint("%d: Want to build a %s(%s)\n" _C_ worker.Player->get_index()
   //Wyrmgus end
			   //Wyrmgus start
//			   _C_ type.Ident.c_str() _C_ type.Name.c_str());
			   _C_ type.Ident.c_str() _C_ type.GetDefaultName(worker.Player).c_str());
			   //Wyrmgus end

	//Wyrmgus start
	if (type.TerrainType || type.BoolFlag[TOWNHALL_INDEX].value) { //if the building is actually a terrain type, or if it is a town hall, then the "nearPos" is already the correct position
		*resultPos = nearPos;
		return true;
	}
	//Wyrmgus end

	const Vec2i &startPos = CMap::get()->Info->IsPointOnMap(nearPos, z) ? nearPos : worker.tilePos;
	
	//Mines and Depots
	for (int i = 1; i < MaxCosts; ++i) {
		const resource *resource = resource::get_all()[i];
		const resource_info *res_info = worker.Type->get_resource_info(resource);
		//Depots
		if (type.can_store(resource)) {
			//Wyrmgus start
//			if (resinfo && resinfo->TerrainHarvester) {
			if (res_info != nullptr && i == WoodCost) {
			//Wyrmgus end
				//Wyrmgus start
//				return AiFindLumberMillPlace(worker, type, startPos, i, resultPos);
				if (AiFindLumberMillPlace(worker, type, startPos, i, resultPos, ignore_exploration, z, settlement)) {
					return true;
				} else {
					return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos, ignore_exploration, z, landmass, settlement);
				}
				//Wyrmgus end
			//Wyrmgus start
//			} else {
			} else if (i != TradeCost) {
			//Wyrmgus end
				//Wyrmgus start
//				return AiFindHallPlace(worker, type, startPos, i, resultPos);
				return AiFindHallPlace(worker, type, startPos, nullptr, resultPos, ignore_exploration, z);
				//Wyrmgus end
			}
		} else {
			//mines
			//Wyrmgus start
//			if (type.GivesResource == i) {
			if (type.get_given_resource() != nullptr && type.get_given_resource()->get_index() == i && type.get_given_resource()->get_index() != TradeCost) {
			//Wyrmgus end
				if (res_info != nullptr) {
					//Mine have to be build ONTOP resources
					//Wyrmgus start
//					return AiFindMiningPlace(worker, type, startPos, i, resultPos);
					return AiFindMiningPlace(worker, type, startPos, i, resultPos, ignore_exploration, z);
					//Wyrmgus end
				} else {
					//Mine can be build without resource restrictions: solar panels, etc
					//Wyrmgus start
//					return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos);
					return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos, ignore_exploration, z, landmass, settlement);
					//Wyrmgus end
				}
			}
		}
	}
	//Wyrmgus start
//	return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos);
	return AiFindBuildingPlace2(worker, type, startPos, nullptr, true, resultPos, ignore_exploration, z, landmass, settlement);
	//Wyrmgus end
}
