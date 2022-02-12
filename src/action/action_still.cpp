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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_still.h"

#include "animation/animation.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "character.h" //for the gender identifiers
//Wyrmgus end
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "settings.h"
//Wyrmgus start
#include "sound/sound.h"
//Wyrmgus end
#include "sound/unit_sound_type.h"
#include "species/species.h"
#include "spell/spell.h"
#include "spell/status_effect.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "util/util.h"
#include "util/vector_util.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionStandGround()
{
	return std::make_unique<COrder_Still>(true);
}

std::unique_ptr<COrder> COrder::NewActionStill()
{
	return std::make_unique<COrder_Still>(false);
}


void COrder_Still::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	if (this->Action == UnitAction::Still) {
		file.printf("{\"action-still\",");
	} else {
		file.printf("{\"action-stand-ground\",");
	}
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	if (this->current_state != state::standby) { //useless to write default value
		file.printf("\"state\", %d", static_cast<int>(this->current_state));
	}
	file.printf("}");
}

bool COrder_Still::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp("state", value)) {
		++j;
		this->current_state = static_cast<state>(LuaToNumber(l, -1, j + 1));
	} else {
		return false;
	}
	return true;
}

bool COrder_Still::IsValid() const
{
	return true;
}

PixelPos COrder_Still::Show(const CViewport &, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (preferences::get()->are_pathlines_enabled()) {
		if (this->Action == UnitAction::StandGround) {
			Video.FillCircleClip(ColorBlack, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		} else {
			Video.FillCircleClip(ColorGray, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		}
	}

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
		return attacker->Type->MinAttackRange < distance && distance <= attacker->get_best_attack_range();
	}
private:
	const CUnit *attacker;
};

void COrder_Still::OnAnimationAttack(CUnit &unit)
{
	if (unit.CanAttack(false) == false) {
		return;
	}
	
	CUnit *goal = this->get_goal();
	if (goal == nullptr) {
		return;
	}
	if (IsTargetInRange(unit)(goal) == false) {
		this->clear_goal();
		return;
	}

	FireMissile(unit, goal, goal->tilePos, goal->MapLayer->ID);
	UnHideUnit(unit);
	unit.reset_step_count();
}

void UnHideUnit(CUnit &unit)
{
	unit.remove_status_effect(status_effect::invisible);
}

/**
**  Move in a random direction
**
**  @return  true if the unit moves, false otherwise
*/
static bool MoveRandomly(CUnit &unit)
{
	int random_movement_probability = unit.Type->get_random_movement_probability();

	if (unit.Player->is_neutral_player() && unit.Type->get_neutral_random_movement_probability() != 0) {
		random_movement_probability = unit.Type->get_neutral_random_movement_probability();
	}

	if (random_movement_probability == 0) {
		return false;
	}

	if (SyncRand(100) > random_movement_probability) {
		return false;
	}

	if (!unit.is_ai_active()) {
		return false;
	}

	// pick random location
	QPoint pos = unit.tilePos;

	const int random_movement_distance = unit.Type->get_random_movement_distance();

	pos += QPoint(SyncRand(random_movement_distance * 2 + 1) - random_movement_distance, SyncRand(random_movement_distance * 2 + 1) - random_movement_distance);

	//restrict to map
	CMap::get()->clamp(pos, unit.MapLayer->ID);

	//move if possible
	if (pos != unit.tilePos) {
		UnmarkUnitFieldFlags(unit);

		if (UnitCanBeAt(unit, pos, unit.MapLayer->ID) && CheckObstaclesBetweenTiles(unit.tilePos, pos, unit.Type->MovementMask, unit.MapLayer->ID)) {
			MarkUnitFieldFlags(unit);
			//Wyrmgus start
			//prefer terrains which this unit's species is native to; only go to other ones if is already in a non-native terrain type
			if (unit.Type->get_species() != nullptr && vector::contains(unit.Type->get_species()->get_native_terrain_types(), CMap::get()->GetTileTopTerrain(unit.tilePos, false, unit.MapLayer->ID))) {
				if (!vector::contains(unit.Type->get_species()->get_native_terrain_types(), CMap::get()->GetTileTopTerrain(pos, false, unit.MapLayer->ID))) {
					return false;
				}
			}
			
			if (unit.Type->BoolFlag[PEOPLEAVERSION_INDEX].value) {
				//if the unit is people-averse, and is in a non-settled region, don't move to a settled region owned by a different player
				const CPlayer *unit_tile_owner = unit.get_center_tile()->get_owner();
				const CPlayer *target_tile_owner = unit.MapLayer->Field(pos)->get_owner();
				if ((unit_tile_owner == nullptr || unit_tile_owner == unit.Player) && target_tile_owner != nullptr && target_tile_owner != unit.Player) {
					return false;
				}

				std::vector<CUnit *> table;
				SelectAroundUnit(unit, std::max(6, unit.Type->get_random_movement_distance()), table, HasNotSamePlayerAs(*unit.Player));

				if (table.empty()) { //only avoid going near a settled area if isn't already surrounded by civilizations' units
					//don't go near settled areas
					const QPoint offset(std::max(6, unit.Type->get_random_movement_distance()), std::max(6, unit.Type->get_random_movement_distance()));
					const QPoint minpos = pos - offset;
					const QPoint maxpos = pos + offset;

					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, unit.MapLayer->ID, HasNotSamePlayerAs(*unit.Player));

					if (!second_table.empty()) {
						return false;
					}
				} else { //even if is already in a settled area, don't go to places adjacent to units owned by players other than the neutral player
					const QPoint offset(1, 1);
					const QPoint minpos = pos - offset;
					const QPoint maxpos = pos + offset;
					std::vector<CUnit *> second_table;
					Select(minpos, maxpos, second_table, unit.MapLayer->ID, HasNotSamePlayerAs(*unit.Player));

					if (!second_table.empty()) {
						return false;
					}
				}
			}

			CommandMove(unit, pos, FlushCommands, unit.MapLayer->ID);
			return true;
		}
		MarkUnitFieldFlags(unit);
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
		|| (unit.Container->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && unit.Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)
		|| (!unit.Player->AiEnabled && !(unit.Type->BoolFlag[FAUNA_INDEX].value && unit.Player->get_type() == player_type::neutral))
		|| unit.Container->CanMove() //is a transporter, not a shelter
		|| !unit.Container->Type->CanTransport() //is not a garrisonable building
		|| (unit.Container->Type->BoolFlag[RECRUITHEROES_INDEX].value && unit.get_character() != nullptr && unit.Player->get_type() == player_type::neutral) //if is a hireable hero in a hero recruitment building, don't leave it
	) {
		return false;
	}
	
	std::vector<CUnit *> table;
	if (unit.Type->BoolFlag[FAUNA_INDEX].value) {
		SelectAroundUnit(*unit.Container, 1, table, HasNotSamePlayerAs(*unit.Player));
	} else {
		SelectAroundUnit(*unit.Container, unit.CurrentSightRange, table, MakeAndPredicate(IsEnemyWithPlayer(*unit.Player), HasNotSamePlayerAs(*CPlayer::get_neutral_player())));
	}

	if (table.size() > 0) {
		CommandUnload(*unit.Container, unit.Container->tilePos, &unit, FlushCommands, unit.Container->MapLayer->ID);
		return true;
	}

	return false;
}

/**
**  PickUpItem
**
**  @return  true if the unit picks up an item, false otherwise
*/
static bool PickUpItem(CUnit &unit)
{
	if (
		!unit.Type->BoolFlag[ORGANIC_INDEX].value
		|| !unit.Player->AiEnabled
	) {
		return false;
	}
	
	if (unit.Variable[HP_INDEX].Value == unit.GetModifiedVariable(HP_INDEX, VariableAttribute::Max) && !unit.HasInventory()) { //only look for items to pick up if the unit is damaged or has an inventory
		return false;
	}

	// look for nearby items to pick up
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, unit.GetReactionRange(), table);

	for (CUnit *near_unit : table) {
		if (near_unit->Removed) {
			continue;
		}

		if (!CanPickUp(unit, *near_unit)) {
			continue;
		}

		bool usable_item = false;

		if (near_unit->Variable[HITPOINTHEALING_INDEX].Value > 0 && (unit.GetModifiedVariable(HP_INDEX, VariableAttribute::Max) - unit.Variable[HP_INDEX].Value) > 0) {
			usable_item = true;
		}
		
		if (!usable_item && near_unit->Variable[MANA_RESTORATION_INDEX].Value > 0 && (unit.GetModifiedVariable(MANA_INDEX, VariableAttribute::Max) - unit.Variable[MANA_INDEX].Value) > 0) {
			usable_item = true;
		}

		if (usable_item) {
			if (UnitReachable(unit, *near_unit, 1, unit.GetReactionRange() * 8)) {
				CommandPickUp(unit, *near_unit, FlushCommands);
				return true;
			}
		}
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
	if (!unit.Removed) { // Removed units can't cast any spells, from bunker)
		for (const wyrmgus::spell *spell : unit.get_autocast_spells()) {
			if ((spell->get_autocast_info() != nullptr || spell->get_ai_cast_info() != nullptr) && AutoCastSpell(unit, *spell)) {
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
				&& unit->Type->get_repair_hp() != 0
				//Wyrmgus start
//				&& unit->Variable[HP_INDEX].Value < unit->Variable[HP_INDEX].Max
				&& unit->Variable[HP_INDEX].Value < unit->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)
				//Wyrmgus end
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
**  @return       unit to repair if found, null otherwise
**
**  @todo         FIXME: find the best unit (most damaged, ...).
*/
static CUnit *UnitToRepairInRange(const CUnit &unit, int range)
{
	const Vec2i offset(range, range);

	return FindUnit_If(unit.tilePos - offset, unit.tilePos + offset, unit.MapLayer->ID, IsAReparableUnitBy(unit));
}

/**
**  Auto repair a unit if possible
**
**  @return  true if the unit is repairing, false otherwise
*/
bool AutoRepair(CUnit &unit)
{
	//Wyrmgus start
//	const int auto_repair_range = unit.Type->DefaultStat.Variables[AUTOREPAIRRANGE_INDEX].Value;
	const int auto_repair_range = unit.Variable[AUTOREPAIRRANGE_INDEX].Value;
	//Wyrmgus end

	if (unit.AutoRepair == false || auto_repair_range == 0) {
		return false;
	}

	assert_throw(unit.can_repair());
	assert_throw(unit.CanMove());

	CUnit *repairedUnit = UnitToRepairInRange(unit, auto_repair_range);

	if (repairedUnit == nullptr) {
		return false;
	}
	const Vec2i invalidPos(-1, -1);
	std::unique_ptr<COrder> saved_order;
	if (unit.CanStoreOrder(unit.CurrentOrder())) {
		saved_order = unit.CurrentOrder()->Clone();
	}

	//Command* will clear unit.SavedOrder
	CommandRepair(unit, invalidPos, repairedUnit, FlushCommands, repairedUnit->MapLayer->ID);
	if (saved_order != nullptr) {
		unit.SavedOrder = std::move(saved_order);
	}

	return true;
}

bool COrder_Still::AutoAttackStand(CUnit &unit)
{
	//Wyrmgus start
//	if (unit.Type->CanAttack == false) {
	if (unit.CanAttack() == false) {
	//Wyrmgus end
		return false;
	}
	// Removed units can only attack in AttackRange, from bunker
	//Wyrmgus start
	//if unit is in a container which is attacking, and the container has a goal, use that goal (if possible) instead
//	CUnit *autoAttackUnit = AttackUnitsInRange(unit);
	CUnit *autoAttackUnit = nullptr;
	if (unit.Container != nullptr && unit.Container->CurrentAction() == UnitAction::Attack && unit.Container->CurrentOrder()->has_goal() && !unit.Container->CurrentOrder()->get_goal()->Destroyed) {
		autoAttackUnit = unit.Container->CurrentOrder()->get_goal();
	} else {
		autoAttackUnit = AttackUnitsInRange(unit);
	}
	//Wyrmgus end

	if (autoAttackUnit == nullptr) {
		return false;
	}
	// If unit is removed, use container's x and y
	const CUnit *firstContainer = unit.GetFirstContainer();
	if (firstContainer->MapDistanceTo(*autoAttackUnit) > unit.get_best_attack_range()) {
		return false;
	}
	if (unit.get_best_attack_range() > 1 && CheckObstaclesBetweenTiles(unit.tilePos, autoAttackUnit->tilePos, tile_flag::air_impassable, autoAttackUnit->MapLayer->ID) == false) {
		return false;
	}
	this->current_state = state::attack; // Mark attacking.
	this->set_goal(autoAttackUnit);
	//Wyrmgus start
//	UnitHeadingFromDeltaXY(unit, autoAttackUnit->tilePos + autoAttackUnit->Type->GetHalfTileSize() - unit.tilePos);
	UnitHeadingFromDeltaXY(unit, PixelSize(PixelSize(autoAttackUnit->tilePos) * wyrmgus::defines::get()->get_tile_size()) + autoAttackUnit->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size());
	//Wyrmgus end
	return true;
}

bool COrder_Still::AutoCastStand(CUnit &unit)
{
	if (!unit.Removed) { // Removed units can't cast any spells, from bunker)
		for (const wyrmgus::spell *spell : unit.get_autocast_spells()) {
			if ((spell->get_autocast_info() != nullptr || spell->get_ai_cast_info() != nullptr) && AutoCastSpell(unit, *spell)) {
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
//	if (unit.Type->CanAttack == false) {
	if (unit.CanAttack() == false) {
	//Wyrmgus end
		return false;
	}
	// Normal units react in reaction range.
	CUnit *goal = AttackUnitsInReactRange(unit);

	if (goal == nullptr) {
		return false;
	}

	std::unique_ptr<COrder> saved_order;

	if (unit.CurrentAction() == UnitAction::Still) {
		//Wyrmgus start
//		saved_order = COrder::NewActionAttack(unit, unit.tilePos);
		saved_order = COrder::NewActionAttack(unit, unit.tilePos, unit.MapLayer->ID);
		//Wyrmgus end
	} else if (unit.CanStoreOrder(unit.CurrentOrder())) {
		saved_order = unit.CurrentOrder()->Clone();
	}
	// Weak goal, can choose other unit, come back after attack
	CommandAttack(unit, goal->tilePos, nullptr, FlushCommands, goal->MapLayer->ID);

	if (saved_order != nullptr) {
		unit.SavedOrder = std::move(saved_order);
	}

	return true;
}

void COrder_Still::Execute(CUnit &unit)
{
	// If unit is not bunkered and removed, wait
	if (unit.Removed
		//Wyrmgus start
//		&& (unit.Container == nullptr || unit.Container->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value == false)) {
		&& (unit.Container == nullptr || !unit.Container->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value || !unit.Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)) { // make both the unit and the transporter have the tag be necessary for the attack to be possible
			if (unit.Container != nullptr) {
				LeaveShelter(unit); // leave shelter if surrounded
			}
		//Wyrmgus end
		return;
	}

	this->Finished = false;

	switch (this->current_state) {
		case state::standby:
			if (!unit.has_status_effect(status_effect::stun)) {
				//only show the idle animation when still if the unit is not stunned
				UnitShowAnimation(unit, unit.get_animation_set()->Still);
			}
			if (SyncRand(100000) == 0) {
				PlayUnitSound(unit, unit_sound_type::idle);
			}
			unit.reset_step_count();
			break;
		case state::attack: // attacking unit in attack range.
			AnimateActionAttack(unit, *this);
			break;
	}

	if (unit.Anim.Unbreakable) {
		//animation can't be aborted here
		return;
	}

	//Wyrmgus start
	if (unit.has_status_effect(status_effect::stun)) {
		//if the unit is stunned, remain still
		return;
	}
	//Wyrmgus end

	this->current_state = state::standby;
	this->Finished = (this->Action == UnitAction::Still);

	if (this->Action == UnitAction::StandGround || unit.Removed || unit.CanMove() == false) {
		if (!unit.get_autocast_spells().empty()) {
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
			|| MoveRandomly(unit) || PickUpItem(unit)) {
			//Wyrmgus end
		}
	}
}
