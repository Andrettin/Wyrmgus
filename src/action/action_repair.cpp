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
//      (c) Copyright 1999-2022 by Vladi Shabanski, Jimmy Salmon and Andrettin
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

#include "action/action_repair.h"

#include "action/action_built.h"
#include "animation/animation.h"
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
#include "pathfinder.h"
#include "player/player.h"
#include "script.h"
#include "sound/sound.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionRepair(CUnit &target)
{
	auto order = std::make_unique<COrder_Repair>();

	if (target.Destroyed) {
		order->goalPos = target.tilePos + target.GetHalfTileSize();
		order->MapLayer = target.MapLayer->ID;
	} else {
		order->set_goal(&target);
		order->ReparableTarget = target.acquire_ref();
	}
	return order;
}

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionRepair(const Vec2i &pos)
std::unique_ptr<COrder> COrder::NewActionRepair(const Vec2i &pos, int z)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	auto order = std::make_unique<COrder_Repair>();

	order->goalPos = pos;
	//Wyrmgus start
	order->MapLayer = z;
	//Wyrmgus end
	return order;
}

COrder_Repair::COrder_Repair() : COrder(UnitAction::Repair)
{
	this->goalPos.x = -1;
	this->goalPos.y = -1;
}

COrder_Repair::~COrder_Repair()
{
}

void COrder_Repair::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-repair\",");

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

	if (this->get_reparable_target() != nullptr) {
		file.printf(" \"repair-target\", \"%s\",", UnitReference(this->get_reparable_target()).c_str());
	}

	file.printf(" \"repaircycle\", %d,", this->RepairCycle);
	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

bool COrder_Repair::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp("repaircycle", value)) {
		++j;
		this->RepairCycle = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp("repair-target", value)) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->ReparableTarget = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp("state", value)) {
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

bool COrder_Repair::IsValid() const
{
	return true;
}

PixelPos COrder_Repair::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PixelPos targetPos;

	if (this->get_reparable_target() != nullptr) {
		if (this->get_reparable_target()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		targetPos = vp.scaled_map_to_screen_pixel_pos(this->get_reparable_target()->get_scaled_map_pixel_pos_center());
	} else {
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}

	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorYellow, targetPos, (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}

	return targetPos;
}

void COrder_Repair::UpdatePathFinderData(PathFinderInput &input)
{
	const CUnit &unit = *input.GetUnit();

	input.SetMinRange(0);
	input.SetMaxRange(this->get_reparable_target() != nullptr ? unit.Type->RepairRange : 0);

	Vec2i tileSize;
	if (this->get_reparable_target() != nullptr) {
		tileSize = this->get_reparable_target()->get_tile_size();
		input.SetGoal(this->get_reparable_target()->tilePos, tileSize, this->get_reparable_target()->MapLayer->ID);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}

bool SubRepairCosts(const CUnit &unit, CPlayer &player, CUnit &goal)
{
	resource_map<int> repair_costs;

	// Check if enough resources are available
	for (const auto &[resource, cost] : goal.Type->get_repair_costs()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		repair_costs[resource] = cost * (goal.CurrentAction() == UnitAction::Built ? ResourcesMultiBuildersMultiplier : 1);
	}

	for (const auto &[resource, cost] : repair_costs) {
		if (!player.check_resource(resource, cost)) {
			player.Notify(NotifyYellow, unit.tilePos,
						  //Wyrmgus start
						  unit.MapLayer->ID,
						  //Wyrmgus end
						  _("We need more %s to repair!"), resource->get_identifier().c_str());
			return true;
		}
	}

	// Subtract the resources
	player.subtract_costs(repair_costs);
	return false;
}

CUnit *COrder_Repair::get_reparable_target() const
{
	if (this->ReparableTarget == nullptr) {
		return nullptr;
	}

	return this->ReparableTarget->get();
}

/**
**  Repair a unit.
**
**  @param unit  unit repairing
**  @param goal  unit being repaired
**
**  @return true when action is finished/canceled.
*/
bool COrder_Repair::RepairUnit(const CUnit &unit, CUnit &goal)
{
	CPlayer &player = *unit.Player;

	if (goal.CurrentAction() == UnitAction::Built) {
		COrder_Built &order = *static_cast<COrder_Built *>(goal.CurrentOrder());

		order.ProgressHp(goal, 100 * this->RepairCycle);
		this->RepairCycle = 0;
		if (ResourcesMultiBuildersMultiplier && SubRepairCosts(unit, player, goal)) {
			return true;
		}
		return false;
	}
	//Wyrmgus start
//	if (goal.Variable[HP_INDEX].Value >= goal.Variable[HP_INDEX].Max) {
	if (goal.Variable[HP_INDEX].Value >= goal.GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
	//Wyrmgus end
		return true;
	}

	assert_throw(goal.Stats->Variables[HP_INDEX].Max > 0);

	if (SubRepairCosts(unit, player, goal)) {
		return true;
	}

	goal.Variable[HP_INDEX].Value += goal.Type->get_repair_hp();
	//Wyrmgus start
//	if (goal.Variable[HP_INDEX].Value >= goal.Variable[HP_INDEX].Max) {
	if (goal.Variable[HP_INDEX].Value >= goal.GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
	//Wyrmgus end
		//Wyrmgus start
//		goal.Variable[HP_INDEX].Value = goal.Variable[HP_INDEX].Max;
		goal.Variable[HP_INDEX].Value = goal.GetModifiedVariable(HP_INDEX, VariableAttribute::Max);
		//Wyrmgus end
		return true;
	}
	return false;
}

/**
**  Animate unit repair
**
**  @param unit  Unit, for that the repair animation is played.
*/
static void AnimateActionRepair(CUnit &unit)
{
	UnitShowAnimation(unit, unit.get_animation_set()->Repair.get());
}

void COrder_Repair::Execute(CUnit &unit)
{
	assert_throw(this->get_reparable_target() == this->get_goal());
	assert_throw(unit.CanMove());
	assert_throw(unit.can_repair());

	switch (this->State) {
		case 0:
			this->State = 1;
		// FALL THROUGH
		case 1: { // Move near to target.
			// FIXME: RESET FIRST!! Why? We move first and than check if
			// something is in sight.
			int err = DoActionMove(unit);
			if (!unit.Anim.Unbreakable) {
				// No goal: if meeting damaged building repair it.
				CUnit *goal = this->get_goal();

				if (goal) {
					if (!goal->IsVisibleAsGoal(*unit.Player)) {
						DebugPrint("repair target gone.\n");
						this->goalPos = goal->tilePos + goal->GetHalfTileSize();
						this->MapLayer = goal->MapLayer->ID;
						this->ReparableTarget.reset();
						this->clear_goal();
						goal = nullptr;
					}
				} else if (unit.Player->AiEnabled) {
					// Ai players workers should stop if target is killed
					err = -1;
				}

				// Have reached target? FIXME: could use return value
				if (goal && unit.MapDistanceTo(*goal) <= unit.Type->RepairRange
					//Wyrmgus start
//					&& goal->Variable[HP_INDEX].Value < goal->Variable[HP_INDEX].Max) {
					&& goal->Variable[HP_INDEX].Value < goal->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
					//Wyrmgus end
					this->State = 2;
					this->RepairCycle = 0;
					//Wyrmgus start
//					const Vec2i dir = goal->tilePos + goal->Type->GetHalfTileSize() - unit.tilePos;
					const Vec2i dir = PixelSize(PixelSize(goal->tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size();
					//Wyrmgus end
					UnitHeadingFromDeltaXY(unit, dir);
				} else if (err < 0) {
					this->Finished = true;
					return;
				}
			}
			break;
		}
		case 2: {// Repair the target.
			AnimateActionRepair(unit);
			this->RepairCycle++;
			if (unit.Anim.Unbreakable) {
				return;
			}

			CUnit *goal = this->get_goal();

			if (goal) {
				if (!goal->IsVisibleAsGoal(*unit.Player)) {
					DebugPrint("repair goal is gone\n");
					this->goalPos = goal->tilePos + goal->GetHalfTileSize();
					this->MapLayer = goal->MapLayer->ID;
					// FIXME: should I clear this here?
					this->clear_goal();
					this->ReparableTarget.reset();
					goal = nullptr;
				} else {
					const int dist = unit.MapDistanceTo(*goal);

					if (dist <= unit.Type->RepairRange) {
						if (RepairUnit(unit, *goal)) {
							this->Finished = true;
							return;
						}
					} else if (dist > unit.Type->RepairRange) {
						// If goal has move, chase after it
						this->State = 0;
					}
				}
			}
			// Target is fine, choose new one.
			//Wyrmgus start
//			if (!goal || goal->Variable[HP_INDEX].Value >= goal->Variable[HP_INDEX].Max) {
			if (!goal || goal->Variable[HP_INDEX].Value >= goal->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
			//Wyrmgus end
				this->Finished = true;
				return;
			}
			// FIXME: automatic repair
		}
		break;
	}
}
