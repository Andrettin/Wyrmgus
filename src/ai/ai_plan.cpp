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
//      (c) Copyright 2002-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "actions.h"
#include "commands.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "pathfinder.h"
#include "player/player.h"
#include "spell/status_effect.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_domain_finder.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"

class _EnemyOnMapTile
{
public:
	_EnemyOnMapTile(const CUnit &unit, const Vec2i _pos, CUnit **enemy) :
		source(&unit) , pos(_pos), best(enemy)
	{
	}

	void operator()(CUnit *const unit) const
	{
		const wyrmgus::unit_type &type = *unit->Type;

		// unusable unit ?
		// if (unit->IsUnusable()) can't attack constructions
		// FIXME: did SelectUnitsOnTile already filter this?
		// Invisible and not Visible
		if (unit->Removed || unit->has_status_effect(status_effect::invisible)
			// || (!UnitVisible(unit, source->Player))
			|| unit->CurrentAction() == UnitAction::Die) {
			return;
		}

		if (unit->Type->get_domain() == unit_domain::air && unit->IsAgressive() == false) {
			return;
		}

		if (pos.x < unit->tilePos.x || pos.x >= unit->tilePos.x + type.get_tile_width()
			|| pos.y < unit->tilePos.y || pos.y >= unit->tilePos.y + type.get_tile_height()) {
			return;
		}

		if (!source->Type->can_target(&type)) {
			return;
		}

		//Wyrmgus start
//		if (!source->Player->is_enemy_of(*unit)) { // a friend or neutral
		if (!source->is_enemy_of(*unit)) { // a friend or neutral
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
//	CMap::get()->Field(pos)->UnitCache.for_each(filter);
	CMap::get()->Field(pos, z)->UnitCache.for_each(filter);
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
		movemask(unit.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)),
		resultPos(resultPos)
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	int maxDist;
	tile_flag movemask;
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
	if (!unit.MapLayer->Field(pos)->player_info->IsTeamExplored(*unit.Player)) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	// Look if found what was required.
	//Wyrmgus start
//	if (CMap::get()->WallOnMap(pos)) {
	if (CMap::get()->WallOnMap(pos, unit.MapLayer->ID)) {
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
	CUnit *unit = *force->get_units()[0];
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : force->get_units()) {
		CUnit *ai_unit = unit_ref->get();
		if (ai_unit->Type->get_domain() == unit_domain::land) {
			unit = ai_unit;
			if (ai_unit->GetMissile().Missile->get_range() == 1) {
				break;
			}
		}
	}

	static constexpr int maxRange = 1000;

	Vec2i wallPos;

	if (FindWall(*unit, maxRange, &wallPos)) {
		force->State = AiForceAttackingState::Waiting;
		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : force->get_units()) {
			CUnit *ai_unit = unit_ref->get();
			//Wyrmgus start
//			if (ai_unit->Type->CanAttack) {
			if (ai_unit->CanAttack()) {
			//Wyrmgus end
				CommandAttack(*ai_unit, wallPos, nullptr, FlushCommands, ai_unit->MapLayer->ID);
			} else {
				CommandMove(*ai_unit, wallPos, FlushCommands, ai_unit->MapLayer->ID);
			}
		}
		return 1;
	}
	return 0;
}

class ReachableTerrainMarker
{
public:
	explicit ReachableTerrainMarker(const CUnit &unit) :
		//Wyrmgus start
		unit(unit),
		//Wyrmgus end
		movemask(unit.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit))
	{
	}

	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	//Wyrmgus start
	const CUnit &unit;
	//Wyrmgus end
	tile_flag movemask;
};

VisitResult ReachableTerrainMarker::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	if (!unit.MapLayer->Field(pos)->player_info->IsTeamExplored(*unit.Player)) {
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
	explicit EnemyFinderWithTransporter(const CUnit &unit, const TerrainTraversal &terrainTransporter, Vec2i *resultPos) :
		unit(unit),
		terrainTransporter(terrainTransporter),
		movemask(unit.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)),
		resultPos(resultPos)
	{
	}

	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);

private:
	bool IsAccessibleForTransporter(const Vec2i &pos) const;

private:
	const CUnit &unit;
	const TerrainTraversal &terrainTransporter;
	tile_flag movemask;
	Vec2i *resultPos;
};

bool EnemyFinderWithTransporter::IsAccessibleForTransporter(const Vec2i &pos) const
{
	return terrainTransporter.IsReached(pos);
}

VisitResult EnemyFinderWithTransporter::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)

	if (!unit.MapLayer->Field(pos)->player_info->IsTeamExplored(*unit.Player)) {
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

static bool is_a_free_transporter(const CUnit *unit)
{
	return unit->Type->CanMove() && unit->BoardCount < unit->Type->MaxOnBoard;
}

static bool is_a_free_transporter_ref(const std::shared_ptr<wyrmgus::unit_ref> &unit_ref)
{
	return is_a_free_transporter(unit_ref->get());
}

template <typename ITERATOR>
int GetTotalBoardCapacity(ITERATOR begin, ITERATOR end)
{
	int totalBoardCapacity = 0;
	for (ITERATOR it = begin; it != end; ++it) {
		const std::shared_ptr<wyrmgus::unit_ref> &unit_ref = *it;
		const CUnit *unit = unit_ref->get();

		if (is_a_free_transporter(unit)) {
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
**  @todo transporter are more selective now (flag with unit_domain::land).
**         We must manage it.
*/
int AiForce::PlanAttack()
{
	CPlayer &player = *AiPlayer->Player;
	DebugPrint("%d: Planning for force #%lu of player #%d\n" _C_ player.get_index()
			   _C_(long unsigned int)(this - & (AiPlayer->Force[0])) _C_ player.get_index());

	TerrainTraversal transporterTerrainTraversal;

	//Wyrmgus start
//	transporterTerrainTraversal.SetSize(CMap::get()->Info->MapWidth, Map.Info.MapHeight);
	transporterTerrainTraversal.SetSize(CMap::get()->Info->MapWidths[this->GoalMapLayer], CMap::get()->Info->MapHeights[this->GoalMapLayer]);
	//Wyrmgus end
	transporterTerrainTraversal.Init();

	CUnit *transporter = nullptr;

	const auto transporter_it = std::find_if(this->get_units().begin(), this->get_units().end(), is_a_free_transporter_ref);
	if (transporter_it != this->get_units().end()) {
		transporter = **transporter_it;
	}

	if (transporter != nullptr) {
		DebugPrint("%d: Transporter #%d\n" _C_ player.get_index() _C_ UnitNumber(*transporter));
		MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
	} else {
		std::vector<CUnit *>::const_iterator it = std::find_if(player.UnitBegin(), player.UnitEnd(), is_a_free_transporter);
		if (it != player.UnitEnd()) {
			transporter = *it;
			MarkReacheableTerrainType(*transporter, &transporterTerrainTraversal);
		} else {
			DebugPrint("%d: No transporter available\n" _C_ player.get_index());
			return 0;
		}
	}

	// Find a land unit of the force.
	// FIXME: if force is split over different places -> broken
	const auto land_unit_it = std::find_if(this->get_units().begin(), this->get_units().end(), unit_domain_finder(unit_domain::land));
	if (land_unit_it == this->get_units().end()) {
		DebugPrint("%d: No land unit in force\n" _C_ player.get_index());
		return 0;
	}

	CUnit *landUnit = **land_unit_it;

	Vec2i pos = this->GoalPos;

	if (AiFindTarget(*landUnit, transporterTerrainTraversal, &pos)) {
		const unsigned int forceIndex = AiPlayer->Force.getIndex(this) + 1;

		if (transporter->GroupId != forceIndex) {
			DebugPrint("%d: Assign any transporter #%d\n" _C_ player.get_index() _C_ UnitNumber(*transporter));

			if (transporter->GroupId) {
				transporter->Player->Ai->Force[transporter->GroupId - 1].Remove(transporter);
			}
			this->Insert(transporter);
			transporter->GroupId = forceIndex;
		}

		int totalBoardCapacity = GetTotalBoardCapacity(this->get_units().begin(), this->get_units().end());

		// Verify we have enough transporter.
		// @note: Minimal check for unitType (flyers...)
		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
			CUnit *unit = unit_ref->get();
			if (CanTransport(*transporter, *unit)) {
				totalBoardCapacity -= unit->Type->BoardSize;
			}
		}

		if (totalBoardCapacity < 0) { // Not enough transporter.
			// Add all other idle transporter.
			for (int i = 0; i < player.GetUnitCount(); ++i) {
				CUnit &unit = player.GetUnit(i);

				if (is_a_free_transporter(&unit) && unit.GroupId == 0 && unit.IsIdle()) {
					DebugPrint("%d: Assign any transporter #%d\n" _C_ player.get_index() _C_ UnitNumber(unit));
					this->Insert(&unit);
					unit.GroupId = forceIndex;
					totalBoardCapacity += unit.Type->MaxOnBoard - unit.BoardCount;
					if (totalBoardCapacity >= 0) {
						break;
					}
				}
			}
		}
		DebugPrint("%d: Can attack\n" _C_ player.get_index());
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
	assert_throw(pos != nullptr);

	int ray = 3;
	const int maxTryCount = 8;
	for (int i = 0; i != maxTryCount; ++i) {
		pos->x = center.x + SyncRand(2 * ray + 1) - ray;
		pos->y = center.y + SyncRand(2 * ray + 1) - ray;

		if (Map.Info.IsPointOnMap(*pos)
			&& Map.Field(*pos)->player_info->IsTeamExplored(*AiPlayer->Player) == false) {
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
		const wyrmgus::unit_type &type = *unit.Type;

		if (type.UnitType != unit_domain::air) {
			if (flyeronly) {
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & tile_flag::land_unit) && type.UnitType != unit_domain::land) {
			if ((request.Mask & tile_flag::land_unit) && type.UnitType != unit_domain::land && type.UnitType != unit_domain::air_low) {
			//Wyrmgus end
				continue;
			}
			//Wyrmgus start
//			if ((request.Mask & tile_flag::sea_unit) && type.UnitType != unit_domain::water) {
			if ((request.Mask & tile_flag::sea_unit) && type.UnitType != unit_domain::water && type.UnitType != unit_domain::air_low) {
			//Wyrmgus end
				continue;
			}
		} else {
			flyeronly = true;
		}

		const int sqDistance = SquareDistance(unit.tilePos, *pos);
		if (bestSquareDistance == -1 || sqDistance <= bestSquareDistance
			|| (bestunit->Type->get_domain() != unit_domain::air && type.UnitType == unit_domain::air)) {
			bestSquareDistance = sqDistance;
			bestunit = &unit;
		}
	}
	return bestunit;
}
*/
//Wyrmgus end

//Wyrmgus start
static CUnit *GetBestScout(const unit_domain domain)
{
	CUnit *bestunit = nullptr;

	bool space_only = (domain == unit_domain::space);
	bool flier_only = (domain == unit_domain::air) || space_only;
	
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
			if (force != -1 && CMap::get()->Info->IsPointOnMap(AiPlayer->Force[force].GoalPos, AiPlayer->Force[force].GoalMapLayer)) {
				continue;
			}
		}
		if (unit.Variable[SIGHTRANGE_INDEX].Value < 1) {
			continue; //don't scout with units who have too low a sight range
		}
		if (!unit.IsIdle()) { //don't choose units that aren't idle as scouts
			continue;
		}
		
		const wyrmgus::unit_type &type = *unit.Type;
		
		if (type.BoolFlag[HARVESTER_INDEX].value) { //don't choose harvesters as scouts (they are already told to explore when they can't find a basic resource)
			continue;
		}

		if (type.get_domain() != unit_domain::space) {
			if (space_only) {
				continue;
			}
		} else {
			space_only = true;
		}

		if (type.get_domain() != unit_domain::air && type.get_domain() != unit_domain::space) {
			if (flier_only) {
				continue;
			}
			if (domain == unit_domain::land && type.get_domain() != unit_domain::land && type.get_domain() != unit_domain::air_low) {
				continue;
			}
			if (domain == unit_domain::water && type.get_domain() != unit_domain::water && type.get_domain() != unit_domain::air_low) {
				continue;
			}
		} else {
			flier_only = true;
		}
		
		if (unit.Type->CanTransport() && unit.BoardCount) { //if a transporter is carrying a unit, then it shouldn't be used for scouting, as it likely is taking part in an attack
			continue;
		}

		int score = unit.Variable[SIGHTRANGE_INDEX].Value - 4;
		score += unit.Variable[SPEED_INDEX].Value - 10;
		
		if (
			bestunit == nullptr
			|| score > best_score
			|| (bestunit->Type->get_domain() != unit_domain::air && type.get_domain() == unit_domain::air)
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
		const int requestid = SyncRand(requestcount);
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
	bool space_scout = false;
	for (size_t i = 0; i != AiPlayer->Scouts.size(); ++i) {
		if (AiPlayer->Scouts[i] == nullptr) {
			fprintf(stderr, "AI Player #%d's scout %d is null.\n", AiPlayer->Player->get_index(), (int) i);
			return;
		}
		if (AiPlayer->Scouts[i]->Type->get_domain() == unit_domain::space) {
			land_scout = true;
			naval_scout = true;
			air_scout = true;
			space_scout = true;
			break;
		} else if (AiPlayer->Scouts[i]->Type->get_domain() == unit_domain::air) {
			land_scout = true;
			naval_scout = true;
			air_scout = true;
			break;
		} else if (AiPlayer->Scouts[i]->Type->get_domain() == unit_domain::air_low) {
			land_scout = true;
			naval_scout = true;
		} else if (AiPlayer->Scouts[i]->Type->get_domain() == unit_domain::land) {
			land_scout = true;
		} else if (AiPlayer->Scouts[i]->Type->get_domain() == unit_domain::water) {
			naval_scout = true;
		}
	}
		
	//if no scouts are already present for a particular type, then choose a suitable unit to scout
	if (!space_scout) { 
		CUnit *bestunit = GetBestScout(unit_domain::space);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			air_scout = true;
			land_scout = true;
			naval_scout = true;
		}
	}
	if (!air_scout) { 
		CUnit *bestunit = GetBestScout(unit_domain::air);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			land_scout = true;
			naval_scout = true;
		}
	}
	if (!land_scout) { 
		CUnit *bestunit = GetBestScout(unit_domain::land);
		if (bestunit != nullptr) {
			AiPlayer->Scouts.push_back(bestunit);
			CommandStopUnit(*bestunit);
			if (bestunit->Type->get_domain() == unit_domain::air_low) {
				naval_scout = true;
			}
		}
	}
	if (!naval_scout) { 
		CUnit *bestunit = GetBestScout(unit_domain::water);
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
	for (const auto &kv_pair : AiPlayer->Transporters) {
		for (CUnit *ai_transporter : kv_pair.second) {
			if (!ai_transporter->IsIdle()) {
				continue;
			}
			
			CUnit *uins = ai_transporter->UnitInside;
			for (int j = 0; j < ai_transporter->InsideCount; ++j, uins = uins->NextContained) {
				if (uins->GroupId == 0) { //if the unit no longer is part of a force, then it likely has been reset and the attack through water has been cancelled, so unload it
					CommandUnload(*ai_transporter, ai_transporter->tilePos, uins, 0, ai_transporter->MapLayer->ID);
				}
			}
		}
	}

	AiPlayer->Transporters.clear();
	for (int i = 0; i != AiPlayer->Player->GetUnitCount(); ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (!unit.IsAliveOnMap()) {
			continue;
		}
		if (!unit.Type->CanTransport()) {
			continue;
		}
		if (unit.Type->get_domain() != unit_domain::water && unit.Type->get_domain() != unit_domain::air && unit.Type->get_domain() != unit_domain::air_low && unit.Type->get_domain() != unit_domain::space) {
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
		
		const landmass *landmass = CMap::get()->get_tile_landmass(unit.tilePos, unit.MapLayer->ID);
		
		AiPlayer->Transporters[landmass].push_back(&unit);
	}
}

int AiGetTransportCapacity(const landmass *water_landmass)
{
	const auto find_iterator = AiPlayer->Transporters.find(water_landmass);

	if (find_iterator == AiPlayer->Transporters.end()) {
		return 0;
	}
	
	int transport_capacity = 0;

	for (const CUnit *ai_transporter : find_iterator->second) {
		transport_capacity += ai_transporter->Type->MaxOnBoard - ai_transporter->BoardCount;
	}
	
	return transport_capacity;
}

int AiGetRequestedTransportCapacity(const landmass *water_landmass)
{
	int transport_capacity = 0;
	
	for (unsigned int i = 0; i < AiPlayer->UnitTypeBuilt.size(); ++i) { //count transport capacity under construction to see if should request more
		const AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[i];
		if (queue.landmass == water_landmass && queue.Type->CanTransport() && (queue.Type->get_domain() == unit_domain::water || queue.Type->get_domain() == unit_domain::air || queue.Type->get_domain() == unit_domain::air_low || queue.Type->get_domain() == unit_domain::space)) {
			transport_capacity += queue.Want * queue.Type->MaxOnBoard;
		}
	}
	
	return transport_capacity;
}
//Wyrmgus end
