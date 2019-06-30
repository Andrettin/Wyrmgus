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
/**@name action_still.cpp - The stand still action. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_still.h"

#include "animation/animation.h"
//Wyrmgus start
#include "character.h" //for the gender identifiers
//Wyrmgus end
#include "commands.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "missile/missile.h"
#include "player.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "script.h"
#include "settings.h"
//Wyrmgus start
#include "sound/sound.h"
//Wyrmgus end
#include "species/species.h"
#include "spell/spells.h"
#include "time/time_of_day.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "video/video.h"

enum {
	SUB_STILL_STANDBY = 0,
	SUB_STILL_ATTACK
};

/* static */ COrder *COrder::NewActionStandGround()
{
	return new COrder_Still(true);
}

/* static */ COrder *COrder::NewActionStill()
{
	return new COrder_Still(false);
}

/* virtual */ void COrder_Still::Save(CFile &file, const CUnit &unit) const
{
	if (this->Action == UnitActionStill) {
		file.printf("{\"action-still\",");
	} else {
		file.printf("{\"action-stand-ground\",");
	}
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	if (this->State != 0) { // useless to write default value
		file.printf("\"state\", %d", this->State);
	}
	file.printf("}");
}

/* virtual */ bool COrder_Still::ParseSpecificData(lua_State *l, int &j, const char *value, CUnit &unit)
{
	if (!strcmp("state", value)) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_Still::IsValid() const
{
	return true;
}

/* virtual */ Vector2i COrder_Still::Show(const CViewport &, const Vector2i &lastScreenPos) const
{
	//Wyrmgus start
	/*
	if (this->Action == UnitActionStandGround) {
		Video.FillCircleClip(ColorBlack, lastScreenPos, 2);
	} else {
		Video.FillCircleClip(ColorGray, lastScreenPos, 2);
	}
	*/
	if (Preference.ShowPathlines) {
		if (this->Action == UnitActionStandGround) {
			Video.FillCircleClip(ColorBlack, lastScreenPos, 2);
		} else {
			Video.FillCircleClip(ColorGray, lastScreenPos, 2);
		}
	}
	//Wyrmgus end
	return lastScreenPos;
}

class IsTargetInRange
{
public:
	explicit IsTargetInRange(const CUnit &_attacker) : attacker(&_attacker) {}

	bool operator()(const CUnit *unit) const
	{
		return unit->IsVisibleAsGoal(*attacker->GetPlayer())
			   && IsDistanceCorrect(attacker->MapDistanceTo(*unit));
	}
private:
	bool IsDistanceCorrect(int distance) const
	{
		return attacker->GetType()->MinAttackRange < distance
			   && distance <= attacker->GetModifiedVariable(ATTACKRANGE_INDEX);
	}
private:
	const CUnit *attacker;
};


/* virtual */ void COrder_Still::OnAnimationAttack(CUnit &unit)
{
	if (unit.CanAttack(false) == false) {
		return;
	}
	
	CUnit *goal = this->GetGoal();
	if (goal == nullptr) {
		return;
	}
	if (IsTargetInRange(unit)(goal) == false) {
		this->ClearGoal();
		return;
	}

	FireMissile(unit, goal, goal->GetTilePos(), goal->GetMapLayer()->GetIndex());
	UnHideUnit(unit);
	unit.StepCount = 0;
}


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void UnHideUnit(CUnit &unit)
{
	unit.Variable[INVISIBLE_INDEX].Value = 0;
}

/**
**  Move in a random direction
**
**  @return  true if the unit moves, false otherwise
*/
static bool MoveRandomly(CUnit &unit)
{
	if (!unit.GetType()->RandomMovementProbability || SyncRand(100) > unit.GetType()->RandomMovementProbability) {
		return false;
	}
	// pick random location
	Vector2i pos = unit.GetTilePos();

	pos.x += SyncRand(unit.GetType()->RandomMovementDistance * 2 + 1) - unit.GetType()->RandomMovementDistance;
	pos.y += SyncRand(unit.GetType()->RandomMovementDistance * 2 + 1) - unit.GetType()->RandomMovementDistance;

	// restrict to map
	CMap::Map.Clamp(pos, unit.GetMapLayer()->GetIndex());

	// move if possible
	if (pos != unit.GetTilePos()) {
		// if the tile the unit is moving to happens to have a layer connector, use it, if appropriate for the unit
		bool found_connector = false;
		CUnitCache &unitcache = CMap::Map.Field(pos, unit.GetMapLayer()->GetIndex())->UnitCache;
		for (CUnitCache::iterator it = unitcache.begin(); it != unitcache.end(); ++it) {
			CUnit *connector = *it;

			if (connector->ConnectingDestination == nullptr || !unit.CanUseItem(connector)) {
				continue;
			}
			
			const CMapLayer *destination_map_layer = connector->ConnectingDestination->GetMapLayer();
			
			if (unit.IsDiurnal() && destination_map_layer->IsUnderground()) {
				continue;
			}
			
			if (unit.IsNocturnal() && !destination_map_layer->IsUnderground()) {
				if (destination_map_layer->GetTimeOfDay() == nullptr || !destination_map_layer->GetTimeOfDay()->IsNight()) {
					continue;
				}
			}
			
			//prefer terrains which this unit's species is native to; only go to other ones if is already in a non-native terrain type
			if (unit.GetType()->GetSpecies() != nullptr && unit.GetType()->GetSpecies()->IsNativeToTerrainType(CMap::Map.GetTileTopTerrain(unit.GetTilePos(), false, unit.GetMapLayer()->GetIndex()))) {
				if (!unit.GetType()->GetSpecies()->IsNativeToTerrainType(CMap::Map.GetTileTopTerrain(connector->ConnectingDestination->GetTileCenterPos(), false, connector->ConnectingDestination->GetMapLayer()->GetIndex()))) {
					continue;
				}
			}
			
			if (!UnitReachable(unit, *connector, 1, unit.GetReactionRange() * 8)) {
				continue;
			}
			
			CommandUse(unit, *connector, FlushCommands);
			return true;
		}
		
		UnmarkUnitFieldFlags(unit);
		if (UnitCanBeAt(unit, pos, unit.GetMapLayer()->GetIndex())) {
			MarkUnitFieldFlags(unit);
			//Wyrmgus start
			//prefer terrains which this unit's species is native to; only go to other ones if is already in a non-native terrain type
			if (unit.GetType()->GetSpecies() != nullptr && unit.GetType()->GetSpecies()->IsNativeToTerrainType(CMap::Map.GetTileTopTerrain(unit.GetTilePos(), false, unit.GetMapLayer()->GetIndex()))) {
				if (!unit.GetType()->GetSpecies()->IsNativeToTerrainType(CMap::Map.GetTileTopTerrain(pos, false, unit.GetMapLayer()->GetIndex()))) {
					return false;
				}
			}
			
			if (unit.GetType()->BoolFlag[PEOPLEAVERSION_INDEX].value) {
				std::vector<CUnit *> table;
				SelectAroundUnit(unit, std::max(6, unit.GetType()->RandomMovementDistance), table, HasNotSamePlayerAs(*unit.GetPlayer()));
				if (!table.size()) { //only avoid going near a settled area if isn't already surrounded by civilizations' units
					//don't go near settled areas
					Vector2i minpos = pos;
					Vector2i maxpos = pos;
					minpos.x = pos.x - std::max(6, unit.GetType()->RandomMovementDistance);
					minpos.y = pos.y - std::max(6, unit.GetType()->RandomMovementDistance);
					maxpos.x = pos.x + std::max(6, unit.GetType()->RandomMovementDistance);
					maxpos.y = pos.y + std::max(6, unit.GetType()->RandomMovementDistance);
					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, unit.GetMapLayer()->GetIndex(), HasNotSamePlayerAs(*unit.GetPlayer()));

					if (second_table.size() > 0) {
						return false;
					}
				} else { //even if is already in a settled area, don't go to places adjacent to units owned by players other than the neutral player
					Vector2i minpos = pos;
					Vector2i maxpos = pos;
					minpos.x = pos.x - 1;
					minpos.y = pos.y - 1;
					maxpos.x = pos.x + 1;
					maxpos.y = pos.y + 1;
					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, unit.GetMapLayer()->GetIndex(), HasNotSamePlayerAs(*unit.GetPlayer()));

					if (second_table.size() > 0) {
						return false;
					}
				}
			}
			
			if (!PlaceReachable(unit, pos, 1, 1, 0, 1, unit.GetReactionRange() * 8, unit.GetMapLayer()->GetIndex())) {
				return false;
			}
			
			CommandMove(unit, pos, FlushCommands, unit.GetMapLayer()->GetIndex());
			return true;
		} else {
			MarkUnitFieldFlags(unit);
		}
	}
	return false;
}

//Wyrmgus start
/**
**  Check if the unit's container has an adjacent unit owned by another non-neutral player
**
**  @return  true if the unit is now sheltered (or if exited a shelter), false otherwise
*/
static bool LeaveShelter(CUnit &unit)
{
	if (
		!unit.Container
		|| (unit.Container->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && unit.GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)
		|| (!unit.GetPlayer()->AiEnabled && !(unit.GetType()->BoolFlag[FAUNA_INDEX].value && unit.GetPlayer()->Type == PlayerNeutral))
		|| unit.Container->CanMove() //is a transporter, not a shelter
		|| !unit.Container->GetType()->CanTransport() //is not a garrisonable building
		|| (unit.Container->GetType()->BoolFlag[RECRUITHEROES_INDEX].value && unit.Character && unit.GetPlayer()->Type == PlayerNeutral) //if is a hireable hero in a hero recruitment building, don't leave it
	) {
		return false;
	}
	
	std::vector<CUnit *> table;
	if (unit.GetType()->BoolFlag[FAUNA_INDEX].value) {
		SelectAroundUnit(*unit.Container, 1, table, HasNotSamePlayerAs(*unit.GetPlayer()));
	} else {
		SelectAroundUnit(*unit.Container, unit.CurrentSightRange, table, MakeAndPredicate(IsEnemyWith(*unit.GetPlayer()), HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral])));
	}

	if (table.size() > 0) {
		CommandUnload(*unit.Container, unit.Container->GetTilePos(), &unit, FlushCommands, unit.Container->GetMapLayer()->GetIndex());
		return true;
	}

	return false;
}

/**
**	@brief	Pick up an item, if any is near the unit
**
**	@return	True if the unit picks up an item, or false otherwise
*/
static bool PickUpItem(CUnit &unit)
{
	if (
		!unit.GetType()->BoolFlag[ORGANIC_INDEX].value
		|| !unit.GetPlayer()->AiEnabled
	) {
		return false;
	}
	
	if (unit.Variable[HP_INDEX].Value == unit.GetModifiedVariable(HP_INDEX, VariableMax) && !unit.HasInventory()) { //only look for items to pick up if the unit is damaged or has an inventory
		return false;
	}

	// look for nearby items to pick up
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, unit.GetReactionRange(), table);

	for (size_t i = 0; i != table.size(); ++i) {
		if (!table[i]->Removed) {
			if (CanPickUp(unit, *table[i])) {
				if (table[i]->Variable[HITPOINTHEALING_INDEX].Value > 0 && (unit.GetModifiedVariable(HP_INDEX, VariableMax) - unit.Variable[HP_INDEX].Value) > 0) {
					if (UnitReachable(unit, *table[i], 1, unit.GetReactionRange() * 8)) {
						CommandPickUp(unit, *table[i], FlushCommands);
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
**	@brief	Move underground, if the unit is a nocturnal animal and it is no longer night
**
**	@return	True if the unit moves underground (or to another map layer which it can reach and where it is night), or false otherwise
*/
static bool MoveUnderground(CUnit &unit)
{
	if (!unit.GetType()->BoolFlag[FAUNA_INDEX].value || !unit.IsNocturnal()) {
		return false; //must be a nocturnal animal
	}
	
	if (unit.GetMapLayer()->IsUnderground()) {
		return false; //already underground
	}
	
	if (unit.GetMapLayer()->GetTimeOfDay() != nullptr && unit.GetMapLayer()->GetTimeOfDay()->IsNight()) {
		return false; //should only move underground if it is not night in the map layer where the unit currently is
	}
	
	// look for a nearby connector to an underground layer
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, unit.GetReactionRange(), table);

	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->ConnectingDestination == nullptr || !unit.CanUseItem(table[i])) {
			continue;
		}
		
		const CMapLayer *destination_map_layer = table[i]->ConnectingDestination->GetMapLayer();
		
		if (!destination_map_layer->IsUnderground()) {
			if (destination_map_layer->GetTimeOfDay() == nullptr || !destination_map_layer->GetTimeOfDay()->IsNight()) {
				continue; //the destination map layer must either be underground, or be in a night time of day
			}
		}
		
		if (!UnitReachable(unit, *table[i], 1, unit.GetReactionRange() * 8)) {
			continue;
		}
		
		CommandUse(unit, *table[i], FlushCommands);
		return true;
	}
	
	return false;
}
//Wyrmgus end

/**
**	@brief	Auto cast a spell if possible
**
**	@return	True if a spell was auto cast, false otherwise
*/
bool AutoCast(CUnit &unit)
{
	if (!unit.Removed) { // Removed units can't cast any spells, from bunker
		for (const CSpell *autocast_spell : unit.AutoCastSpells) {
			if (
				(autocast_spell->AutoCast || autocast_spell->AICast)
				&& AutoCastSpell(unit, *autocast_spell)
			) {
				return true;
			}
		}
	}
	return false;
}

class IsAReparableUnitBy
{
public:
	explicit IsAReparableUnitBy(const CUnit &_worker) : worker(&_worker) {}
	bool operator()(CUnit *unit) const
	{
		return (unit->IsTeamed(*worker)
				&& unit->GetType()->GetRepairHP()
				//Wyrmgus start
//				&& unit->Variable[HP_INDEX].Value < unit->Variable[HP_INDEX].Max
				&& unit->Variable[HP_INDEX].Value < unit->GetModifiedVariable(HP_INDEX, VariableMax)
				//Wyrmgus end
				&& unit->IsVisibleAsGoal(*worker->GetPlayer()));
	}
private:
	const CUnit *worker;
};


/**
**  Try to find a repairable unit around and return it.
**
**  @param unit   unit which can repair.
**  @param range  range to find a repairable unit.
**
**  @return       unit to repair if found, null otherwise
**
**  @todo         FIXME: find the best unit (most damaged, ...).
*/
static CUnit *UnitToRepairInRange(const CUnit &unit, int range)
{
	const Vector2i offset(range, range);

	return FindUnit_If(unit.GetTilePos() - offset, unit.GetTilePos() + offset, unit.GetMapLayer()->GetIndex(), IsAReparableUnitBy(unit));
}

/**
**  Auto repair a unit if possible
**
**  @return  true if the unit is repairing, false otherwise
*/
bool AutoRepair(CUnit &unit)
{
	//Wyrmgus start
//	const int repairRange = unit.GetType()->DefaultStat.Variables[AUTOREPAIRRANGE_INDEX].Value;
	const int repairRange = unit.Variable[AUTOREPAIRRANGE_INDEX].Value;
	//Wyrmgus end

	if (unit.AutoRepair == false || repairRange == 0) {
		return false;
	}
	CUnit *repairedUnit = UnitToRepairInRange(unit, repairRange);

	if (repairedUnit == nullptr) {
		return false;
	}
	const Vector2i invalidPos(-1, -1);
	COrder *savedOrder = nullptr;
	if (unit.CanStoreOrder(unit.CurrentOrder())) {
		savedOrder = unit.CurrentOrder()->Clone();
	}

	//Command* will clear unit.SavedOrder
	CommandRepair(unit, invalidPos, repairedUnit, FlushCommands, repairedUnit->GetMapLayer()->GetIndex());
	if (savedOrder != nullptr) {
		unit.SavedOrder = savedOrder;
	}
	return true;
}

bool COrder_Still::AutoAttackStand(CUnit &unit)
{
	//Wyrmgus start
//	if (unit.GetType()->CanAttack == false) {
	if (unit.CanAttack() == false) {
	//Wyrmgus end
		return false;
	}
	// Removed units can only attack in AttackRange, from bunker
	//Wyrmgus start
	//if unit is in a container which is attacking, and the container has a goal, use that goal (if possible) instead
//	CUnit *autoAttackUnit = AttackUnitsInRange(unit);
	CUnit *autoAttackUnit = unit.Container && unit.Container->CurrentAction() == UnitActionAttack && unit.Container->CurrentOrder()->HasGoal() ? unit.Container->CurrentOrder()->GetGoal() : AttackUnitsInRange(unit);
	//Wyrmgus end

	if (autoAttackUnit == nullptr) {
		return false;
	}
	// If unit is removed, use container's x and y
	const CUnit *firstContainer = unit.GetFirstContainer();
	if (firstContainer->MapDistanceTo(*autoAttackUnit) > unit.GetModifiedVariable(ATTACKRANGE_INDEX)) {
		return false;
	}
	if (CMap::Map.IsLayerUnderground(autoAttackUnit->GetMapLayer()->GetIndex()) && unit.GetModifiedVariable(ATTACKRANGE_INDEX) > 1 && CheckObstaclesBetweenTiles(unit.GetTilePos(), autoAttackUnit->GetTilePos(), MapFieldAirUnpassable, autoAttackUnit->GetMapLayer()->GetIndex()) == false) {
		return false;
	}
	this->State = SUB_STILL_ATTACK; // Mark attacking.
	this->SetGoal(autoAttackUnit);
	//Wyrmgus start
//	UnitHeadingFromDeltaXY(unit, autoAttackUnit->GetTilePos() + autoAttackUnit->GetType()->GetHalfTileSize() - unit.GetTilePos());
	UnitHeadingFromDeltaXY(unit, (autoAttackUnit->GetTilePos() * CMap::Map.GetMapLayerPixelTileSize(autoAttackUnit->GetMapLayer()->GetIndex())) + autoAttackUnit->GetHalfTilePixelSize() - (unit.GetTilePos() * CMap::Map.GetMapLayerPixelTileSize(autoAttackUnit->GetMapLayer()->GetIndex())) - unit.GetHalfTilePixelSize());
	//Wyrmgus end
	return true;
}

bool COrder_Still::AutoCastStand(CUnit &unit)
{
	if (!unit.Removed) { // Removed units can't cast any spells, from bunker)
		for (const CSpell *autocast_spell : unit.AutoCastSpells) {
			if (
				(autocast_spell->AutoCast || autocast_spell->AICast)
				&& AutoCastSpell(unit, *autocast_spell)
			) {
				return true;
			}
		}
	}
	return false;
}


/**
**	@brief	Auto attack nearby units if possible
*/
bool AutoAttack(CUnit &unit)
{
	//Wyrmgus start
//	if (unit.GetType()->CanAttack == false) {
	if (unit.CanAttack() == false) {
	//Wyrmgus end
		return false;
	}
	// Normal units react in reaction range.
	CUnit *goal = AttackUnitsInReactRange(unit);

	if (goal == nullptr) {
		return false;
	}
	COrder *savedOrder = nullptr;

	if (unit.CurrentAction() == UnitActionStill) {
		//Wyrmgus start
//		savedOrder = COrder::NewActionAttack(unit, unit.GetTilePos());
		savedOrder = COrder::NewActionAttack(unit, unit.GetTilePos(), unit.GetMapLayer()->GetIndex());
		//Wyrmgus end
	} else if (unit.CanStoreOrder(unit.CurrentOrder())) {
		savedOrder = unit.CurrentOrder()->Clone();
	}
	// Weak goal, can choose other unit, come back after attack
	CommandAttack(unit, goal->GetTilePos(), nullptr, FlushCommands, goal->GetMapLayer()->GetIndex());

	if (savedOrder != nullptr) {
		unit.SavedOrder = savedOrder;
	}
	return true;
}


/* virtual */ void COrder_Still::Execute(CUnit &unit)
{
	// If unit is not bunkered and removed, wait
	if (unit.Removed
		//Wyrmgus start
//		&& (unit.Container == nullptr || unit.Container->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value == false)) {
		&& (unit.Container == nullptr || !unit.Container->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value || !unit.GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)) { // make both the unit and the transporter have the tag be necessary for the attack to be possible
			if (unit.Container != nullptr) {
				LeaveShelter(unit); // leave shelter if surrounded
			}
		//Wyrmgus end
		return ;
	}
	this->Finished = false;

	switch (this->State) {
		case SUB_STILL_STANDBY:
			//Wyrmgus start
//			UnitShowAnimation(unit, unit.GetType()->Animations->Still);
			if (unit.Variable[STUN_INDEX].Value == 0) { //only show the idle animation when still if the unit is not stunned
				UnitShowAnimation(unit, unit.GetAnimations()->Still);
			}
			if (SyncRand(100000) == 0) {
				PlayUnitSound(unit, VoiceIdle);
			}
			unit.StepCount = 0;
			//Wyrmgus end
			break;
		case SUB_STILL_ATTACK: // attacking unit in attack range.
			AnimateActionAttack(unit, *this);
			break;
	}
	if (unit.Anim.Unbreakable) { // animation can't be aborted here
		return;
	}
	//Wyrmgus start
	if (unit.Variable[STUN_INDEX].Value > 0) { //if unit is stunned, remain still
		return;
	}
	//Wyrmgus end
	this->State = SUB_STILL_STANDBY;
	this->Finished = (this->Action == UnitActionStill);
	if (this->Action == UnitActionStandGround || unit.Removed || unit.CanMove() == false) {
		if (!unit.AutoCastSpells.empty()) {
			this->AutoCastStand(unit);
		}
		if (unit.IsAgressive()) {
			this->AutoAttackStand(unit);
		}
	} else {
		if (AutoCast(unit) || (unit.IsAgressive() && AutoAttack(unit))
			|| AutoRepair(unit)
			//Wyrmgus start
//			|| MoveRandomly(unit)) {
			|| MoveRandomly(unit) || PickUpItem(unit) || MoveUnderground(unit)) {
			//Wyrmgus end
		}
	}
}
