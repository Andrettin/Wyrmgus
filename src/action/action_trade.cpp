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
//      (c) Copyright 2017-2022 by Andrettin
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

#include "action/action_trade.h"

#include "animation/animation.h"
#include "animation/animation_set.h"
#include "character.h"
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "game/game.h"
#include "iolib.h"
#include "luacallback.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile.h"
#include "network/network.h"
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/unit_sound_type.h"
#include "spell/spell.h"
#include "translator.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionTrade(CUnit &dest, CUnit &home_market)
{
	auto order = std::make_unique<COrder_Trade>();

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
	
	order->HomeMarket = &home_market;

	return order;
}

void COrder_Trade::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-trade\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	file.printf(" \"map-layer\", %d,", this->MapLayer);

	file.printf(" \"state\", %d", static_cast<int>(this->state));

	file.printf("}");
}

bool COrder_Trade::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "state")) {
		++j;
		this->state = static_cast<trade_state>(LuaToNumber(l, -1, j + 1));
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	} else {
		return false;
	}
	return true;
}

bool COrder_Trade::IsValid() const
{
	return true;
}

PixelPos COrder_Trade::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (this->has_goal()) {
		if (this->get_goal()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
	} else {
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
	}

	return COrder::Show(vp, lastScreenPos, render_commands);
}

QPoint COrder_Trade::get_shown_target_pos(const CViewport &vp) const
{
	if (this->has_goal()) {
		return vp.scaled_map_to_screen_pixel_pos(this->get_goal()->get_scaled_map_pixel_pos_center());
	} else {
		return vp.TilePosToScreen_Center(this->goalPos);
	}
}

void COrder_Trade::UpdatePathFinderData(PathFinderInput &input)
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

void COrder_Trade::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still);
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	CUnit *goal = this->get_goal();

	// Reached target
	if (this->state == trade_state::target_reached || (goal && goal->Container == &unit)) {

		if (!goal || (!goal->IsVisibleAsGoal(*unit.Player) && goal->Container != &unit)) {
			DebugPrint("Goal gone\n");
			this->Finished = true;
			return;
		}

		if (goal && (goal->Type->BoolFlag[ITEM_INDEX].value || goal->Type->BoolFlag[POWERUP_INDEX].value || goal->ConnectingDestination != nullptr)) {
			std::string goal_name = goal->GetMessageName();
			if (goal->get_unique() == nullptr) {
				goal_name = "the " + goal_name;
			}
			if (unit.HasInventory() && goal->Type->BoolFlag[ITEM_INDEX].value && unit.can_equip_item(goal)) { //if the item is an equipment, equip it (only for units with inventories), or deequip it (if it is already equipped)
				if (!unit.IsItemEquipped(goal)) {
					unit.EquipItem(*goal);
				} else {
					unit.DeequipItem(*goal);
				}
			} else if (unit.CanUseItem(goal)) {
				if (goal->ConnectingDestination != nullptr) {
					int old_z = unit.MapLayer->ID;
					SaveSelection();
					unit.Remove(nullptr);
					unit.drop_out_on_side(unit.Direction, goal->ConnectingDestination);
					RestoreSelection();
					if (unit.Player == CPlayer::GetThisPlayer() && Selected.size() > 0 && &unit == Selected[0] && old_z == UI.CurrentMapLayer->ID) {
						ChangeCurrentMapLayer(unit.MapLayer->ID);
						UI.SelectedViewport->Center(unit.get_scaled_map_pixel_pos_center());
					}
					PlayUnitSound(goal->ConnectingDestination, wyrmgus::unit_sound_type::used);
				} else if (goal->Type->get_given_resource() != nullptr && goal->ResourcesHeld > 0) {
					if (unit.Player == CPlayer::GetThisPlayer()) {
						unit.Player->Notify(notification_type::green, unit.tilePos, unit.MapLayer->ID, _("Gained %d %s"), goal->ResourcesHeld, DefaultResourceNames[goal->Type->get_given_resource()->get_index()].c_str());
					}
					unit.Player->change_resource(goal->Type->get_given_resource(), goal->ResourcesHeld, false);
					unit.Player->change_resource_total(goal->Type->get_given_resource(), goal->ResourcesHeld * unit.Player->get_income_modifier(goal->Type->get_given_resource()) / 100);
				}
			} else { //cannot use
				if (unit.Player == CPlayer::GetThisPlayer()) {
					unit.Player->Notify(notification_type::red, unit.tilePos, unit.MapLayer->ID, _("%s cannot use %s"), unit.GetMessageName().c_str(), goal_name.c_str());
				}
				this->Finished = true;
				return;
			}
			PlayUnitSound(goal, unit_sound_type::used);
			if (goal->Type->BoolFlag[POWERUP_INDEX].value || wyrmgus::is_consumable_item_class(goal->Type->get_item_class())) { //only destroy item if it is consumable
				if (goal->Container == nullptr) {
					goal->Remove(nullptr);
					LetUnitDie(*goal);
				} else {
					if (game::get()->is_persistency_enabled() && goal->Container->get_character() != nullptr && goal->Container->Player == CPlayer::GetThisPlayer() && goal->Type->BoolFlag[ITEM_INDEX].value && goal->Container->HasInventory()) {
						wyrmgus::persistent_item *item = goal->Container->get_character()->get_item(goal);
						goal->Container->get_character()->remove_item(item);
						goal->Container->get_character()->save();
					}
					UnitLost(*goal);
					goal->clear_orders();
					goal->Release();
				}
			}
		}
		
		this->Finished = true;
		return;
	}
	if (this->state == trade_state::init) { // first entry
		this->state = trade_state::initialized;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			this->Finished = true;
			return;
		case PF_REACHED: {
			if (!goal) { // goal has died
				this->Finished = true;
				return;
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
				unit.MapLayer = goal->Goal->MapLayer;
				unit.drop_out_on_side(unit.Direction, nullptr);

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
					|| (dest.NewOrder->Action == UnitAction::Attack && !unit.CanAttack(true))
					|| (dest.NewOrder->Action == UnitAction::Board && unit.Type->get_domain() != unit_domain::land)) {
					this->Finished = true;
					return;
				} else {
					if (dest.NewOrder->has_goal()) {
						if (dest.NewOrder->get_goal()->Destroyed) {
							dest.NewOrder.reset();
							this->Finished = true;
							return;
						}

						unit.Orders.insert(unit.Orders.begin() + 1, dest.NewOrder->Clone());
						this->Finished = true;
						return;
					}
				}
			}
			this->goalPos = goal->tilePos;
			this->MapLayer = goal->MapLayer->ID;
			this->state = trade_state::target_reached;
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
		return;
	}
}
