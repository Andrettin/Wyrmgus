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
//      (c) Copyright 2002-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/actions.h"
#include "commands.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "missile/missile_type.h"
#include "pathfinder/pathfinder.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

class _EnemyOnMapTile
{
public:
	_EnemyOnMapTile(const CUnit &unit, const Vector2i &_pos, CUnit **enemy) :
		source(&unit) , pos(_pos), best(enemy)
	{
	}

	void operator()(CUnit *const unit) const
	{
		const CUnitType &type = *unit->GetType();
		// unusable unit ?
		// if (unit->IsUnusable()) can't attack constructions
		// FIXME: did SelectUnitsOnTile already filter this?
		// Invisible and not Visible
		if (unit->Removed || unit->Variable[INVISIBLE_INDEX].Value
			// || (!UnitVisible(unit, source->Player))
			|| unit->CurrentAction() == UnitActionDie) {
			return;
		}
		if (unit->GetType()->UnitType == UnitTypeFly && unit->IsAgressive() == false) {
			return;
		}
		if (pos.x < unit->GetTilePos().x || pos.x >= unit->GetTilePos().x + type.GetTileSize().x
			|| pos.y < unit->GetTilePos().y || pos.y >= unit->GetTilePos().y + type.GetTileSize().y) {
			return;
		}
		if (!CanTarget(*source->GetType(), type)) {
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
	const Vector2i pos;
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
static CUnit *EnemyOnMapTile(const CUnit &source, const Vector2i &pos, const int z)
{
	CUnit *enemy = nullptr;

	_EnemyOnMapTile filter(source, pos, &enemy);
	CMap::Map.Field(pos, z)->UnitCache.for_each(filter);
	return enemy;
}

class WallFinder
{
public:
	WallFinder(const CUnit &unit, int maxDist, Vector2i *resultPos) :
		//Wyrmgus start
		unit(unit),
		//Wyrmgus end
		maxDist(maxDist),
		movemask(unit.GetType()->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		resultPos(resultPos)
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	int maxDist;
	int movemask;
	Vector2i *resultPos;
};

VisitResult WallFinder::Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from)
{
	if (!unit.GetMapLayer()->Field(pos)->playerInfo.IsTeamExplored(*unit.GetPlayer())) {
		return VisitResult_DeadEnd;
	}
	// Look if found what was required.
	if (unit.GetMapLayer()->WallOnMap(pos)) {
		DebugPrint("Wall found %d, %d\n" _C_ pos.x _C_ pos.y);
		if (resultPos) {
			*resultPos = from;
		}
		return VisitResult_Finished;
	}
	if (unit.GetMapLayer()->Field(pos)->CheckMask(movemask)) { // reachable
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult_Ok;
		} else {
			return VisitResult_DeadEnd;
		}
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

static bool FindWall(const CUnit &unit, int range, Vector2i *wallPos)
{
	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(unit.GetMapLayer()->GetWidth(), unit.GetMapLayer()->GetHeight());
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
		if (aiunit->GetType()->UnitType == UnitTypeLand) {
			unit = aiunit;
			//Wyrmgus start
//			if (aiunit->GetType()->Missile.Missile->Range == 1) {
			if (aiunit->GetMissile().Missile->Range == 1) {
			//Wyrmgus end
				break;
			}
		}
	}
	const int maxRange = 1000;
	Vector2i wallPos;

	if (FindWall(*unit, maxRange, &wallPos)) {
		force->State = AiForceAttackingState_Waiting;
		for (unsigned int i = 0; i < force->Units.size(); ++i) {
			CUnit &aiunit = *force->Units[i];
			//Wyrmgus start
//			if (aiunit.GetType()->CanAttack) {
			if (aiunit.CanAttack()) {
			//Wyrmgus end
				CommandAttack(aiunit, wallPos, nullptr, FlushCommands, aiunit.GetMapLayer()->GetIndex());
			} else {
				CommandMove(aiunit, wallPos, FlushCommands, aiunit.GetMapLayer()->GetIndex());
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
		movemask(unit.GetType()->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit))
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	int movemask;
};

VisitResult ReachableTerrainMarker::Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from)
{
	if (!unit.GetMapLayer()->Field(pos)->playerInfo.IsTeamExplored(*unit.GetPlayer())) {
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	if (CanMoveToMask(pos, movemask, unit.GetMapLayer()->GetIndex())) { // reachable
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
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
	EnemyFinderWithTransporter(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vector2i *resultPos) :
		unit(unit),
		terrainTransporter(terrainTransporter),
		movemask(unit.GetType()->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		resultPos(resultPos)
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from);
private:
	bool IsAccessibleForTransporter(const Vector2i &pos) const;
private:
	const CUnit &unit;
	const TerrainTraversal &terrainTransporter;
	int movemask;
	Vector2i *resultPos;
};

bool EnemyFinderWithTransporter::IsAccessibleForTransporter(const Vector2i &pos) const
{
	return terrainTransporter.IsReached(pos);
}

VisitResult EnemyFinderWithTransporter::Visit(TerrainTraversal &terrainTraversal, const Vector2i &pos, const Vector2i &from)
{
	if (!unit.GetMapLayer()->Field(pos)->playerInfo.IsTeamExplored(*unit.GetPlayer())) {
		return VisitResult_DeadEnd;
	}

	if (EnemyOnMapTile(unit, pos, unit.GetMapLayer()->GetIndex()) && CanMoveToMask(from, movemask, unit.GetMapLayer()->GetIndex())) {
		DebugPrint("Target found %d,%d\n" _C_ pos.x _C_ pos.y);
		*resultPos = pos;
		return VisitResult_Finished;
	}

	if (CanMoveToMask(pos, movemask, unit.GetMapLayer()->GetIndex()) || IsAccessibleForTransporter(pos)) { // reachable
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

//Wyrmgus start
//static bool AiFindTarget(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vector2i *resultPos)
static bool AiFindTarget(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vector2i *resultPos)
//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(unit.GetMapLayer()->GetWidth(), unit.GetMapLayer()->GetHeight());
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
		return unit->GetType()->CanMove() && unit->BoardCount < unit->GetType()->MaxOnBoard;
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
			totalBoardCapacity += unit->GetType()->MaxOnBoard - unit->BoardCount;
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
**  @todo transporter are more selective now (flag with unittypeland).
**         We must manage it.
*/
int AiForce::PlanAttack()
{
	CPlayer &player = *AiPlayer->Player;
	DebugPrint("%d: Planning for force #%lu of player #%d\n" _C_ player.GetIndex()
			   _C_(long unsigned int)(this - & (AiPlayer->Force[0])) _C_ player.GetIndex());

	TerrainTraversal transporterTerrainTraversal;

	//Wyrmgus start
//	transporterTerrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	transporterTerrainTraversal.SetSize(CMap::Map.Info.MapWidths[this->GoalMapLayer], CMap::Map.Info.MapHeights[this->GoalMapLayer]);
	//Wyrmgus end
	transporterTerrainTraversal.Init();

	CUnit *transporter = Units.find(IsAFreeTransporter());

	if (transporter != nullptr) {
		DebugPrint("%d: Transporter #%d\n" _C_ player.GetIndex() _C_ UnitNumber(*transporter));
		MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
	} else {
		std::vector<CUnit *>::iterator it = std::find_if(player.UnitBegin(), player.UnitEnd(), IsAFreeTransporter());
		if (it != player.UnitEnd()) {
			transporter = *it;
			MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
		} else {
			DebugPrint("%d: No transporter available\n" _C_ player.GetIndex());
			return 0;
		}
	}

	// Find a land unit of the force.
	// FIXME: if force is split over different places -> broken
	CUnit *landUnit = Units.find(CUnitTypeFinder(UnitTypeLand));
	if (landUnit == nullptr) {
		DebugPrint("%d: No land unit in force\n" _C_ player.GetIndex());
		return 0;
	}

	Vector2i pos = this->GoalPos;

	if (AiFindTarget(*landUnit, transporterTerrainTraversal, &pos)) {
		const unsigned int forceIndex = AiPlayer->Force.getIndex(this) + 1;

		if (transporter->GroupId != forceIndex) {
			DebugPrint("%d: Assign any transporter #%d\n" _C_ player.GetIndex() _C_ UnitNumber(*transporter));

			if (transporter->GroupId) {
				transporter->GetPlayer()->Ai->Force[transporter->GroupId - 1].Remove(*transporter);
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
				totalBoardCapacity -= unit.GetType()->GetBoardSize();
			}
		}
		if (totalBoardCapacity < 0) { // Not enough transporter.
			IsAFreeTransporter isAFreeTransporter;
			// Add all other idle transporter.
			for (int i = 0; i < player.GetUnitCount(); ++i) {
				CUnit &unit = player.GetUnit(i);

				if (isAFreeTransporter(&unit) && unit.GroupId == 0 && unit.IsIdle()) {
					DebugPrint("%d: Assign any transporter #%d\n" _C_ player.GetIndex() _C_ UnitNumber(unit));
					Insert(unit);
					unit.GroupId = forceIndex;
					totalBoardCapacity += unit.GetType()->MaxOnBoard - unit.BoardCount;
					if (totalBoardCapacity >= 0) {
						break;
					}
				}
			}
		}
		DebugPrint("%d: Can attack\n" _C_ player.GetIndex());
		GoalPos = pos;
		State = AiForceAttackingState_Boarding;
		return 1;
	}
	return 0;
}

//Wyrmgus start
/*
static bool ChooseRandomUnexploredPositionNear(const Vector2i &center, Vector2i *pos)
{
	Assert(pos != nullptr);

	int ray = 3;
	const int maxTryCount = 8;
	for (int i = 0; i != maxTryCount; ++i) {
		pos->x = center.x + SyncRand() % (2 * ray + 1) - ray;
		pos->y = center.y + SyncRand() % (2 * ray + 1) - ray;

		if (CMap::Map.Info.IsPointOnMap(*pos)
			&& CMap::Map.Field(*pos)->playerInfo.IsTeamExplored(*AiPlayer->Player) == false) {
			return true;
		}
		ray = 3 * ray / 2;
	}
	return false;
}

static CUnit *GetBestExplorer(const AiExplorationRequest &request, Vector2i *pos)
{
	// Choose a target, "near"
	const Vector2i &center = request.pos;
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
		if (CMap::Map.Info.IsPointOnMap(unit.GetTilePos()) == false) {
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
		const CUnitType &type = *unit.GetType();

		if (type.UnitType != UnitTypeFly) {
			if (flyeronly) {
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & MapFieldLandUnit) && type.UnitType != UnitTypeLand) {
			if ((request.Mask & MapFieldLandUnit) && type.UnitType != UnitTypeLand && type.UnitType != UnitTypeFlyLow) {
			//Wyrmgus end
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & MapFieldSeaUnit) && type.UnitType != UnitTypeNaval) {
			if ((request.Mask & MapFieldSeaUnit) && type.UnitType != UnitTypeNaval && type.UnitType != UnitTypeFlyLow) {
			//Wyrmgus end
				continue;
			}
		} else {
			flyeronly = true;
		}

		const int sqDistance = SquareDistance(unit.GetTilePos(), *pos);
		if (bestSquareDistance == -1 || sqDistance <= bestSquareDistance
			|| (bestunit->GetType()->UnitType != UnitTypeFly && type.UnitType == UnitTypeFly)) {
			bestSquareDistance = sqDistance;
			bestunit = &unit;
		}
	}
	return bestunit;
}
*/
//Wyrmgus end

//Wyrmgus start
static CUnit *GetBestScout(int unit_type)
{
	CUnit *bestunit = nullptr;

	bool flyeronly = (unit_type == UnitTypeFly);
	
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
		
		const CUnitType &type = *unit.GetType();
		
		if (type.BoolFlag[HARVESTER_INDEX].value) { //don't choose harvesters as scouts (they are already told to explore when they can't find a basic resource)
			continue;
		}

		if (type.UnitType != UnitTypeFly) {
			if (flyeronly) {
				continue;
			}
			if (unit_type == UnitTypeLand && type.UnitType != UnitTypeLand && type.UnitType != UnitTypeFlyLow) {
				continue;
			}
			if (unit_type == UnitTypeNaval && type.UnitType != UnitTypeNaval && type.UnitType != UnitTypeFlyLow) {
				continue;
			}
		} else {
			flyeronly = true;
		}
		
		if (unit.GetType()->CanTransport() && unit.BoardCount) { //if a transporter is carrying a unit, then it shouldn't be used for scouting, as it likely is taking part in an attack
			continue;
		}

		int score = unit.Variable[SIGHTRANGE_INDEX].Value - 4;
		score += unit.Variable[SPEED_INDEX].Value - 10;
		
		if (
			bestunit == nullptr
			|| score > best_score
			|| (bestunit->GetType()->UnitType != UnitTypeFly && type.UnitType == UnitTypeFly)
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

		Vector2i pos;
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
	for (CUnit *scout_unit : AiPlayer->Scouts) {
		if (scout_unit == nullptr) {
			fprintf(stderr, "One of AI Player #%d's scouts is null.\n", AiPlayer->Player->GetIndex());
			return;
		}
		if (scout_unit->GetType()->UnitType == UnitTypeFly) {
			land_scout = true;
			naval_scout = true;
			air_scout = true;
			break;
		} else if (scout_unit->GetType()->UnitType == UnitTypeFlyLow) {
			land_scout = true;
			naval_scout = true;
		} else if (scout_unit->GetType()->UnitType == UnitTypeLand) {
			land_scout = true;
		} else if (scout_unit->GetType()->UnitType == UnitTypeNaval) {
			naval_scout = true;
		}
	}
		
	//if no scouts are already present for a particular type, then choose a suitable unit to scout
	if (!air_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeFly);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			land_scout = true;
			naval_scout = true;
		}
	}
	if (!land_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeLand);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			if (bestunit->GetType()->UnitType == UnitTypeFlyLow) {
				naval_scout = true;
			}
		}
	}
	if (!naval_scout) { 
		CUnit *bestunit = GetBestScout(UnitTypeNaval);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
		}
	}
	
	for (CUnit *scout_unit : AiPlayer->Scouts) {
		// move AI scouts
		if (scout_unit->IsIdle()) {
			scout_unit->Scout();
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
					CommandUnload(ai_transporter, ai_transporter.GetTilePos(), uins, 0, ai_transporter.GetMapLayer()->GetIndex());
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
		if (!unit.GetType()->CanTransport()) {
			continue;
		}
		if (unit.GetType()->UnitType != UnitTypeNaval && unit.GetType()->UnitType != UnitTypeFly && unit.GetType()->UnitType != UnitTypeFlyLow) {
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
		
		int landmass = CMap::Map.GetTileLandmass(unit.GetTilePos(), unit.GetMapLayer()->GetIndex());
		
		AiPlayer->Transporters[landmass].push_back(&unit);
	}
	//Wyrmgus end
}

int AiGetTransportCapacity(int water_landmass)
{
	int transport_capacity = 0;
	
	for (size_t i = 0; i < AiPlayer->Transporters[water_landmass].size(); ++i) {
		const CUnit &ai_transporter = *AiPlayer->Transporters[water_landmass][i];
		transport_capacity += ai_transporter.GetType()->MaxOnBoard - ai_transporter.BoardCount;
	}
	
	return transport_capacity;
}

int AiGetRequestedTransportCapacity(int water_landmass)
{
	int transport_capacity = 0;
	
	for (unsigned int i = 0; i < AiPlayer->UnitTypeBuilt.size(); ++i) { //count transport capacity under construction to see if should request more
		const AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[i];
		if (queue.Landmass == water_landmass && queue.Type->CanTransport() && (queue.Type->UnitType == UnitTypeNaval || queue.Type->UnitType == UnitTypeFly || queue.Type->UnitType == UnitTypeFlyLow)) {
			transport_capacity += queue.Want * queue.Type->MaxOnBoard;
		}
	}
	
	return transport_capacity;
}
//Wyrmgus end
