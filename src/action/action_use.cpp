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
/**@name action_use.cpp - The use action. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

#include "action/action_use.h"

#include "animation/animation.h"
#include "character.h"
#include "commands.h"
#include "iolib.h"
#include "item/item.h"
#include "item/item_class.h"
#include "luacallback.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "missile/missile.h"
#include "network/network.h"
#include "pathfinder/pathfinder.h"
#include "script.h"
#include "sound/sound.h"
#include "spell/spells.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "video/video.h"

enum {
	State_Init = 0,
	State_Initialized = 1,

	State_TargetReached = 128,
};



/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionUse(CUnit &dest)
{
	COrder_Use *order = new COrder_Use;

	// Destination could be killed.
	// Should be handled in action, but is not possible!
	// Unit::Refs is used as timeout counter.
	if (dest.Destroyed) {
		order->goalPos = dest.GetTilePos() + dest.GetHalfTileSize();
		order->MapLayer = dest.GetMapLayer()->GetIndex();
	} else {
		order->SetGoal(&dest);
		order->Range = 1;
	}
	return order;
}

/* virtual */ void COrder_Use::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-use\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	file.printf(" \"map-layer\", %d,", this->MapLayer);

	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

/* virtual */ bool COrder_Use::ParseSpecificData(lua_State *l, int &j, const char *value, CUnit &unit)
{
	if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
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

/* virtual */ bool COrder_Use::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Use::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		if (this->GetGoal()->GetMapLayer() != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		if (this->MapLayer != UI.CurrentMapLayer->GetIndex()) {
			return lastScreenPos;
		}
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorGreen, targetPos, 3);
	}
	return targetPos;
}

/* virtual */ void COrder_Use::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

	Vec2i tileSize;
	if (this->HasGoal()) {
		CUnit *goal = this->GetGoal();
		tileSize = goal->GetTileSize();
		input.SetGoal(goal->GetTilePos(), tileSize, goal->GetMapLayer()->GetIndex());
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}


/* virtual */ void COrder_Use::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.GetAnimations()->Still);
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	CUnit *goal = this->GetGoal();

	// Reached target
	if (this->State == State_TargetReached || (goal && goal->Container == &unit) || this == unit.CriticalOrder) {

		if (!goal || (!goal->IsVisibleAsGoal(*unit.GetPlayer()) && goal->Container != &unit)) {
			DebugPrint("Goal gone\n");
			this->Finished = true;
			return ;
		}

		if (goal && (goal->GetType()->BoolFlag[ITEM_INDEX].value || goal->GetType()->BoolFlag[POWERUP_INDEX].value || goal->ConnectingDestination != nullptr)) {
			std::string goal_name = goal->GetMessageName();
			if (!goal->Unique) {
				goal_name = "the " + goal_name;
			}
			if (unit.HasInventory() && goal->GetType()->BoolFlag[ITEM_INDEX].value && unit.CanEquipItem(goal)) { //if the item is an equipment, equip it (only for units with inventories), or deequip it (if it is already equipped)
				if (!unit.IsItemEquipped(goal)) {
					unit.EquipItem(goal);
				} else {
					unit.DeequipItem(goal);
				}
			} else if (unit.CanUseItem(goal)) {
				if (goal->ConnectingDestination != nullptr) {
					int old_z = unit.GetMapLayer()->GetIndex();
					SaveSelection();
					unit.Remove(nullptr);
					DropOutOnSide(unit, unit.Direction, goal->ConnectingDestination);
					RestoreSelection();
					if (unit.GetPlayer() == CPlayer::GetThisPlayer() && Selected.size() > 0 && &unit == Selected[0] && old_z == UI.CurrentMapLayer->GetIndex()) {
						ChangeCurrentMapLayer(unit.GetMapLayer()->GetIndex());
						UI.SelectedViewport->Center(unit.GetMapPixelPosCenter());
					}
					PlayUnitSound(*goal->ConnectingDestination, VoiceUsed);
				} else if (goal->Spell != nullptr) {
					CommandSpellCast(unit, unit.GetTilePos(), nullptr, *goal->Spell, FlushCommands, unit.GetMapLayer()->GetIndex());
				} else if (goal->Work != nullptr) {
					unit.ReadWork(goal->Work);
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyGreen, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s read %s: %s"), unit.GetMessageName().c_str(), goal_name.c_str(), GetUpgradeEffectsString(goal->Work->Ident).c_str());
					}
				} else if (goal->Elixir != nullptr) {
					unit.ConsumeElixir(goal->Elixir);
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyGreen, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s consumed %s: %s"), unit.GetMessageName().c_str(), goal_name.c_str(), GetUpgradeEffectsString(goal->Elixir->Ident).c_str());
					}
				} else if (goal->GetType()->GivesResource && goal->GetResourcesHeld() > 0) {
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyGreen, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("Gained %d %s"), goal->GetResourcesHeld(), DefaultResourceNames[goal->GetType()->GivesResource].c_str());
					}
					unit.GetPlayer()->ChangeResource(goal->GetType()->GivesResource, goal->GetResourcesHeld());
					unit.GetPlayer()->TotalResources[goal->GetType()->GivesResource] += (goal->GetResourcesHeld() * unit.GetPlayer()->Incomes[goal->GetType()->GivesResource]) / 100;
				} else if (goal->Variable[HITPOINTHEALING_INDEX].Value > 0) {
					int hp_healed = std::min(goal->Variable[HITPOINTHEALING_INDEX].Value, (unit.GetModifiedVariable(HP_INDEX, VariableMax) - unit.Variable[HP_INDEX].Value));
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyGreen, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s healed for %d HP"), unit.GetMessageName().c_str(), hp_healed);
					}
					unit.Variable[HP_INDEX].Value += hp_healed;
					
					if (unit.HasInventory() && unit.Variable[HP_INDEX].Value < unit.GetModifiedVariable(HP_INDEX, VariableMax)) { //if unit is still damaged, see if there are further healing items for it to use
						unit.HealingItemAutoUse();
					}
				} else if (goal->Variable[HITPOINTHEALING_INDEX].Value < 0 && unit.GetType()->UnitType != UnitTypeFly && unit.GetType()->UnitType != UnitTypeFlyLow) {
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyRed, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s suffered a %d HP loss"), unit.GetMessageName().c_str(), (goal->Variable[HITPOINTHEALING_INDEX].Value * -1));
					}
					HitUnit(NoUnitP, unit, goal->Variable[HITPOINTHEALING_INDEX].Value * -1);
				} else if (goal->GetType()->BoolFlag[SLOWS_INDEX].value && unit.GetType()->UnitType != UnitTypeFly && unit.GetType()->UnitType != UnitTypeFlyLow) {
					unit.Variable[SLOW_INDEX].Value = 1000;
					if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
						unit.GetPlayer()->Notify(NotifyRed, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s has been slowed"), unit.GetMessageName().c_str());
					}
				}
			} else { //cannot use
				if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
					unit.GetPlayer()->Notify(NotifyRed, unit.GetTilePos(), unit.GetMapLayer()->GetIndex(), _("%s cannot use %s"), unit.GetMessageName().c_str(), goal_name.c_str());
				}
				this->Finished = true;
				return;
			}
			PlayUnitSound(*goal, VoiceUsed);
			if (goal->GetType()->BoolFlag[POWERUP_INDEX].value || (goal->GetType()->BoolFlag[ITEM_INDEX].value && goal->GetType()->ItemClass->IsConsumable())) { //only destroy item if it is consumable
				if (goal->Container == nullptr) {
					goal->Remove(nullptr);
					LetUnitDie(*goal);
				} else {
					if (!IsNetworkGame() && goal->Container->Character && goal->Container->GetPlayer()->AiEnabled == false && goal->GetType()->BoolFlag[ITEM_INDEX].value && goal->Container->HasInventory()) {
						CPersistentItem *item = goal->Container->Character->GetItem(goal);
						goal->Container->Character->Items.erase(std::remove(goal->Container->Character->Items.begin(), goal->Container->Character->Items.end(), item), goal->Container->Character->Items.end());
						delete item;
						SaveHero(goal->Container->Character);
					}
					UnitLost(*goal);
					UnitClearOrders(*goal);
					goal->Release();
				}
			}
		}
		
		this->Finished = true;
		return;
	}
	if (this->State == State_Init) { // first entry
		this->State = State_Initialized;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			if ((unit.GetMapLayer()->Field(unit.GetTilePos())->GetFlags() & MapFieldBridge) && !unit.GetType()->BoolFlag[BRIDGE_INDEX].value && unit.GetType()->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.GetTilePos(), unit.GetTilePos(), table, unit.GetMapLayer()->GetIndex());
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->GetType()->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->GetTilePos() : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->GetMapLayer()->GetIndex() : this->MapLayer);
						}
						return;
					}
				}
			}
			this->Finished = true;
			return ;
		case PF_REACHED: {
			if (!goal) { // goal has died
				this->Finished = true;
				return ;
			}
			// Handle Teleporter Units
			// FIXME: BAD HACK
			// goal shouldn't be busy and portal should be alive
			if (goal->GetType()->BoolFlag[TELEPORTER_INDEX].value && goal->Goal && goal->Goal->IsAlive() && unit.MapDistanceTo(*goal) <= 1) {
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
				unit.TilePos = goal->Goal->GetTilePos();
				unit.MapLayer = goal->Goal->MapLayer;
				DropOutOnSide(unit, unit.Direction, nullptr);

				// FIXME: we must check if the units supports the new order.
				CUnit &dest = *goal->Goal;

				if (dest.NewOrder == nullptr
					|| (dest.NewOrder->Action == UnitActionResource && !unit.GetType()->BoolFlag[HARVESTER_INDEX].value)
					|| (dest.NewOrder->Action == UnitActionAttack && !unit.CanAttack(true))
					|| (dest.NewOrder->Action == UnitActionBoard && unit.GetType()->UnitType != UnitTypeLand)) {
					this->Finished = true;
					return ;
				} else {
					if (dest.NewOrder->HasGoal()) {
						if (dest.NewOrder->GetGoal()->Destroyed) {
							delete dest.NewOrder;
							dest.NewOrder = nullptr;
							this->Finished = true;
							return ;
						}
						unit.Orders.insert(unit.Orders.begin() + 1, dest.NewOrder->Clone());
						this->Finished = true;
						return ;
					}
				}
			}
			this->goalPos = goal->GetTilePos();
			this->MapLayer = goal->GetMapLayer()->GetIndex();
			this->State = State_TargetReached;
		}
		// FALL THROUGH
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.GetPlayer())) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->GetTilePos() + goal->GetHalfTileSize();
		this->MapLayer = goal->GetMapLayer()->GetIndex();
		this->ClearGoal();
		goal = nullptr;
	}

	if (unit.Anim.Unbreakable) {
		return ;
	}
}
