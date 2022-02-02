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

/**
**  @todo FIXME: I should rewrite this action, if only the
**               new orders are supported.
*/

#include "stratagus.h"

#include "action/action_attack.h"

#include "animation/animation.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "pathfinder.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "settings.h"
#include "sound/sound.h"
#include "spell/spell.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/video.h"

static constexpr int WEAK_TARGET = 2; //weak target, could be changed
static constexpr int MOVE_TO_TARGET = 4; ///move to target state
static constexpr int ATTACK_TARGET = 5; //attack target state

/**
**  Animate unit attack!
**
**  @param unit  Unit, for that the attack animation is played.
**
**  @todo manage correctly unit with no animation attack.
*/
void AnimateActionAttack(CUnit &unit, COrder &order)
{
	//  No animation.
	//  So direct fire missile.
	//  FIXME : wait a little.
	//Wyrmgus start
	/*
	if (unit.Type->Animations && unit.Type->Animations->RangedAttack && unit.IsAttackRanged(order.GetGoal(), order.GetGoalPos())) {
		UnitShowAnimation(unit, unit.Type->Animations->RangedAttack);
	} else {
		if (!unit.Type->Animations || !unit.Type->Animations->Attack) {
			order.OnAnimationAttack(unit);
			return;
		}
		UnitShowAnimation(unit, unit.Type->Animations->Attack);
	}
	*/
	if (unit.get_animation_set() && unit.get_animation_set()->RangedAttack && unit.IsAttackRanged(order.get_goal(), order.GetGoalPos(), order.GetGoalMapLayer())) {
		UnitShowAnimation(unit, unit.get_animation_set()->RangedAttack.get());
	} else {
		if (!unit.get_animation_set() || !unit.get_animation_set()->Attack) {
			order.OnAnimationAttack(unit);
			return;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Attack.get());
	}
	//Wyrmgus end
}

std::unique_ptr<COrder> COrder::NewActionAttack(const CUnit &attacker, CUnit &target)
{
	auto order = std::make_unique<COrder_Attack>(false);

	order->goalPos = target.tilePos + target.GetHalfTileSize();
	//Wyrmgus start
	order->MapLayer = target.MapLayer->ID;
	//Wyrmgus end
	// Removed, Dying handled by action routine.
	order->set_goal(&target);
	order->Range = attacker.get_best_attack_range();
	order->MinRange = attacker.Type->MinAttackRange;
	if (attacker.Player->AiEnabled && attacker.Variable[SPEED_INDEX].Value > target.Variable[SPEED_INDEX].Value && attacker.get_best_attack_range() > target.get_best_attack_range()) { //makes fast AI ranged units move away from slower targets that have smaller range
		order->MinRange = attacker.get_best_attack_range();
	}

	if (!attacker.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && !target.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && !attacker.Player->is_enemy_of(*target.Player) && target.Player->get_type() == player_type::computer && (attacker.Player->get_type() == player_type::computer || attacker.Player->get_type() == player_type::person)) {
		attacker.Player->set_enemy_diplomatic_stance_with(target.Player);
	}

	return order;
}

std::unique_ptr<COrder> COrder::NewActionAttack(const CUnit &attacker, const Vec2i &dest, int z)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(dest, z));

	auto order = std::make_unique<COrder_Attack>(false);

	if (CMap::get()->WallOnMap(dest, z) && CMap::get()->Field(dest, z)->player_info->IsTeamExplored(*attacker.Player)) {
		// FIXME: look into action_attack.cpp about this ugly problem
		order->goalPos = dest;
		order->MapLayer = z;
		order->Range = attacker.get_best_attack_range();
		order->MinRange = attacker.Type->MinAttackRange;
	} else {
		order->goalPos = dest;
		//Wyrmgus start
		order->MapLayer = z;
		//Wyrmgus end
	}
	return order;
}

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionAttackGround(const CUnit &attacker, const Vec2i &dest)
std::unique_ptr<COrder> COrder::NewActionAttackGround(const CUnit &attacker, const Vec2i &dest, int z)
//Wyrmgus end
{
	auto order = std::make_unique<COrder_Attack>(true);

	order->goalPos = dest;
	//Wyrmgus start
	order->MapLayer = z;
	order->Range = attacker.get_best_attack_range();
	order->MinRange = attacker.Type->MinAttackRange;

	return order;
}


void COrder_Attack::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	assert_throw(Action == UnitAction::Attack || Action == UnitAction::AttackGround);

	if (Action == UnitAction::Attack) {
		file.printf("{\"action-attack\",");
	} else {
		file.printf("{\"action-attack-ground\",");
	}
	file.printf(" \"range\", %d,", this->Range);
	file.printf(" \"min-range\", %d,", this->MinRange);

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	file.printf(" \"state\", %d", this->State);
	file.printf("}");
}

bool COrder_Attack::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
		
	if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "min-range")) {
		++j;
		this->MinRange = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

bool COrder_Attack::IsValid() const
{
	if (Action == UnitAction::Attack) {
		if (this->has_goal()) {
			return this->get_goal()->IsAliveOnMap();
		} else {
			return CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer);
		}
	} else {
		assert_throw(Action == UnitAction::AttackGround);
		return CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer);
	}
}

PixelPos COrder_Attack::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PixelPos targetPos;

	if (this->has_goal()) {
		if (this->get_goal()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		targetPos = vp.scaled_map_to_screen_pixel_pos(this->get_goal()->get_scaled_map_pixel_pos_center());
	} else {
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorRed, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorRed, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(IsWeakTargetSelected() ? ColorBlue : ColorRed, targetPos, (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}
	return targetPos;
}

void COrder_Attack::UpdatePathFinderData(PathFinderInput &input)
{
	Vec2i tileSize;
	if (this->has_goal()) {
		CUnit *goal = this->get_goal();
		tileSize = goal->get_tile_size();
		//Wyrmgus start
//		input.SetGoal(goal->tilePos, tileSize);
		input.SetGoal(goal->tilePos, tileSize, goal->MapLayer->ID);
		//Wyrmgus end
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		//Wyrmgus start
//		input.SetGoal(this->goalPos, tileSize);
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
		//Wyrmgus end
	}

	input.SetMinRange(this->MinRange);
	int distance = this->Range;
	if (input.GetUnit()->get_best_attack_range() > 1) {
		if (!CheckObstaclesBetweenTiles(input.GetUnitPos(), this->has_goal() ? this->get_goal()->tilePos : this->goalPos, tile_flag::air_impassable, this->MapLayer)) {
			distance = 1;
		}
	}
	input.SetMaxRange(distance);
}

void COrder_Attack::OnAnimationAttack(CUnit &unit)
{
	//Wyrmgus start
//	assert_throw(unit.Type->CanAttack);
	if (!unit.CanAttack(false)) {
		return;
	}
	//Wyrmgus end

	//Wyrmgus start
//	FireMissile(unit, this->get_goal(), this->goalPos);
	FireMissile(unit, this->get_goal(), this->goalPos, this->MapLayer);
	//Wyrmgus end
	UnHideUnit(unit); // unit is invisible until attacks
	unit.reset_step_count();
}

bool COrder_Attack::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/)
{
	CUnit *goal = this->get_goal();

	if (goal) {
		if (goal->IsAlive() == false) {
			this->clear_goal();
			this->goalPos = goal->tilePos;
			this->MapLayer = goal->MapLayer->ID;
			return false;
		}
		if (goal == attacker) {
			return true;
		}
		//Wyrmgus start
//		if (goal->CurrentAction() == UnitAction::Attack) {
		if (goal->CurrentAction() == UnitAction::Attack && unit.MapDistanceTo(*goal) <= unit.get_best_attack_range()) {
		//Wyrmgus end
			const COrder_Attack &order = *static_cast<COrder_Attack *>(goal->CurrentOrder());
			if (order.get_goal() == &unit) {
				//we already fight with one of attackers;
				return true;
			}
		}
	}
	return false;
}

bool COrder_Attack::IsWeakTargetSelected() const
{
	return (this->State & WEAK_TARGET) != 0;
}

/**
**  Check for dead goal.
**
**  @warning  The caller must check, if he likes the restored SavedOrder!
**
**  @todo     If a unit enters an building, than the attack choose an
**            other goal, perhaps it is better to wait for the goal?
**
**  @param unit  Unit using the goal.
**
**  @return      true if order have changed, false else.
*/
bool COrder_Attack::check_for_invalid_goal(CUnit &unit)
{
	CUnit *goal = this->get_goal();

	//position
	if (goal == nullptr) {
		return false;
	}

	//valid target
	if (goal->IsVisibleAsGoal(*unit.Player) && unit.Type->can_target(goal)) {
		return false;
	}

	// Goal could be destroyed or unseen
	// So, cannot use type.
	this->goalPos = goal->tilePos;
	this->MapLayer = goal->MapLayer->ID;
	this->MinRange = 0;
	this->Range = 0;
	this->clear_goal();

	// If we have a saved order continue this saved order.
	if (unit.RestoreOrder()) {
		return true;
	}

	return false;
}

/**
**  Change invalid target for new target in range.
**
**  @param unit  Unit to check if goal is in range
**
**  @return      true if order(action) have changed, false else (if goal change return false).
*/
bool COrder_Attack::CheckForTargetInRange(CUnit &unit)
{
	// Target is dead?
	if (this->check_for_invalid_goal(unit)) {
		return true;
	}
	
	// No goal: if meeting enemy attack it.
	if (!this->has_goal()
		&& this->Action != UnitAction::AttackGround
		//Wyrmgus start
//		&& !CMap::get()->WallOnMap(this->goalPos)) {
		&& !CMap::get()->WallOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
		CUnit *goal = AttackUnitsInReactRange(unit);

		if (goal) {
			//Wyrmgus start
//			std::unique_ptr<COrder> saved_order = COrder::NewActionAttack(unit, this->goalPos);
			std::unique_ptr<COrder> saved_order = COrder::NewActionAttack(unit, this->goalPos, this->MapLayer);
			//Wyrmgus end

			if (unit.CanStoreOrder(saved_order.get()) == false) {
				saved_order.reset();
			} else {
				unit.SavedOrder = std::move(saved_order);
			}
			this->set_goal(goal);
			this->MinRange = unit.Type->MinAttackRange;
			if (unit.Player->AiEnabled && unit.Variable[SPEED_INDEX].Value > goal->Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > goal->get_best_attack_range()) { //makes fast AI ranged units move away from slower targets that have smaller range
				this->MinRange = unit.get_best_attack_range();
			}
			this->Range = unit.get_best_attack_range();
			this->goalPos = goal->tilePos;
			this->MapLayer = goal->MapLayer->ID;
			this->State |= WEAK_TARGET; // weak target
		}
		// Have a weak target, try a better target.
	} else if (this->has_goal() && (this->State & WEAK_TARGET || unit.Player->AiEnabled)) {
		CUnit *goal = this->get_goal();
		CUnit *newTarget = AttackUnitsInReactRange(unit);

		if (newTarget && ThreatCalculate(unit, *newTarget) < ThreatCalculate(unit, *goal)) {
			std::unique_ptr<COrder> saved_order;
			if (unit.CanStoreOrder(this)) {
				saved_order = this->Clone();
			}
			if (saved_order != nullptr) {
				unit.SavedOrder = std::move(saved_order);
			}
			this->set_goal(newTarget);
			this->goalPos = newTarget->tilePos;
			this->MapLayer = newTarget->MapLayer->ID;
			
			//set the MinRange here as well, so that hit-and-run attacks will be conducted properly
			this->MinRange = unit.Type->MinAttackRange;
			if (unit.Player->AiEnabled && unit.Variable[SPEED_INDEX].Value > newTarget->Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > newTarget->get_best_attack_range()) { //makes fast AI ranged units move away from slower targets that have smaller range
				this->MinRange = unit.get_best_attack_range();
			}
		}
	}

	assert_throw(!unit.Type->BoolFlag[VANISHES_INDEX].value && !unit.Destroyed && !unit.Removed);
	return false;
}

/**
**  Controls moving a unit to its target when attacking
**
**  @param unit  Unit that is attacking and moving
*/
void COrder_Attack::MoveToTarget(CUnit &unit)
{
	assert_throw(!unit.Type->BoolFlag[VANISHES_INDEX].value && !unit.Destroyed && !unit.Removed);
	assert_throw(unit.CurrentOrder() == this);
	assert_throw(unit.CanMove());
	assert_throw(this->has_goal() || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer));

	int err = DoActionMove(unit);

	if (unit.Anim.Unbreakable) {
		return;
	}

	// Look if we have reached the target.
	if (err == 0 && !this->has_goal()) {
		// Check if we're in range when attacking a location and we are waiting
		if (unit.MapDistanceTo(this->goalPos, this->MapLayer) <= unit.get_best_attack_range()) {
			if (CheckObstaclesBetweenTiles(unit.tilePos, goalPos, tile_flag::air_impassable, MapLayer)) {
				err = PF_REACHED;
			}
		}
	}
	if (err >= 0) {
		if (CheckForTargetInRange(unit)) {
			return;
		}
		return;
	}
	if (err == PF_REACHED) {
		CUnit *goal = this->get_goal();
		// Have reached target? FIXME: could use the new return code?
		if (goal && unit.MapDistanceTo(*goal) <= unit.get_best_attack_range()) {
			if (CheckObstaclesBetweenTiles(unit.tilePos, goalPos, tile_flag::air_impassable, MapLayer)) {
				// Reached another unit, now attacking it
				unsigned char oldDir = unit.Direction;
				//Wyrmgus start
//				const Vec2i dir = goal->tilePos + goal->Type->GetHalfTileSize() - unit.tilePos;
				const Vec2i dir = PixelSize(PixelSize(goal->tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size();
				//Wyrmgus end
				UnitHeadingFromDeltaXY(unit, dir);
				if (unit.Type->BoolFlag[SIDEATTACK_INDEX].value) {
					unsigned char leftTurn = (unit.Direction - 2 * NextDirection) % (NextDirection * 8);
					unsigned char rightTurn = (unit.Direction + 2 * NextDirection) % (NextDirection * 8);
					if (abs(leftTurn - oldDir) < abs(rightTurn - oldDir)) {
						unit.Direction = leftTurn;
					} else {
						unit.Direction = rightTurn;
					}
					UnitUpdateHeading(unit);
				}
				this->State++;
				return;
			}
		}
		// Attacking wall or ground.
		if (((goal && goal->Type && goal->Type->BoolFlag[WALL_INDEX].value)
			//Wyrmgus start
//			 || (!goal && (Map.WallOnMap(this->goalPos) || this->Action == UnitAction::AttackGround)))
			 || (!goal && (this->Action == UnitAction::AttackGround || CMap::get()->WallOnMap(this->goalPos, this->MapLayer))))
			//Wyrmgus end
			&& unit.MapDistanceTo(this->goalPos, this->MapLayer) <= unit.get_best_attack_range()) {
			if (CheckObstaclesBetweenTiles(unit.tilePos, goalPos, tile_flag::air_impassable, MapLayer)) {
				// Reached wall or ground, now attacking it
				unsigned char oldDir = unit.Direction;
				UnitHeadingFromDeltaXY(unit, this->goalPos - unit.tilePos);
				if (unit.Type->BoolFlag[SIDEATTACK_INDEX].value) {
					unsigned char leftTurn = (unit.Direction - 2 * NextDirection) % (NextDirection * 8);
					unsigned char rightTurn = (unit.Direction + 2 * NextDirection) % (NextDirection * 8);
					if (abs(leftTurn - oldDir) < abs(rightTurn - oldDir)) {
						unit.Direction = leftTurn;
					} else {
						unit.Direction = rightTurn;
					}
					UnitUpdateHeading(unit);
				}
				this->State &= WEAK_TARGET;
				this->State |= ATTACK_TARGET;
				return;
			}
		}
	}
	// Unreachable.

	if (err == PF_UNREACHABLE) {
		if (!this->has_goal()) {
			// When attack-moving we have to allow a bigger range
			this->Range++;
			unit.Wait = 5;
			return;
		} else {
			this->clear_goal();
		}
	}

	// Return to old task?
	if (!unit.RestoreOrder()) {
		this->Finished = true;
	}
}

/**
**  Handle attacking the target.
**
**  @param unit  Unit, for that the attack is handled.
*/
void COrder_Attack::AttackTarget(CUnit &unit)
{
	assert_throw(this->has_goal() || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer));

	AnimateActionAttack(unit, *this);
	if (unit.Anim.Unbreakable) {
		return;
	}

	//Wyrmgus start
//	if (!this->has_goal() && (this->Action == UnitAction::AttackGround || CMap::get()->WallOnMap(this->goalPos))) {
	if (!this->has_goal() && (this->Action == UnitAction::AttackGround || CMap::get()->WallOnMap(this->goalPos, this->MapLayer))) {
	//Wyrmgus end
		return;
	}

	// Target is dead ? Change order ?
	if (this->check_for_invalid_goal(unit)) {
		return;
	}

	CUnit *goal = this->get_goal();

	// No target choose one.
	if (!goal) {
		goal = AttackUnitsInReactRange(unit);

		// No new goal, continue way to destination.
		if (!goal) {
			// Return to old task ?
			if (unit.RestoreOrder()) {
				return;
			}
			this->State = MOVE_TO_TARGET;
			return;
		}
		// Save current command to come back.
		//Wyrmgus start
//		std::unique_ptr<COrder> saved_order = COrder::NewActionAttack(unit, this->goalPos);
		std::unique_ptr<COrder> saved_order = COrder::NewActionAttack(unit, this->goalPos, this->MapLayer);
		//Wyrmgus end

		if (unit.CanStoreOrder(saved_order.get()) == false) {
			saved_order.reset();
		} else {
			unit.SavedOrder = std::move(saved_order);
		}
		this->set_goal(goal);
		this->goalPos = goal->tilePos;
		this->MapLayer = goal->MapLayer->ID;
		this->MinRange = unit.Type->MinAttackRange;
		if (unit.Player->AiEnabled && unit.Variable[SPEED_INDEX].Value > goal->Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > goal->get_best_attack_range()) { //makes fast AI ranged units move away from slower targets that have smaller range
			this->MinRange = unit.get_best_attack_range();
		}
		this->Range = unit.get_best_attack_range();
		this->State |= WEAK_TARGET;

		// Have a weak target, try a better target.
		// FIXME: if out of range also try another target quick
	} else {
		if ((this->State & WEAK_TARGET)) {
			CUnit *newTarget = AttackUnitsInReactRange(unit);
			if (newTarget && ThreatCalculate(unit, *newTarget) < ThreatCalculate(unit, *goal)) {
				if (unit.CanStoreOrder(this)) {
					unit.SavedOrder = this->Clone();
				}
				goal = newTarget;
				this->set_goal(newTarget);
				this->goalPos = newTarget->tilePos;
				this->MapLayer = newTarget->MapLayer->ID;
				this->MinRange = unit.Type->MinAttackRange;
				if (unit.Player->AiEnabled && unit.Variable[SPEED_INDEX].Value > newTarget->Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > newTarget->get_best_attack_range()) { //makes fast AI ranged units move away from slower targets that have smaller range
					this->MinRange = unit.get_best_attack_range();
				}
				this->State = MOVE_TO_TARGET;
			}
		}
	}

	// Still near to target, if not goto target.
	const int dist = unit.MapDistanceTo(*goal);
	if (dist > unit.get_best_attack_range()
		|| (CheckObstaclesBetweenTiles(unit.tilePos, goal->tilePos, tile_flag::air_impassable, MapLayer) == false)) {
		//Wyrmgus start
		// towers don't chase after goal
		/*
		if (unit.CanMove()) {
			if (unit.CanStoreOrder(this)) {
				if (dead) {
					//Wyrmgus start
//					unit.SavedOrder = COrder::NewActionAttack(unit, this->goalPos);
					unit.SavedOrder = COrder::NewActionAttack(unit, this->goalPos, this->MapLayer);
					//Wyrmgus end
				} else {
					unit.SavedOrder = this->Clone();
				}
			}
		}
		*/
		//Wyrmgus end
		unit.Frame = 0;
		this->State &= WEAK_TARGET;
		this->State |= MOVE_TO_TARGET;
	}
	if (
		dist < unit.Type->MinAttackRange
		|| (unit.Player->AiEnabled && dist < unit.get_best_attack_range() && unit.Variable[SPEED_INDEX].Value > goal->Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > goal->get_best_attack_range()) //makes fast AI ranged units move away from slower targets that have smaller range
	) {
		this->State = MOVE_TO_TARGET;
	}

	// Turn always to target
	if (goal) {
		//Wyrmgus start
//		const Vec2i dir = goal->tilePos + goal->Type->GetHalfTileSize() - unit.tilePos;
		const Vec2i dir = PixelSize(PixelSize(goal->tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size();
		//Wyrmgus end
		unsigned char oldDir = unit.Direction;
		UnitHeadingFromDeltaXY(unit, dir);
		if (unit.Type->BoolFlag[SIDEATTACK_INDEX].value) {
			unsigned char leftTurn = (unit.Direction - 2 * NextDirection) % (NextDirection * 8);
			unsigned char rightTurn = (unit.Direction + 2 * NextDirection) % (NextDirection * 8);
			if (abs(leftTurn - oldDir) < abs(rightTurn - oldDir)) {
				unit.Direction = leftTurn;
			} else {
				unit.Direction = rightTurn;
			}
			UnitUpdateHeading(unit);
		}
	}
}

/**
**  Unit attacks!
**
**  if (SubAction & WEAK_TARGET) is true the goal is a weak goal.
**  This means the unit AI (little AI) could choose a new better goal.
**
**  @todo  Lets do some tries to reach the target.
**         If target place is not reachable, choose better goal to reduce
**         the pathfinder load.
**
**  @param unit  Unit, for that the attack is handled.
*/
void COrder_Attack::Execute(CUnit &unit)
{
	assert_throw(this->has_goal() || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer));

	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	
	//Wyrmgus start
	if (!unit.CanAttack(true) && !this->has_goal()) {
		//if unit is a transporter that can't attack, return false if the original target no longer exists
		this->Finished = true;
		return;
	}
	//Wyrmgus end

	switch (this->State) {
		case 0: { // First entry
			// did Order change ?
			if (CheckForTargetInRange(unit)) {
				return;
			}
			// Can we already attack ?
			if (this->has_goal()) {
				CUnit &goal = *this->get_goal();
				const int dist = goal.MapDistanceTo(unit);

				if (unit.Type->MinAttackRange < dist &&
					dist <= unit.get_best_attack_range()
					//Wyrmgus start
					&& !(unit.Player->AiEnabled && dist < unit.get_best_attack_range() && unit.Variable[SPEED_INDEX].Value > goal.Variable[SPEED_INDEX].Value && unit.get_best_attack_range() > goal.get_best_attack_range()) //makes fast AI ranged units move away from slower targets that have smaller range
				) {
					//Wyrmgus end
					if (CheckObstaclesBetweenTiles(unit.tilePos, goalPos, tile_flag::air_impassable, MapLayer)) {
						//Wyrmgus start
//						const Vec2i dir = goal.tilePos + goal.Type->GetHalfTileSize() - unit.tilePos;
						const Vec2i dir = PixelSize(PixelSize(goal.tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal.get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size();
						//Wyrmgus end
						unsigned char oldDir = unit.Direction;
						UnitHeadingFromDeltaXY(unit, dir);
						if (unit.Type->BoolFlag[SIDEATTACK_INDEX].value) {
							unsigned char leftTurn = (unit.Direction - 2 * NextDirection) % (NextDirection * 8);
							unsigned char rightTurn = (unit.Direction + 2 * NextDirection) % (NextDirection * 8);
							if (abs(leftTurn - oldDir) < abs(rightTurn - oldDir)) {
								unit.Direction = leftTurn;
							} else {
								unit.Direction = rightTurn;
							}
							UnitUpdateHeading(unit);
						}
						this->State |= ATTACK_TARGET;
						AttackTarget(unit);
						return;
					}
				}
			//Wyrmgus start
			// add instance for attack ground without moving
			} else if (this->Action == UnitAction::AttackGround && unit.MapDistanceTo(this->goalPos, this->MapLayer) <= unit.get_best_attack_range() && unit.Type->MinAttackRange < unit.MapDistanceTo(this->goalPos, this->MapLayer)) {
				if (CheckObstaclesBetweenTiles(unit.tilePos, goalPos, tile_flag::air_impassable, MapLayer)) {
					// Reached wall or ground, now attacking it
					unsigned char oldDir = unit.Direction;
					UnitHeadingFromDeltaXY(unit, this->goalPos - unit.tilePos);
					if (unit.Type->BoolFlag[SIDEATTACK_INDEX].value) {
						unsigned char leftTurn = (unit.Direction - 2 * NextDirection) % (NextDirection * 8);
						unsigned char rightTurn = (unit.Direction + 2 * NextDirection) % (NextDirection * 8);
						if (abs(leftTurn - oldDir) < abs(rightTurn - oldDir)) {
							unit.Direction = leftTurn;
						} else {
							unit.Direction = rightTurn;
						}
						UnitUpdateHeading(unit);
					}
					this->State &= WEAK_TARGET;
					this->State |= ATTACK_TARGET;
					return;
				}
			//Wyrmgus end
			}
			this->State = MOVE_TO_TARGET;
			// FIXME: should use a reachable place to reduce pathfinder time.
		}
		// FALL THROUGH
		case MOVE_TO_TARGET:
		case MOVE_TO_TARGET + WEAK_TARGET:
			if (!unit.CanMove()) {
				this->Finished = true;
				return;
			}
			MoveToTarget(unit);
			break;

		case ATTACK_TARGET:
		case ATTACK_TARGET + WEAK_TARGET:
			AttackTarget(unit);
			break;

		case WEAK_TARGET:
			DebugPrint("FIXME: wrong entry.\n");
			break;
	}
}
