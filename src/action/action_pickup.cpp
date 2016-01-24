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
/**@name action_pickup.cpp - The item pick up action. */
//
//      (c) Copyright 2015 by Andrettin
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

#include "action/action_pickup.h"

#include "animation.h"
//Wyrmgus start
#include "character.h"
#include "commands.h"
//Wyrmgus end
#include "iolib.h"
#include "luacallback.h"
#include "missile.h"
#include "network.h"
#include "pathfinder.h"
#include "script.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "ui.h"
#include "unit.h"
#include "unit_find.h"
#include "unittype.h"
#include "video.h"

enum {
	State_Init = 0,
	State_Initialized = 1,

	State_TargetReached = 128,
};



/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionPickUp(CUnit &dest)
{
	COrder_PickUp *order = new COrder_PickUp;

	// Destination could be killed.
	// Should be handled in action, but is not possible!
	// Unit::Refs is used as timeout counter.
	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.Type->GetHalfTileSize();
	} else {
		order->SetGoal(&dest);
		order->Range = 1;
	}
	return order;
}

/* virtual */ void COrder_PickUp::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-pick-up\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);

	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

/* virtual */ bool COrder_PickUp::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
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
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_PickUp::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_PickUp::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	//Wyrmgus start
//	Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
//	Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
//	Video.FillCircleClip(ColorGreen, targetPos, 3);
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorGreen, targetPos, 3);
	}
	//Wyrmgus end
	return targetPos;
}

/* virtual */ void COrder_PickUp::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

	Vec2i tileSize;
	if (this->HasGoal()) {
		CUnit *goal = this->GetGoal();
		tileSize.x = goal->Type->TileWidth;
		tileSize.y = goal->Type->TileHeight;
		input.SetGoal(goal->tilePos, tileSize);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize);
	}
}


/* virtual */ void COrder_PickUp::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		//Wyrmgus start
//		UnitShowAnimation(unit, unit.Type->Animations->Still);
		UnitShowAnimation(unit, unit.GetAnimations()->Still);
		//Wyrmgus end
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	CUnit *goal = this->GetGoal();

	// Reached target
	if (this->State == State_TargetReached) {

		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			DebugPrint("Goal gone\n");
			this->Finished = true;
			return ;
		}

		if (unit.HasInventory() && goal && goal->Type->BoolFlag[ITEM_INDEX].value) {
			goal->TTL = 0; //remove item destruction timer when picked up
			
			goal->Remove(&unit);
			if (!IsNetworkGame() && unit.Character && unit.Character->Persistent && unit.Player->AiEnabled == false) { //if the unit has a persistent character, store the item for it
				CItem *item = new CItem;
				unit.Character->Items.push_back(item);
				item->Type = const_cast<CUnitType *>(&(*goal->Type));
				if (goal->Prefix != NULL) {
					item->Prefix = const_cast<CUpgrade *>(&(*goal->Prefix));
				}
				if (goal->Suffix != NULL) {
					item->Suffix = const_cast<CUpgrade *>(&(*goal->Suffix));
				}
				if (goal->Spell != NULL) {
					item->Spell = const_cast<SpellType *>(&(*goal->Spell));
				}
				if (goal->Work != NULL) {
					item->Work = const_cast<CUpgrade *>(&(*goal->Work));
				}
				if (goal->Unique) {
					item->Name = goal->Name;
					item->Unique = goal->Unique;
				}
				item->Bound = goal->Bound;
				SaveHero(unit.Character);
			}
		} else if (
			goal
				&& (
					goal->Type->BoolFlag[POWERUP_INDEX].value
					|| (!unit.HasInventory() && goal->Type->BoolFlag[ITEM_INDEX].value && IsItemClassConsumable(goal->Type->ItemClass))
				)
			) {
			CommandUse(unit, *goal, FlushCommands);
		}
		
		this->Finished = true;
		return;
	}
	if (this->State == State_Init) { // first entry
		this->State = State_Initialized;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			//Wyrmgus start
			if ((Map.Field(unit.tilePos)->Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.tilePos, unit.tilePos, table);
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->tilePos : this->goalPos, FlushCommands);
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
				unit.Remove(NULL);
				if (goal->Type->TeleportEffectIn) {
					goal->Type->TeleportEffectIn->pushPreamble();
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(unit));
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(*goal));
					goal->Type->TeleportEffectIn->pushInteger(unit.GetMapPixelPosCenter().x);
					goal->Type->TeleportEffectIn->pushInteger(unit.GetMapPixelPosCenter().y);
					goal->Type->TeleportEffectIn->run();
				}
				unit.tilePos = goal->Goal->tilePos;
				DropOutOnSide(unit, unit.Direction, NULL);

				// FIXME: we must check if the units supports the new order.
				CUnit &dest = *goal->Goal;
				if (dest.Type->TeleportEffectOut) {
					dest.Type->TeleportEffectOut->pushPreamble();
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(unit));
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(dest));
					dest.Type->TeleportEffectOut->pushInteger(unit.GetMapPixelPosCenter().x);
					dest.Type->TeleportEffectOut->pushInteger(unit.GetMapPixelPosCenter().y);
					dest.Type->TeleportEffectOut->run();
				}

				if (dest.NewOrder == NULL
					|| (dest.NewOrder->Action == UnitActionResource && !unit.Type->BoolFlag[HARVESTER_INDEX].value)
					|| (dest.NewOrder->Action == UnitActionAttack && !unit.CanAttack())
					|| (dest.NewOrder->Action == UnitActionBoard && unit.Type->UnitType != UnitTypeLand)) {
					this->Finished = true;
					return ;
				} else {
					if (dest.NewOrder->HasGoal()) {
						if (dest.NewOrder->GetGoal()->Destroyed) {
							delete dest.NewOrder;
							dest.NewOrder = NULL;
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
			this->State = State_TargetReached;
		}
		// FALL THROUGH
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->tilePos + goal->Type->GetHalfTileSize();
		this->ClearGoal();
		goal = NULL;
	}

	if (unit.Anim.Unbreakable) {
		return ;
	}
}

//@}
