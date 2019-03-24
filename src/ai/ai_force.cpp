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
/**@name ai_force.cpp - AI force functions. */
//
//      (c) Copyright 2001-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "action/action_attack.h"
#include "action/action_board.h"
#include "ai/force_template.h"
#include "commands.h"
#include "faction.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder/pathfinder.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/dependency.h"

/*----------------------------------------------------------------------------
--  Types
----------------------------------------------------------------------------*/
constexpr int AIATTACK_RANGE = 0;
constexpr int AIATTACK_ALLMAP = 1;
constexpr int AIATTACK_BUILDING = 2;
constexpr int AIATTACK_AGRESSIVE = 3;

//Wyrmgus start
class EnemyUnitFinder
{
public:
	EnemyUnitFinder(const CUnit &unit, CUnit **result_unit, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, int find_type, bool include_neutral, bool allow_water) :
	//Wyrmgus end
		unit(unit),
		movemask(unit.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)),
		attackrange(unit.GetModifiedVariable(ATTACKRANGE_INDEX)),
		find_type(find_type),
		include_neutral(include_neutral),
		allow_water(allow_water),
		result_unit(result_unit),
		result_enemy_wall_pos(result_enemy_wall_pos),
		result_enemy_wall_map_layer(result_enemy_wall_map_layer)
	{
		*result_unit = nullptr;
	}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &unit;
	unsigned int movemask;
	const int attackrange;
	const int find_type;
	bool include_neutral;
	bool allow_water;
	CUnit **result_unit;
	Vec2i *result_enemy_wall_pos;
	int *result_enemy_wall_map_layer;
};

VisitResult EnemyUnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	if (!unit.MapLayer->Field(pos)->playerInfo.IsTeamExplored(*unit.Player)) {
		return VisitResult_DeadEnd;
	}
	
	if (unit.MapLayer->Field(pos)->CheckMask(MapFieldWall) && !CMap::Map.Info.IsPointOnMap(*result_enemy_wall_pos, *result_enemy_wall_map_layer)) {
		int tile_owner = unit.MapLayer->Field(pos)->Owner;
		if (
			tile_owner != -1
			&& (
				unit.IsEnemy(*CPlayer::Players[tile_owner])
				|| (include_neutral && !unit.IsAllied(*CPlayer::Players[tile_owner]) && unit.Player->Index != tile_owner && !unit.Player->HasBuildingAccess(*CPlayer::Players[tile_owner]))
			)
		) {
			*result_enemy_wall_pos = pos;
			*result_enemy_wall_map_layer = unit.MapLayer->ID;
		}
	}
	
	if (!CanMoveToMask(pos, movemask, unit.MapLayer->ID)) { // unreachable
		if (allow_water) {
			unsigned int water_movemask = movemask;
			water_movemask &= ~(MapFieldWaterAllowed | MapFieldCoastAllowed);
			if (CanMoveToMask(pos, water_movemask, unit.MapLayer->ID)) {
				return VisitResult_Ok; //if movement through water is allowed (with transport ships), then don't make water tiles a dead end, but don't look for units in them either
			}
		}
		
		return VisitResult_DeadEnd;
	}

	std::vector<CUnit *> table;
	Vec2i minpos = pos - Vec2i(attackrange, attackrange);
	Vec2i maxpos = pos + Vec2i(unit.Type->TileSize - 1 + attackrange);
	Select(minpos, maxpos, table, unit.MapLayer->ID, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]));
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit *dest = table[i];
		const CUnitType &dtype = *dest->Type;

		if (!dest->IsAliveOnMap()) {
			continue;
		}
		
		if (
			(
				!unit.IsEnemy(*dest) // a friend or neutral
				&& (!include_neutral || unit.IsAllied(*dest) || unit.Player->Index == dest->Player->Index || unit.Player->HasBuildingAccess(*dest->Player))
			)
			|| !CanTarget(*unit.Type, dtype)
		) {
			continue;
		}

		// Don't attack invulnerable units
		if (dtype.BoolFlag[INDESTRUCTIBLE_INDEX].value || dest->Variable[UNHOLYARMOR_INDEX].Value) {
			continue;
		}
		
		if ((find_type != AIATTACK_BUILDING || dtype.BoolFlag[BUILDING_INDEX].value) && (find_type != AIATTACK_AGRESSIVE || dest->IsAgressive())) {
			*result_unit = dest;
			return VisitResult_Finished;
		} else if (*result_unit == nullptr) { // if trying to search for buildings or aggressive units specifically, still put the first found unit (even if it doesn't fit those parameters) as the result unit, so that it can be returned if no unit with the specified parameters is found
			*result_unit = dest;
		}
	}

	return VisitResult_Ok;
}
//Wyrmgus end

template <const int FIND_TYPE>
class AiForceEnemyFinder
{
public:
	//Wyrmgus start
//	AiForceEnemyFinder(int force, const CUnit **enemy) : enemy(enemy)
	AiForceEnemyFinder(int force, const CUnit **enemy, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, const bool include_neutral, const bool allow_water) : enemy(enemy), result_enemy_wall_pos(result_enemy_wall_pos), result_enemy_wall_map_layer(result_enemy_wall_map_layer), IncludeNeutral(include_neutral), allow_water(allow_water)
	//Wyrmgus end
	{
		Assert(enemy != nullptr);
		*enemy = nullptr;
		AiPlayer->Force[force].Units.for_each_if(*this);
	}

	//Wyrmgus start
//	AiForceEnemyFinder(AiForce &force, const CUnit **enemy) : enemy(enemy)
	AiForceEnemyFinder(AiForce &force, const CUnit **enemy, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, const bool include_neutral, const bool allow_water) : enemy(enemy), result_enemy_wall_pos(result_enemy_wall_pos), result_enemy_wall_map_layer(result_enemy_wall_map_layer), IncludeNeutral(include_neutral), allow_water(allow_water)
	//Wyrmgus end
	{
		Assert(enemy != nullptr);
		*enemy = nullptr;
		force.Units.for_each_if(*this);
	}

	bool found() const { return *enemy != nullptr; }

	//Wyrmgus start
//	bool operator()(const CUnit *const unit) const
	bool operator()(const CUnit *const unit)
	//Wyrmgus end
	{
		//Wyrmgus start
//		if (unit->Type->CanAttack == false) {
		if (unit->CanAttack() == false || std::find(CheckedTypes.begin(), CheckedTypes.end(), unit->Type) != CheckedTypes.end()) { // don't check for multiple units of the same type, since the result will be the same in almost all cases, so we save performance
		//Wyrmgus end
			return *enemy == nullptr;
		}
		//Wyrmgus start
		CheckedTypes.push_back(unit->Type);
		//Wyrmgus end
		if (FIND_TYPE == AIATTACK_RANGE) {
			//Wyrmgus start
//			*enemy = AttackUnitsInReactRange(*unit);
			*enemy = AttackUnitsInReactRange(*unit, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), IncludeNeutral);
			//Wyrmgus end
		//Wyrmgus start
		} else {
			TerrainTraversal terrainTraversal;

			terrainTraversal.SetSize(unit->MapLayer->GetWidth(), unit->MapLayer->GetHeight());
			terrainTraversal.Init();

			terrainTraversal.PushUnitPosAndNeighbor(*unit);

			CUnit *result_unit = nullptr;

			EnemyUnitFinder enemyUnitFinder(*unit, &result_unit, result_enemy_wall_pos, result_enemy_wall_map_layer, FIND_TYPE, IncludeNeutral, allow_water);

			terrainTraversal.Run(enemyUnitFinder);
			*enemy = result_unit;
		//Wyrmgus end
		//Wyrmgus start
		/*
		} else if (FIND_TYPE == AIATTACK_ALLMAP) {
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), false, IncludeNeutral);
			//Wyrmgus end
		} else if (FIND_TYPE == AIATTACK_BUILDING) {
			//Wyrmgus start
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, IsBuildingType());
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), IsBuildingType()), false, IncludeNeutral);
			//Wyrmgus end
			//Wyrmgus start
			//why make sure the enemy is null?
//			Assert(!*enemy);
			//Wyrmgus end
			if (*enemy == nullptr || !(*enemy)->Type->BoolFlag[BUILDING_INDEX].value) {
				//Wyrmgus start
//				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), false, IncludeNeutral);
				//Wyrmgus end
			}
		} else if (FIND_TYPE == AIATTACK_AGRESSIVE) {
			//Wyrmgus start
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, IsAggresiveUnit());
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), IsAggresiveUnit()), false, IncludeNeutral);
			//Wyrmgus end
			//Wyrmgus start
			//why ask that the enemy be null?
//			Assert(!*enemy || (*enemy)->IsAgressive());
			//Wyrmgus end
			if (*enemy == nullptr) {
				//Wyrmgus start
//				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), false, IncludeNeutral);
				//Wyrmgus end
			}
		*/
		//Wyrmgus end
		}
		return *enemy == nullptr;
	}
private:
	const CUnit **enemy;
	//Wyrmgus start
	Vec2i *result_enemy_wall_pos;
	int *result_enemy_wall_map_layer;
	const bool IncludeNeutral;
	const bool allow_water;
	std::vector<const CUnitType *> CheckedTypes;
	//Wyrmgus end
};

class IsAnAlliedUnitOf
{
public:
	explicit IsAnAlliedUnitOf(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsVisibleAsGoal(*player) && (unit->Player->Index == player->Index
												  || unit->IsAllied(*player));
	}
private:
	const CPlayer *player;
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int UnitTypeEquivs[UnitTypeMax + 1]; /// equivalence between unittypes

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Remove any equivalence between unittypes
*/
void AiResetUnitTypeEquiv()
{
	for (int i = 0; i <= UnitTypeMax; ++i) {
		UnitTypeEquivs[i] = i;
	}
}

/**
**  Make two unittypes equivalents from the AI's point of vue
**
**  @param a  the first unittype
**  @param b  the second unittype
*/
void AiNewUnitTypeEquiv(const CUnitType &a, const CUnitType &b)
{
	int find = UnitTypeEquivs[a.Slot];
	int replace = UnitTypeEquivs[b.Slot];

	//always record equivalencies with the lowest unit type
	if (find < replace) {
		std::swap(find, replace);
	}

	// Then just find & replace in UnitTypeEquivs...
	for (unsigned int i = 0; i <= UnitTypeMax; ++i) {
		if (UnitTypeEquivs[i] == find) {
			UnitTypeEquivs[i] = replace;
		}
	}
}


/**
**	Find all unittypes equivalent to a given one
**
**	@param	unit_type	the unit type to find equivalence for
**	@param	result		int array which will hold the result. (Size UnitTypeMax+1)
**
**	@return	the number of unittype found
*/
int AiFindUnitTypeEquiv(const CUnitType &unit_type, int *result)
{
	const int search = UnitTypeEquivs[unit_type.Slot];
	int count = 0;

	for (int i = 0; i < UnitTypeMax + 1; ++i) {
		if (UnitTypeEquivs[i] == search) {
			// Found one
			result[count] = i;
			++count;
		}
	}
	return count;
}

class UnitTypePrioritySorter_Decreasing
{
public:
	bool operator()(int lhs, int rhs) const
	{
		return CUnitType::UnitTypes[lhs]->MapDefaultStat.Variables[PRIORITY_INDEX].Value > CUnitType::UnitTypes[rhs]->MapDefaultStat.Variables[PRIORITY_INDEX].Value;
	}
};

/**
**  Find All unittypes equivalent to a given one, and which are available
**  UnitType are returned in the preferred order (ie palladin >> knight...)
**
**  @param unittype     The unittype to find equivalence for
**  @param usableTypes  int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return             the number of unittype found
*/
int AiFindAvailableUnitTypeEquiv(const CUnitType &unittype, int *usableTypes)
{
	// 1 - Find equivalents
	int usableTypesCount = AiFindUnitTypeEquiv(unittype, usableTypes);
	// 2 - Remove unavailable unittypes
	for (int i = 0; i < usableTypesCount;) {
		if (!CheckDependencies(CUnitType::UnitTypes[usableTypes[i]], AiPlayer->Player)) {
			// Not available, remove it
			usableTypes[i] = usableTypes[usableTypesCount - 1];
			--usableTypesCount;
		} else {
			++i;
		}
	}
	// 3 - Sort by level
	std::sort(usableTypes, usableTypes + usableTypesCount, UnitTypePrioritySorter_Decreasing());
	return usableTypesCount;
}

/* =========================== FORCES ========================== */

class AiForceCounter
{
public:
	AiForceCounter(CUnitCache &units, unsigned int *d, const size_t len) : data(d)
	{
		memset(data, 0, len);
		units.for_each(*this);
	}
	inline void operator()(const CUnit *const unit) const
	{
		data[UnitTypeEquivs[unit->Type->Slot]]++;
		
		int unit_class = unit->Type->Class;
		if (unit_class != -1) {
			for (size_t i = 0; i < ClassUnitTypes[unit_class].size(); ++i) {
				const CUnitType *class_unit_type = ClassUnitTypes[unit_class][i];
				if (class_unit_type != unit->Type) {
					data[UnitTypeEquivs[class_unit_type->Slot]]++; //also increases for other units of the class; shouldn't be a problem because we assume that only one unit type per class would be requested
				}
			}
		}
	}
private:
	unsigned int *data;//[UnitTypeMax + 1];
};

void AiForce::CountTypes(unsigned int *counter, const size_t len)
{
	AiForceCounter(Units, counter, len);
}

/**
**  Check if the units belongs to the force/base.
**
**  @param type   Type to check.
**
**  @return       True if it fits, false otherwise.
*/
bool AiForce::IsBelongsTo(const CUnitType &type)
{
	bool flag = false;
	unsigned int counter[UnitTypeMax + 1];

	// Count units in force.
	CountTypes(counter, sizeof(counter));

	// Look what should be in the force.
	Completed = true;
	for (unsigned int i = 0; i < UnitTypes.size(); ++i) {
		const AiUnitType &aitype = UnitTypes[i];
		const int slot = aitype.Type->Slot;

		if (counter[slot] < aitype.Want) { //the counter includes other units of the same class
			if (UnitTypeEquivs[type.Slot] == slot || type.Class == aitype.Type->Class) {
				if (counter[slot] < aitype.Want - 1) {
					Completed = false;
				}
				flag = true;
			} else {
				Completed = false;
			}
		}
	}
	return flag;
}

void AiForce::Insert(CUnit &unit)
{
	Units.Insert(&unit);
	unit.RefsIncrease();
}

/* static */ void AiForce::InternalRemoveUnit(CUnit *unit)
{
	unit->GroupId = 0;
	unit->RefsDecrease();
}


/**
**  Ai clean units in a force.
*/
void AiForce::RemoveDeadUnit()
{
	// Release all killed units.
	for (unsigned int i = 0; i != Units.size();) {
		CUnit &aiunit = *Units[i];

		if (!aiunit.IsAlive()) {
			InternalRemoveUnit(&aiunit);
			Units.Remove(i);
			continue;
		}
		++i;
	}
}

class AiForceRallyPointFinder
{
public:
	//Wyrmgus start
//	AiForceRallyPointFinder(const CUnit &startUnit, int distance, const Vec2i &startPos, Vec2i *resultPos) :
	AiForceRallyPointFinder(const CUnit &startUnit, int distance, const Vec2i &startPos, Vec2i *resultPos, int z) :
	//Wyrmgus end
		startUnit(startUnit), distance(distance), startPos(startPos),
		movemask(startUnit.Type->MovementMask & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit | MapFieldBuilding)),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos), z(z)
		//Wyrmgus end
	{}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &startUnit;
	const int distance;
	const Vec2i startPos;
	const int movemask;
	Vec2i *resultPos;
	//Wyrmgus start
	const int z;
	//Wyrmgus end
};

VisitResult AiForceRallyPointFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
	if (!CMap::Map.Field(pos, z)->playerInfo.IsTeamExplored(*startUnit.Player)) { // don't pick unexplored positions
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	
	//Wyrmgus start
	if (!CanMoveToMask(pos, movemask, z)) { // unreachable, put this at the beginning to improve performance
		return VisitResult_DeadEnd;
	}
	//Wyrmgus end
	
	const int minDist = 15;
	//Wyrmgus start
//	if (AiEnemyUnitsInDistance(*startUnit.Player, nullptr, pos, minDist) == false
//		&& Distance(pos, startPos) <= abs(distance - minDist)) {
//		*resultPos = pos;
//		return VisitResult_Finished;
//	}
	
	if (AiEnemyUnitsInDistance(*startUnit.Player, nullptr, pos, minDist, z) > 0) { // if there are enemies within the minimum distance here, then it is a dead end
		return VisitResult_DeadEnd;
	}
	
	if (Distance(pos, startPos) <= abs(distance - minDist)) {
		*resultPos = pos;
		return VisitResult_Finished;
	}
	//Wyrmgus end
	
	
	//Wyrmgus start
	/*
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult_Ok;
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
	*/
	
	return VisitResult_Ok;
	//Wyrmgus end
}

//Wyrmgus start
//bool AiForce::NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos)
bool AiForce::NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos, int z)
//Wyrmgus end
{
	Assert(this->Units.size() > 0);
	const CUnit &leader = *(this->Units[0]);
	//Wyrmgus start
//	const int distance = leader.MapDistanceTo(startPos);
	const int distance = leader.MapDistanceTo(startPos, z);
	//Wyrmgus end

	WaitOnRallyPoint = AI_WAIT_ON_RALLY_POINT;

	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	Assert(CMap::Map.Info.IsPointOnMap(startPos, z));
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	AiForceRallyPointFinder aiForceRallyPointFinder(leader, distance, leader.tilePos, resultPos);
	AiForceRallyPointFinder aiForceRallyPointFinder(leader, distance, leader.tilePos, resultPos, z);
	//Wyrmgus end

	return terrainTraversal.Run(aiForceRallyPointFinder);
}

//Wyrmgus start
bool AiForce::CheckTransporters(const Vec2i &pos, int z)
{
	int home_landmass = CMap::Map.GetTileLandmass(this->HomePos, this->HomeMapLayer);
	int goal_landmass = CMap::Map.GetTileLandmass(pos, z);
	int water_landmass = 0;
	for (size_t i = 0; i != CMap::Map.BorderLandmasses[goal_landmass].size(); ++i) {
		if (std::find(CMap::Map.BorderLandmasses[home_landmass].begin(), CMap::Map.BorderLandmasses[home_landmass].end(), CMap::Map.BorderLandmasses[goal_landmass][i]) != CMap::Map.BorderLandmasses[home_landmass].end()) {
			water_landmass = CMap::Map.BorderLandmasses[goal_landmass][i];
			break;
		}
	}
	
	if (!water_landmass) { //not optimal that this is possible, perhaps should give the closest water "landmass" on the way, even if it doesn't border the goal itself?
		return true;
	}
	
	int transport_capacity = AiGetTransportCapacity(water_landmass);
	
	int transport_capacity_needed = 0;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		const CUnit &ai_unit = *this->Units[i];
		if (ai_unit.Removed) { //already in a transporter
			continue;
		}
		
		if (CMap::Map.GetTileLandmass(ai_unit.tilePos, ai_unit.MapLayer->ID) == goal_landmass) { //already unloaded to the enemy's landmass
			continue;
		}
		
		transport_capacity_needed += ai_unit.Type->BoardSize;
	}
	
	int net_transport_capacity_needed = transport_capacity_needed - transport_capacity;
	
	if (net_transport_capacity_needed > 0) {
		net_transport_capacity_needed -= AiGetRequestedTransportCapacity(water_landmass);
		if (net_transport_capacity_needed > 0) { //if the quantity required is still above zero even after counting the transport capacity under construction, then request more
			AiTransportCapacityRequest(net_transport_capacity_needed, water_landmass);
		}
		return false;
	}
	
	//has enough transporters, see then if all units are already boarded
	//put the possible transporters in a vector
	std::vector<CUnit *> transporters;
	for (size_t i = 0; i != AiPlayer->Transporters[water_landmass].size(); ++i) {
		CUnit *ai_transporter = AiPlayer->Transporters[water_landmass][i];
		
		if (!ai_transporter->IsIdle()) { //is already moving, may be going to a unit to transport it
			continue;
		}
		if ((ai_transporter->Type->MaxOnBoard - ai_transporter->BoardCount) <= 0) { //already full
			continue;
		}
		
		if (std::find(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), ai_transporter) != AiPlayer->Scouts.end()) { //the transporters have to stop scouting
			AiPlayer->Scouts.erase(std::remove(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), ai_transporter), AiPlayer->Scouts.end());
		}
		
		transporters.push_back(ai_transporter);
	}
		
	bool has_unboarded_unit = false;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit &ai_unit = *this->Units[i];
		
		if (ai_unit.Removed) { //already in a transporter
			continue;
		}
		
		if (CMap::Map.GetTileLandmass(ai_unit.tilePos, ai_unit.MapLayer->ID) == goal_landmass) { //already unloaded to the enemy's landmass
			continue;
		}
		
		has_unboarded_unit = true;
		
		if (!ai_unit.IsIdle()) { //if the unit isn't idle, don't give it the order to board (it may already be boarding a transporter)
			continue;
		}
		
		const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
		ai_unit.Wait += delay;

		for (size_t j = 0; j != transporters.size(); ++j) {
			CUnit *ai_transporter = transporters[j];
			
			if ((ai_transporter->Type->MaxOnBoard - ai_transporter->BoardCount) < ai_unit.Type->BoardSize) { //the unit's board size is too big to fit in the transporter
				continue;
			}
			
			ai_transporter->Wait += delay;
		
			CommandBoard(ai_unit, *ai_transporter, FlushCommands);
			CommandFollow(*ai_transporter, ai_unit, FlushCommands);
			transporters.erase(std::remove(transporters.begin(), transporters.end(), ai_transporter), transporters.end()); //only tell one unit to board a particular transporter at a single time, to avoid units getting in the way of one another
			break;
		}
		
		if (transporters.size() == 0) {
			break;
		}
	}
	
	//everyone is already boarded, time to move to the enemy shores!
	bool has_loaded_unit = false;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit &ai_unit = *this->Units[i];
		
		if (CMap::Map.GetTileLandmass(ai_unit.tilePos, ai_unit.MapLayer->ID) == goal_landmass) { //already unloaded to the enemy's landmass
			continue;
		}

		has_loaded_unit = true;

		if (ai_unit.Container) { //in a transporter
			if (!ai_unit.Container->IsIdle()) { //already moving (presumably to unload)
				continue;
			}
			
			if (ai_unit.Container->Type->MaxOnBoard > ai_unit.Container->BoardCount && has_unboarded_unit) { //send the transporter to unload units if it is full, or if all of the force's units are already boarded
				//sending a transporter that is full to unload, even if all of the force's units aren't boarded yet, is useful to prevent transporter congestion
				continue;
			}
			
			const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
			ai_unit.Container->Wait += delay;
			//tell the transporter to unload to the goal pos
			CommandUnload(*ai_unit.Container, pos, nullptr, FlushCommands, z, goal_landmass);
		}
	}
	
	if (has_unboarded_unit || has_loaded_unit) {
		return false;
	}
	
	return true;
}
//Wyrmgus end

int AiForce::GetForceType() const
{
	if (this->Units.size() == 0) {
		return LandForceType;
	}
	
	int force_type = AirForceType;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];
		
		if (unit->Type->UnitType == UnitTypeNaval && unit->CanAttack() && !unit->Type->CanTransport()) { //one naval unit that can attack makes this a naval force
			return NavalForceType;
		}
		
		if (unit->Type->UnitType != UnitTypeFly) { //all units must be of UnitTypeFly for it to be considered an air force
			force_type = LandForceType;
		}
	}
	
	return force_type;
}

bool AiForce::IsNaval() const
{
	return this->GetForceType() == NavalForceType;
}

bool AiForce::IsAirForce() const
{
	return this->GetForceType() == AirForceType;
}

bool AiForce::IsHeroOnlyForce() const
{
	if (this->Units.size() == 0) {
		return false;
	}
	
	for (size_t i = 0; i < this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];
		if (!unit->Character) {
			return false;
		}
	}
	
	return true;
}

//Wyrmgus start
//void AiForce::Attack(const Vec2i &pos)
void AiForce::Attack(const Vec2i &pos, int z)
//Wyrmgus end
{
	bool isDefenceForce = false;
	RemoveDeadUnit();

	if (Units.size() == 0) {
		this->Attacking = false;
		//Wyrmgus start
		AiPlayer->Scouting = false;
		//Wyrmgus end
		this->State = AiForceAttackingState_Waiting;
		return;
	}
	//Wyrmgus start
	if (AiPlayer->Scouting) {
		return;
	}
	//Wyrmgus end
	if (!this->Attacking) {
		// Remember the original force position so we can return there after attack
		if (this->Role == AiForceRoleDefend
			|| (this->Role == AiForceRoleAttack && this->State == AiForceAttackingState_Waiting)) {
			this->HomePos = this->Units[this->Units.size() - 1]->tilePos;
			this->HomeMapLayer = this->Units[this->Units.size() - 1]->MapLayer->ID;
		}
		this->Attacking = true;
	}
	Vec2i goalPos(pos);

	bool isNaval = this->IsNaval();
	bool isTransporter = false;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];
		//Wyrmgus start
//		if (unit->Type->CanTransport() && unit->IsAgressive() == false) {
		if (unit->Type->CanTransport()) {
		//Wyrmgus end
			isTransporter = true;
			break;
		}
	}
	if (CMap::Map.Info.IsPointOnMap(goalPos, z) == false) {
		//Wyrmgus start
		bool include_neutral = AiPlayer->Player->AtPeace();
		//Wyrmgus end
		/* Search in entire map */
		const CUnit *enemy = nullptr;
		Vec2i enemy_wall_pos(-1, -1);
		int enemy_wall_map_layer = -1;
		if (isTransporter) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_AGRESSIVE>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_AGRESSIVE>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, false);
			//Wyrmgus end
		} else if (isNaval) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, true);
			//Wyrmgus end
		}
		if (enemy) {
			goalPos = enemy->tilePos;
			//Wyrmgus start
			z = enemy->MapLayer->ID;
			if (!AiPlayer->Player->IsEnemy(*enemy->Player) && enemy->Player->Type != PlayerNeutral) {
				AiPlayer->Player->SetDiplomacyEnemyWith(*enemy->Player);
				if (AiPlayer->Player->IsSharedVision(*enemy->Player)) {
					CommandSharedVision(AiPlayer->Player->Index, false, enemy->Player->Index);
				}
			}
			//Wyrmgus end
		//Wyrmgus start
		} else if (CMap::Map.Info.IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
			goalPos = enemy_wall_pos;
			z = enemy_wall_map_layer;
			int enemy_wall_owner = CMap::Map.Field(enemy_wall_pos, enemy_wall_map_layer)->Owner;
			if (!AiPlayer->Player->IsEnemy(*CPlayer::Players[enemy_wall_owner]) && CPlayer::Players[enemy_wall_owner]->Type != PlayerNeutral) {
				AiPlayer->Player->SetDiplomacyEnemyWith(*CPlayer::Players[enemy_wall_owner]);
				if (AiPlayer->Player->IsSharedVision(*CPlayer::Players[enemy_wall_owner])) {
					CommandSharedVision(AiPlayer->Player->Index, false, CPlayer::Players[enemy_wall_owner]->Index);
				}
			}
		} else {
			AiPlayer->Scouting = true;			
			return;
		//Wyrmgus end
		}
	} else {
		isDefenceForce = true;
	}
	//Wyrmgus start
	//if one of the force's units is being used as a scout, stop its function as a scout when the force is used to attack
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *unit = this->Units[i];
		if (std::find(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), unit) != AiPlayer->Scouts.end()) {
			AiPlayer->Scouts.erase(std::remove(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), unit), AiPlayer->Scouts.end());
		}
	}
	//Wyrmgus end
	if (CMap::Map.Info.IsPointOnMap(goalPos, z) == false || isTransporter) {
		DebugPrint("%d: Need to plan an attack with transporter\n" _C_ AiPlayer->Player->Index);
		if (State == AiForceAttackingState_Waiting && !PlanAttack()) {
			DebugPrint("%d: Can't transport\n" _C_ AiPlayer->Player->Index);
			Attacking = false;
		}
		return;
	}
	
	//Wyrmgus start
	if (!isTransporter && !isNaval) {
		bool needs_transport = false;
		for (size_t i = 0; i != this->Units.size(); ++i) {
			CUnit *const unit = this->Units[i];

			if (unit->Type->UnitType != UnitTypeFly && unit->Type->UnitType != UnitTypeFlyLow && CMap::Map.GetTileLandmass(unit->tilePos, unit->MapLayer->ID) != CMap::Map.GetTileLandmass(goalPos, z)) {
				needs_transport = true;
				break;
			}
		}
		
		if (needs_transport) { //if is a land force attacking another landmass, see if there are enough transporters to carry it, and whether they are doing the appropriate actions
			if (!this->CheckTransporters(goalPos, z)) {
				this->GoalPos = goalPos;
				this->GoalMapLayer = z;
				return;
			}
		}
	}
	//Wyrmgus end
	
	if (this->State == AiForceAttackingState_Waiting && isDefenceForce == false) {
		Vec2i resultPos;
		//Wyrmgus start
//		NewRallyPoint(goalPos, &resultPos);
		NewRallyPoint(goalPos, &resultPos, z);
		if (resultPos.x == 0 && resultPos.y == 0) {
			resultPos = goalPos;
		}
		//Wyrmgus end
		this->GoalPos = resultPos;
		//Wyrmgus start
		this->GoalMapLayer = z;
		//Wyrmgus end
		this->State = AiForceAttackingState_GoingToRallyPoint;
	} else {
		this->GoalPos = goalPos;
		//Wyrmgus start
		this->GoalMapLayer = z;
		//Wyrmgus end
		this->State = AiForceAttackingState_Attacking;
	}
	//  Send all units in the force to enemy.
	
	CUnit *leader = nullptr;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];

		if (unit->IsAgressive()) {
			leader = unit;
			break;
		}
	}

	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];
		
		//Wyrmgus start
		if (!unit->IsIdle()) {
			continue;
		}
		//Wyrmgus end
		
		if (unit->Container == nullptr) {
			//Wyrmgus start
//			const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
			const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
			//Wyrmgus end

			//Wyrmgus start
//			unit->Wait = delay;
			unit->Wait += delay;
			//Wyrmgus end
			if (unit->IsAgressive()) {
				//Wyrmgus start
//				CommandAttack(*unit, this->GoalPos,  nullptr, FlushCommands);
				CommandAttack(*unit, this->GoalPos,  nullptr, FlushCommands, this->GoalMapLayer);
				//Wyrmgus end
			} else {
				if (leader) {
					CommandDefend(*unit, *leader, FlushCommands);
				} else {
					//Wyrmgus start
//					CommandMove(*unit, this->GoalPos, FlushCommands);
					CommandMove(*unit, this->GoalPos, FlushCommands, this->GoalMapLayer);
					//Wyrmgus end
				}
			}
		}
	}
}

void AiForce::ReturnToHome()
{
	if (CMap::Map.Info.IsPointOnMap(this->HomePos, this->HomeMapLayer)) {
		for (size_t i = 0; i != this->Units.size(); ++i) {
			CUnit &unit = *this->Units[i];
			
			//Wyrmgus start
			if (!unit.IsIdle()) {
				continue;
			}
			//Wyrmgus end
		
			//Wyrmgus start
//			CommandMove(unit, this->HomePos, FlushCommands);
			CommandMove(unit, this->HomePos, FlushCommands, this->HomeMapLayer);
			//Wyrmgus end
		}
	}
	const Vec2i invalidPos(-1, -1);

	this->HomePos = invalidPos;
	this->GoalPos = invalidPos;
	//Wyrmgus start
	this->HomeMapLayer = 0;
	this->GoalMapLayer = 0;
	//Wyrmgus end
	this->Defending = false;
	this->Attacking = false;
	//Wyrmgus start
	AiPlayer->Scouting = false;
	//Wyrmgus end
	this->State = AiForceAttackingState_Waiting;
}

AiForceManager::AiForceManager()
{
	forces.resize(AI_MAX_FORCES);
	memset(script, -1, AI_MAX_FORCES * sizeof(char));
}

unsigned int AiForceManager::FindFreeForce(AiForceRole role, int begin, bool allow_hero_only_force)
{
	/* find free force */
	unsigned int f = begin;
	for (; f < forces.size(); ++f) {
		if (forces[f].State == AiForceAttackingState_Free) {
			break;
		}
		
		if (allow_hero_only_force && forces[f].IsHeroOnlyForce()) {
			break;
		}
	}
	
	if (f == forces.size()) {
		forces.resize(f + 1);
	}
	forces[f].State = AiForceAttackingState_Waiting;
	forces[f].Role = role;
	return f;
}

/**
**  Find unit in force
**
**  @param    unit  Unit to search for.
**
**  @return   Force number, or -1 if not found
*/

int AiForceManager::GetForce(const CUnit &unit)
{
	for (unsigned int i = 0; i < forces.size(); ++i) {
		AiForce &force = forces[i];

		for (unsigned int j = 0; j < force.Units.size(); ++j) {
			CUnit &aiunit = *force.Units[j];

			if (UnitNumber(unit) == UnitNumber(aiunit)) {
				return i;
			}
		}
	}
	return -1;
}

/**
**  Cleanup units in forces.
*/
void AiForceManager::RemoveDeadUnit()
{
	for (unsigned int i = 0; i < forces.size(); ++i) {
		forces[i].RemoveDeadUnit();
	}
}

/**
**  Ai assign unit to force.
**
**  @param unit  Unit to assign to force.
*/
//Wyrmgus start
//bool AiForceManager::Assign(CUnit &unit, int force)
bool AiForceManager::Assign(CUnit &unit, int force, bool hero)
//Wyrmgus end
{
	if (unit.GroupId != 0) {
		return false;
	}
	if (force != -1) {
		AiForce &f = forces[force];
		//Wyrmgus start
//		if (f.IsBelongsTo(*unit.Type)) {
		if (f.IsBelongsTo(*unit.Type) || hero) {
		//Wyrmgus end
			if (hero && f.Units.size() > 0) { //make heroes move to where the rest of the force is
				CommandMove(unit, f.Units[0]->tilePos, FlushCommands, f.Units[0]->MapLayer->ID);
			}
			f.Insert(unit);
			unit.GroupId = force + 1;
			return true;
		}
	} else {
		// Check to which force it belongs
		for (unsigned int i = 0; i < forces.size(); ++i) {
			AiForce &f = forces[i];
			// No troops for attacking force
			if (f.IsAttacking()) {
				continue;
			}
			
			if (this->Assign(unit, i, hero && i > 0)) {
				return true;
			}
		}
	}
	return false;
}

void AiForceManager::CheckUnits(int *counter)
{
	int attacking[UnitTypeMax];
	memset(attacking, 0, sizeof(attacking));

	// Look through the forces what is missing.
	for (unsigned int i = 0; i < forces.size(); ++i) {
		const AiForce &force = forces[i];

		if (force.State > AiForceAttackingState_Free && force.IsAttacking()) {
			for (unsigned int j = 0; j < force.Size(); ++j) {
				const CUnit *unit = force.Units[j];
				attacking[unit->Type->Slot]++;
			}
		}
	}
	// create missing units
	for (unsigned int i = 0; i < forces.size(); ++i) {
		AiForce &force = forces[i];

		// No troops for attacking force
		if (force.State == AiForceAttackingState_Free || force.IsAttacking()) {
			continue;
		}
		for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
			const AiUnitType &aiut = force.UnitTypes[j];
			const unsigned int t = aiut.Type->Slot;
			const int unit_class = aiut.Type->Class;
			const int wantedCount = aiut.Want;
			int e = AiPlayer->Player->GetUnitTypeAiActiveCount(CUnitType::UnitTypes[t]);
			if (t < AiHelpers.Equiv.size()) {
				for (unsigned int k = 0; k < AiHelpers.Equiv[t].size(); ++k) {
					e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][k]);
				}
			}
			if (unit_class != -1) {
				for (size_t k = 0; k < ClassUnitTypes[unit_class].size(); ++k) {
					const CUnitType *class_unit_type = ClassUnitTypes[unit_class][k];
					if (class_unit_type != aiut.Type) {
						e += AiPlayer->Player->GetUnitTypeAiActiveCount(class_unit_type);
					}
				}
			}
			const int requested = wantedCount - (e + counter[t] - attacking[t]);

			if (requested > 0) {  // Request it.
				AiAddUnitTypeRequest(*aiut.Type, requested);
				counter[t] += requested;
				force.Completed = false;
			}
			counter[t] -= wantedCount;
		}
	}
}

/**
**  Cleanup units in forces.
*/
void AiRemoveDeadUnitInForces()
{
	AiPlayer->Force.RemoveDeadUnit();
}

/**
**  Ai assign unit to force.
**
**  @param unit  Unit to assign to force.
*/
bool AiAssignToForce(CUnit &unit)
{
	return AiPlayer->Force.Assign(unit);
}

/**
**  Assign free units to force.
*/
void AiAssignFreeUnitsToForce(int force)
{
	const int n = AiPlayer->Player->GetUnitCount();

	AiRemoveDeadUnitInForces();
	for (int i = 0; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (unit.Active && unit.GroupId == 0) {
			AiPlayer->Force.Assign(unit, force, unit.Character != nullptr && !unit.Type->BoolFlag[HARVESTER_INDEX].value);
		}
	}
}

/**
**  Attack at position with force.
**
**  @param force  Force number to attack with.
**  @param x      X tile map position to be attacked.
**  @param y      Y tile map position to be attacked.
*/
//Wyrmgus start
//void AiAttackWithForceAt(unsigned int force, int x, int y)
void AiAttackWithForceAt(unsigned int force, int x, int y, int z)
//Wyrmgus end
{
	const Vec2i pos(x, y);

	if (!(force < AI_MAX_FORCE_INTERNAL)) {
		DebugPrint("Force out of range: %d" _C_ force);
		return ;
	}

	if (!CMap::Map.Info.IsPointOnMap(pos, z)) {
		DebugPrint("(%d, %d) not in the map(%d, %d)" _C_ pos.x _C_ pos.y
				   //Wyrmgus start
//				   _C_ CMap::Map.Info.MapWidth _C_ CMap::Map.Info.MapHeight);
				   _C_ CMap::Map.Info.MapWidths[z] _C_ CMap::Map.Info.MapHeights[z]);
				   //Wyrmgus end
		return ;
	}
	//Wyrmgus start
//	AiPlayer->Force[force].Attack(pos);
	AiPlayer->Force[force].Attack(pos, z);
	//Wyrmgus end
}

/**
**  Attack opponent with force.
**
**  @param force  Force number to attack with.
*/
void AiAttackWithForce(unsigned int force)
{
	if (!(force < AI_MAX_FORCE_INTERNAL)) {
		DebugPrint("Force out of range: %d" _C_ force);
		return ;
	}

	unsigned int intForce = AiPlayer->Force.getScriptForce(force);
	// The AI finds the first unassigned force, moves all data to it and cleans
	// the first force, so we can reuse it
	if (!AiPlayer->Force[intForce].Defending) {
		unsigned int top;
		unsigned int f = AiPlayer->Force.FindFreeForce(AiForceRoleDefault, AI_MAX_FORCE_INTERNAL);
		AiPlayer->Force[f].Reset();
		AiPlayer->Force[f].FormerForce = force;
		AiPlayer->Force[f].Role = AiPlayer->Force[intForce].Role;

		while (AiPlayer->Force[intForce].Size()) {
			CUnit &aiunit = *AiPlayer->Force[intForce].Units[AiPlayer->Force[intForce].Size() - 1];
			aiunit.GroupId = f + 1;
			AiPlayer->Force[intForce].Units.Remove(&aiunit);
			AiPlayer->Force[f].Units.Insert(&aiunit);
		}

		while (AiPlayer->Force[intForce].UnitTypes.size()) {
			top = AiPlayer->Force[intForce].UnitTypes.size() - 1;
			AiPlayer->Force[f].UnitTypes.push_back(AiPlayer->Force[intForce].UnitTypes[top]);
			AiPlayer->Force[intForce].UnitTypes.pop_back();
		}
		AiPlayer->Force[intForce].Reset();
		AiPlayer->Force[f].Completed = true;
		force = f;
	}

	const Vec2i invalidPos(-1, -1);
	//Wyrmgus start
	int z = AiPlayer->Player->StartMapLayer;
	if (AiPlayer->Force[force].Units.size() > 0) {
		z = AiPlayer->Force[force].Units[0]->MapLayer->ID;
	}
	
//	AiPlayer->Force[force].Attack(invalidPos);
	AiPlayer->Force[force].Attack(invalidPos, z);
	//Wyrmgus end
}

/**
**  Attack opponent with forces.
**  Merge forces in array into one attack force and attack with it
**  Merge is make because units in one force help each other during attack
**
**  @param forces  Array with Force numbers to attack with (array should be finished with -1).
*/
void AiAttackWithForces(int *forces)
{
	const Vec2i invalidPos(-1, -1);
	bool found = false;
	unsigned int top;
	unsigned int f = AiPlayer->Force.FindFreeForce(AiForceRoleDefault, AI_MAX_FORCE_INTERNAL);

	AiPlayer->Force[f].Reset();

	for (int i = 0; forces[i] != -1; ++i) {
		int force = forces[i];

		if (!AiPlayer->Force[force].Defending) {
			found = true;

			AiPlayer->Force[f].Role = AiPlayer->Force[force].Role;

			while (AiPlayer->Force[force].Size()) {
				CUnit &aiunit = *AiPlayer->Force[force].Units[AiPlayer->Force[force].Size() - 1];
				aiunit.GroupId = f + 1;
				AiPlayer->Force[force].Units.Remove(&aiunit);
				AiPlayer->Force[f].Units.Insert(&aiunit);
			}
			while (AiPlayer->Force[force].UnitTypes.size()) {
				top = AiPlayer->Force[force].UnitTypes.size() - 1;
				AiPlayer->Force[f].UnitTypes.push_back(AiPlayer->Force[force].UnitTypes[top]);
				AiPlayer->Force[force].UnitTypes.pop_back();
			}
			AiPlayer->Force[force].Reset();
		} else {
			//Wyrmgus start
			int z = AiPlayer->Player->StartMapLayer;
			if (AiPlayer->Force[force].Units.size() > 0) {
				z = AiPlayer->Force[force].Units[0]->MapLayer->ID;
			}
			
//			AiPlayer->Force[force].Attack(invalidPos);
			AiPlayer->Force[force].Attack(invalidPos, z);
			//Wyrmgus end
		}
	}
	if (found) {
		AiPlayer->Force[f].Completed = true;
		//Wyrmgus start
		int z = AiPlayer->Player->StartMapLayer;
		if (AiPlayer->Force[f].Units.size() > 0) {
			z = AiPlayer->Force[f].Units[0]->MapLayer->ID;
		}
			
//		AiPlayer->Force[f].Attack(invalidPos);
		AiPlayer->Force[f].Attack(invalidPos, z);
		//Wyrmgus end
	} else {
		AiPlayer->Force[f].Reset(true);
	}
}


/**
**  Load all unit before attack.
**
**  @param aiForce force to group.
*/
static void AiGroupAttackerForTransport(AiForce &aiForce)
{
	Assert(aiForce.State == AiForceAttackingState_Boarding);

	unsigned int nbToTransport = 0;
	unsigned int transporterIndex = 0;
	bool forceIsReady = true;

	for (; transporterIndex < aiForce.Size(); ++transporterIndex) {
		const CUnit &unit = *aiForce.Units[transporterIndex];

		if (unit.Type->CanTransport() && unit.Type->MaxOnBoard - unit.BoardCount > 0) {
			nbToTransport = unit.Type->MaxOnBoard - unit.BoardCount;
			break;
		}
	}
	if (transporterIndex == aiForce.Size()) {
		aiForce.State = AiForceAttackingState_AttackingWithTransporter;
		return ;
	}
	for (unsigned int i = 0; i < aiForce.Size(); ++i) {
		const CUnit &unit = *aiForce.Units[i];
		const CUnit &transporter = *aiForce.Units[transporterIndex];

		if (CanTransport(transporter, unit) && unit.Container == nullptr) {
			forceIsReady = false;
			break;
		}
	}
	if (forceIsReady == true) {
		aiForce.State = AiForceAttackingState_AttackingWithTransporter;
		return ;
	}
	for (unsigned int i = 0; i < aiForce.Size(); ++i) {
		CUnit &unit = *aiForce.Units[i];
		CUnit &transporter = *aiForce.Units[transporterIndex];

		if (unit.CurrentAction() == UnitActionBoard
			&& static_cast<COrder_Board *>(unit.CurrentOrder())->GetGoal() == &transporter) {
			CommandFollow(transporter, unit, 0);
		}
		if (CanTransport(transporter, unit) && (unit.IsIdle() 
			|| (unit.CurrentAction() == UnitActionBoard && !unit.Moving
			&& static_cast<COrder_Board *>(unit.CurrentOrder())->GetGoal() != &transporter)) && unit.Container == nullptr) {
				CommandBoard(unit, transporter, FlushCommands);
				CommandFollow(transporter, unit, 0);
				if (--nbToTransport == 0) { // full : next transporter.
					for (++transporterIndex; transporterIndex < aiForce.Size(); ++transporterIndex) {
						const CUnit &nextTransporter = *aiForce.Units[transporterIndex];

						if (nextTransporter.Type->CanTransport()) {
							nbToTransport = nextTransporter.Type->MaxOnBoard - nextTransporter.BoardCount;
							break ;
						}
					}
					if (transporterIndex == aiForce.Size()) { // No more transporter.
						break ;
					}
				}
		}
	}
}

/**
** Force on attack ride. We attack until there is no unit or enemy left.
**
** @param force Force pointer.
*/
void AiForce::Update()
{
	Assert(Defending == false);
	if (Size() == 0) {
		Attacking = false;
		if (!Defending && State > AiForceAttackingState_Waiting) {
			DebugPrint("%d: Attack force #%lu was destroyed, giving up\n"
					   _C_ AiPlayer->Player->Index _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
			Reset(true);
		}
		return;
	}
	//Wyrmgus start
	if (AiPlayer->Scouting) {
		return;
	}
	//Wyrmgus end
	//Wyrmgus start
	//if one of the force's units is being used as a scout, stop its function as a scout when the force is used to attack
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *unit = this->Units[i];
		if (std::find(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), unit) != AiPlayer->Scouts.end()) {
			AiPlayer->Scouts.erase(std::remove(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), unit), AiPlayer->Scouts.end());
		}
	}
	//Wyrmgus end
	//if force still has no goal, run its Attack function again to get a target
	if (CMap::Map.Info.IsPointOnMap(GoalPos, GoalMapLayer) == false) {
		const Vec2i invalidPos(-1, -1);
		//Wyrmgus start
		int z = AiPlayer->Player->StartMapLayer;
		if (Units.size() > 0) {
			z = Units[0]->MapLayer->ID;
		}
			
//		Attack(invalidPos);
		Attack(invalidPos, z);
		//Wyrmgus end
		return;
	}
	//Wyrmgus end
	Attacking = false;
	for (unsigned int i = 0; i < Size(); ++i) {
		CUnit *aiunit = Units[i];

		if (aiunit->Type->CanAttack) {
			Attacking = true;
			break;
		}
	}
	if (Attacking == false) {
		if (!Defending && State > AiForceAttackingState_Waiting) {
			DebugPrint("%d: Attack force #%lu has lost all agresive units, giving up\n"
					   _C_ AiPlayer->Player->Index _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
			Reset(true);
		}
		return ;
	}
#if 0
	if (State == AiForceAttackingState_Waiting) {
		if (!this->PlanAttack()) {
			DebugPrint("Can't transport, look for walls\n");
			if (!AiFindWall(this)) {
				Attacking = false;
				return ;
			}
		}
		State = AiForceAttackingState_Boarding;
	}
#endif
	if (State == AiForceAttackingState_Boarding) {
		AiGroupAttackerForTransport(*this);
		return ;
	}
	if (State == AiForceAttackingState_AttackingWithTransporter) {
		// Move transporters to goalpos
		std::vector<CUnit *> transporters;
		bool emptyTrans = true;
		for (unsigned int i = 0; i != Size(); ++i) {
			CUnit &aiunit = *Units[i];
			
			if (aiunit.CanMove() && aiunit.Type->MaxOnBoard) {
				transporters.push_back(&aiunit);
				if (aiunit.BoardCount > 0) {
					emptyTrans = false;
				}
			}
		}
		if (transporters.empty()) {
			// Our transporters have been destroyed
			DebugPrint("%d: Attack force #%lu has lost all agresive units, giving up\n"
				_C_ AiPlayer->Player->Index _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
			Reset(true);
		} else if (emptyTrans) {
			// We have emptied our transporters, go go go
			State = AiForceAttackingState_GoingToRallyPoint;
		} else {
			for (size_t i = 0; i != transporters.size(); ++i) {
				CUnit &trans = *transporters[i];
				//Wyrmgus start
//				const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
				const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
				//Wyrmgus end
				
				//Wyrmgus start
//				trans.Wait = delay;
				trans.Wait += delay;
				//Wyrmgus end
				CommandUnload(trans, this->GoalPos, nullptr, FlushCommands, this->GoalMapLayer);
			}
		}
		return;
	}
	
	//Wyrmgus start
	bool needs_transport = false;
	for (size_t i = 0; i != this->Units.size(); ++i) {
		CUnit *const unit = this->Units[i];

		if (unit->Type->UnitType != UnitTypeFly && unit->Type->UnitType != UnitTypeFlyLow && unit->Type->UnitType != UnitTypeNaval && CMap::Map.GetTileLandmass(unit->tilePos, unit->MapLayer->ID) != CMap::Map.GetTileLandmass(this->GoalPos, this->GoalMapLayer)) {
			needs_transport = true;
			break;
		}
	}
		
	if (needs_transport) { //if is a land force attacking another landmass, see if there are enough transporters to carry it, and whether they are doing the appropriate actions
		if (!this->CheckTransporters(this->GoalPos, this->GoalMapLayer)) {
			return;
		}
	}
	//Wyrmgus end
	
	CUnit *leader = nullptr;
	for (unsigned int i = 0; i != Size(); ++i) {
		CUnit &aiunit = *Units[i];

		if (aiunit.IsAgressive()) {
			leader = &aiunit;
			break;
		}
	}

	//Wyrmgus start
//	const int thresholdDist = 5; // Hard coded value
	const int thresholdDist = std::max(5, (int) Units.size() / 8);
	//Wyrmgus end
	Assert(CMap::Map.Info.IsPointOnMap(GoalPos, GoalMapLayer));
	//Wyrmgus start
	bool include_neutral = AiPlayer->Player->AtPeace();
	//Wyrmgus end
	if (State == AiForceAttackingState_GoingToRallyPoint) {
		// Check if we are near the goalpos
		//Wyrmgus start
//		int minDist = Units[0]->MapDistanceTo(this->GoalPos);
		int minDist = Units[0]->MapDistanceTo(this->GoalPos, this->GoalMapLayer);
		//Wyrmgus end
		int maxDist = minDist;

		for (size_t i = 0; i != Size(); ++i) {
			//Wyrmgus start
//			const int distance = Units[i]->MapDistanceTo(this->GoalPos);
			const int distance = Units[i]->MapDistanceTo(this->GoalPos, this->GoalMapLayer);
			//Wyrmgus end
			minDist = std::min(minDist, distance);
			maxDist = std::max(maxDist, distance);
		}

		if (WaitOnRallyPoint > 0 && minDist <= thresholdDist) {
			--WaitOnRallyPoint;
		}
		if (maxDist <= thresholdDist || !WaitOnRallyPoint) {
			const CUnit *unit = nullptr;

			//Wyrmgus start
			Vec2i enemy_wall_pos(-1, -1);
			int enemy_wall_map_layer = -1;
			
//			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit);
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, true);
			//Wyrmgus end
			if (!unit) {
				//Wyrmgus start
//				AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit);
				AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, true);
				//Wyrmgus end
				if (!unit && !CMap::Map.Info.IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
					//Wyrmgus start
					/*
					// No enemy found, give up
					// FIXME: should the force go home or keep trying to attack?
					DebugPrint("%d: Attack force #%lu can't find a target, giving up\n"
							   _C_ AiPlayer->Player->Index _C_(long unsigned int)(this - & (AiPlayer->Force[0])));
					Attacking = false;
					State = AiForceAttackingState_Waiting;
					*/
					GoalPos.x = -1;
					GoalPos.y = -1;
					//Wyrmgus start
					GoalMapLayer = 0;
					//Wyrmgus end
					AiPlayer->Scouting = true;
					//Wyrmgus end
					return;
				}
			}
			if (unit) {
				this->GoalPos = unit->tilePos;
				this->GoalMapLayer = unit->MapLayer->ID;
				if (!AiPlayer->Player->IsEnemy(*unit->Player) && unit->Player->Type != PlayerNeutral) {
					AiPlayer->Player->SetDiplomacyEnemyWith(*unit->Player);
					if (AiPlayer->Player->IsSharedVision(*unit->Player)) {
						CommandSharedVision(AiPlayer->Player->Index, false, unit->Player->Index);
					}
				}
			} else if (CMap::Map.Info.IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
				this->GoalPos = enemy_wall_pos;
				this->GoalMapLayer = enemy_wall_map_layer;
				int enemy_wall_owner = CMap::Map.Field(enemy_wall_pos, enemy_wall_map_layer)->Owner;
				if (!AiPlayer->Player->IsEnemy(*CPlayer::Players[enemy_wall_owner]) && CPlayer::Players[enemy_wall_owner]->Type != PlayerNeutral) {
					AiPlayer->Player->SetDiplomacyEnemyWith(*CPlayer::Players[enemy_wall_owner]);
					if (AiPlayer->Player->IsSharedVision(*CPlayer::Players[enemy_wall_owner])) {
						CommandSharedVision(AiPlayer->Player->Index, false, CPlayer::Players[enemy_wall_owner]->Index);
					}
				}
			}
			
			State = AiForceAttackingState_Attacking;
			for (size_t i = 0; i != this->Size(); ++i) {
				CUnit &aiunit = *this->Units[i];
				
				//Wyrmgus start
				if (!aiunit.IsIdle()) {
					continue;
				}
				//Wyrmgus end
				
				//Wyrmgus start
//				const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
				const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
				//Wyrmgus end

				//Wyrmgus start
//				aiunit.Wait = delay;
				aiunit.Wait += delay;
				//Wyrmgus end
				if (aiunit.IsAgressive()) {
					CommandAttack(aiunit, this->GoalPos, nullptr, FlushCommands, this->GoalMapLayer);
				} else {
					if (leader) {
						CommandDefend(aiunit, *leader, FlushCommands);
					} else {
						//Wyrmgus start
//						CommandMove(aiunit, this->GoalPos, FlushCommands);
						CommandMove(aiunit, this->GoalPos, FlushCommands, this->GoalMapLayer);
						//Wyrmgus end
					}
				}
			}
		}
	}

	std::vector<CUnit *> idleUnits;
	for (unsigned int i = 0; i != Size(); ++i) {
		CUnit &aiunit = *Units[i];

		if (aiunit.IsIdle()) {
			idleUnits.push_back(&aiunit);
		}
	}

	if (idleUnits.empty()) {
		return;
	}

	if (State == AiForceAttackingState_Attacking && idleUnits.size() == this->Size()) {
		const CUnit *unit = nullptr;
		Vec2i enemy_wall_pos(-1, -1);
		int enemy_wall_map_layer = -1;

		bool isNaval = this->IsNaval();
		if (isNaval) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit);
			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit);
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, include_neutral, true);
			//Wyrmgus end
		}
		if (!unit && !CMap::Map.Info.IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
			//Wyrmgus start
			/*
			// No enemy found, give up
			// FIXME: should the force go home or keep trying to attack?
			DebugPrint("%d: Attack force #%lu can't find a target, giving up\n"
					   _C_ AiPlayer->Player->Index _C_(long unsigned int)(this - & (AiPlayer->Force[0])));
			Attacking = false;
			State = AiForceAttackingState_Waiting;
			*/
			GoalPos.x = -1;
			GoalPos.y = -1;
			//Wyrmgus start
			GoalMapLayer = 0;
			//Wyrmgus end
			AiPlayer->Scouting = true;
			//Wyrmgus end
			return;
		} else {
			Vec2i resultPos;
			if (unit) {
				NewRallyPoint(unit->tilePos, &resultPos, unit->MapLayer->ID);
				if (resultPos.x == 0 && resultPos.y == 0) {
					resultPos = unit->tilePos;
				}
				this->GoalPos = resultPos;
				this->GoalMapLayer = unit->MapLayer->ID;
				if (!AiPlayer->Player->IsEnemy(*unit->Player) && unit->Player->Type != PlayerNeutral) {
					AiPlayer->Player->SetDiplomacyEnemyWith(*unit->Player);
					if (AiPlayer->Player->IsSharedVision(*unit->Player)) {
						CommandSharedVision(AiPlayer->Player->Index, false, unit->Player->Index);
					}
				}
			} else if (CMap::Map.Info.IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
				NewRallyPoint(enemy_wall_pos, &resultPos, enemy_wall_map_layer);
				if (resultPos.x == 0 && resultPos.y == 0) {
					resultPos = enemy_wall_pos;
				}
				this->GoalPos = resultPos;
				this->GoalMapLayer = enemy_wall_map_layer;
				int enemy_wall_owner = CMap::Map.Field(enemy_wall_pos, enemy_wall_map_layer)->Owner;
				if (!AiPlayer->Player->IsEnemy(*CPlayer::Players[enemy_wall_owner]) && CPlayer::Players[enemy_wall_owner]->Type != PlayerNeutral) {
					AiPlayer->Player->SetDiplomacyEnemyWith(*CPlayer::Players[enemy_wall_owner]);
					if (AiPlayer->Player->IsSharedVision(*CPlayer::Players[enemy_wall_owner])) {
						CommandSharedVision(AiPlayer->Player->Index, false, CPlayer::Players[enemy_wall_owner]->Index);
					}
				}
			}
			this->State = AiForceAttackingState_GoingToRallyPoint;
		}
	}
	//Wyrmgus start
	/*
	for (size_t i = 0; i != idleUnits.size(); ++i) {
		CUnit &aiunit = *idleUnits[i];
		//Wyrmgus start
//		const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
		const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
		//Wyrmgus end

		//Wyrmgus start
//		aiunit.Wait = delay;
		aiunit.Wait += delay;
		//Wyrmgus end
		if (leader) {
			if (aiunit.IsAgressive()) {
				if (State == AiForceAttackingState_Attacking) {
					//Wyrmgus start
//					CommandAttack(aiunit, leader->tilePos, nullptr, FlushCommands);
					CommandAttack(aiunit, leader->tilePos, nullptr, FlushCommands, leader->MapLayer);
					//Wyrmgus end
				} else {
					//Wyrmgus start
//					CommandAttack(aiunit, this->GoalPos, nullptr, FlushCommands);
					CommandAttack(aiunit, this->GoalPos, nullptr, FlushCommands, this->GoalMapLayer);
					//Wyrmgus end
				}
			} else {
				CommandDefend(aiunit, *leader, FlushCommands);
			}
		} else {
			if (aiunit.IsAgressive()) {
				//Wyrmgus start
//				CommandAttack(aiunit, this->GoalPos, nullptr, FlushCommands);
				CommandAttack(aiunit, this->GoalPos, nullptr, FlushCommands, this->GoalMapLayer);
				//Wyrmgus end
			} else {
				//Wyrmgus start
//				CommandMove(aiunit, this->GoalPos, FlushCommands);
				CommandMove(aiunit, this->GoalPos, FlushCommands, this->GoalMapLayer);
				//Wyrmgus end
			}
		}
	}
	*/
	//Wyrmgus end
}

void AiForceManager::CheckForceRecruitment()
{
	if (AiPlayer->Player->AiName == "passive") {
		return;
	}
	
	if (AiPlayer->Player->NumTownHalls < 1) { //don't build up forces if the AI has no town hall yet
		return;
	}

	bool all_forces_completed = true;
	int completed_forces = 0;
	int force_type_count[MaxForceTypes];
	memset(force_type_count, 0, sizeof(force_type_count));
	
	for (unsigned int f = 0; f < forces.size(); ++f) {
		AiForce &force = forces[f];
		if (force.Completed && force.Units.size() > 0) {
			if (!force.IsHeroOnlyForce()) {
				completed_forces++;
				force_type_count[force.GetForceType()]++;
			}
			//attack with forces that are completed, but aren't attacking or defending
			if (!force.Attacking && !force.Defending) {
				const Vec2i invalidPos(-1, -1);
				int z = force.Units[0]->MapLayer->ID;
				
				force.Attack(invalidPos, z);
			}
		}
		
		if (!force.Completed) {
			all_forces_completed = false;
		}
	}
	
	int completed_force_pop = 0;
	
	if (all_forces_completed) {
		for (size_t f = 0; f < forces.size(); ++f) {
			AiForce &force = forces[f];
			if (force.Completed) {
				for (size_t j = 0; j < force.Units.size(); ++j) {
					CUnit *force_unit = force.Units[j];
					completed_force_pop += force_unit->Variable[DEMAND_INDEX].Value;
				}
			}
		}
	}
	
	if (all_forces_completed && AiPlayer->Player->Race != -1 && AiPlayer->Player->GetFaction() != nullptr && completed_forces < AI_MAX_COMPLETED_FORCES && completed_force_pop < AI_MAX_COMPLETED_FORCE_POP) { //all current forces completed and not too many forces are in existence, create a new one
		int force_type_weights[MaxForceTypes];
		for (int i = 0; i < MaxForceTypes; ++i) {
			force_type_weights[i] = AiPlayer->Player->GetFaction()->GetForceTypeWeight(i);
		}
		
		std::vector<int> force_types;
		while ((int) force_types.size() < MaxForceTypes) {
			for (int i = 0; i < MaxForceTypes; ++i) {
				if (force_type_count[i] <= 0) {
					force_types.push_back(i);
				}
				force_type_count[i] -= force_type_weights[i];
			}
		}

		for (size_t k = 0; k < force_types.size(); ++k) {
			std::vector<CForceTemplate *> faction_force_templates = AiPlayer->Player->GetFaction()->GetForceTemplates(force_types[k]);
			std::vector<CForceTemplate *> potential_force_templates;
			int priority = 0;
			for (size_t i = 0; i < faction_force_templates.size(); ++i) {
				if (faction_force_templates[i]->Priority < priority) {
					break; //force templates are ordered by priority, so there is no need to go further
				}
				bool valid = true;
				for (size_t j = 0; j < faction_force_templates[i]->Units.size(); ++j) {
					int class_id = faction_force_templates[i]->Units[j].first;
					int unit_type_id = CFaction::GetFactionClassUnitType(AiPlayer->Player->GetFaction(), class_id);
					CUnitType *type = nullptr;
					if (unit_type_id != -1) {
						type = CUnitType::UnitTypes[unit_type_id];
					}
					if (!type || !AiRequestedTypeAllowed(*AiPlayer->Player, *type)) {
						valid = false;
						break;
					}
					
					if (AiPlayer->NeededMask & AiPlayer->Player->GetUnitTypeCostsMask(type)) { //don't request the force if it is going to use up a resource that is currently needed
						valid = false;
						break;
					}
				}
				if (valid) {
					if (faction_force_templates[i]->Priority > priority) {
						priority = faction_force_templates[i]->Priority;
						potential_force_templates.clear();
					}
					for (int j = 0; j < faction_force_templates[i]->Weight; ++j) {
						potential_force_templates.push_back(faction_force_templates[i]);
					}
				}
			}
			
			CForceTemplate *force_template = potential_force_templates.size() ? potential_force_templates[SyncRand(potential_force_templates.size())] : nullptr;
		
			if (force_template) {
				unsigned int new_force_id = this->FindFreeForce(AiForceRoleDefault, 1, true);
				AiForce &new_force = forces[new_force_id];
				new_force.Reset(true);
				new_force.State = AiForceAttackingState_Waiting;
				new_force.Role = AiForceRoleDefault;
				for (size_t i = 0; i < force_template->Units.size(); ++i) {
					int class_id = force_template->Units[i].first;
					int unit_type_id = CFaction::GetFactionClassUnitType(AiPlayer->Player->GetFaction(), class_id);
					CUnitType *type = nullptr;
					if (unit_type_id != -1) {
						type = CUnitType::UnitTypes[unit_type_id];
					}
					int count = force_template->Units[i].second;
					
					AiUnitType newaiut;
					newaiut.Want = count;
					newaiut.Type = type;
					new_force.UnitTypes.push_back(newaiut);
				}
				AiAssignFreeUnitsToForce(new_force_id);
				break;
			}
		}
	}
}

void AiForceManager::Update()
{
	for (unsigned int f = 0; f < forces.size(); ++f) {
		AiForce &force = forces[f];
		//  Look if our defenders still have enemies in range.

		if (force.Defending) {
			force.RemoveDeadUnit();

			if (force.Size() == 0) {
				force.Attacking = false;
				force.Defending = false;
				force.State = AiForceAttackingState_Waiting;
				continue;
			}
			const int nearDist = 5;

			if (CMap::Map.Info.IsPointOnMap(force.GoalPos, force.GoalMapLayer) == false) {
				force.ReturnToHome();
			} else {
				//  Check if some unit from force reached goal point
				for (unsigned int i = 0; i != force.Size(); ++i) {
					//Wyrmgus start
//					if (force.Units[i]->MapDistanceTo(force.GoalPos) <= nearDist) {
					if (force.Units[i]->MapDistanceTo(force.GoalPos, force.GoalMapLayer) <= nearDist) {
					//Wyrmgus end
						//  Look if still enemies in attack range.
						const CUnit *dummy = nullptr;
						Vec2i dummy_wall_pos(-1, -1);
						int dummy_wall_map_layer = -1;
						//Wyrmgus start
//						if (!AiForceEnemyFinder<AIATTACK_RANGE>(force, &dummy).found()) {
						if (!AiForceEnemyFinder<AIATTACK_RANGE>(force, &dummy, &dummy_wall_pos, &dummy_wall_map_layer, false, false).found()) {
						//Wyrmgus end
							force.ReturnToHome();
						}
					}
				}
				
				if (force.Defending == false) {
					// force is no longer defending
					return;
				}

				// Find idle units and order them to defend
				// Don't attack if there aren't our units near goal point
				std::vector<CUnit *> nearGoal;
				const Vec2i offset(15, 15);
				Select(force.GoalPos - offset, force.GoalPos + offset, nearGoal,
						//Wyrmgus start
						force.GoalMapLayer,
						//Wyrmgus end
					   IsAnAlliedUnitOf(*force.Units[0]->Player));
				if (nearGoal.empty()) {
					force.ReturnToHome();
				} else {
					std::vector<CUnit *> idleUnits;
					for (unsigned int i = 0; i != force.Size(); ++i) {
						CUnit &aiunit = *force.Units[i];

						if (aiunit.IsIdle() && aiunit.IsAliveOnMap()) {
							idleUnits.push_back(&aiunit);
						}
					}
					for (unsigned int i = 0; i != idleUnits.size(); ++i) {
						CUnit *const unit = idleUnits[i];

						if (unit->Container == nullptr) {
							//Wyrmgus start
//							const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
							const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
							//Wyrmgus end

							//Wyrmgus start
//							unit->Wait = delay;
							unit->Wait += delay;
							//Wyrmgus end
							//Wyrmgus start
//							if (unit->Type->CanAttack) {
							if (unit->CanAttack() && unit->IsAgressive()) {
							//Wyrmgus end
								CommandAttack(*unit, force.GoalPos, nullptr, FlushCommands, force.GoalMapLayer);
							} else {
								CommandMove(*unit, force.GoalPos, FlushCommands, force.GoalMapLayer);
							}
						}
					}
				}
			}
		} else if (force.Attacking) {
			force.RemoveDeadUnit();
			force.Update();
		}
	}
}

//Wyrmgus start
void AiForceManager::UpdatePerHalfMinute()
{
	this->CheckForceRecruitment();
}

void AiForceManager::UpdatePerMinute()
{
	/*
	for (unsigned int f = 0; f < forces.size(); ++f) {
		AiForce &force = forces[f];

		//if any force is completely idle, reset its goal, so that it chooses a new one to attack
		std::vector<CUnit *> idleUnits;
		for (unsigned int i = 0; i != force.Size(); ++i) {
			CUnit &aiunit = *force.Units[i];

			if (
				aiunit.IsIdle()
				|| std::find(AiPlayer->Scouts.begin(), AiPlayer->Scouts.end(), &aiunit) != AiPlayer->Scouts.end() //count scouts as idle, since they will be moving, but not due to orders for the force
			) {
				idleUnits.push_back(&aiunit);
			}
		}

		if (idleUnits.empty()) {
			continue;
		}

		if (idleUnits.size() == force.Size()) {
			force.GoalPos.x = -1;
			force.GoalPos.y = -1;
			force.GoalMapLayer = 0;
		}
	}
	*/
}
//Wyrmgus end

/**
**  Entry point of force manager, periodic called.
*/
void AiForceManager()
{
	AiPlayer->Force.Update();
	AiAssignFreeUnitsToForce();
}

void AiForceManagerEachHalfMinute()
{
	AiPlayer->Force.UpdatePerHalfMinute();
}

void AiForceManagerEachMinute()
{
	AiPlayer->Force.UpdatePerMinute();
}
