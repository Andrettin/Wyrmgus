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
//      (c) Copyright 2001-2019 by Lutz Sammer and Andrettin
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

#include "ai_local.h"

#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unittype.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//Wyrmgus start
//static bool IsPosFree(const Vec2i &pos, const CUnit &exceptionUnit)
static bool IsPosFree(const Vec2i &pos, const CUnit &exceptionUnit, int z)
//Wyrmgus end
{
	if (CMap::Map.Info.IsPointOnMap(pos, z) == false) {
		return false;
	}
	const CMapField &mf = *CMap::Map.Field(pos, z);
	const CUnitCache &unitCache = mf.UnitCache;
	if (std::find(unitCache.begin(), unitCache.end(), &exceptionUnit) != unitCache.end()) {
		return true;
	}
	const unsigned int blockedFlag = (MapFieldUnpassable | MapFieldWall | MapFieldRocks | MapFieldForest | MapFieldBuilding);
	if (mf.Flags & blockedFlag) {
		return false;
	}
	const unsigned int passableFlag = (MapFieldWaterAllowed | MapFieldCoastAllowed | MapFieldLandAllowed);
	return ((mf.Flags & passableFlag) != 0);
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
//Wyrmgus start
//static bool AiCheckSurrounding(const CUnit &worker, const CUnitType &type, const Vec2i &pos, bool &backupok)
static bool AiCheckSurrounding(const CUnit &worker, const CUnitType &type, const Vec2i &pos, bool &backupok, int z)
//Wyrmgus end
{
	const int surroundRange = type.AiAdjacentRange != -1 ? type.AiAdjacentRange : 1;
	const Vec2i pos_topLeft(pos.x - surroundRange, pos.y - surroundRange);
	const Vec2i pos_bottomRight(pos + type.TileSize + surroundRange - 1);
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

class BuildingPlaceFinder
{
public:
	//Wyrmgus start
//	BuildingPlaceFinder(const CUnit &worker, const CUnitType &type, bool checkSurround, Vec2i *resultPos) :
	BuildingPlaceFinder(const CUnit &worker, const CUnitType &type, bool checkSurround, Vec2i *resultPos, bool ignore_exploration, int z, int landmass, CSite *settlement) :
	//Wyrmgus end
		worker(worker), type(type),
			movemask(worker.Type->MovementMask 
			& ~((type.BoolFlag[SHOREBUILDING_INDEX].value ? (MapFieldCoastAllowed | MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit) 
			:  (MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)))),
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
	const CUnitType &type;
	unsigned int movemask;
	bool checkSurround;
	Vec2i *resultPos;
	//Wyrmgus start
	int z;
	int landmass;
	CSite *settlement;
	bool IgnoreExploration;
	//Wyrmgus end
};

VisitResult BuildingPlaceFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !CMap::Map.IsFieldExplored(player, pos)) {
		return VisitResult_DeadEnd;
	}
#endif
	*/
	if (!IgnoreExploration && !CMap::Map.Field(pos, z)->playerInfo.IsTeamExplored(*worker.Player)) {
		return VisitResult_DeadEnd;
	}
	
	if (CMap::Map.Field(pos, z)->Owner != -1 && CMap::Map.Field(pos, z)->Owner != worker.Player->Index && !Players[CMap::Map.Field(pos, z)->Owner].HasNeutralFactionType() && !worker.Player->HasNeutralFactionType()) { //buildings cannot be built on other players' land; we return dead end instead of ok because we don't want units to go over another player's territory to build structures elsewhere, resulting in a lot of exclaves; the exception are neutral factions, which should be composed largely of enclaves and exclaves
		return VisitResult_DeadEnd;
	}
	
//	if (CanBuildUnitType(&worker, type, pos, 1)
	if (
		(!landmass || CMap::Map.GetTileLandmass(pos, z) == landmass)
		&& CanBuildUnitType(&worker, type, pos, 1, IgnoreExploration, z)
		&& !AiEnemyUnitsInDistance(*worker.Player, nullptr, pos, 8, z)
		&& (!settlement || settlement == worker.Player->GetNearestSettlement(pos, z, type.TileSize))
	) {
		//Wyrmgus end
		bool backupok;
		//Wyrmgus start
//		if (AiCheckSurrounding(worker, type, pos, backupok) && checkSurround) {
		if (AiCheckSurrounding(worker, type, pos, backupok, z) && checkSurround) {
		//Wyrmgus end
			*resultPos = pos;
			return VisitResult_Finished;
		} else if (backupok && resultPos->x == -1) {
			*resultPos = pos;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
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
//Wyrmgus start
//static bool AiFindBuildingPlace2(const CUnit &worker, const CUnitType &type, const Vec2i &startPos, const CUnit *startUnit, bool checkSurround, Vec2i *resultPos)
static bool AiFindBuildingPlace2(const CUnit &worker, const CUnitType &type, const Vec2i &startPos, const CUnit *startUnit, bool checkSurround, Vec2i *resultPos, bool ignore_exploration, int z, int landmass = 0, CSite *settlement = nullptr)
//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	if (startUnit != nullptr) {
		terrainTraversal.PushUnitPosAndNeighbor(*startUnit);
	} else {
		Assert(CMap::Map.Info.IsPointOnMap(startPos, z));
		terrainTraversal.PushPos(startPos);
	}

	//Wyrmgus start
//	BuildingPlaceFinder buildingPlaceFinder(worker, type, checkSurround, resultPos);
	BuildingPlaceFinder buildingPlaceFinder(worker, type, checkSurround, resultPos, ignore_exploration, z, landmass, settlement);
	//Wyrmgus end

	terrainTraversal.Run(buildingPlaceFinder);
	return CMap::Map.Info.IsPointOnMap(*resultPos, z);
}

class HallPlaceFinder
{
public:
	//Wyrmgus start
//	HallPlaceFinder(const CUnit &worker, const CUnitType &type, int resource, Vec2i *resultPos) :
	HallPlaceFinder(const CUnit &worker, const CUnitType &type, int resource, Vec2i *resultPos, bool ignore_exploration, int z) :
	//Wyrmgus end
		worker(worker), type(type),
		movemask(worker.Type->MovementMask
			& ~((type.BoolFlag[SHOREBUILDING_INDEX].value ? (MapFieldCoastAllowed | MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit) 
			:  (MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)))),
		resource(resource),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos),
		IgnoreExploration(ignore_exploration),
		z(z)
		//Wyrmgus end
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	bool IsAUsableMine(const CUnit &mine) const;
private:
	const CUnit &worker;
	const CUnitType &type;
	const unsigned int movemask;
	const int resource;
	Vec2i *resultPos;
	//Wyrmgus start
	bool IgnoreExploration;
	int z;
	//Wyrmgus end
};

bool HallPlaceFinder::IsAUsableMine(const CUnit &mine) const
{
	//Wyrmgus start
	if (!mine.Type->BoolFlag[BUILDING_INDEX].value || !mine.Type->GivesResource) {
		return false; //if isn't a building, then it isn't a mine, but a metal rock/resource pile, which is not attractive enough to build a hall nearby; if its type doesn't give a resource, then it isn't a mine but an industrial building, which should already be in a settlement
	}
	//Wyrmgus end
	
	// Check units around mine
	const Vec2i offset(5, 5);
	const Vec2i minpos = mine.tilePos - offset;
	const Vec2i typeSize(mine.Type->TileSize - 1);
	const Vec2i maxpos = mine.tilePos + typeSize + offset;
	std::vector<CUnit *> units;

	Select(minpos, maxpos, units, mine.MapLayer->ID);

	const size_t nunits = units.size();
	int buildings = 0;

	for (size_t j = 0; j < nunits; ++j) {
		const CUnit &unit = *units[j];
		// Enemy near mine
		//Wyrmgus start
//		if (AiPlayer->Player->IsEnemy(*unit.Player)) {
		if (worker.Player->IsEnemy(*unit.Player)) {
		//Wyrmgus end
			return false;
		}
		// Town hall near mine
		//Wyrmgus start
//		if (unit.Type->CanStore[resource]) {
		if (unit.Type->CanStore[mine.GivesResource]) {
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
	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !CMap::Map.IsFieldExplored(player, pos)) {
		return VisitResult_DeadEnd;
	}
#endif
	*/
	if (!IgnoreExploration && !CMap::Map.Field(pos, z)->playerInfo.IsTeamExplored(*worker.Player)) {
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	//Wyrmgus start
	if (CMap::Map.Field(pos, z)->Owner != -1 && CMap::Map.Field(pos, z)->Owner != worker.Player->Index && !Players[CMap::Map.Field(pos, z)->Owner].HasNeutralFactionType() && !worker.Player->HasNeutralFactionType()) {
		return VisitResult_DeadEnd;
	}
	
//	CUnit *mine = ResourceOnMap(pos, resource);
	CUnit *mine = nullptr;
	if (resource != -1) {
		mine = ResourceOnMap(pos, resource, z, false);
	} else {
		for (int i = 1; i < MaxCosts; ++i) {
			if (type.CanStore[i]) {
				mine = ResourceOnMap(pos, i, z, false);
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
			return VisitResult_Finished;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
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
							const CUnitType &type,
							const Vec2i &startPos,
							int resource,
							//Wyrmgus start
//							Vec2i *resultPos)
							Vec2i *resultPos, bool ignore_exploration, int z)
							//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	Assert(CMap::Map.Info.IsPointOnMap(startPos, z));
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
	//Wyrmgus start
//	LumberMillPlaceFinder(const CUnit &worker, const CUnitType &type, int resource, Vec2i *resultPos) :
	LumberMillPlaceFinder(const CUnit &worker, const CUnitType &type, int resource, Vec2i *resultPos, bool ignore_exploration, int z) :
	//Wyrmgus end
		worker(worker), type(type),
		movemask(worker.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		resource(resource),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos),
		IgnoreExploration(ignore_exploration),
		z(z)
		//Wyrmgus end
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &worker;
	const CUnitType &type;
	unsigned int movemask;
	int resource;
	Vec2i *resultPos;
	//Wyrmgus start
	bool IgnoreExploration;
	int z;
	//Wyrmgus end
};

VisitResult LumberMillPlaceFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
	/*
#if 0
	if (!player.AiEnabled && !CMap::Map.IsFieldExplored(player, pos)) {
		return VisitResult_DeadEnd;
	}
#endif
	*/
	if (!IgnoreExploration && !CMap::Map.Field(pos, z)->playerInfo.IsTeamExplored(*worker.Player)) {
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	//Wyrmgus start
	if (CMap::Map.Field(pos, z)->Owner != -1 && CMap::Map.Field(pos, z)->Owner != worker.Player->Index && !Players[CMap::Map.Field(pos, z)->Owner].HasNeutralFactionType() && !worker.Player->HasNeutralFactionType()) {
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	
	if (CMap::Map.Field(pos, z)->IsTerrainResourceOnMap(resource)) {
		//Wyrmgus start
//		if (AiFindBuildingPlace2(worker, type, from, nullptr, true, resultPos)) {
		if (AiFindBuildingPlace2(worker, type, from, nullptr, true, resultPos, IgnoreExploration, z)) {
		//Wyrmgus end
			return VisitResult_Finished;
		}
	}
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)
	if (CanMoveToMask(pos, movemask, z)
	//Wyrmgus end
		|| (worker.Type->RepairRange == InfiniteRepairRange && type.BoolFlag[BUILDEROUTSIDE_INDEX].value)) { // reachable, or unit can build from outside and anywhere
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
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
//Wyrmgus start
//static bool AiFindLumberMillPlace(const CUnit &worker, const CUnitType &type, const Vec2i &startPos, int resource, Vec2i *resultPos)
static bool AiFindLumberMillPlace(const CUnit &worker, const CUnitType &type, const Vec2i &startPos, int resource, Vec2i *resultPos, bool ignore_exploration, int z)
//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	Assert(CMap::Map.Info.IsPointOnMap(startPos, z));
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	LumberMillPlaceFinder lumberMillPlaceFinder(worker, type, resource, resultPos);
	LumberMillPlaceFinder lumberMillPlaceFinder(worker, type, resource, resultPos, ignore_exploration, z);
	//Wyrmgus end

	return terrainTraversal.Run(lumberMillPlaceFinder);
}

static bool AiFindMiningPlace(const CUnit &worker,
							  const CUnitType &type,
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
//Wyrmgus start
//bool AiFindBuildingPlace(const CUnit &worker, const CUnitType &type, const Vec2i &nearPos, Vec2i *resultPos)
bool AiFindBuildingPlace(const CUnit &worker, const CUnitType &type, const Vec2i &nearPos, Vec2i *resultPos, bool ignore_exploration, int z, int landmass, CSite *settlement)
//Wyrmgus end
{
	// Find a good place for a new hall
	//Wyrmgus start
//	DebugPrint("%d: Want to build a %s(%s)\n" _C_ AiPlayer->Player->Index
	DebugPrint("%d: Want to build a %s(%s)\n" _C_ worker.Player->Index
   //Wyrmgus end
			   //Wyrmgus start
//			   _C_ type.Ident.c_str() _C_ type.Name.c_str());
			   _C_ type.Ident.c_str() _C_ type.GetDefaultName(*worker.Player).c_str());
			   //Wyrmgus end

	//Wyrmgus start
	if (type.TerrainType || type.BoolFlag[TOWNHALL_INDEX].value) { //if the building is actually a terrain type, or if it is a town hall, then the "nearPos" is already the correct position
		*resultPos = nearPos;
		return true;
	}
	//Wyrmgus end

	const Vec2i &startPos = CMap::Map.Info.IsPointOnMap(nearPos, z) ? nearPos : worker.tilePos;
	
	//Mines and Depots
	for (int i = 1; i < MaxCosts; ++i) {
		ResourceInfo *resinfo = worker.Type->ResInfo[i];
		//Depots
		if (type.CanStore[i]) {
			//Wyrmgus start
//			if (resinfo && resinfo->TerrainHarvester) {
			if (resinfo && i == WoodCost) {
			//Wyrmgus end
				//Wyrmgus start
//				return AiFindLumberMillPlace(worker, type, startPos, i, resultPos);
				if (AiFindLumberMillPlace(worker, type, startPos, i, resultPos, ignore_exploration, z)) {
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
				return AiFindHallPlace(worker, type, startPos, -1, resultPos, ignore_exploration, z);
				//Wyrmgus end
			}
		} else {
			//mines
			//Wyrmgus start
//			if (type.GivesResource == i) {
			if (type.GivesResource == i && type.GivesResource != TradeCost) {
			//Wyrmgus end
				//Wyrmgus start
//				if (resinfo && resinfo->RefineryHarvester) {
				if (resinfo) {
				//Wyrmgus end
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
