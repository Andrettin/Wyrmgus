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
//      (c) Copyright 2001-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "action/action_attack.h"
#include "action/action_board.h"
#include "ai/ai_force_template.h"
#include "ai/ai_force_type.h"
#include "commands.h"
#include "database/defines.h"
#include "game/game.h"
#include "map/landmass.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "network/network.h"
#include "pathfinder/pathfinder.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_type.h"
#include "quest/campaign.h"
#include "script/condition/condition.h"
#include "spell/status_effect.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

static constexpr int AIATTACK_RANGE = 0;
static constexpr int AIATTACK_ALLMAP = 1;
static constexpr int AIATTACK_BUILDING = 2;
static constexpr int AIATTACK_AGRESSIVE = 3;

//Wyrmgus start
class EnemyUnitFinder final
{
public:
	explicit EnemyUnitFinder(const CUnit &unit, CUnit **result_unit, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, const int find_type, const bool allow_water)
	//Wyrmgus end
		: unit(unit),
		movemask(unit.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)),
		attackrange(unit.get_best_attack_range()),
		find_type(find_type),
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
	tile_flag movemask;
	const int attackrange;
	const int find_type;
	bool allow_water;
	CUnit **result_unit;
	Vec2i *result_enemy_wall_pos;
	int *result_enemy_wall_map_layer;
};

VisitResult EnemyUnitFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	if (!unit.MapLayer->Field(pos)->player_info->IsTeamExplored(*unit.Player)) {
		return VisitResult::DeadEnd;
	}
	
	if (unit.MapLayer->Field(pos)->CheckMask(tile_flag::wall) && !CMap::get()->Info->IsPointOnMap(*result_enemy_wall_pos, *result_enemy_wall_map_layer)) {
		const CPlayer *tile_owner = unit.MapLayer->Field(pos)->get_owner();
		if (tile_owner != nullptr && unit.is_enemy_of(*tile_owner)) {
			*result_enemy_wall_pos = pos;
			*result_enemy_wall_map_layer = unit.MapLayer->ID;
		}
	}
	
	if (!CanMoveToMask(pos, movemask, unit.MapLayer->ID)) { // unreachable
		if (allow_water) {
			tile_flag water_movemask = movemask;
			water_movemask &= ~(tile_flag::water_allowed | tile_flag::coast_allowed);
			if (CanMoveToMask(pos, water_movemask, unit.MapLayer->ID)) {
				//if movement through water is allowed (with transport ships), then don't make water tiles a dead end, but don't look for units in them either
				return VisitResult::Ok;
			}
		}
		
		return VisitResult::DeadEnd;
	}

	std::vector<CUnit *> table;
	Vec2i minpos = pos - Vec2i(attackrange, attackrange);
	Vec2i maxpos = pos + Vec2i(unit.Type->get_tile_size() - QSize(1, 1) + QSize(attackrange, attackrange));
	Select(minpos, maxpos, table, unit.MapLayer->ID, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
	for (CUnit *dest : table) {
		const unit_type &dtype = *dest->Type;

		if (!dest->IsAliveOnMap()) {
			continue;
		}
		
		if (!unit.is_enemy_of(*dest) || !unit.Type->can_target(dest)) {
			continue;
		}

		// Don't attack invulnerable units
		if (dtype.BoolFlag[INDESTRUCTIBLE_INDEX].value || dest->has_status_effect(status_effect::unholy_armor)) {
			continue;
		}
		
		if ((find_type != AIATTACK_BUILDING || dtype.BoolFlag[BUILDING_INDEX].value) && (find_type != AIATTACK_AGRESSIVE || dest->IsAgressive())) {
			*result_unit = dest;
			return VisitResult::Finished;
		} else if (*result_unit == nullptr) { // if trying to search for buildings or aggressive units specifically, still put the first found unit (even if it doesn't fit those parameters) as the result unit, so that it can be returned if no unit with the specified parameters is found
			*result_unit = dest;
		}
	}

	return VisitResult::Ok;
}
//Wyrmgus end

template <const int FIND_TYPE>
class AiForceEnemyFinder final
{
public:
	//Wyrmgus start
//	AiForceEnemyFinder(int force, const CUnit **enemy) : enemy(enemy)
	explicit AiForceEnemyFinder(const int force, const CUnit **enemy, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, const bool allow_water) 
		: enemy(enemy), result_enemy_wall_pos(result_enemy_wall_pos), result_enemy_wall_map_layer(result_enemy_wall_map_layer), allow_water(allow_water)
	//Wyrmgus end
	{
		assert_throw(enemy != nullptr);
		*enemy = nullptr;
		vector::for_each_unless(AiPlayer->Force[force].get_units(), *this);
	}

	//Wyrmgus start
//	AiForceEnemyFinder(AiForce &force, const CUnit **enemy) : enemy(enemy)
	explicit AiForceEnemyFinder(AiForce &force, const CUnit **enemy, Vec2i *result_enemy_wall_pos, int *result_enemy_wall_map_layer, const bool allow_water)
		: enemy(enemy), result_enemy_wall_pos(result_enemy_wall_pos), result_enemy_wall_map_layer(result_enemy_wall_map_layer), allow_water(allow_water)
	//Wyrmgus end
	{
		assert_throw(enemy != nullptr);
		*enemy = nullptr;
		vector::for_each_unless(force.get_units(), *this);
	}

	bool found() const { return *enemy != nullptr; }

	bool operator()(const CUnit *const unit)
	{
		//Wyrmgus start
//		if (unit->Type->CanAttack == false) {
		if (unit->CanAttack() == false || vector::contains(CheckedTypes, unit->Type)) { //don't check for multiple units of the same type, since the result will be the same in almost all cases, so we save performance
		//Wyrmgus end
			return *enemy == nullptr;
		}
		//Wyrmgus start
		CheckedTypes.push_back(unit->Type);
		//Wyrmgus end
		if constexpr (FIND_TYPE == AIATTACK_RANGE) {
			//Wyrmgus start
//			*enemy = AttackUnitsInReactRange(*unit);
			*enemy = AttackUnitsInReactRange(*unit, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
			//Wyrmgus end
		//Wyrmgus start
		} else {
			TerrainTraversal terrainTraversal;

			terrainTraversal.SetSize(unit->MapLayer->get_width(), unit->MapLayer->get_height());
			terrainTraversal.Init();

			terrainTraversal.PushUnitPosAndNeighbor(*unit);

			CUnit *result_unit = nullptr;

			EnemyUnitFinder enemyUnitFinder(*unit, &result_unit, result_enemy_wall_pos, result_enemy_wall_map_layer, FIND_TYPE, allow_water);

			terrainTraversal.Run(enemyUnitFinder);
			*enemy = result_unit;
		//Wyrmgus end
		//Wyrmgus start
		/*
		} else if (FIND_TYPE == AIATTACK_ALLMAP) {
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::get_neutral_player()), false);
			//Wyrmgus end
		} else if (FIND_TYPE == AIATTACK_BUILDING) {
			//Wyrmgus start
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, IsBuildingType());
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::get_neutral_player()), IsBuildingType()), false);
			//Wyrmgus end
			//Wyrmgus start
			//why make sure the enemy is null?
//			assert_throw(!*enemy);
			//Wyrmgus end
			if (*enemy == nullptr || !(*enemy)->Type->BoolFlag[BUILDING_INDEX].value) {
				//Wyrmgus start
//				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::get_neutral_player()), false);
				//Wyrmgus end
			}
		} else if (FIND_TYPE == AIATTACK_AGRESSIVE) {
			//Wyrmgus start
//			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, IsAggresiveUnit());
			*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::get_neutral_player()), IsAggresiveUnit()), false);
			//Wyrmgus end
			//Wyrmgus start
			//why ask that the enemy be null?
//			assert_throw(!*enemy || (*enemy)->IsAgressive());
			//Wyrmgus end
			if (*enemy == nullptr) {
				//Wyrmgus start
//				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth);
				*enemy = AttackUnitsInDistance(*unit, MaxMapWidth, HasNotSamePlayerAs(*CPlayer::get_neutral_player()), false);
				//Wyrmgus end
			}
		*/
		//Wyrmgus end
		}
		return *enemy == nullptr;
	}

	bool operator()(const std::shared_ptr<wyrmgus::unit_ref> &unit_ref)
	{
		return (*this)(unit_ref->get());
	}

private:
	const CUnit **enemy;
	//Wyrmgus start
	Vec2i *result_enemy_wall_pos;
	int *result_enemy_wall_map_layer;
	const bool allow_water;
	std::vector<const wyrmgus::unit_type *> CheckedTypes;
	//Wyrmgus end
};

class IsAnAlliedUnitOf
{
public:
	explicit IsAnAlliedUnitOf(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsVisibleAsGoal(*player) && (unit->Player == player || unit->is_allied_with(*player));
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
void AiNewUnitTypeEquiv(const unit_type &a, const unit_type &b)
{
	int find = UnitTypeEquivs[a.Slot];
	int replace = UnitTypeEquivs[b.Slot];

	// Always record equivalences with the lowest unittype.
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
**  Find All unittypes equivalent to a given one
**
**  @param unittype  the unittype to find equivalence for
**  @param result    int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return          the number of unittype found
*/
int AiFindUnitTypeEquiv(const wyrmgus::unit_type &unittype, int *result)
{
	const int search = UnitTypeEquivs[unittype.Slot];
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
		return unit_type::get_all()[lhs]->DefaultStat.Variables[PRIORITY_INDEX].Value > unit_type::get_all()[rhs]->DefaultStat.Variables[PRIORITY_INDEX].Value;
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
int AiFindAvailableUnitTypeEquiv(const wyrmgus::unit_type &unittype, int *usableTypes)
{
	// 1 - Find equivalents
	int usableTypesCount = AiFindUnitTypeEquiv(unittype, usableTypes);
	// 2 - Remove unavailable unittypes
	for (int i = 0; i < usableTypesCount;) {
		if (!check_conditions(wyrmgus::unit_type::get_all()[usableTypes[i]], AiPlayer->Player)) {
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

class AiForceCounter final
{
public:
	explicit AiForceCounter(const std::vector<std::shared_ptr<unit_ref>> &units, unit_type_map<unsigned> &data) : data(data)
	{
		this->data.clear();

		for (const std::shared_ptr<unit_ref> &unit : units) {
			(*this)(*unit);
		}
	}

	void operator()(const CUnit *const unit) const
	{
		++this->data[unit_type::get_all()[UnitTypeEquivs[unit->Type->Slot]]];
		
		const unit_class *unit_class = unit->Type->get_unit_class();

		if (unit_class != nullptr) {
			for (const wyrmgus::unit_type *class_unit_type : unit_class->get_unit_types()) {
				if (class_unit_type != unit->Type) {
					++this->data[unit_type::get_all()[UnitTypeEquivs[class_unit_type->Slot]]];
					//also increases for other units of the class; shouldn't be a problem because we assume that only one unit type per class would be requested
				}
			}
		}
	}

private:
	unit_type_map<unsigned> &data;
};

void AiForce::count_types(unit_type_map<unsigned> &counter)
{
	AiForceCounter(this->get_units(), counter);
}

/**
**  Check if the units belongs to the force/base.
**
**  @param type   Type to check.
**
**  @return       True if it fits, false otherwise.
*/
bool AiForce::can_be_assigned_to(const unit_type &type)
{
	bool flag = false;
	unit_type_map<unsigned> counter;

	//count unit types in force
	this->count_types(counter);

	// Look what should be in the force.
	this->Completed = true;

	for (const AiUnitType &aitype : this->UnitTypes) {
		const unit_type *unit_type = aitype.Type;

		if (counter[unit_type] < aitype.Want) { //the counter includes other units of the same class
			if (UnitTypeEquivs[type.Slot] == unit_type->get_index() || type.get_unit_class() == aitype.Type->get_unit_class()) {
				if (counter[unit_type] < aitype.Want - 1) {
					this->Completed = false;
				}

				flag = true;
			} else {
				this->Completed = false;
			}
		}
	}

	return flag;
}

void AiForce::Insert(std::shared_ptr<wyrmgus::unit_ref> &&unit)
{
	this->units.push_back(std::move(unit));
}

void AiForce::Insert(CUnit *unit)
{
	this->Insert(unit->acquire_ref());
}

void AiForce::InternalRemoveUnit(CUnit *unit)
{
	unit->GroupId = 0;
}

/**
**  Ai clean units in a force.
*/
void AiForce::remove_dead_units()
{
	// Release all killed units.
	for (size_t i = 0; i != this->get_units().size();) {
		CUnit *ai_unit = *this->get_units()[i];

		if (!ai_unit->IsAlive()) {
			AiForce::InternalRemoveUnit(ai_unit);
			this->units.erase(this->units.begin() + i);
			continue;
		}
		++i;
	}
}

class AiForceRallyPointFinder
{
public:
	//Wyrmgus start
//	explicit AiForceRallyPointFinder(const CUnit &startUnit, int distance, const Vec2i &startPos, Vec2i *resultPos) :
	explicit AiForceRallyPointFinder(const CUnit &startUnit, int distance, const Vec2i &startPos, Vec2i *resultPos, int z) :
	//Wyrmgus end
		startUnit(startUnit), distance(distance), startPos(startPos),
		movemask(startUnit.Type->MovementMask & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit | tile_flag::building | tile_flag::air_building)),
		//Wyrmgus start
//		resultPos(resultPos)
		resultPos(resultPos), z(z)
		//Wyrmgus end
	{
	}

	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CUnit &startUnit;
	const int distance;
	const Vec2i startPos;
	const tile_flag movemask;
	Vec2i *resultPos;
	//Wyrmgus start
	const int z;
	//Wyrmgus end
};

VisitResult AiForceRallyPointFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	Q_UNUSED(terrainTraversal)
	Q_UNUSED(from)

	//Wyrmgus start
	if (!CMap::get()->Field(pos, z)->player_info->IsTeamExplored(*startUnit.Player)) { // don't pick unexplored positions
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	
	//Wyrmgus start
	if (!CanMoveToMask(pos, movemask, z)) { // unreachable, put this at the beginning to improve performance
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end
	
	const int minDist = 15;
	//Wyrmgus start
//	if (AiEnemyUnitsInDistance(*startUnit.Player, nullptr, pos, minDist) == false
//		&& Distance(pos, startPos) <= abs(distance - minDist)) {
//		*resultPos = pos;
//		return VisitResult::Finished;
//	}
	
	if (AiEnemyUnitsInDistance(*startUnit.Player, nullptr, pos, minDist, z) > 0) { // if there are enemies within the minimum distance here, then it is a dead end
		return VisitResult::DeadEnd;
	}
	
	if (point::distance_to(pos, startPos) <= abs(distance - minDist)) {
		*resultPos = pos;
		return VisitResult::Finished;
	}
	//Wyrmgus end
	
	
	//Wyrmgus start
	/*
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) { // reachable
	if (CanMoveToMask(pos, movemask, z)) { // reachable
	//Wyrmgus end
		return VisitResult::Ok;
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
	*/
	
	return VisitResult::Ok;
	//Wyrmgus end
}

AiForce::AiForce() : Role(AiForceRole::Default), State(AiForceAttackingState::Free), WaitOnRallyPoint(AI_WAIT_ON_RALLY_POINT)
{
}

AiForce::~AiForce()
{
}

//Wyrmgus start
//bool AiForce::NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos)
bool AiForce::NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos, int z)
//Wyrmgus end
{
	assert_throw(!this->get_units().empty());
	const CUnit &leader = **(this->get_units().front());
	//Wyrmgus start
//	const int distance = leader.MapDistanceTo(startPos);
	const int distance = leader.MapDistanceTo(startPos, z);
	//Wyrmgus end

	WaitOnRallyPoint = AI_WAIT_ON_RALLY_POINT;

	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::get()->Info->MapWidth, CMap::get()->Info->MapHeight);
	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	assert_throw(CMap::get()->Info->IsPointOnMap(startPos, z));
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	AiForceRallyPointFinder aiForceRallyPointFinder(leader, distance, leader.tilePos, resultPos);
	AiForceRallyPointFinder aiForceRallyPointFinder(leader, distance, leader.tilePos, resultPos, z);
	//Wyrmgus end

	return terrainTraversal.Run(aiForceRallyPointFinder);
}

bool AiForce::check_transporters(const QPoint &pos, const int z)
{
	const landmass *home_landmass = CMap::get()->get_tile_landmass(this->HomePos, this->HomeMapLayer);

	return AiPlayer->check_unit_transport(this->get_units(), home_landmass, pos, z);
}

std::shared_ptr<unit_ref> AiForce::take_last_unit()
{
	std::shared_ptr<unit_ref> unit_ref = vector::take_back(this->units);

	AiForce::InternalRemoveUnit(unit_ref->get());

	return unit_ref;
}

void AiForce::Remove(CUnit *unit)
{
	for (size_t i = 0; i < this->get_units().size(); ++i) {
		if (*this->get_units()[i] == unit) {
			AiForce::InternalRemoveUnit(unit);
			this->units.erase(this->units.begin() + i);
			return;
		}
	}
}

/**
**  Reset the force. But don't change its role and its demand.
*/
void AiForce::Reset(const bool types)
{
	FormerForce = -1;
	Completed = false;
	Defending = false;
	Attacking = false;
	WaitOnRallyPoint = AI_WAIT_ON_RALLY_POINT;
	if (types) {
		UnitTypes.clear();
		State = AiForceAttackingState::Free;
	} else {
		State = AiForceAttackingState::Waiting;
	}

	for (const std::shared_ptr<wyrmgus::unit_ref> &unit : this->get_units()) {
		AiForce::InternalRemoveUnit(*unit);
	}
	this->units.clear();

	HomePos.x = HomePos.y = GoalPos.x = GoalPos.y = -1;
	//Wyrmgus start
	HomeMapLayer = 0;
	GoalMapLayer = 0;
	//Wyrmgus end
}

ai_force_type AiForce::get_force_type() const
{
	if (this->get_units().empty()) {
		return ai_force_type::land;
	}
	
	ai_force_type force_type = ai_force_type::space;
	for (const std::shared_ptr<unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (unit->Type->get_domain() == unit_domain::water && unit->CanAttack() && !unit->Type->CanTransport()) { //one naval unit that can attack makes this a naval force
			return wyrmgus::ai_force_type::naval;
		}
		
		if (unit->Type->get_domain() != unit_domain::air && unit->Type->get_domain() != unit_domain::space) { //all units must be of unit_domain::air for it to be considered an air force
			force_type = ai_force_type::land;
		} else if (unit->Type->get_domain() != unit_domain::space) {
			force_type = ai_force_type::air;
		}
	}
	
	return force_type;
}

bool AiForce::IsNaval() const
{
	return this->get_force_type() == wyrmgus::ai_force_type::naval;
}

bool AiForce::IsAirForce() const
{
	return this->get_force_type() == wyrmgus::ai_force_type::air;
}

bool AiForce::IsHeroOnlyForce() const
{
	if (this->get_units().empty()) {
		return false;
	}
	
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (unit->get_character() == nullptr) {
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
	this->remove_dead_units();

	if (this->get_units().empty()) {
		this->Attacking = false;
		//Wyrmgus start
		AiPlayer->Scouting = false;
		//Wyrmgus end
		this->State = AiForceAttackingState::Waiting;
		return;
	}

	//Wyrmgus start
	if (AiPlayer->Scouting) {
		return;
	}
	//Wyrmgus end
	if (!this->Attacking) {
		// Remember the original force position so we can return there after attack
		if (this->Role == AiForceRole::Defend
			|| (this->Role == AiForceRole::Attack && this->State == AiForceAttackingState::Waiting)) {
			this->HomePos = (*this->get_units().back())->tilePos;
			this->HomeMapLayer = (*this->get_units().back())->MapLayer->ID;
		}
		this->Attacking = true;
	}
	Vec2i goalPos(pos);

	bool isNaval = this->IsNaval();
	bool isTransporter = false;
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		//Wyrmgus start
//		if (unit->Type->CanTransport() && unit->IsAgressive() == false) {
		if (unit->Type->CanTransport()) {
		//Wyrmgus end
			isTransporter = true;
			break;
		}
	}
	if (CMap::get()->Info->IsPointOnMap(goalPos, z) == false) {
		/* Search in entire map */
		const CUnit *enemy = nullptr;
		Vec2i enemy_wall_pos(-1, -1);
		int enemy_wall_map_layer = -1;
		if (isTransporter) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_AGRESSIVE>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_AGRESSIVE>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, false);
			//Wyrmgus end
		} else if (isNaval) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &enemy);
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &enemy, &enemy_wall_pos, &enemy_wall_map_layer, true);
			//Wyrmgus end
		}
		if (enemy) {
			goalPos = enemy->tilePos;
			//Wyrmgus start
			z = enemy->MapLayer->ID;
			if (!AiPlayer->Player->is_enemy_of(*enemy->Player) && enemy->Player->get_type() != player_type::neutral && !enemy->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value) {
				AiPlayer->Player->set_enemy_diplomatic_stance_with(enemy->Player);
			}
			//Wyrmgus end
		//Wyrmgus start
		} else if (CMap::get()->Info->IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
			goalPos = enemy_wall_pos;
			z = enemy_wall_map_layer;
			CPlayer *enemy_wall_owner = CMap::get()->Field(enemy_wall_pos, enemy_wall_map_layer)->get_owner();
			if (!AiPlayer->Player->is_enemy_of(*enemy_wall_owner) && enemy_wall_owner->get_type() != player_type::neutral) {
				AiPlayer->Player->set_enemy_diplomatic_stance_with(enemy_wall_owner);
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
	for (const std::shared_ptr<unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (vector::contains(AiPlayer->Scouts, unit)) {
			vector::remove(AiPlayer->Scouts, unit);
		}
	}
	//Wyrmgus end

	if (CMap::get()->Info->IsPointOnMap(goalPos, z) == false || isTransporter) {
		DebugPrint("%d: Need to plan an attack with transporter\n" _C_ AiPlayer->Player->get_index());
		if (State == AiForceAttackingState::Waiting && !PlanAttack()) {
			DebugPrint("%d: Can't transport\n" _C_ AiPlayer->Player->get_index());
			Attacking = false;
		}
		return;
	}
	
	//Wyrmgus start
	if (!isTransporter && !isNaval) {
		bool needs_transport = false;
		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
			CUnit *unit = unit_ref->get();
			if (unit->Type->get_domain() != unit_domain::air && unit->Type->get_domain() != unit_domain::air_low && unit->Type->get_domain() != unit_domain::space && CMap::get()->get_tile_landmass(unit->tilePos, unit->MapLayer->ID) != CMap::get()->get_tile_landmass(goalPos, z)) {
				needs_transport = true;
				break;
			}
		}
		
		if (needs_transport) { //if is a land force attacking another landmass, see if there are enough transporters to carry it, and whether they are doing the appropriate actions
			if (!this->check_transporters(goalPos, z)) {
				this->GoalPos = goalPos;
				this->GoalMapLayer = z;
				return;
			}
		}
	}
	//Wyrmgus end
	
	if (this->State == AiForceAttackingState::Waiting && isDefenceForce == false) {
		Vec2i resultPos(-1, -1);
		NewRallyPoint(goalPos, &resultPos, z);

		if (resultPos.x != -1 && resultPos.y != -1) {
			this->GoalPos = resultPos;
			this->State = AiForceAttackingState::GoingToRallyPoint;
		} else {
			this->GoalPos = goalPos;
			this->State = AiForceAttackingState::Attacking;
		}

		this->GoalMapLayer = z;
	} else {
		this->GoalPos = goalPos;
		//Wyrmgus start
		this->GoalMapLayer = z;
		//Wyrmgus end
		this->State = AiForceAttackingState::Attacking;
	}
	//  Send all units in the force to enemy.
	
	CUnit *leader = nullptr;
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (unit->IsAgressive()) {
			leader = unit;
			break;
		}
	}

	for (size_t i = 0; i != this->get_units().size(); ++i) {
		CUnit *unit = *this->get_units()[i];
		
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
	if (CMap::get()->Info->IsPointOnMap(this->HomePos, this->HomeMapLayer)) {
		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
			CUnit *unit = unit_ref->get();
			//Wyrmgus start
			if (!unit->IsIdle()) {
				continue;
			}
			//Wyrmgus end
		
			//Wyrmgus start
//			CommandMove(*unit, this->HomePos, FlushCommands);
			CommandMove(*unit, this->HomePos, FlushCommands, this->HomeMapLayer);
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
	this->State = AiForceAttackingState::Waiting;
}

AiForceManager::AiForceManager()
{
	forces.resize(AI_MAX_FORCES);
	memset(script, -1, AI_MAX_FORCES * sizeof(char));
}

unsigned int AiForceManager::FindFreeForce(const AiForceRole role, int begin, bool allow_hero_only_force)
{
	/* find free force */
	unsigned int f = begin;
	for (; f < forces.size(); ++f) {
		if (forces[f].State == AiForceAttackingState::Free) {
			break;
		}
		
		if (allow_hero_only_force && forces[f].IsHeroOnlyForce()) {
			break;
		}
	}
	
	if (f == forces.size()) {
		forces.resize(f + 1);
	}
	forces[f].State = AiForceAttackingState::Waiting;
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
		const AiForce &force = forces[i];

		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : force.get_units()) {
			CUnit *ai_unit = unit_ref->get();
			if (UnitNumber(unit) == UnitNumber(*ai_unit)) {
				return i;
			}
		}
	}

	return -1;
}

/**
**  Cleanup units in forces.
*/
void AiForceManager::remove_dead_units()
{
	for (unsigned int i = 0; i < this->forces.size(); ++i) {
		this->forces[i].remove_dead_units();
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
//		if (f.can_be_assigned_to(*unit.Type)) {
		if (f.can_be_assigned_to(*unit.Type) || hero) {
		//Wyrmgus end
			if (hero && !f.get_units().empty()) {
				//make heroes move to where the rest of the force is
				CommandMove(unit, (*f.get_units().front())->tilePos, FlushCommands, (*f.get_units().front())->MapLayer->ID);
			}
			f.Insert(&unit);
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

		if (force.State > AiForceAttackingState::Free && force.IsAttacking()) {
			for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : force.get_units()) {
				const CUnit *unit = unit_ref->get();
				attacking[unit->Type->Slot]++;
			}
		}
	}
	// create missing units
	for (unsigned int i = 0; i < forces.size(); ++i) {
		AiForce &force = forces[i];

		// No troops for attacking force
		if (force.State == AiForceAttackingState::Free || force.IsAttacking()) {
			continue;
		}
		for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
			const AiUnitType &aiut = force.UnitTypes[j];
			const unsigned int t = aiut.Type->Slot;
			const wyrmgus::unit_class *unit_class = aiut.Type->get_unit_class();
			const int wantedCount = aiut.Want;
			int e = AiPlayer->Player->GetUnitTypeAiActiveCount(wyrmgus::unit_type::get_all()[t]);
			if (t < AiHelpers.Equiv.size()) {
				for (unsigned int k = 0; k < AiHelpers.Equiv[t].size(); ++k) {
					e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][k]);
				}
			}
			if (unit_class != nullptr) {
				for (const wyrmgus::unit_type *class_unit_type : unit_class->get_unit_types()) {
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
void AiRemoveDeadUnitsInForces()
{
	AiPlayer->Force.remove_dead_units();
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

	AiRemoveDeadUnitsInForces();
	for (int i = 0; i < n; ++i) {
		CUnit &unit = AiPlayer->Player->GetUnit(i);

		if (unit.Active && unit.GroupId == 0) {
			AiPlayer->Force.Assign(unit, force, unit.get_character() != nullptr && !unit.Type->BoolFlag[HARVESTER_INDEX].value);
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
		return;
	}

	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		DebugPrint("(%d, %d) not in the map(%d, %d)" _C_ pos.x _C_ pos.y
				   //Wyrmgus start
//				   _C_ CMap::get()->Info->MapWidth _C_ CMap::get()->Info->MapHeight);
				   _C_ CMap::get()->Info->MapWidths[z] _C_ CMap::get()->Info->MapHeights[z]);
				   //Wyrmgus end
		return;
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
		return;
	}

	unsigned int intForce = AiPlayer->Force.getScriptForce(force);
	// The AI finds the first unassigned force, moves all data to it and cleans
	// the first force, so we can reuse it
	if (!AiPlayer->Force[intForce].Defending) {
		unsigned int top;
		unsigned int f = AiPlayer->Force.FindFreeForce(AiForceRole::Default, AI_MAX_FORCE_INTERNAL);
		AiPlayer->Force[f].Reset();
		AiPlayer->Force[f].FormerForce = force;
		AiPlayer->Force[f].Role = AiPlayer->Force[intForce].Role;

		while (AiPlayer->Force[intForce].Size()) {
			std::shared_ptr<wyrmgus::unit_ref> ai_unit = AiPlayer->Force[intForce].take_last_unit();
			(*ai_unit)->GroupId = f + 1;
			AiPlayer->Force[f].Insert(std::move(ai_unit));
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
	if (!AiPlayer->Force[force].get_units().empty()) {
		z = (*AiPlayer->Force[force].get_units()[0])->MapLayer->ID;
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
void AiAttackWithForces(const std::array<int, AI_MAX_FORCE_INTERNAL + 1> &forces)
{
	const Vec2i invalidPos(-1, -1);
	bool found = false;
	unsigned int top;
	unsigned int f = AiPlayer->Force.FindFreeForce(AiForceRole::Default, AI_MAX_FORCE_INTERNAL);

	AiPlayer->Force[f].Reset();

	for (int i = 0; forces[i] != -1; ++i) {
		int force = forces[i];

		if (!AiPlayer->Force[force].Defending) {
			found = true;

			AiPlayer->Force[f].Role = AiPlayer->Force[force].Role;

			while (AiPlayer->Force[force].Size()) {
				std::shared_ptr<wyrmgus::unit_ref> ai_unit = AiPlayer->Force[force].take_last_unit();
				(*ai_unit)->GroupId = f + 1;
				AiPlayer->Force[f].Insert(std::move(ai_unit));
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
			if (!AiPlayer->Force[force].get_units().empty()) {
				z = (*AiPlayer->Force[force].get_units()[0])->MapLayer->ID;
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
		if (!AiPlayer->Force[f].get_units().empty()) {
			z = (*AiPlayer->Force[f].get_units()[0])->MapLayer->ID;
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
	assert_throw(aiForce.State == AiForceAttackingState::Boarding);

	unsigned int nbToTransport = 0;
	unsigned int transporterIndex = 0;
	bool forceIsReady = true;

	for (; transporterIndex < aiForce.get_units().size(); ++transporterIndex) {
		const CUnit *unit = *aiForce.get_units()[transporterIndex];

		if (unit->Type->CanTransport() && unit->Type->MaxOnBoard - unit->BoardCount > 0) {
			nbToTransport = unit->Type->MaxOnBoard - unit->BoardCount;
			break;
		}
	}

	if (transporterIndex == aiForce.Size()) {
		aiForce.State = AiForceAttackingState::AttackingWithTransporter;
		return;
	}

	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : aiForce.get_units()) {
		const CUnit *unit = unit_ref->get();
		const CUnit *transporter = *aiForce.get_units()[transporterIndex];

		if (CanTransport(*transporter, *unit) && unit->Container == nullptr) {
			forceIsReady = false;
			break;
		}
	}

	if (forceIsReady == true) {
		aiForce.State = AiForceAttackingState::AttackingWithTransporter;
		return;
	}

	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : aiForce.get_units()) {
		CUnit *unit = unit_ref->get();
		CUnit *transporter = *aiForce.get_units()[transporterIndex];

		if (unit->CurrentAction() == UnitAction::Board
			&& static_cast<COrder_Board *>(unit->CurrentOrder())->get_goal() == transporter) {
			CommandFollow(*transporter, *unit, 0);
		}
		if (CanTransport(*transporter, *unit) && (unit->IsIdle()
			|| (unit->CurrentAction() == UnitAction::Board && !unit->Moving
			&& static_cast<COrder_Board *>(unit->CurrentOrder())->get_goal() != transporter)) && unit->Container == nullptr) {
				CommandBoard(*unit, *transporter, FlushCommands);
				CommandFollow(*transporter, *unit, 0);
				if (--nbToTransport == 0) { // full : next transporter.
					for (++transporterIndex; transporterIndex < aiForce.get_units().size(); ++transporterIndex) {
						const CUnit *nextTransporter = *aiForce.get_units()[transporterIndex];

						if (nextTransporter->Type->CanTransport()) {
							nbToTransport = nextTransporter->Type->MaxOnBoard - nextTransporter->BoardCount;
							break;
						}
					}
					if (transporterIndex == aiForce.Size()) { // No more transporter.
						break;
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
	assert_throw(Defending == false);
	if (Size() == 0) {
		Attacking = false;
		if (!Defending && State > AiForceAttackingState::Waiting) {
			DebugPrint("%d: Attack force #%lu was destroyed, giving up\n"
					   _C_ AiPlayer->Player->get_index() _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
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
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (vector::contains(AiPlayer->Scouts, unit)) {
			vector::remove(AiPlayer->Scouts, unit);
		}
	}
	//Wyrmgus end

	//if force still has no goal, run its Attack function again to get a target
	if (CMap::get()->Info->IsPointOnMap(GoalPos, GoalMapLayer) == false) {
		const Vec2i invalidPos(-1, -1);
		//Wyrmgus start
		int z = AiPlayer->Player->StartMapLayer;
		if (!this->get_units().empty()) {
			z = (*this->get_units()[0])->MapLayer->ID;
		}
			
//		Attack(invalidPos);
		Attack(invalidPos, z);
		//Wyrmgus end
		return;
	}
	//Wyrmgus end
	Attacking = false;

	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *ai_unit = unit_ref->get();
		if (ai_unit->Type->CanAttack) {
			Attacking = true;
			break;
		}
	}
	if (Attacking == false) {
		if (!Defending && State > AiForceAttackingState::Waiting) {
			DebugPrint("%d: Attack force #%lu has lost all agresive units, giving up\n"
					   _C_ AiPlayer->Player->get_index() _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
			Reset(true);
		}
		return;
	}

#if 0
	if (State == AiForceAttackingState::Waiting) {
		if (!this->PlanAttack()) {
			DebugPrint("Can't transport, look for walls\n");
			if (!AiFindWall(this)) {
				Attacking = false;
				return;
			}
		}
		State = AiForceAttackingState::Boarding;
	}
#endif

	if (State == AiForceAttackingState::Boarding) {
		AiGroupAttackerForTransport(*this);
		return;
	}

	if (State == AiForceAttackingState::AttackingWithTransporter) {
		// Move transporters to goalpos
		std::vector<CUnit *> transporters;
		bool emptyTrans = true;

		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
			CUnit *ai_unit = unit_ref->get();
			if (ai_unit->CanMove() && ai_unit->Type->MaxOnBoard) {
				transporters.push_back(ai_unit);
				if (ai_unit->BoardCount > 0) {
					emptyTrans = false;
				}
			}
		}

		if (transporters.empty()) {
			// Our transporters have been destroyed
			DebugPrint("%d: Attack force #%lu has lost all agresive units, giving up\n"
				_C_ AiPlayer->Player->get_index() _C_(long unsigned int)(this  - & (AiPlayer->Force[0])));
			Reset(true);
		} else if (emptyTrans) {
			// We have emptied our transporters, go go go
			State = AiForceAttackingState::GoingToRallyPoint;
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
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *unit = unit_ref->get();
		if (unit->Type->get_domain() != unit_domain::air && unit->Type->get_domain() != unit_domain::air_low && unit->Type->get_domain() != unit_domain::space && unit->Type->get_domain() != unit_domain::water && CMap::get()->get_tile_landmass(unit->tilePos, unit->MapLayer->ID) != CMap::get()->get_tile_landmass(this->GoalPos, this->GoalMapLayer)) {
			needs_transport = true;
			break;
		}
	}
		
	if (needs_transport) { //if is a land force attacking another landmass, see if there are enough transporters to carry it, and whether they are doing the appropriate actions
		if (!this->check_transporters(this->GoalPos, this->GoalMapLayer)) {
			return;
		}
	}
	//Wyrmgus end
	
	CUnit *leader = nullptr;
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *ai_unit = unit_ref->get();
		if (ai_unit->IsAgressive()) {
			leader = ai_unit;
			break;
		}
	}

	//Wyrmgus start
//	const int thresholdDist = 5; // Hard coded value
	const int thresholdDist = std::max(5, static_cast<int>(this->get_units().size()) / 8);
	//Wyrmgus end
	assert_throw(CMap::get()->Info->IsPointOnMap(GoalPos, GoalMapLayer));

	if (State == AiForceAttackingState::GoingToRallyPoint) {
		// Check if we are near the goalpos
		//Wyrmgus start
//		int minDist = this->get_units()[0]->MapDistanceTo(this->GoalPos);
		int minDist = (*this->get_units()[0])->MapDistanceTo(this->GoalPos, this->GoalMapLayer);
		//Wyrmgus end
		int maxDist = minDist;

		for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
			CUnit *unit = unit_ref->get();
			//Wyrmgus start
//			const int distance = unit->MapDistanceTo(this->GoalPos);
			const int distance = unit->MapDistanceTo(this->GoalPos, this->GoalMapLayer);
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
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, true);
			//Wyrmgus end
			if (!unit) {
				//Wyrmgus start
//				AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit);
				AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, true);
				//Wyrmgus end
				if (!unit && !CMap::get()->Info->IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
					//Wyrmgus start
					/*
					// No enemy found, give up
					// FIXME: should the force go home or keep trying to attack?
					DebugPrint("%d: Attack force #%lu can't find a target, giving up\n"
							   _C_ AiPlayer->Player->get_index() _C_(long unsigned int)(this - & (AiPlayer->Force[0])));
					Attacking = false;
					State = AiForceAttackingState::Waiting;
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
				if (!AiPlayer->Player->is_enemy_of(*unit->Player) && unit->Player->get_type() != player_type::neutral && !unit->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value) {
					AiPlayer->Player->set_enemy_diplomatic_stance_with(unit->Player);
				}
			} else if (CMap::get()->Info->IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
				this->GoalPos = enemy_wall_pos;
				this->GoalMapLayer = enemy_wall_map_layer;
				CPlayer *enemy_wall_owner = CMap::get()->Field(enemy_wall_pos, enemy_wall_map_layer)->get_owner();
				if (!AiPlayer->Player->is_enemy_of(*enemy_wall_owner) && enemy_wall_owner->get_type() != player_type::neutral) {
					AiPlayer->Player->set_enemy_diplomatic_stance_with(enemy_wall_owner);
				}
			}
			
			State = AiForceAttackingState::Attacking;
			for (size_t i = 0; i != this->get_units().size(); ++i) {
				CUnit *ai_unit = *this->get_units()[i];
				
				//Wyrmgus start
				if (!ai_unit->IsIdle()) {
					continue;
				}
				//Wyrmgus end
				
				//Wyrmgus start
//				const int delay = i / 5; // To avoid lot of CPU consuption, send them with a small time difference.
				const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.
				//Wyrmgus end

				//Wyrmgus start
//				ai_unit->Wait = delay;
				ai_unit->Wait += delay;
				//Wyrmgus end

				if (ai_unit->IsAgressive()) {
					CommandAttack(*ai_unit, this->GoalPos, nullptr, FlushCommands, this->GoalMapLayer);
				} else {
					if (leader) {
						CommandDefend(*ai_unit, *leader, FlushCommands);
					} else {
						//Wyrmgus start
//						CommandMove(*ai_unit, this->GoalPos, FlushCommands);
						CommandMove(*ai_unit, this->GoalPos, FlushCommands, this->GoalMapLayer);
						//Wyrmgus end
					}
				}
			}
		}
	}

	std::vector<CUnit *> idleUnits;
	for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : this->get_units()) {
		CUnit *ai_unit = unit_ref->get();
		if (ai_unit->IsIdle()) {
			idleUnits.push_back(ai_unit);
		}
	}

	if (idleUnits.empty()) {
		return;
	}

	if (State == AiForceAttackingState::Attacking && idleUnits.size() == this->Size()) {
		const CUnit *unit = nullptr;
		Vec2i enemy_wall_pos(-1, -1);
		int enemy_wall_map_layer = -1;

		bool isNaval = this->IsNaval();
		if (isNaval) {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit);
			AiForceEnemyFinder<AIATTACK_ALLMAP>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit);
			AiForceEnemyFinder<AIATTACK_BUILDING>(*this, &unit, &enemy_wall_pos, &enemy_wall_map_layer, true);
			//Wyrmgus end
		}
		if (!unit && !CMap::get()->Info->IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
			//Wyrmgus start
			/*
			// No enemy found, give up
			// FIXME: should the force go home or keep trying to attack?
			DebugPrint("%d: Attack force #%lu can't find a target, giving up\n"
					   _C_ AiPlayer->Player->get_index() _C_(long unsigned int)(this - & (AiPlayer->Force[0])));
			Attacking = false;
			State = AiForceAttackingState::Waiting;
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
			QPoint goal_pos(-1, -1);
			int z = -1;

			if (unit != nullptr) {
				goal_pos = unit->tilePos;
				z = unit->MapLayer->ID;

				if (!AiPlayer->Player->is_enemy_of(*unit->Player) && unit->Player->get_type() != player_type::neutral && !unit->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value) {
					AiPlayer->Player->set_enemy_diplomatic_stance_with(unit->Player);
				}
			} else if (CMap::get()->Info->IsPointOnMap(enemy_wall_pos, enemy_wall_map_layer)) {
				goal_pos = enemy_wall_pos;
				z = enemy_wall_map_layer;

				CPlayer *enemy_wall_owner = CMap::get()->Field(enemy_wall_pos, enemy_wall_map_layer)->get_owner();
				if (!AiPlayer->Player->is_enemy_of(*enemy_wall_owner) && enemy_wall_owner->get_type() != player_type::neutral) {
					AiPlayer->Player->set_enemy_diplomatic_stance_with(enemy_wall_owner);
				}
			}

			Vec2i result_pos(-1, -1);
			NewRallyPoint(goal_pos, &result_pos, z);

			if (result_pos.x != -1 && result_pos.y != -1) {
				this->GoalPos = result_pos;
				this->State = AiForceAttackingState::GoingToRallyPoint;
			} else {
				this->GoalPos = goal_pos;
				this->State = AiForceAttackingState::Attacking;
			}

			this->GoalMapLayer = unit->MapLayer->ID;
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
				if (State == AiForceAttackingState::Attacking) {
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
	
	if (AiPlayer->Player->NumTownHalls < 1 && AiPlayer->Player->get_faction() != nullptr) {
		//don't build up forces if the AI has no town hall yet, but has workers to rebuild it
		const unit_type *town_hall_type = AiPlayer->Player->get_faction()->get_class_unit_type(defines::get()->get_town_hall_class());
		if (town_hall_type != nullptr && !AiPlayer->Player->get_builders(town_hall_type).empty()) {
			return;
		}
	}

	bool all_forces_completed = true;
	int completed_forces = 0;
	int force_type_count[static_cast<int>(wyrmgus::ai_force_type::count)];
	memset(force_type_count, 0, sizeof(force_type_count));
	
	for (unsigned int f = 0; f < forces.size(); ++f) {
		AiForce &force = forces[f];
		if (force.Completed && !force.get_units().empty()) {
			if (!force.IsHeroOnlyForce()) {
				completed_forces++;
				force_type_count[static_cast<int>(force.get_force_type())]++;
			}
			//attack with forces that are completed, but aren't attacking or defending
			if (!force.Attacking && !force.Defending) {
				const Vec2i invalidPos(-1, -1);
				const int z = (*force.get_units()[0])->MapLayer->ID;
				
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
				for (const std::shared_ptr<wyrmgus::unit_ref> &force_unit_ref : force.get_units()) {
					const CUnit *force_unit = force_unit_ref->get();
					completed_force_pop += force_unit->Variable[DEMAND_INDEX].Value;
				}
			}
		}
	}
	
	if (all_forces_completed && AiPlayer->Player->Race != -1 && AiPlayer->Player->get_faction() != nullptr && completed_forces < AI_MAX_COMPLETED_FORCES && completed_force_pop < AI_MAX_COMPLETED_FORCE_POP) { //all current forces completed and not too many forces are in existence, create a new one
		std::array<int, static_cast<int>(wyrmgus::ai_force_type::count)> force_type_weights{};
		for (size_t i = 0; i < force_type_weights.size(); ++i) {
			force_type_weights[i] = AiPlayer->Player->get_faction()->get_force_type_weight(static_cast<wyrmgus::ai_force_type>(i));
		}
		
		std::vector<wyrmgus::ai_force_type> force_types;
		while (static_cast<int>(force_types.size()) < static_cast<int>(wyrmgus::ai_force_type::count)) {
			for (int i = 0; i < static_cast<int>(wyrmgus::ai_force_type::count); ++i) {
				if (force_type_count[i] <= 0) {
					force_types.push_back(static_cast<wyrmgus::ai_force_type>(i));
				}
				force_type_count[i] -= force_type_weights[i];
			}
		}

		for (size_t k = 0; k < force_types.size(); ++k) {
			const std::vector<std::unique_ptr<wyrmgus::ai_force_template>> &faction_force_templates = AiPlayer->Player->get_faction()->get_ai_force_templates(force_types[k]);
			std::vector<wyrmgus::ai_force_template *> potential_force_templates;
			int priority = 0;
			for (const std::unique_ptr<wyrmgus::ai_force_template> &force_template : faction_force_templates) {
				if (force_template->get_priority() < priority) {
					break; //force templates are ordered by priority, so there is no need to go further
				}
				bool valid = true;
				for (size_t j = 0; j < force_template->get_units().size(); ++j) {
					const wyrmgus::unit_class *unit_class = force_template->get_units()[j].first;
					const wyrmgus::unit_type *unit_type = AiPlayer->Player->get_faction()->get_class_unit_type(unit_class);
					if (unit_type == nullptr || !AiRequestedTypeAllowed(*AiPlayer->Player, *unit_type)) {
						valid = false;
						break;
					}
					
					if (AiPlayer->NeededMask & AiPlayer->Player->GetUnitTypeCostsMask(unit_type)) { //don't request the force if it is going to use up a resource that is currently needed
						valid = false;
						break;
					}
				}
				if (valid) {
					if (force_template->get_priority() > priority) {
						priority = force_template->get_priority();
						potential_force_templates.clear();
					}
					for (int j = 0; j < force_template->get_weight(); ++j) {
						potential_force_templates.push_back(force_template.get());
					}
				}
			}
			
			wyrmgus::ai_force_template *force_template = nullptr;
			if (!potential_force_templates.empty()) {
				force_template = wyrmgus::vector::get_random(potential_force_templates);
			}
		
			if (force_template) {
				unsigned int new_force_id = this->FindFreeForce(AiForceRole::Default, 1, true);
				AiForce &new_force = forces[new_force_id];
				new_force.Reset(true);
				new_force.State = AiForceAttackingState::Waiting;
				new_force.Role = AiForceRole::Default;
				for (size_t i = 0; i < force_template->get_units().size(); ++i) {
					const wyrmgus::unit_class *unit_class = force_template->get_units()[i].first;
					const wyrmgus::unit_type *unit_type = AiPlayer->Player->get_faction()->get_class_unit_type(unit_class);
					const int count = force_template->get_units()[i].second;
					
					AiUnitType newaiut;
					newaiut.Want = count;
					newaiut.Type = unit_type;
					new_force.UnitTypes.push_back(std::move(newaiut));
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
			force.remove_dead_units();

			if (force.Size() == 0) {
				force.Attacking = false;
				force.Defending = false;
				force.State = AiForceAttackingState::Waiting;
				continue;
			}
			const int nearDist = 5;

			if (CMap::get()->Info->IsPointOnMap(force.GoalPos, force.GoalMapLayer) == false) {
				force.ReturnToHome();
			} else {
				//  Check if some unit from force reached goal point
				for (const std::shared_ptr<wyrmgus::unit_ref> &unit_ref : force.get_units()) {
					const CUnit *unit = unit_ref->get();
					//Wyrmgus start
//					if (unit->MapDistanceTo(force.GoalPos) <= nearDist) {
					if (unit->MapDistanceTo(force.GoalPos, force.GoalMapLayer) <= nearDist) {
					//Wyrmgus end
						//  Look if still enemies in attack range.
						const CUnit *dummy = nullptr;
						Vec2i dummy_wall_pos(-1, -1);
						int dummy_wall_map_layer = -1;
						//Wyrmgus start
//						if (!AiForceEnemyFinder<AIATTACK_RANGE>(force, &dummy).found()) {
						if (!AiForceEnemyFinder<AIATTACK_RANGE>(force, &dummy, &dummy_wall_pos, &dummy_wall_map_layer, false).found()) {
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
					   IsAnAlliedUnitOf(*(*force.get_units()[0])->Player));
				if (nearGoal.empty()) {
					force.ReturnToHome();
				} else {
					std::vector<CUnit *> idleUnits;
					for (unsigned int i = 0; i != force.Size(); ++i) {
						CUnit &aiunit = **force.get_units()[i];

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
			force.remove_dead_units();
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
