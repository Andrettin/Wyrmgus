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
/**@name action_spellcast.cpp - The spell cast action. */
//
//      (c) Copyright 2000-2021 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//      and Andrettin
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

/*
** This is inherited from action_attack.c, actually spell casting will
** be considered a 'special' case attack action... //Vladi
*/

#include "stratagus.h"

#include "action/action_spellcast.h"

#include "animation.h"
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
#include "script.h"
#include "sound/sound.h"
#include "spell/spell.h"
#include "spell/spell_target_type.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_type.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionSpellCast(const wyrmgus::spell &spell, const Vec2i &pos, CUnit *target, int z, bool isAutocast)
{
	auto order = std::make_unique<COrder_SpellCast>(isAutocast);

	order->Range = spell.get_range();
	if (target) {
		// Destination could be killed.
		// Should be handled in action, but is not possible!
		// Unit::get_ref_count() is used as timeout counter.
		if (target->Destroyed) {
			// FIXME: where check if spell needs a unit as destination?
			// FIXME: target->Type is now set to 0. maybe we shouldn't bother.
			const Vec2i diag(order->Range, order->Range);
			order->goalPos = target->tilePos /* + target->GetHalfTileSize() */ - diag;
			order->MapLayer = target->MapLayer->ID;
			order->Range <<= 1;
		} else {
			order->set_goal(target);
		}
	} else {
		order->goalPos = pos;
		order->MapLayer = z;
	}
	order->SetSpell(spell);

	return order;
}

void COrder_SpellCast::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-spell-cast\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	file.printf("\"state\", %d,", this->State);
	file.printf(" \"spell\", \"%s\"", this->Spell->get_identifier().c_str());

	file.printf("}");
}

bool COrder_SpellCast::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "spell")) {
		++j;
		this->Spell = wyrmgus::spell::get(LuaToString(l, -1, j + 1));
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
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

bool COrder_SpellCast::IsValid() const
{
	Assert(Action == UnitAction::SpellCast);
	if (this->has_goal()) {
		return this->get_goal()->IsAliveOnMap();
	} else {
		return CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer);
	}
}

PixelPos COrder_SpellCast::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
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
		Video.FillCircleClip(ColorBlue, lastScreenPos, 2 * defines::get()->get_scale_factor(), render_commands);
		Video.DrawLineClip(ColorBlue, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorBlue, targetPos, 3 * defines::get()->get_scale_factor(), render_commands);
	}

	return targetPos;
}

void COrder_SpellCast::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	//Wyrmgus start
//	input.SetMaxRange(this->Range);
	int distance = this->Range;
	if (input.GetUnit()->GetModifiedVariable(ATTACKRANGE_INDEX) > 1) {
		if (!CheckObstaclesBetweenTiles(input.GetUnitPos(), this->has_goal() ? this->get_goal()->tilePos : this->goalPos, tile_flag::air_impassable, this->MapLayer)) {
			distance = 1;
		}
	}
	input.SetMaxRange(distance);
	//Wyrmgus end

	Vec2i tileSize;
	if (this->has_goal()) {
		CUnit *goal = this->get_goal();
		tileSize = goal->get_tile_size();
		input.SetGoal(goal->tilePos, tileSize, goal->MapLayer->ID);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}

/**
**  Call when animation step is "attack"
*/
void COrder_SpellCast::OnAnimationAttack(CUnit &unit)
{
	UnHideUnit(unit); // unit is invisible until attacks
	CUnit *goal = this->get_goal();
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		unit.ReCast = 0;
	} else {
		unit.ReCast = SpellCast(unit, *this->Spell, goal, this->goalPos, CMap::get()->MapLayers[this->MapLayer].get());
	}
}

/**
**  Get goal position
*/
const Vec2i COrder_SpellCast::GetGoalPos() const
{
	const Vec2i invalidPos(-1, -1);
	if (goalPos != invalidPos) {
		return goalPos;
	}
	if (this->has_goal()) {
		return this->get_goal()->tilePos;
	}
	return invalidPos;
}

//Wyrmgus start
/**
**  Get goal map layer
*/
const int COrder_SpellCast::GetGoalMapLayer() const
{
	const Vec2i invalidPos(-1, -1);
	if (goalPos != invalidPos) {
		return MapLayer;
	}
	if (this->has_goal()) {
		return this->get_goal()->MapLayer->ID;
	}
	return 0;
}
//Wyrmgus end

/**
**  Animate unit spell cast
**
**  @param unit  Unit, for that spell cast/attack animation is played.
*/
static void AnimateActionSpellCast(CUnit &unit, COrder_SpellCast &order)
{
	const wyrmgus::animation_set *animations = unit.get_animation_set();

	if (!animations || (!animations->Attack && !animations->SpellCast)) {
		// if don't have animations just cast spell
		order.OnAnimationAttack(unit);
		return;
	}
	if (animations->SpellCast) {
		UnitShowAnimation(unit, animations->SpellCast.get());
	} else {
		UnitShowAnimation(unit, animations->Attack.get());
	}
}

/**
**  Check for dead goal.
**
**  @warning  The caller must check, if he likes the restored SavedOrder!
**
**  @todo     If a unit enters into a building, then the caster chooses an
**            other goal, perhaps it is better to wait for the goal?
**
**  @param unit  Unit using the goal.
**
**  @return      true if order have changed, false else.
*/
bool COrder_SpellCast::CheckForDeadGoal(CUnit &unit)
{
	const CUnit *goal = this->get_goal();

	// Position or valid target, it is ok.
	if (!goal || goal->IsVisibleAsGoal(*unit.Player)) {
		return false;
	}

	// Goal could be destroyed or unseen
	// So, cannot use type.
	this->goalPos = goal->tilePos;
	this->MapLayer = goal->MapLayer->ID;
	this->Range = 0;
	this->clear_goal();

	// If we have a saved order continue this saved order.
	if (unit.RestoreOrder()) {
		return true;
	}
	return false;
}

/**
**  Handle moving to the target.
**
**  @param unit  Unit, for that the spell cast is handled.
*/
bool COrder_SpellCast::SpellMoveToTarget(CUnit &unit)
{
	// Unit can't move
	int err = 1;
	if (unit.CanMove()) {
		err = DoActionMove(unit);
		if (unit.Anim.Unbreakable) {
			return false;
		}
	}

	// when reached DoActionMove changes unit action
	// FIXME: use return codes from pathfinder
	CUnit *goal = this->get_goal();
	
	//Wyrmgus start
	bool obstacle_check = CheckObstaclesBetweenTiles(unit.tilePos, goal ? goal->tilePos : this->goalPos, tile_flag::air_impassable, this->MapLayer);
	//Wyrmgus end

	//Wyrmgus start
//	if (goal && unit.MapDistanceTo(*goal) <= this->Range) {
	if (goal && unit.MapDistanceTo(*goal) <= this->Range && obstacle_check) {
	//Wyrmgus end
		// there is goal and it is in range
		//Wyrmgus start
//		UnitHeadingFromDeltaXY(unit, goal->tilePos + goal->Type->GetHalfTileSize() - unit.tilePos);
		UnitHeadingFromDeltaXY(unit, PixelSize(PixelSize(goal->tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size());
		//Wyrmgus end
		this->State++; // cast the spell
		return false;
	//Wyrmgus start
//	} else if (!goal && unit.MapDistanceTo(this->goalPos, this->MapLayer) <= this->Range) {
	} else if (!goal && unit.MapDistanceTo(this->goalPos, this->MapLayer) <= this->Range && obstacle_check) {
	//Wyrmgus end
		// there is no goal and target spot is in range
		UnitHeadingFromDeltaXY(unit, this->goalPos - unit.tilePos);
		this->State++; // cast the spell
		return false;
	} else if (err == PF_UNREACHABLE || !unit.CanMove()) {
		//Wyrmgus start
		//if is unreachable and is on a raft, see if the raft can move closer to the target
		if (unit.MapLayer->Field(unit.tilePos)->has_flag(tile_flag::bridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeType::Land) {
			std::vector<CUnit *> table;
			Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
			for (size_t i = 0; i != table.size(); ++i) {
				if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
					if (table[i]->CurrentAction() == UnitAction::Still) {
						CommandStopUnit(*table[i]);
						CommandMove(*table[i], this->has_goal() ? this->get_goal()->tilePos : this->goalPos, FlushCommands, this->has_goal() ? this->get_goal()->MapLayer->ID : this->MapLayer);
					}
					return false;
				}
			}
		}
		//Wyrmgus end
		// goal/spot unreachable and out of range -- give up
		return true;
	}
	return false;
}

void COrder_SpellCast::Execute(CUnit &unit)
{
	COrder_SpellCast &order = *this;

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
	const wyrmgus::spell &spell = order.GetSpell();
	switch (this->State) {
		case 0:
			// Check if we can cast the spell.
			if (!CanCastSpell(unit, spell, order.get_goal())) {
				// Notify player about this problem
				if (unit.Variable[MANA_INDEX].Value < spell.get_mana_cost()) {
					unit.Player->Notify(NotifyYellow, unit.tilePos,
										//Wyrmgus start
										unit.MapLayer->ID,
//										_("%s: not enough mana for spell: %s"),
										_("%s does not have enough mana to use the %s ability."),
//										unit.Type->Name.c_str(), spell.Name.c_str());
										unit.GetMessageName().c_str(), spell.get_name().c_str());
										//Wyrmgus end
				} else if (unit.get_spell_cooldown_timer(&spell) > 0) {
					unit.Player->Notify(NotifyYellow, unit.tilePos,
										//Wyrmgus start
										unit.MapLayer->ID,
//										_("%s: spell is not ready yet: %s"),
										_("%s's ability %s is not ready yet."),
//										unit.Type->Name.c_str(), spell.Name.c_str());
										unit.GetMessageName().c_str(), spell.get_name().c_str());
										//Wyrmgus end
				} else if (unit.Player->CheckCosts(spell.get_costs(), false)) {
					unit.Player->Notify(NotifyYellow, unit.tilePos,
										//Wyrmgus start
										unit.MapLayer->ID,
//										_("%s: not enough resources to cast spell: %s"),
										_("You do not have enough resources for %s to use the %s ability."),
//										unit.Type->Name.c_str(), spell.Name.c_str());
										unit.GetMessageName().c_str(), spell.get_name().c_str());
										//Wyrmgus end
				//Wyrmgus start
				} else if (spell.get_target() == wyrmgus::spell_target_type::unit && order.get_goal() == nullptr) {
					unit.Player->Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID,
										_("%s needs a target to use the %s ability."),
										unit.GetMessageName().c_str(), spell.get_name().c_str());
				//Wyrmgus end
				} else {
					unit.Player->Notify(NotifyYellow, unit.tilePos,
										//Wyrmgus start
										unit.MapLayer->ID,
//										_("%s: can't cast spell: %s"),
										_("%s cannot use the %s ability."),
//										unit.Type->Name.c_str(), spell.Name.c_str());
										unit.GetMessageName().c_str(), spell.get_name().c_str());
										//Wyrmgus end
				}

				if (unit.Player->AiEnabled) {
					DebugPrint("FIXME: do we need an AI callback?\n");
				}
				if (!unit.RestoreOrder()) {
					this->Finished = true;
				}
				return ;
			}
			if (CheckForDeadGoal(unit)) {
				return;
			}
			// FIXME FIXME FIXME: Check if already in range and skip straight to 2(casting)
			unit.ReCast = 0; // repeat spell on next pass? (defaults to `no')
			this->State = 1;
		// FALL THROUGH
		case 1:                         // Move to the target.
			//Wyrmgus start
//			if (spell.Range != wyrmgus::spell::infinite_range) {
			if (spell.get_range() != wyrmgus::spell::infinite_range && spell.get_range() != 0) {
			//Wyrmgus end
				if (SpellMoveToTarget(unit) == true) {
					if (!unit.RestoreOrder()) {
						this->Finished = true;
					}
					return ;
				}
				return ;
			} else {
				this->State = 2;
			}
		// FALL THROUGH
		case 2:                         // Cast spell on the target.
			if (!spell.is_caster_only() || spell.forces_use_animation()) {
				AnimateActionSpellCast(unit, *this);
				if (unit.Anim.Unbreakable) {
					return ;
				}
			} else {
				// FIXME: what todo, if unit/goal is removed?
				CUnit *goal = order.get_goal();
				if (goal && goal != &unit && !goal->IsVisibleAsGoal(*unit.Player)) {
					unit.ReCast = 0;
				} else {
					unit.ReCast = SpellCast(unit, spell, goal, order.goalPos, CMap::get()->MapLayers[order.MapLayer].get());
				}
			}

			if (unit.IsAlive() == false) { // unit has died when casting spell (demolish)
				return;
			}
			// Target is dead ? Change order ?
			if (CheckForDeadGoal(unit)) {
				return;
			}

			// Check, if goal has moved (for ReCast)
			if (unit.ReCast) {
				if (order.has_goal() && unit.MapDistanceTo(*order.get_goal()) > this->Range) {
					this->State = 0;
					return;
				}
				if (this->isAutocast) {
					//Wyrmgus start
//					if (order.has_goal() && order.get_goal()->tilePos != order.goalPos) {
					if (order.has_goal() && (order.get_goal()->tilePos != order.goalPos || order.get_goal()->MapLayer->ID != order.MapLayer)) {
					//Wyrmgus end
						order.goalPos = order.get_goal()->tilePos;
						order.MapLayer = order.get_goal()->MapLayer->ID;
					}
					if (unit.Player->AiEnabled) {
						if (!unit.RestoreOrder()) {
							this->Finished = true;
						}
						return ;
					}
				}
			}
			if (!unit.ReCast && unit.CurrentAction() != UnitAction::Die) {
				if (!unit.RestoreOrder()) {
					this->Finished = true;
				}
				return ;
			}
			break;

		default:
			this->State = 0; // Reset path, than move to target
			break;
	}
}
