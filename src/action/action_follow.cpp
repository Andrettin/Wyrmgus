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
/**@name action_follow.cpp - The follow action. */
//
//      (c) Copyright 2001-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_follow.h"

#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "luacallback.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "pathfinder.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionFollow(CUnit &dest)
{
	auto order = std::make_unique<COrder_Follow>();

	// Destination could be killed.
	// Should be handled in action, but is not possible!
	// Unit::get_ref_count() is used as timeout counter.
	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.GetHalfTileSize();
		order->MapLayer = dest.MapLayer->ID;
	} else {
		order->set_goal(&dest);
		order->Range = 1;
	}
	return order;
}

void COrder_Follow::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-follow\",");

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

	file.printf(" \"state\", %d", static_cast<int>(this->state));

	file.printf("}");
}

bool COrder_Follow::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp(value, "state")) {
		++j;
		this->state = static_cast<follow_state>(LuaToNumber(l, -1, j + 1));
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

bool COrder_Follow::IsValid() const
{
	return true;
}

PixelPos COrder_Follow::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
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
		Video.FillCircleClip(ColorGreen, lastScreenPos, (2 * defines::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorGreen, targetPos, (3 * defines::get()->get_scale_factor()).to_int(), render_commands);
	}

	return targetPos;
}

void COrder_Follow::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

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

void COrder_Follow::Execute(CUnit &unit)
{
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
	CUnit *goal = this->get_goal();

	// Reached target
	if (this->state == follow_state::target_reached) {

		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			DebugPrint("Goal gone\n");
			this->Finished = true;
			return ;
		}

		// Don't follow after immobile units
		if (goal && goal->CanMove() == false) {
			this->Finished = true;
			return;
		}

		//Wyrmgus start
//		if (goal->tilePos == this->goalPos) {
		if (goal->tilePos == this->goalPos && goal->MapLayer->ID == this->MapLayer) {
		//Wyrmgus end
			// Move to the next order
			if (unit.Orders.size() > 1) {
				this->Finished = true;
				return ;
			}

			unit.Wait = 10;
			if (this->Range > 1) {
				this->Range = 1;
				this->state = follow_state::init;
			}
			return ;
		}
		this->state = follow_state::init;
	}
	if (this->state == follow_state::init) { // first entry
		this->state = follow_state::initialized;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			//Wyrmgus start
			if (unit.MapLayer->Field(unit.tilePos)->has_flag(tile_flag::bridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->get_domain() == unit_domain::land) {
				std::vector<CUnit *> table;
				Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitAction::Still) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->has_goal() ? this->get_goal()->tilePos : this->goalPos, FlushCommands, this->has_goal() ? this->get_goal()->MapLayer->ID : this->MapLayer);
						}
						return;
					}
				}
			}
			//Wyrmgus end
			// Some tries to reach the goal
			this->Range++;
			break;
		case PF_REACHED: {
			if (!goal) { // goal has died
				this->Finished = true;
				return ;
			}
			// Handle Teleporter Units
			// FIXME: BAD HACK
			// goal shouldn't be busy and portal should be alive
			if (goal->Type->BoolFlag[TELEPORTER_INDEX].value && goal->Goal && goal->Goal->IsAlive() && unit.MapDistanceTo(*goal) <= 1) {
				if (!goal->IsIdle()) { // wait
					unit.Wait = 10;
					return;
				}
				// Check if we have enough mana
				if (goal->Goal->Type->TeleportCost > goal->Variable[MANA_INDEX].Value) {
					this->Finished = true;
					return;
				} else {
					goal->Variable[MANA_INDEX].Value -= goal->Goal->Type->TeleportCost;
				}
				// Everything is OK, now teleport the unit
				unit.Remove(nullptr);
				if (goal->Type->TeleportEffectIn) {
					goal->Type->TeleportEffectIn->pushPreamble();
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(unit));
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(*goal));
					goal->Type->TeleportEffectIn->pushInteger(unit.get_map_pixel_pos_center().x);
					goal->Type->TeleportEffectIn->pushInteger(unit.get_map_pixel_pos_center().y);
					goal->Type->TeleportEffectIn->run();
				}
				unit.tilePos = goal->Goal->tilePos;
				//Wyrmgus start
				unit.MapLayer = goal->Goal->MapLayer;
				//Wyrmgus end
				DropOutOnSide(unit, unit.Direction, nullptr);

				// FIXME: we must check if the units supports the new order.
				CUnit &dest = *goal->Goal;
				if (dest.Type->TeleportEffectOut) {
					dest.Type->TeleportEffectOut->pushPreamble();
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(unit));
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(dest));
					dest.Type->TeleportEffectOut->pushInteger(unit.get_map_pixel_pos_center().x);
					dest.Type->TeleportEffectOut->pushInteger(unit.get_map_pixel_pos_center().y);
					dest.Type->TeleportEffectOut->run();
				}

				if (dest.NewOrder == nullptr
					|| (dest.NewOrder->Action == UnitAction::Resource && !unit.Type->BoolFlag[HARVESTER_INDEX].value)
					//Wyrmgus start
//					|| (dest.NewOrder->Action == UnitAction::Attack && !unit.Type->CanAttack)
					|| (dest.NewOrder->Action == UnitAction::Attack && !unit.CanAttack(true))
					//Wyrmgus end
					|| (dest.NewOrder->Action == UnitAction::Board && unit.Type->get_domain() != unit_domain::land)) {
					this->Finished = true;
					return ;
				} else {
					if (dest.NewOrder->has_goal()) {
						if (dest.NewOrder->get_goal()->Destroyed) {
							dest.NewOrder.reset();
							this->Finished = true;
							return ;
						}
						unit.Orders.insert(unit.Orders.begin() + 1, dest.NewOrder->Clone());
						this->Finished = true;
						return ;
					}
				}
			}
			this->goalPos = goal->tilePos;
			this->MapLayer = goal->MapLayer->ID;
			this->state = follow_state::target_reached;
		}
		// FALL THROUGH
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->tilePos + goal->GetHalfTileSize();
		this->MapLayer = goal->MapLayer->ID;
		this->clear_goal();
		goal = nullptr;
	}

	if (unit.Anim.Unbreakable) {
		return ;
	}
	// If our leader is dead or stops or attacks:
	// Attack any enemy in reaction range.
	// If don't set the goal, the unit can than choose a
	//  better goal if moving nearer to enemy.
	//Wyrmgus start
//	if (unit.Type->CanAttack
	if (unit.CanAttack()
	//Wyrmgus end
		//Wyrmgus start
		&& unit.MapDistanceTo(this->goalPos, this->MapLayer) <= this->Range //only try to auto-attack if already reached the goal
//		&& (!goal || goal->CurrentAction() == UnitAction::Attack || goal->CurrentAction() == UnitAction::Still)) {
		&& (!goal || goal->CurrentAction() == UnitAction::Attack || goal->CurrentAction() == UnitAction::Still || goal->CurrentAction() == UnitAction::StandGround)) {
		//Wyrmgus end
		CUnit *target = AttackUnitsInReactRange(unit);
		if (target) {
			// Save current command to come back.
			std::unique_ptr<COrder> saved_order;
			if (unit.CanStoreOrder(unit.CurrentOrder())) {
				saved_order = this->Clone();
			}

			this->Finished = true;
			//Wyrmgus start
//			unit.Orders.insert(unit.Orders.begin() + 1, COrder::NewActionAttack(unit, target->tilePos));
			unit.Orders.insert(unit.Orders.begin() + 1, COrder::NewActionAttack(unit, target->tilePos, target->MapLayer->ID));
			//Wyrmgus end

			if (saved_order != nullptr) {
				unit.SavedOrder = std::move(saved_order);
			}
		}
	}
}
