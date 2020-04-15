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
/**@name ai_plan.cpp - AI planning functions. */
//
//      (c) Copyright 2002-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "actions.h"
#include "commands.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "missile.h"
#include "pathfinder.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "unit/unit_type_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

class _EnemyOnMapTile
{
public:
	_EnemyOnMapTile(const CUnit &unit, const Vec2i _pos, CUnit **enemy) :
		source(&unit) , pos(_pos), best(enemy)
	{
	}

	void operator()(CUnit *const unit) const
	{
		const CUnitType &type = *unit->Type;
		// unusable unit ?
		// if (unit->IsUnusable()) can't attack constructions
		// FIXME: did SelectUnitsOnTile already filter this?
		// Invisible and not Visible
		if (unit->Removed || unit->Variable[INVISIBLE_INDEX].Value
			// || (!UnitVisible(unit, source->Player))
			|| unit->CurrentAction() == UnitAction::Die) {
			return;
		}
		if (unit->Type->UnitType == UnitTypeType::Fly && unit->IsAgressive() == false) {
			return;
		}
		if (pos.x < unit->tilePos.x || pos.x >= unit->tilePos.x + type.TileSize.x
			|| pos.y < unit->tilePos.y || pos.y >= unit->tilePos.y + type.TileSize.y) {
			return;
		}
		if (!CanTarget(*source->Type, type)) {
			return;
		}
		//Wyrmgus start
//		if (!source->Player->IsEnemy(*unit)) { // a friend or neutral
		if (!source->IsEnemy(*unit)) { // a friend or neutral
		//Wyrmgus end
			return;
		}
		// Choose the best target.
		if (!*best || (*best)->Variable[PRIORITY_INDEX].Value < unit->Variable[PRIORITY_INDEX].Value) {
			*best = unit;
		}
	}

private:
	const CUnit *const source;
	const Vec2i pos;
	CUnit **best;
};

/**
**  Choose enemy on map tile.
**
**  @param source  Unit which want to attack.
**  @param pos     position on map, tile-based.
**
**  @return        Returns ideal target on map tile.
*/
//Wyrmgus start
//static CUnit *EnemyOnMapTile(const CUnit &source, const Vec2i &pos)
static CUnit *EnemyOnMapTile(const CUnit &source, const Vec2i &pos, int z)
//Wyrmgus end
{
	CUnit *enemy = nullptr;

	_EnemyOnMapTile filter(source, pos, &enemy);
	//Wyrmgus start
//	CMap::Map.Field(pos)->UnitCache.for_each(filter);
	CMap::Map.Field(pos, z)->UnitCache.for_each(filter);
	//Wyrmgus end
	return enemy;
}

class WallFinder
{
public:
	WallFinder(const CUnit &unit, int maxDist, Vec2i *resultPos) :
		//Wyrmgus start
		unit(unit),
		//Wyrmgus end
		maxDist(maxDist),
		movemask(unit.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		resultPos(resultPos)
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	int maxDist;
	int movemask;
	Vec2i *resultPos;
};

VisitResult WallFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
	/*
#if 0
	if (!unit.Player->AiEnabled && !Map.IsFieldExplored(*unit.Player, pos)) {
		return VisitResult::DeadEnd;
	}
#endif
	*/
	if (!unit.MapLayer->Field(pos)->playerInfo.IsTeamExplored(*unit.Player)) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	// Look if found what was required.
	//Wyrmgus start
//	if (CMap::Map.WallOnMap(pos)) {
	if (CMap::Map.WallOnMap(pos, unit.MapLayer->ID)) {
	//Wyrmgus end
		DebugPrint("Wall found %d, %d\n" _C_ pos.x _C_ pos.y);
		if (resultPos) {
			*resultPos = from;
		}
		return VisitResult::Finished;
	}
	if (unit.MapLayer->Field(pos)->CheckMask(movemask)) { // reachable
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult::Ok;
		} else {
			return VisitResult::DeadEnd;
		}
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

static bool FindWall(const CUnit &unit, int range, Vec2i *wallPos)
{
	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(unit.MapLayer->get_width(), unit.MapLayer->get_height());
	terrainTraversal.Init();

	terrainTraversal.PushUnitPosAndNeighbor(unit);

	WallFinder wallFinder(unit, range, wallPos);

	return terrainTraversal.Run(wallFinder);
}

/**
**  Find possible walls to target.
**
**  @param force  Attack force.
**
**  @return       True if wall found.
*/
int AiFindWall(AiForce *force)
{
	// Find a unit to use.  Best choice is a land unit with range 1.
	// Next best choice is any land unit.  Otherwise just use the first.
	CUnit *unit = force->Units[0];
	for (unsigned int i = 0; i < force->Units.size(); ++i) {
		CUnit *aiunit = force->Units[i];
		if (aiunit->Type->UnitType == UnitTypeType::Land) {
			unit = aiunit;
			//Wyrmgus start
//			if (aiunit->Type->Missile.Missile->Range == 1) {
			if (aiunit->GetMissile().Missile->Range == 1) {
			//Wyrmgus end
				break;
			}
		}
	}
	const int maxRange = 1000;
	Vec2i wallPos;

	if (FindWall(*unit, maxRange, &wallPos)) {
		force->State = AiForceAttackingState::Waiting;
		for (unsigned int i = 0; i < force->Units.size(); ++i) {
			CUnit &aiunit = *force->Units[i];
			//Wyrmgus start
//			if (aiunit.Type->CanAttack) {
			if (aiunit.CanAttack()) {
			//Wyrmgus end
				CommandAttack(aiunit, wallPos, nullptr, FlushCommands, aiunit.MapLayer->ID);
			} else {
				CommandMove(aiunit, wallPos, FlushCommands, aiunit.MapLayer->ID);
			}
		}
		return 1;
	}
	return 0;
}

class ReachableTerrainMarker
{
public:
	ReachableTerrainMarker(const CUnit &unit) :
		//Wyrmgus start
		unit(unit),
		//Wyrmgus end
		movemask(unit.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit))
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	int movemask;
};

VisitResult ReachableTerrainMarker::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	if (!unit.MapLayer->Field(pos)->playerInfo.IsTeamExplored(*unit.Player)) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	if (CanMoveToMask(pos, movemask, unit.MapLayer->ID)) { // reachable
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

static void MarkReacheableTerrainType(const CUnit &unit, TerrainTraversal *terrainTraversal)
{
	terrainTraversal->PushUnitPosAndNeighbor(unit);

	ReachableTerrainMarker reachableTerrainMarker(unit);

	terrainTraversal->Run(reachableTerrainMarker);
}

class EnemyFinderWithTransporter
{
public:
	EnemyFinderWithTransporter(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vec2i *resultPos) :
		unit(unit),
		terrainTransporter(terrainTransporter),
		movemask(unit.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		resultPos(resultPos)
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	bool IsAccessibleForTransporter(const Vec2i &pos) const;
private:
	const CUnit &unit;
	const TerrainTraversal &terrainTransporter;
	int movemask;
	Vec2i *resultPos;
};

bool EnemyFinderWithTransporter::IsAccessibleForTransporter(const Vec2i &pos) const
{
	return terrainTransporter.IsReached(pos);
}

VisitResult EnemyFinderWithTransporter::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	if (!unit.MapLayer->Field(pos)->playerInfo.IsTeamExplored(*unit.Player)) {
		return VisitResult::DeadEnd;
	}

	if (EnemyOnMapTile(unit, pos, unit.MapLayer->ID) && CanMoveToMask(from, movemask, unit.MapLayer->ID)) {
		DebugPrint("Target found %d,%d\n" _C_ pos.x _C_ pos.y);
		*resultPos = pos;
		return VisitResult::Finished;
	}

	if (CanMoveToMask(pos, movemask, unit.MapLayer->ID) || IsAccessibleForTransporter(pos)) { // reachable
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

//Wyrmgus start
//static bool AiFindTarget(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vec2i *resultPos)
static bool AiFindTarget(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vec2i *resultPos)
//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(Map.Info.MapWidth, Map.Info.MapHeight);
	terrainTraversal.SetSize(unit.MapLayer->get_width(), unit.MapLayer->get_height());
	//Wyrmgus end
	terrainTraversal.Init();

	terrainTraversal.PushUnitPosAndNeighbor(unit);

	EnemyFinderWithTransporter enemyFinderWithTransporter(unit, terrainTransporter, resultPos);

	return terrainTraversal.Run(enemyFinderWithTransporter);
}

class IsAFreeTransporter
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->Type->CanMove() && unit->BoardCount < unit->Type->MaxOnBoard;
	}
};

template <typename ITERATOR>
int GetTotalBoardCapacity(ITERATOR begin, ITERATOR end)
{
	int totalBoardCapacity = 0;
	IsAFreeTransporter isAFreeTransporter;

	for (ITERATOR it = begin; it != end; ++it) {
		const CUnit *unit = *it;

		if (isAFreeTransporter(unit)) {
			totalBoardCapacity += unit->Type->MaxOnBoard - unit->BoardCount;
		}
	}
	return totalBoardCapacity;
}

/**
**  Plan an attack with a force.
**  We know, that we must use a transporter.
**
**  @return       True if target found, false otherwise.
**
**  @todo         Perfect planning.
**                Only works for water transporter!
**  @todo transporter are more selective now (flag with UnitTypeType::Land).
**         We must manage it.
*/
int AiForce::PlanAttack()
{
	CPlayer &player = *AiPlayer->Player;
	DebugPrint("%d: Planning for force #%lu of player #%d\n" _C_ player.Index
			   _C_(long unsigned int)(this - & (AiPlayer->Force[0])) _C_ player.Index);

	TerrainTraversal transporterTerrainTraversal;

	//Wyrmgus start
//	transporterTerrainTraversal.SetSize(CMap::Map.Info.MapWidth, Map.Info.MapHeight);
	transporterTerrainTraversal.SetSize(CMap::Map.Info.MapWidths[this->GoalMapLayer], CMap::Map.Info.MapHeights[this->GoalMapLayer]);
	//Wyrmgus end
	transporterTerrainTraversal.Init();

	CUnit *transporter = Units.find(IsAFreeTransporter());

	if (transporter != nullptr) {
		DebugPrint("%d: Transporter #%d\n" _C_ player.Index _C_ UnitNumber(*transporter));
		MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
	} else {
		std::vector<CUnit *>::iterator it = std::find_if(player.UnitBegin(), player.UnitEnd(), IsAFreeTransporter());
		if (it != player.UnitEnd()) {
			transporter = *it;
			MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
		} else {
			DebugPrint("%d: No transporter available\n" _C_ player.Index);
			return 0;
		}
	}

	// Find a land unit of the force.
	// FIXME: if force is split over different places -> broken
	CUnit *landUnit = Units.find(CUnitTypeFinder(UnitTypeType::Land));
	if (landUnit == nullptr) {
		DebugPrint("%d: No land unit in force\n" _C_ player.Index);
		return 0;
	}

	Vec2i pos = this->GoalPos;

	if (AiFindTarget(*landUnit, transporterTerrainTraversal, &pos)) {
		const unsigned int forceIndex = AiPlayer->Force.getIndex(this) + 1;

		if (transporter->GroupId != forceIndex) {
			DebugPrint("%d: Assign any transporter #%d\n" _C_ player.Index _C_ UnitNumber(*transporter));

			if (transporter->GroupId) {
				transporter->Player->Ai->Force[transporter->GroupId - 1].Remove(*transporter);
			}
			Insert(*transporter);
			transporter->GroupId = forceIndex;
		}

		int totalBoardCapacity = GetTotalBoardCapacity(Units.begin(), Units.end());

		// Verify we have enough transporter.
		// @note: Minimal check for unitType (flyers...)
		for (unsigned int i = 0; i < Size(); ++i) {
			CUnit &unit = *Units[i];

			if (CanTransport(*transporter, unit)) {
				totalBoardCapacity -= unit.Type->BoardSize;
			}
		}
		if (totalBoardCapacity < 0) { // Not enough transporter.
			IsAFreeTransporter isAFreeTransporter;
			// Add all other idle transporter.
			for (int i = 0; i < player.GetUnitCount(); ++i) {
				CUnit &unit = player.GetUnit(i);

				if (isAFreeTransporter(&unit) && unit.GroupId == 0 && unit.IsIdle()) {
					DebugPrint("%d: Assign any transporter #%d\n" _C_ player.Index _C_ UnitNumber(unit));
					Insert(unit);
					unit.GroupId = forceIndex;
					totalBoardCapacity += unit.Type->MaxOnBoard - unit.BoardCount;
					if (totalBoardCapacity >= 0) {
						break;
					}
				}
			}
		}
		DebugPrint("%d: Can attack\n" _C_ player.Index);
		GoalPos = pos;
		State = AiForceAttackingState::Boarding;
		return 1;
	}
	return 0;
}

//Wyrmgus start
/*
static bool ChooseRandomUnexploredPositionNear(const Vec2i &center, Vec2i *pos)
{
	Assert(pos != nullptr);

	int ray = 3;
	const int maxTryCount = 8;
	for (int i = 0; i != maxTryCount; ++i) {
		pos->x = center.x + SyncRand() % (2 * ray + 1) - ray;
		pos->y = center.y + SyncRand() % (2 * ray + 1) - ray;

		if (Map.Info.IsPointOnMap(*pos)
			&& Map.Field(*pos)->playerInfo.IsTeamExplored(*AiPlayer->Player) == false) {
			return true;
		}
		ray = 3 * ray / 2;
	}
	return false;
}

static CUnit *GetBestExplorer(const AiExplorationRequest &request, Vec2i *pos)
{
	// Choose a target, "near"
	const Vec2i &center = request.pos;
	if (ChooseRandomUnexploredPositionNear(center, pos) == false) {
		return nullptr;
	}
	// We have an unexplored tile in sight (pos)

	CUnit *bestunit = nullptr;
	// Find an idle unit, responding to the mask
	bool flyeronly = false;
	int bestSquareDistance = -1;
	for (int i = 0; i != AiPlayer->Player->GetUnitCount(); ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (!unit.IsIdle()) {
			continue;
		}
		if (Map.Info.IsPointOnMap(unit.tilePos) == false) {
			continue;
		}
		if (unit.CanMove() == false) {
			continue;
		}
		//Wyrmgus start
		if (!unit.Active) {
			continue; //only explore with AI active units
		}
		//Wyrmgus end
		const CUnitType &type = *unit.Type;

		if (type.UnitType != UnitTypeType::Fly) {
			if (flyeronly) {
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & MapFieldLandUnit) && type.UnitType != UnitTypeType::Land) {
			if ((request.Mask & MapFieldLandUnit) && type.UnitType != UnitTypeType::Land && type.UnitType != UnitTypeType::FlyLow) {
			//Wyrmgus end
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & MapFieldSeaUnit) && type.UnitType != UnitTypeType::Naval) {
			if ((request.Mask & MapFieldSeaUnit) && type.UnitType != UnitTypeType::Naval && type.UnitType != UnitTypeType::FlyLow) {
			//Wyrmgus end
				continue;
			}
		} else {
			flyeronly = true;
		}

		const int sqDistance = SquareDistance(unit.tilePos, *pos);
		if (bestSquareDistance == -1 || sqDistance <= bestSquareDistance
			|| (bestunit->Type->UnitType != UnitTypeType::Fly && type.UnitType == UnitTypeType::Fly)) {
			bestSquareDistance = sqDistance;
			bestunit = &unit;
		}
	}
	return bestunit;
}
*/
//Wyrmgus end

//Wyrmgus start
static CUnit *GetBestScout(const UnitTypeType unit_type)
{
	CUnit *bestunit = nullptr;

	bool flyeronly = (unit_type == UnitTypeType::Fly);
	
	int best_score = 0;
	for (int i = 0; i != AiPlayer->Player->GetUnitCount(); ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (!unit.IsAliveOnMap()) {
			continue;
		}
		if (unit.CanMove() == false) {
			continue;
		}
		if (!unit.Active) {
			continue; //only scout with AI active units
		}
		if (unit.GroupId != 0) { //don't scout with units that are parts of forces that have a goal
			int force = AiPlayer->Force.GetForce(unit);
			if (force != -1 && CMap::Map.Info.IsPointOnMap(AiPlayer->Force[force].GoalPos, AiPlayer->Force[force].GoalMapLayer)) {
				continue;
			}
		}
		if (unit.Variable[SIGHTRANGE_INDEX].Value < 1) {
			continue; //don't scout with units who have too low a sight range
		}
		if (!unit.IsIdle()) { //don't choose units that aren't idle as scouts
			continue;
		}
		
		const CUnitType &type = *unit.Type;
		
		if (type.BoolFlag[HARVESTER_INDEX].value) { //don't choose harvesters as scouts (they are already told to explore when they can't find a basic resource)
			continue;
		}

		if (type.UnitType != UnitTypeType::Fly) {
			if (flyeronly) {
				continue;
			}
			if (unit_type == UnitTypeType::Land && type.UnitType != UnitTypeType::Land && type.UnitType != UnitTypeType::FlyLow) {
				continue;
			}
			if (unit_type == UnitTypeType::Naval && type.UnitType != UnitTypeType::Naval && type.UnitType != UnitTypeType::FlyLow) {
				continue;
			}
		} else {
			flyeronly = true;
		}
		
		if (unit.Type->CanTransport() && unit.BoardCount) { //if a transporter is carrying a unit, then it shouldn't be used for scouting, as it likely is taking part in an attack
			continue;
		}

		int score = unit.Variable[SIGHTRANGE_INDEX].Value - 4;
		score += unit.Variable[SPEED_INDEX].Value - 10;
		
		if (
			bestunit == nullptr
			|| score > best_score
			|| (bestunit->Type->UnitType != UnitTypeType::Fly && type.UnitType == UnitTypeType::Fly)
		) {
			best_score = score;
			bestunit = &unit;
		}
	}
	return bestunit;
}
//Wyrmgus end

/**
**  Respond to ExplorationRequests
*/
void AiSendExplorers()
{
	//Wyrmgus start
	/*
	// Count requests...
	const int requestcount = AiPlayer->FirstExplorationRequest.size();

	// Nothing => abort
	if (!requestcount) {
		return;
	}
	const int maxTryCount = 5;
	for (int i = 0; i != maxTryCount; ++i) {
		// Choose a request
		const int requestid = SyncRand() % requestcount;
		const AiExplorationRequest &request = AiPlayer->FirstExplorationRequest[requestid];

		Vec2i pos;
		CUnit *bestunit = GetBestExplorer(request, &pos);
		if (bestunit != nullptr) {
			CommandMove(*bestunit, pos, FlushCommands);
			AiPlayer->LastExplorationGameCycle = GameCycle;
			break;
		}
	}
	// Remove all requests
	AiPlayer->FirstExplorationRequest.clear();
	*/
	
	//instead of exploring that way, make a scout randomly walk to explore
	bool land_scout = false;
	bool naval_scout = false;
	bool air_scout = false;
	for (size_t i = 0; i != AiPlayer->Scouts.size(); ++i) {
		if (AiPlayer->Scouts[i] == nullptr) {
			fprintf(stderr, "AI Player #%d's scout %d is null.\n", AiPlayer->Player->Index, (int) i);
			return;
		}
		if (AiPlayer->Scouts[i]->Type->UnitType == UnitTypeType::Fly) {
			land_scout = true;
			naval_scout = true;
			air_scout = true;
			break;
		} else if (AiPlayer->Scouts[i]->Type->UnitType == UnitTypeType::FlyLow) {
			land_scout = true;
			naval_scout = true;
		} else if (AiPlayer->Scouts[i]->Type->UnitType == UnitTypeType::Land) {
			land_scout = true;
		} else if (AiPlayer->Scouts[i]->Type->UnitType == UnitTypeType::Naval) {
			naval_scout = true;
		}
	}
		
	//if no scouts are already present for a particular type, then choose a suitable unit to scout
	if (!air_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeType::Fly);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			land_scout = true;
			naval_scout = true;
		}
	}
	if (!land_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeType::Land);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			if (bestunit->Type->UnitType == UnitTypeType::FlyLow) {
				naval_scout = true;
			}
		}
	}
	if (!naval_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeType::Naval);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
		}
	}
	
	for (size_t i = 0; i != AiPlayer->Scouts.size(); ++i) {
		// move AI scouts
		if (AiPlayer->Scouts[i]->IsIdle()) {
			AiPlayer->Scouts[i]->Scout();
		}
	}
	//Wyrmgus end
}

//Wyrmgus start
void AiCheckTransporters()
{
	//Wyrmgus start
	for (std::map<int, std::vector<CUnit *>>::const_iterator iterator = AiPlayer->Transporters.begin(); iterator != AiPlayer->Transporters.end(); ++iterator) {
		for (size_t i = 0; i != iterator->second.size(); ++i) {
			CUnit &ai_transporter = *iterator->second[i];
			
			if (!ai_transporter.IsIdle()) {
				continue;
			}
			
			CUnit *uins = ai_transporter.UnitInside;
			for (int j = 0; j < ai_transporter.InsideCount; ++j, uins = uins->NextContained) {
				if (uins->GroupId == 0) { //if the unit no longer is part of a force, then it likely has been reset and the attack through water has been cancelled, so unload it
					CommandUnload(ai_transporter, ai_transporter.tilePos, uins, 0, ai_transporter.MapLayer->ID);
				}
			}
		}
	}
	//Wyrmgus end

	AiPlayer->Transporters.clear();
	for (int i = 0; i != AiPlayer->Player->GetUnitCount(); ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (!unit.IsAliveOnMap()) {
			continue;
		}
		if (!unit.Type->CanTransport()) {
			continue;
		}
		if (unit.Type->UnitType != UnitTypeType::Naval && unit.Type->UnitType != UnitTypeType::Fly && unit.Type->UnitType != UnitTypeType::FlyLow) {
			continue;
		}
		if (unit.CanMove() == false) {
			continue;
		}
		if (!unit.Active) {
			continue;
		}
		if (unit.GroupId != 0) { //don't use units in forces
			continue;
		}
		
		int landmass = CMap::Map.GetTileLandmass(unit.tilePos, unit.MapLayer->ID);
		
		AiPlayer->Transporters[landmass].push_back(&unit);
	}
	//Wyrmgus end
}

int AiGetTransportCapacity(int water_landmass)
{
	int transport_capacity = 0;
	
	for (size_t i = 0; i < AiPlayer->Transporters[water_landmass].size(); ++i) {
		const CUnit &ai_transporter = *AiPlayer->Transporters[water_landmass][i];
		transport_capacity += ai_transporter.Type->MaxOnBoard - ai_transporter.BoardCount;
	}
	
	return transport_capacity;
}

int AiGetRequestedTransportCapacity(int water_landmass)
{
	int transport_capacity = 0;
	
	for (unsigned int i = 0; i < AiPlayer->UnitTypeBuilt.size(); ++i) { //count transport capacity under construction to see if should request more
		const AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[i];
		if (queue.Landmass == water_landmass && queue.Type->CanTransport() && (queue.Type->UnitType == UnitTypeType::Naval || queue.Type->UnitType == UnitTypeType::Fly || queue.Type->UnitType == UnitTypeType::FlyLow)) {
			transport_capacity += queue.Want * queue.Type->MaxOnBoard;
		}
	}
	
	return transport_capacity;
}
//Wyrmgus end
