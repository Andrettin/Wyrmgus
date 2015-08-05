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
//      (c) Copyright 1998-2006 by Lutz Sammer and Jimmy Salmon
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

#include "stratagus.h"

#include "action/action_still.h"

#include "animation.h"
#include "commands.h"
#include "iolib.h"
#include "map.h"
#include "missile.h"
#include "player.h"
#include "script.h"
#include "settings.h"
//Wyrmgus start
#include "sound.h"
//Wyrmgus end
#include "spells.h"
#include "tileset.h"
#include "unit.h"
#include "unit_find.h"
#include "unittype.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end
#include "video.h"

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

/* virtual */ bool COrder_Still::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
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

/* virtual */ PixelPos COrder_Still::Show(const CViewport &, const PixelPos &lastScreenPos) const
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
		return unit->IsVisibleAsGoal(*attacker->Player)
			   && IsDistanceCorrect(attacker->MapDistanceTo(*unit));
	}
private:
	bool IsDistanceCorrect(int distance) const
	{
		return attacker->Type->MinAttackRange < distance
			   && distance <= attacker->Stats->Variables[ATTACKRANGE_INDEX].Max;
	}
private:
	const CUnit *attacker;
};


/* virtual */ void COrder_Still::OnAnimationAttack(CUnit &unit)
{
	CUnit *goal = this->GetGoal();
	if (goal == NULL) {
		return;
	}
	if (IsTargetInRange(unit)(goal) == false) {
		this->ClearGoal();
		return;
	}

	FireMissile(unit, goal, goal->tilePos);
	UnHideUnit(unit);
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
	if (unit.Type->RandomMovementProbability == false
		|| ((SyncRand() % 100) > unit.Type->RandomMovementProbability)) {
		return false;
	}
	// pick random location
	Vec2i pos = unit.tilePos;

	pos.x += SyncRand(unit.Type->RandomMovementDistance * 2 + 1) - unit.Type->RandomMovementDistance;
	pos.y += SyncRand(unit.Type->RandomMovementDistance * 2 + 1) - unit.Type->RandomMovementDistance;

	// restrict to map
	Map.Clamp(pos);

	// move if possible
	if (pos != unit.tilePos) {
		UnmarkUnitFieldFlags(unit);
		if (UnitCanBeAt(unit, pos)) {
			MarkUnitFieldFlags(unit);
			//Wyrmgus start
			if (unit.Type->BoolFlag[PEOPLEAVERSION_INDEX].value) {
				std::vector<CUnit *> table;
				SelectAroundUnit(unit, std::max(6, unit.Type->RandomMovementDistance), table, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
				if (!table.size()) { //only avoid going near a settled area if isn't already surrounded by civilizations' units
					//don't go near settled areas
					Vec2i minpos = pos;
					Vec2i maxpos = pos;
					minpos.x = pos.x - std::max(6, unit.Type->RandomMovementDistance);
					minpos.y = pos.y - std::max(6, unit.Type->RandomMovementDistance);
					maxpos.x = pos.x + std::max(6, unit.Type->RandomMovementDistance);
					maxpos.y = pos.y + std::max(6, unit.Type->RandomMovementDistance);
					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, HasNotSamePlayerAs(Players[PlayerNumNeutral]));

					if (second_table.size() > 0) {
						return false;
					}
				} else { //even if is already in a settled area, don't go to places adjacent to units owned by players other than the neutral player
					Vec2i minpos = pos;
					Vec2i maxpos = pos;
					minpos.x = pos.x - 1;
					minpos.y = pos.y - 1;
					maxpos.x = pos.x + 1;
					maxpos.y = pos.y + 1;
					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, HasNotSamePlayerAs(Players[PlayerNumNeutral]));

					if (second_table.size() > 0) {
						return false;
					}
				}
			}
			//Wyrmgus end
			CommandMove(unit, pos, FlushCommands);
			return true;
		}
		MarkUnitFieldFlags(unit);
	}
	return false;
}

//Wyrmgus start
/**
**  Feed
**
**  @return  true if the unit breeds, false otherwise
*/
static bool Feed(CUnit &unit)
{
	if (!unit.Type->BoolFlag[ORGANIC_INDEX].value
		|| unit.Player->Type != PlayerNeutral || !unit.Type->BoolFlag[FAUNA_INDEX].value //only for fauna
		|| unit.Variable[HUNGER_INDEX].Value < 250 //don't feed if not hungry enough
		) {
		return false;
	}

	// look for nearby food
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, unit.CurrentSightRange, table);

	for (size_t i = 0; i != table.size(); ++i) {
		if (!table[i]->Removed && UnitReachable(unit, *table[i], unit.CurrentSightRange)) {
			if (
				((table[i]->Type->BoolFlag[DETRITUS_INDEX].value || (table[i]->CurrentAction() == UnitActionDie && table[i]->Type->BoolFlag[FLESH_INDEX].value)) && unit.Type->BoolFlag[DETRITIVORE_INDEX].value)
				|| (table[i]->Type->BoolFlag[FLESH_INDEX].value && (table[i]->Type->BoolFlag[ITEM_INDEX].value || table[i]->CurrentAction() == UnitActionDie) && unit.Type->BoolFlag[CARNIVORE_INDEX].value)
				|| (table[i]->Type->BoolFlag[VEGETABLE_INDEX].value && unit.Type->BoolFlag[HERBIVORE_INDEX].value)
				|| (table[i]->Type->BoolFlag[INSECT_INDEX].value && unit.Type->BoolFlag[INSECTIVORE_INDEX].value)
				|| (table[i]->Type->BoolFlag[DAIRY_INDEX].value && (unit.Type->BoolFlag[HERBIVORE_INDEX].value || unit.Type->BoolFlag[CARNIVORE_INDEX].value || unit.Type->BoolFlag[DETRITIVORE_INDEX].value || unit.Type->BoolFlag[INSECTIVORE_INDEX].value) && unit.Variable[HUNGER_INDEX].Value >= 900) //animals only eat cheese when very hungry
			) {
				int distance = unit.MapDistanceTo(table[i]->tilePos);
				int reach = 1;
				if (table[i]->Type->BoolFlag[DIMINUTIVE_INDEX].value || unit.Type->BoolFlag[DIMINUTIVE_INDEX].value || table[i]->CurrentAction() == UnitActionDie) {
					reach = 0;
				}
				if (reach < distance) {
					CommandMove(unit, table[i]->tilePos, FlushCommands);
				} else {
					if (!table[i]->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value && !unit.Type->BoolFlag[DIMINUTIVE_INDEX].value) { //if food is non-indestructible, and isn't too tiny to consume the food, kill the food object
						if (table[i]->IsAlive()) {
							LetUnitDie(*table[i]);
						}
					}
					unit.Variable[HUNGER_INDEX].Value = 0;
				}
				return true;
			}
		}
	}
	return false;
}

/**
**  Excrete
**
**  @return  true if the unit excretes, false otherwise
*/
static bool Excrete(CUnit &unit)
{
	if (!unit.Type->BoolFlag[ORGANIC_INDEX].value
		|| unit.Type->Excrement.empty()
		|| unit.Variable[HUNGER_INDEX].Value > 100 //only excrete if well-fed
		|| ((SyncRand() % 500) >= 1)
		|| (unit.Player->Type != PlayerNeutral && (SyncRand() % 10) >= 1) //since non-fauna units (i.e. cavalry) don't get hungry, there needs to be an extra limitation, to make them not excrete too often
	) {
		return false;
	}

	Vec2i pos = unit.tilePos;

	/*
	if (unit.Direction == LookingN) {
		pos.y += 1;
	} else if (unit.Direction == LookingNE) {
		pos.x -= 1;
		pos.y += 1;
	} else if (unit.Direction == LookingE) {
		pos.x -= 1;
	} else if (unit.Direction == LookingSE) {
		pos.x -= 1;
		pos.y -= 1;
	} else if (unit.Direction == LookingS) {
		pos.y -= 1;
	} else if (unit.Direction == LookingSW) {
		pos.x += 1;
		pos.y -= 1;
	} else if (unit.Direction == LookingW) {
		pos.x += 1;
	} else if (unit.Direction == LookingNW) {
		pos.x += 1;
		pos.y += 1;
	}
	*/

	// restrict to map
	Map.Clamp(pos);
	
	CUnit *newUnit = MakeUnitAndPlace(pos, *UnitTypeByIdent(unit.Type->Excrement), &Players[PlayerNumNeutral]);
	newUnit->Direction = unit.Direction;
	UnitUpdateHeading(*newUnit);
	return true;
}

/**
**  Breed with another animal of same species and opposite gender
**
**  @return  true if the unit breeds, false otherwise
*/
static bool Breed(CUnit &unit)
{
	if (!unit.Type->BoolFlag[ORGANIC_INDEX].value
		|| unit.Player->Type != PlayerNeutral || !unit.Type->BoolFlag[FAUNA_INDEX].value //only for fauna
		|| Players[PlayerNumNeutral].UnitTypesCount[unit.Type->Slot] >= (((Map.Info.MapWidth * Map.Info.MapHeight) / 512) / (unit.Type->TileWidth * unit.Type->TileHeight)) //there shouldn't be more than 32 critters of this type in a 128x128 map, if it is to reproduce
		|| unit.Variable[HUNGER_INDEX].Value > 500 //only breed if not hungry
		|| ((SyncRand() % 200) >= 1)) {
		return false;
	}

	if (unit.LearnedAbilities[CUpgrade::Get(unit.Type->ChildUpgrade)->ID]) { //children can't reproduce
		return false;
	}
	
	if (unit.Variable[GENDER_INDEX].Value == 1 || unit.Variable[GENDER_INDEX].Value == 2) { //if is male or female, check if has a potential mate nearby
		// look for an adjacent unit of the same type
		std::vector<CUnit *> table;
		SelectAroundUnit(unit, 1, table, HasSamePlayerAndTypeAs(unit));

		for (size_t i = 0; i != table.size(); ++i) {
			if (table[i]->Variable[GENDER_INDEX].Value != unit.Variable[GENDER_INDEX].Value) {
				CUnit *newUnit = MakeUnit(*unit.Type, unit.Player);
				DropOutOnSide(*newUnit, LookingW, &unit);
				newUnit->Variable[BIRTHCYCLE_INDEX].Enable = 1;
				newUnit->Variable[BIRTHCYCLE_INDEX].Max = GameCycle;
				newUnit->Variable[BIRTHCYCLE_INDEX].Value = GameCycle;
				newUnit->Variable[HUNGER_INDEX].Value = 500; //children start off with 50% hunger
				IndividualUpgradeAcquire(*newUnit, CUpgrade::Get(newUnit->Type->ChildUpgrade));
				unit.Variable[HUNGER_INDEX].Value += 100;
				table[i]->Variable[HUNGER_INDEX].Value += 100;
				return true;
			}
		}
	} else if (unit.Variable[GENDER_INDEX].Value == 3 && (SyncRand() % 5) < 1) { //if is asexual (like slimes), reproduce endogenously (with a lower chance than normal reproduction
		CUnit *newUnit = MakeUnit(*unit.Type, unit.Player);
		DropOutOnSide(*newUnit, LookingW, &unit);
		newUnit->Variable[BIRTHCYCLE_INDEX].Enable = 1;
		newUnit->Variable[BIRTHCYCLE_INDEX].Max = GameCycle;
		newUnit->Variable[BIRTHCYCLE_INDEX].Value = GameCycle;
		newUnit->Variable[HUNGER_INDEX].Value = 500; //children start off with 50% hunger
		IndividualUpgradeAcquire(*newUnit, CUpgrade::Get(newUnit->Type->ChildUpgrade));
		unit.Variable[HUNGER_INDEX].Value += 100;
		return true;
	}
	return false;
}
//Wyrmgus end

/**
**  Auto cast a spell if possible
**
**  @return  true if a spell was auto cast, false otherwise
*/
bool AutoCast(CUnit &unit)
{
	if (unit.AutoCastSpell && !unit.Removed) { // Removed units can't cast any spells, from bunker)
		for (unsigned int i = 0; i < SpellTypeTable.size(); ++i) {
			if (unit.AutoCastSpell[i]
				&& (SpellTypeTable[i]->AutoCast || SpellTypeTable[i]->AICast)
				&& AutoCastSpell(unit, *SpellTypeTable[i])) {
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
				&& unit->Type->RepairHP
				&& unit->Variable[HP_INDEX].Value < unit->Variable[HP_INDEX].Max
				&& unit->IsVisibleAsGoal(*worker->Player));
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
**  @return       unit to repair if found, NULL otherwise
**
**  @todo         FIXME: find the best unit (most damaged, ...).
*/
static CUnit *UnitToRepairInRange(const CUnit &unit, int range)
{
	const Vec2i offset(range, range);

	return FindUnit_If(unit.tilePos - offset, unit.tilePos + offset, IsAReparableUnitBy(unit));
}

/**
**  Auto repair a unit if possible
**
**  @return  true if the unit is repairing, false otherwise
*/
bool AutoRepair(CUnit &unit)
{
	//Wyrmgus start
//	const int repairRange = unit.Type->DefaultStat.Variables[AUTOREPAIRRANGE_INDEX].Value;
	const int repairRange = unit.Variable[AUTOREPAIRRANGE_INDEX].Value;
	//Wyrmgus end

	if (unit.AutoRepair == false || repairRange == 0) {
		return false;
	}
	CUnit *repairedUnit = UnitToRepairInRange(unit, repairRange);

	if (repairedUnit == NULL) {
		return false;
	}
	const Vec2i invalidPos(-1, -1);
	COrder *savedOrder = NULL;
	if (unit.CanStoreOrder(unit.CurrentOrder())) {
		savedOrder = unit.CurrentOrder()->Clone();
	}

	//Command* will clear unit.SavedOrder
	CommandRepair(unit, invalidPos, repairedUnit, FlushCommands);
	if (savedOrder != NULL) {
		unit.SavedOrder = savedOrder;
	}
	return true;
}

bool COrder_Still::AutoAttackStand(CUnit &unit)
{
	if (unit.Type->CanAttack == false) {
		return false;
	}
	// Removed units can only attack in AttackRange, from bunker
	//Wyrmgus start
	//if unit is in a container which is attacking, and the container has a goal, use that goal (if possible) instead
//	CUnit *autoAttackUnit = AttackUnitsInRange(unit);
	CUnit *autoAttackUnit = unit.Container && unit.Container->CurrentAction() == UnitActionAttack && unit.Container->CurrentOrder()->HasGoal() ? unit.Container->CurrentOrder()->GetGoal() : AttackUnitsInRange(unit);
	//Wyrmgus end

	if (autoAttackUnit == NULL) {
		return false;
	}
	// If unit is removed, use container's x and y
	const CUnit *firstContainer = unit.Container ? unit.Container : &unit;
	if (firstContainer->MapDistanceTo(*autoAttackUnit) > unit.Stats->Variables[ATTACKRANGE_INDEX].Max) {
		return false;
	}
	//Wyrmgus start
//	if (GameSettings.Inside && CheckObstaclesBetweenTiles(unit.tilePos, autoAttackUnit->tilePos, MapFieldRocks | MapFieldForest) == false) {
	if (CheckObstaclesBetweenTiles(unit.tilePos, autoAttackUnit->tilePos, MapFieldAirUnpassable) == false || (GameSettings.Inside && CheckObstaclesBetweenTiles(unit.tilePos, autoAttackUnit->tilePos, MapFieldRocks | MapFieldForest) == false)) {
	//Wyrmgus end
		return false;
	}
	this->State = SUB_STILL_ATTACK; // Mark attacking.
	this->SetGoal(autoAttackUnit);
	//Wyrmgus start
//	UnitHeadingFromDeltaXY(unit, autoAttackUnit->tilePos + autoAttackUnit->Type->GetHalfTileSize() - unit.tilePos);
	UnitHeadingFromDeltaXY(unit, Vec2i(autoAttackUnit->tilePos.x * PixelTileSize.x, autoAttackUnit->tilePos.y * PixelTileSize.y) + autoAttackUnit->Type->GetHalfTilePixelSize() - Vec2i(unit.tilePos.x * PixelTileSize.x, unit.tilePos.y * PixelTileSize.y) - unit.Type->GetHalfTilePixelSize());
	//Wyrmgus end
	return true;
}

bool COrder_Still::AutoCastStand(CUnit &unit)
{
	if (!unit.Removed) { // Removed units can't cast any spells, from bunker)
		for (unsigned int i = 0; i < SpellTypeTable.size(); ++i) {
			if (unit.AutoCastSpell[i]
				&& (SpellTypeTable[i]->AutoCast || SpellTypeTable[i]->AICast)
				&& AutoCastSpell(unit, *SpellTypeTable[i])) {
				return true;
			}
		}
	}
	return false;
}


/**
**  Auto attack nearby units if possible
*/
bool AutoAttack(CUnit &unit)
{
	if (unit.Type->CanAttack == false) {
		return false;
	}
	// Normal units react in reaction range.
	CUnit *goal = AttackUnitsInReactRange(unit);

	if (goal == NULL) {
		return false;
	}
	COrder *savedOrder = NULL;

	if (unit.CurrentAction() == UnitActionStill) {
		savedOrder = COrder::NewActionAttack(unit, unit.tilePos);
	} else if (unit.CanStoreOrder(unit.CurrentOrder())) {
		savedOrder = unit.CurrentOrder()->Clone();
	}
	// Weak goal, can choose other unit, come back after attack
	CommandAttack(unit, goal->tilePos, NULL, FlushCommands);

	if (savedOrder != NULL) {
		unit.SavedOrder = savedOrder;
	}
	return true;
}


/* virtual */ void COrder_Still::Execute(CUnit &unit)
{
	// If unit is not bunkered and removed, wait
	if (unit.Removed
		//Wyrmgus start
//		&& (unit.Container == NULL || unit.Container->Type->AttackFromTransporter == false)) {
		&& (unit.Container == NULL || !unit.Container->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value || !unit.Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)) { // make both the unit and the transporter have the tag be necessary for the attack to be possible
		//Wyrmgus end
		return ;
	}
	this->Finished = false;

	//Wyrmgus start
	CAnimations *animations = unit.Type->Animations;
	VariationInfo *varinfo = unit.Type->VarInfo[unit.Variation];
	if (varinfo && varinfo->Animations) {
		animations = varinfo->Animations;
	}
	//Wyrmgus end
	switch (this->State) {
		case SUB_STILL_STANDBY:
			//Wyrmgus start
//			UnitShowAnimation(unit, unit.Type->Animations->Still);
			UnitShowAnimation(unit, animations->Still);
			if (SyncRand(1000) == 0) {
				PlayUnitSound(unit, VoiceIdle);
			}
			//Wyrmgus end
			break;
		case SUB_STILL_ATTACK: // attacking unit in attack range.
			//Wyrmgus start
//			AnimateActionAttack(unit, *this);
			bool ranged = false;
			if (
				(
					(this->GetGoal() && (unit.MapDistanceTo(*this->GetGoal()) > 1 || (unit.Type->UnitType != UnitTypeFly && this->GetGoal()->Type->UnitType == UnitTypeFly) || (unit.Type->UnitType == UnitTypeFly && this->GetGoal()->Type->UnitType != UnitTypeFly)))
					|| unit.Container
				)
				&& unit.Variable[ATTACKRANGE_INDEX].Value > 1
			) {
				ranged = true;
			}
			AnimateActionAttack(unit, *this, ranged);
			//Wyrmgus end
			break;
	}
	if (unit.Anim.Unbreakable) { // animation can't be aborted here
		return;
	}
	this->State = SUB_STILL_STANDBY;
	this->Finished = (this->Action == UnitActionStill);
	if (this->Action == UnitActionStandGround || unit.Removed || unit.CanMove() == false) {
		if (unit.AutoCastSpell) {
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
			|| Feed(unit) || MoveRandomly(unit) || Excrete(unit) || Breed(unit)) {
			//Wyrmgus end
		}
	}
}


//@}
