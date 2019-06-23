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
/**@name mouse.cpp - The mouse handling. */
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

#define ICON_SIZE_X (UI.ButtonPanel.Buttons[0].Style->Width)
#define ICON_SIZE_Y (UI.ButtonPanel.Buttons[0].Style->Height)

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <ctype.h>
#include <stdexcept>

#include "stratagus.h"

#include "ui/ui.h"

#include "action/action_build.h"
#include "action/action_train.h"
#include "action/actions.h"
//Wyrmgus start
#include "ai/ai_local.h" //for using AiHelpers
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "dependency/dependency.h"
#include "item/item.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "menus.h"
#include "missile/missile.h"
#include "missile/missile_type.h"
#include "network/network.h"
#include "player.h"
#include "sound/sound.h"
#include "sound/unit_sound.h"
#include "spell/spells.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/widgets.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video/cursor.h"
#include "video/font.h"
#include "video/video.h"
#include "world/plane.h"
//Wyrmgus start
#include "world/province.h"
//Wyrmgus end
#include "world/world.h"

#include <algorithm>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int MouseButtons;                            /// Current pressed mouse buttons

int KeyModifiers;                            /// Current keyboard modifiers

CUnit *UnitUnderCursor;                      /// Unit under cursor
int ButtonAreaUnderCursor = -1;              /// Button area under cursor
int ButtonUnderCursor = -1;                  /// Button under cursor
int OldButtonUnderCursor = -1;               /// Button under cursor
//Wyrmgus start
/*
bool GameMenuButtonClicked;                  /// Menu button was clicked
bool GameDiplomacyButtonClicked;             /// Diplomacy button was clicked
*/
//Wyrmgus end
bool LeaveStops;                             /// Mouse leaves windows stops scroll

enum _cursor_on_ CursorOn = CursorOnUnknown; /// Cursor on field

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/
static void HandlePieMenuMouseSelection();

CUnit *GetUnitUnderCursor()
{
	return UnitUnderCursor;
}

/**
**  Cancel building cursor mode.
*/
void CancelBuildingMode()
{
	CursorBuilding = nullptr;
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
}

static bool CanBuildOnArea(const CUnit &unit, const Vec2i &pos)
{
	for (int j = 0; j < unit.GetType()->TileSize.y; ++j) {
		for (int i = 0; i < unit.GetType()->TileSize.x; ++i) {
			const Vec2i tempPos(i, j);
			if (!UI.CurrentMapLayer->Field(pos + tempPos)->playerInfo.IsTeamExplored(*CPlayer::GetThisPlayer())) {
				return false;
			}
		}
	}
	return true;
}

static void DoRightButton_ForForeignUnit(CUnit *dest)
{
	CUnit &unit = *Selected[0];

	if (unit.GetPlayer()->GetIndex() != PlayerNumNeutral || dest == nullptr
		|| !(dest->GetPlayer() == CPlayer::GetThisPlayer() || dest->IsTeamed(*CPlayer::GetThisPlayer()))) {
		return;
	}
	// tell to go and harvest from a unit
	const int res = unit.GivesResource;

	//Wyrmgus start
//	if (res
//		&& dest->GetType()->BoolFlag[HARVESTER_INDEX].value
//		&& dest->GetType()->ResInfo[res]
//		&& dest->GetResourcesHeld() < dest->GetType()->ResInfo[res]->ResourceCapacity
//		&& unit.GetType()->BoolFlag[CANHARVEST_INDEX].value) {
	if (
		dest->CanHarvest(&unit)
		&& dest->GetResourcesHeld() < dest->GetType()->ResInfo[res]->ResourceCapacity
	) {
	//Wyrmgus end
		unit.Blink = 4;
		//  Right mouse with SHIFT appends command to old commands.
		const int flush = !(KeyModifiers & ModifierShift);
		SendCommandResource(*dest, unit, flush);
	}
}

static bool DoRightButton_Transporter(CUnit &unit, CUnit *dest, int flush, int &acknowledged)
{
	//  Enter transporters ?
	if (dest == nullptr) {
		return false;
	}
	// dest is the transporter
	if (dest->GetType()->CanTransport()) {
		// Let the transporter move to the unit. And QUEUE!!!
		if (dest->CanMove() && CanTransport(*dest, unit) && dest->GetPlayer() == CPlayer::GetThisPlayer()) {
			DebugPrint("Send command follow\n");
			// is flush value correct ?
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
			SendCommandFollow(*dest, unit, 0);
		}
		// FIXME : manage correctly production units.
		if ((!unit.CanMove() && dest->GetPlayer() == CPlayer::GetThisPlayer()) || CanTransport(*dest, unit)) {
			dest->Blink = 4;
			DebugPrint("Board transporter\n");
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
			SendCommandBoard(unit, *dest, flush);
			return true;
		}
	}
	//  unit is the transporter
	//  FIXME : Make it more configurable ? NumSelect == 1, lua option
	if (CanTransport(unit, *dest)) {
		// Let the transporter move to the unit. And QUEUE!!!
		if (unit.CanMove()) {
			DebugPrint("Send command follow\n");
			// is flush value correct ?
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
			SendCommandFollow(unit, *dest, 0);
		} else if (!dest->CanMove()) {
			DebugPrint("Want to transport but no unit can move\n");
			return true;
		}
		if (dest->GetPlayer() == CPlayer::GetThisPlayer()) {
			dest->Blink = 4;
			DebugPrint("Board transporter\n");
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
			SendCommandBoard(*dest, unit, flush);
			return true;
		}
	}
	return false;
}

/**
**	@brief	Autocast a spell if the target that was right-clicked on fits the autocast conditions for that spell
**
**	@param	unit			The potential caster
**	@param	dest			The target unit that was clicked on
**	@param	pos				The target position that was clicked on
**	@param	flush			Whether to flush the current order
**	@param	acknowledged	Whether the potential caster has already voiced an acknowledgement due to the right-click
**
**  @return	True if a spell was autocast, false otherwise
*/
static bool DoRightButton_AutoCast(CUnit &unit, CUnit *dest, const Vec2i &pos, int flush, int &acknowledged)
{
	if (dest == nullptr) {
		return false;
	}
	
	if (!unit.AutoCastSpells.empty()) {
		for (CSpell *spell : unit.GetType()->Spells) {
			if (unit.CanAutoCastSpell(spell)) {
				const AutoCastInfo *autocast = spell->GetAutoCastInfo(unit.GetPlayer()->AiEnabled);
				
				if (!spell->CheckAutoCastGenericConditions(unit, autocast, true)) {
					continue;
				}
				
				if (spell->IsUnitValidAutoCastTarget(dest, unit, autocast)) {
					dest->Blink = 4;
					if (!acknowledged) {
						PlayUnitSound(unit, VoiceAttack);
						acknowledged = 1;
					}
					
					SendCommandSpellCast(unit, pos, dest, spell->Slot, flush, UI.CurrentMapLayer->GetIndex());
					
					return true;
				}
			}
		}
	}
	
	return false;
}

static bool DoRightButton_Harvest_Unit(CUnit &unit, CUnit &dest, int flush, int &acknowledged)
{
	// Return a loaded harvester to deposit
	if (unit.GetResourcesHeld() > 0
	//Wyrmgus start
//		&& dest.GetType()->CanStore[unit.GetCurrentResource()]
//		&& (dest.GetPlayer() == unit.GetPlayer()
//			|| (dest.GetPlayer()->IsAllied(*unit.GetPlayer()) && unit.GetPlayer()->IsAllied(*dest.GetPlayer())))) {
		&& unit.CanReturnGoodsTo(&dest)) {
	//Wyrmgus end
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandReturnGoods(unit, &dest, flush);
		return true;
	}
	// Go and harvest from a unit
	//Wyrmgus start
//	const int res = dest.GetType()->GivesResource;
	const int res = dest.GivesResource;
	//Wyrmgus end
	const CUnitType &type = *unit.GetType();
	//Wyrmgus start
//	if (res && type.ResInfo[res] && dest.GetType()->BoolFlag[CANHARVEST_INDEX].value
//		&& (dest.GetPlayer() == unit.GetPlayer() || dest.GetPlayer()->GetIndex() == PlayerNumNeutral)) {
	if (unit.CanHarvest(&dest)) {
	//Wyrmgus end
			if (unit.GetCurrentResource() != res || unit.GetResourcesHeld() < type.ResInfo[res]->ResourceCapacity) {
				dest.Blink = 4;
				SendCommandResource(unit, dest, flush);
				if (!acknowledged) {
					PlayUnitSound(unit, VoiceHarvesting);
					acknowledged = 1;
				}
				return true;
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.GetCurrentResource());
				if (depot) {
					dest.Blink = 4;
					if (!acknowledged) {
						PlayUnitSound(unit, VoiceAcknowledging);
						acknowledged = 1;
					}
					SendCommandReturnGoods(unit, depot, flush);
					//Wyrmgus start
					SendCommandResource(unit, dest, 0);
					//Wyrmgus end
					return true;
				}
			}
	//Wyrmgus start
	// make unit build harvesting building on top if right-clicked
	} else if (res && type.ResInfo[res] && !dest.GetType()->BoolFlag[CANHARVEST_INDEX].value
		&& (dest.GetPlayer() == unit.GetPlayer() || dest.GetPlayer()->GetIndex() == PlayerNumNeutral)) {
			if (unit.GetCurrentResource() != res || unit.GetResourcesHeld() < type.ResInfo[res]->ResourceCapacity) {
				for (CUnitType *unit_type : CUnitType::GetAll()) {
					if (unit_type && unit_type->GivesResource == res && unit_type->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(&unit, *unit_type, dest.GetTilePos(), 1, false, dest.GetMapLayer()->GetIndex())) {
						if (CheckDependencies(unit_type, unit.GetPlayer())) {
							if (unit_type->GetIndex() < (int) AiHelpers.Build.size() && std::find(AiHelpers.Build[unit_type->GetIndex()].begin(), AiHelpers.Build[unit_type->GetIndex()].end(), unit.GetType()) != AiHelpers.Build[unit_type->GetIndex()].end()) {
								dest.Blink = 4;
								SendCommandBuildBuilding(unit, dest.GetTilePos(), *unit_type, flush, dest.GetMapLayer()->GetIndex());
								if (!acknowledged) {
									PlayUnitSound(unit, VoiceBuild);
									acknowledged = 1;
								}
								break;
							}
						} else if (CheckDependencies(unit_type, unit.GetPlayer(), false, true)) { //passes predependency check, even though didn't pass dependency check before, so give a message about the requirements
							CPlayer::GetThisPlayer()->Notify(NotifyYellow, dest.GetTilePos(), dest.GetMapLayer()->GetIndex(), "%s", _("The requirements have not been fulfilled"));
							break;
						}
					}
				}
				return true;
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.GetCurrentResource());
				if (depot) {
					dest.Blink = 4;
					if (!acknowledged) {
						PlayUnitSound(unit, VoiceAcknowledging);
						acknowledged = 1;
					}
					SendCommandReturnGoods(unit, depot, flush);
					//Wyrmgus start
					for (CUnitType *unit_type : CUnitType::GetAll()) {
						if (unit_type && unit_type->GivesResource == res && unit_type->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(&unit, *unit_type, dest.GetTilePos(), 1, false, dest.GetMapLayer()->GetIndex())) {
							if (CheckDependencies(unit_type, unit.GetPlayer())) {
								SendCommandBuildBuilding(unit, dest.GetTilePos(), *unit_type, 0, dest.GetMapLayer()->GetIndex());
								break;
							} else if (CheckDependencies(unit_type, unit.GetPlayer(), false, true)) { //passes predependency check, even though didn't pass dependency check before, so give a message about the requirements
								CPlayer::GetThisPlayer()->Notify(NotifyYellow, dest.GetTilePos(), dest.GetMapLayer()->GetIndex(), "%s", _("The requirements have not been fulfilled"));
								break;
							}
						}
					}
					//Wyrmgus end
					return true;
				}
			}
	//Wyrmgus end
	}
	return false;
}

static bool DoRightButton_Harvest_Pos(CUnit &unit, const Vec2i &pos, int flush, int &acknowledged)
{
	if (!UI.CurrentMapLayer->Field(pos)->playerInfo.IsTeamExplored(*unit.GetPlayer())) {
		return false;
	}
	const CUnitType &type = *unit.GetType();
	// FIXME: support harvesting more types of terrain.
	for (int res = 0; res < MaxCosts; ++res) {
		if (type.ResInfo[res]
			//Wyrmgus start
//			&& type.ResInfo[res]->TerrainHarvester
			&& UI.CurrentMapLayer->Field(pos)->IsTerrainResourceOnMap(res)
			//Wyrmgus end
			//Wyrmgus start
//			&& ((unit.GetCurrentResource() != res)
//				|| (unit.GetResourcesHeld() < type.ResInfo[res]->ResourceCapacity))) {
			) {
			//Wyrmgus end
			//Wyrmgus start
			/*
			SendCommandResourceLoc(unit, pos, flush);
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceHarvesting);
				acknowledged = 1;
			}
			*/
			if (unit.GetCurrentResource() != res || unit.GetResourcesHeld() < type.ResInfo[res]->ResourceCapacity) {
				SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
				if (!acknowledged) {
					PlayUnitSound(unit, VoiceHarvesting);
					acknowledged = 1;
				}
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.GetCurrentResource());
				if (depot) {
					if (!acknowledged) {
						PlayUnitSound(unit, VoiceAcknowledging);
						acknowledged = 1;
					}
					SendCommandReturnGoods(unit, depot, flush);
					SendCommandResourceLoc(unit, pos, 0, UI.CurrentMapLayer->GetIndex());
				}
			}
			//Wyrmgus end
			return true;
		}
	}
	return false;
}

static bool DoRightButton_Worker(CUnit &unit, CUnit *dest, const Vec2i &pos, int flush, int &acknowledged)
{
	const CUnitType &type = *unit.GetType();

	// Go and repair
	if (type.RepairRange && dest != nullptr
		&& dest->GetType()->GetRepairHP()
		//Wyrmgus start
//		&& dest->Variable[HP_INDEX].Value < dest->Variable[HP_INDEX].Max
		&& dest->Variable[HP_INDEX].Value < dest->GetModifiedVariable(HP_INDEX, VariableMax)
		//Wyrmgus end
		&& (dest->GetPlayer() == unit.GetPlayer() || unit.IsAllied(*dest))) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceRepairing);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandRepair(unit, pos, dest, flush);
		SendCommandRepair(unit, pos, dest, flush, UI.CurrentMapLayer->GetIndex());
		//Wyrmgus end
		return true;
	}
	// Harvest
	if (type.BoolFlag[HARVESTER_INDEX].value) {
		if (dest != nullptr) {
			if (DoRightButton_Harvest_Unit(unit, *dest, flush, acknowledged)) {
				return true;
			}
		} else {
			if (DoRightButton_Harvest_Pos(unit, pos, flush, acknowledged)) {
				return true;
			}
		}
	}
	//Wyrmgus start
	if (DoRightButton_Transporter(unit, dest, flush, acknowledged)) {
		return true;
	}
	
	// Pick up an item
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit
		&& CanPickUp(unit, *dest)) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, *dest, flush);
		return true;
	}
	
	// Go through a connector
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && unit.CanUseItem(dest)) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, *dest, flush);
		return true;
	}
	
	//Wyrmgus start
	//if the clicked unit is a settlement site, build on it
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && dest->GetType() == SettlementSiteUnitType && (dest->GetPlayer()->GetIndex() == PlayerNumNeutral || dest->GetPlayer() == unit.GetPlayer())) {
		for (CUnitType *unit_type : CUnitType::GetAll()) {
			if (unit_type && unit_type->BoolFlag[TOWNHALL_INDEX].value && CheckDependencies(unit_type, unit.GetPlayer()) && CanBuildUnitType(&unit, *unit_type, dest->GetTilePos(), 1, false, dest->GetMapLayer()->GetIndex())) {
				if (unit_type->GetIndex() < (int) AiHelpers.Build.size() && std::find(AiHelpers.Build[unit_type->GetIndex()].begin(), AiHelpers.Build[unit_type->GetIndex()].end(), unit.GetType()) != AiHelpers.Build[unit_type->GetIndex()].end()) {
					dest->Blink = 4;
					SendCommandBuildBuilding(unit, dest->GetTilePos(), *unit_type, flush, dest->GetMapLayer()->GetIndex());
					if (!acknowledged) {
						PlayUnitSound(unit, VoiceBuild);
						acknowledged = 1;
					}
					return true;
				}
			}
		}
	}
	//Wyrmgus end
	// Follow another unit
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit
		//Wyrmgus start
//		&& (dest->GetPlayer() == unit.GetPlayer() || unit.IsAllied(*dest) || dest->GetPlayer()->GetIndex() == PlayerNumNeutral)) {
		&& (dest->GetPlayer() == unit.GetPlayer() || unit.IsAllied(*dest) || (dest->GetPlayer()->GetIndex() == PlayerNumNeutral && !unit.IsEnemy(*dest) && !dest->GetType()->BoolFlag[OBSTACLE_INDEX].value))) {
		//Wyrmgus end
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		if (dest->GetType()->CanMove() == false && !dest->GetType()->BoolFlag[TELEPORTER_INDEX].value) {
		if ((dest->GetType()->CanMove() == false && !dest->GetType()->BoolFlag[TELEPORTER_INDEX].value) || dest->GetType()->BoolFlag[BRIDGE_INDEX].value) {
		//Wyrmgus end
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		} else {
			SendCommandFollow(unit, *dest, flush);
		}
		return true;
	}
	//Wyrmgus start
	// make workers attack enemy units if those are right-clicked upon
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && unit.CurrentAction() != UnitActionBuilt && (unit.IsEnemy(*dest) || dest->GetType()->BoolFlag[OBSTACLE_INDEX].value)) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAttack);
			acknowledged = 1;
		}
		if (CanTarget(type, *dest->GetType())) {
			//Wyrmgus start
//			SendCommandAttack(unit, pos, dest, flush);
			SendCommandAttack(unit, pos, dest, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		} else { // No valid target
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		}
		return true;
	}
	//Wyrmgus end
	// Move
	if (!acknowledged) {
		PlayUnitSound(unit, VoiceAcknowledging);
		acknowledged = 1;
	}
	//Wyrmgus start
//	SendCommandMove(unit, pos, flush);
	SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
	//Wyrmgus end
	return true;
}

static bool DoRightButton_AttackUnit(CUnit &unit, CUnit &dest, const Vec2i &pos, int flush, int &acknowledged)
{
	const CUnitType &type = *unit.GetType();
	const int action = type.MouseAction;

	//Wyrmgus start
//	if (action == MouseActionSpellCast || unit.IsEnemy(dest)) {
	if (action == MouseActionSpellCast || unit.IsEnemy(dest) || dest.GetType()->BoolFlag[OBSTACLE_INDEX].value) {
	//Wyrmgus end
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAttack);
			acknowledged = 1;
		}
		if (action == MouseActionSpellCast && unit.GetType()->Spells.size() > 0) {
			// This is for demolition squads and such
			Assert(unit.GetType()->Spells.size() > 0);
			int spellnum = type.Spells[0]->Slot;
			SendCommandSpellCast(unit, pos, &dest, spellnum, flush, UI.CurrentMapLayer->GetIndex());
		} else {
			if (CanTarget(type, *dest.GetType())) {
				SendCommandAttack(unit, pos, &dest, flush, UI.CurrentMapLayer->GetIndex());
			} else { // No valid target
				SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
			}
		}
		return true;
	}
	//Wyrmgus start
	// Pick up an item
	if (&dest != &unit && CanPickUp(unit, dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, dest, flush);
		return true;
	}
	
	// Go through a connector
	if (&dest != &unit && unit.CanUseItem(&dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, dest, flush);
		return true;
	}
	//Wyrmgus end
	if ((dest.GetPlayer() == unit.GetPlayer() || unit.IsAllied(dest) || dest.GetPlayer()->GetIndex() == PlayerNumNeutral) && &dest != &unit) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		if (!dest.GetType()->CanMove() && !dest.GetType()->BoolFlag[TELEPORTER_INDEX].value) {
		if ((!dest.GetType()->CanMove() && !dest.GetType()->BoolFlag[TELEPORTER_INDEX].value) || dest.GetType()->BoolFlag[BRIDGE_INDEX].value) {
		//Wyrmgus end
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
		} else {
			SendCommandFollow(unit, dest, flush);
		}
		return true;
	}
	return false;
}

static void DoRightButton_Attack(CUnit &unit, CUnit *dest, const Vec2i &pos, int flush, int &acknowledged)
{
	if (dest != nullptr && unit.CurrentAction() != UnitActionBuilt) {
		if (DoRightButton_AttackUnit(unit, *dest, pos, flush, acknowledged)) {
			return;
		}
	}

	if (UI.CurrentMapLayer->WallOnMap(pos) && (UI.CurrentMapLayer->Field(pos)->Owner == -1 || CPlayer::GetThisPlayer()->IsEnemy(*CPlayer::Players[UI.CurrentMapLayer->Field(pos)->Owner]))) {
		if (!UI.CurrentMapLayer->Field(pos)->OverlayTerrain->UnitType->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
			return;
		}
	}

	// empty space
	if ((KeyModifiers & ModifierControl)) {
		if (RightButtonAttacks) {
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
		} else {
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAttack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		}
	} else {
		if (RightButtonAttacks) {
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAttack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		} else {
			// Note: move is correct here, right default is move
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAcknowledging);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		}
	}
	// FIXME: ALT-RIGHT-CLICK, move but fight back if attacked.
}

static bool DoRightButton_Follow(CUnit &unit, CUnit &dest, int flush, int &acknowledged)
{
	//Wyrmgus start
	// Pick up an item
	if (CanPickUp(unit, dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, dest, flush);
		return true;
	}
	
	// Go through a connector
	if (unit.CanUseItem(&dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, dest, flush);
		return true;
	}
	//Wyrmgus end
	if (dest.GetPlayer() == unit.GetPlayer() || unit.IsAllied(dest) || dest.GetPlayer()->GetIndex() == PlayerNumNeutral) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		if (dest.GetType()->CanMove() == false && !dest.GetType()->BoolFlag[TELEPORTER_INDEX].value) {
		if ((dest.GetType()->CanMove() == false && !dest.GetType()->BoolFlag[TELEPORTER_INDEX].value) || dest.GetType()->BoolFlag[BRIDGE_INDEX].value) {
		//Wyrmgus end
			//Wyrmgus start
//			SendCommandMove(unit, dest.GetTilePos(), flush);
			SendCommandMove(unit, dest.GetTilePos(), flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		} else {
			SendCommandFollow(unit, dest, flush);
		}
		return true;
	}
	return false;
}

static bool DoRightButton_Harvest_Reverse(CUnit &unit, CUnit &dest, int flush, int &acknowledged)
{
	const CUnitType &type = *unit.GetType();

	// tell to return a loaded harvester to deposit
	if (dest.GetResourcesHeld() > 0
	//Wyrmgus start
//		&& type.CanStore[dest.GetCurrentResource()]
		&& dest.CanReturnGoodsTo(&unit)
	//Wyrmgus end
		&& dest.GetPlayer() == unit.GetPlayer()) {
		dest.Blink = 4;
		SendCommandReturnGoods(dest, &unit, flush);
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		return true;
	}
	// tell to go and harvest from a building
	//Wyrmgus start
//	const int res = type.GivesResource;
	const int res = unit.GivesResource;
	//Wyrmgus end
	//Wyrmgus start
//	if (res
//		&& dest.GetType()->ResInfo[res]
//		&& dest.GetResourcesHeld() < dest.GetType()->ResInfo[res]->ResourceCapacity
//		&& type.BoolFlag[CANHARVEST_INDEX].value
	if (
		dest.CanHarvest(&unit)
		&& dest.GetResourcesHeld() < dest.GetType()->ResInfo[res]->ResourceCapacity
	//Wyrmgus end
		&& dest.GetPlayer() == unit.GetPlayer()) {
		unit.Blink = 4;
		SendCommandResource(dest, unit, flush);
		return true;
	}
	return false;
}

static bool DoRightButton_NewOrder(CUnit &unit, CUnit *dest, const Vec2i &pos, int flush, int &acknowledged)
{
	// Go and harvest from a unit
	//Wyrmgus start
//	if (dest != nullptr && dest->GetType()->GivesResource && dest->GetType()->BoolFlag[CANHARVEST_INDEX].value
//		&& (dest->GetPlayer() == unit.GetPlayer() || dest->GetPlayer()->GetIndex() == PlayerNumNeutral)) {
	if (unit.CanHarvest(dest)) {
		//Wyrmgus end
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandResource(unit, *dest, flush);
		return true;
	}
	// FIXME: support harvesting more types of terrain.
	const CMapField &mf = *UI.CurrentMapLayer->Field(pos);
	if (mf.playerInfo.IsTeamExplored(*unit.GetPlayer()) && mf.IsTerrainResourceOnMap()) {
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandResourceLoc(unit, pos, flush);
		SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
		//Wyrmgus end
		return true;
	}
	return false;
}

static void DoRightButton_ForSelectedUnit(CUnit &unit, CUnit *dest, const Vec2i &pos, int &acknowledged)
{
	// don't self targeting.
	if (dest == &unit) {
		return;
	}
	if (unit.Removed) {
		return;
	}
	const CUnitType &type = *unit.GetType();
	const int action = type.MouseAction;
	//  Right mouse with SHIFT appends command to old commands.
	const int flush = !(KeyModifiers & ModifierShift);

	//Wyrmgus start
	if (action == MouseActionRallyPoint) {
		SendCommandRallyPoint(unit, pos, UI.CurrentMapLayer->GetIndex());
		return;
	}
	//Wyrmgus end
	
	//  Control + alt click - ground attack
	if ((KeyModifiers & ModifierControl) && (KeyModifiers & ModifierAlt)) {
		if (unit.GetType()->BoolFlag[GROUNDATTACK_INDEX].value) {
			if (!acknowledged) {
				PlayUnitSound(unit, VoiceAttack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttackGround(unit, pos, flush);
			SendCommandAttackGround(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			return;
		}
	}
	//  Control + right click on unit is follow anything.
	if ((KeyModifiers & ModifierControl) && dest) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandFollow(unit, *dest, flush);
		return;
	}

	//  Alt + right click on unit is defend anything.
	if ((KeyModifiers & ModifierAlt) && dest) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		SendCommandDefend(unit, *dest, flush);
		return;
	}

	//Wyrmgus start
	//  Ctrl + right click on an empty space moves + stand ground
	if ((KeyModifiers & ModifierControl) && !dest) {
		if (!acknowledged) {
			PlayUnitSound(unit, VoiceAcknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandMove(unit, pos, flush);
		SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
		//Wyrmgus end
		SendCommandStandGround(unit, 0);
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
	if (DoRightButton_AutoCast(unit, dest, pos, flush, acknowledged)) {
		return;
	}

//	if (DoRightButton_Transporter(unit, dest, flush, acknowledged)) {
//		return;
//	}

	//  Handle resource workers.
//	if (action == MouseActionHarvest) {
	if (action == MouseActionHarvest && (!dest || !dest->GetType()->CanTransport() || dest->GetPlayer() == CPlayer::GetThisPlayer() || dest->GetPlayer()->Type != PlayerNeutral)) { //don't harvest neutral garrisonable units (i.e. tree stumps) by right-clicking
	//Wyrmgus end
		DoRightButton_Worker(unit, dest, pos, flush, acknowledged);
		return;
	}

	//Wyrmgus start
	if (DoRightButton_Transporter(unit, dest, flush, acknowledged)) {
		return;
	}
	//Wyrmgus end
	
	//  Fighters
	if (action == MouseActionSpellCast || action == MouseActionAttack) {
		DoRightButton_Attack(unit, dest, pos, flush, acknowledged);
		return;
	}

	// FIXME: attack/follow/board ...
	if (dest != nullptr && (action == MouseActionMove || action == MouseActionSail)) {
		if (DoRightButton_Follow(unit, *dest, flush, acknowledged)) {
			return;
		}
	}

	// Manage harvester from the destination side.
	if (dest != nullptr && dest->GetType()->BoolFlag[HARVESTER_INDEX].value) {
		if (DoRightButton_Harvest_Reverse(unit, *dest, flush, acknowledged)) {
			return;
		}
	}

	// Manage new order.
	if (!unit.CanMove()) {
		if (DoRightButton_NewOrder(unit, dest, pos, flush, acknowledged)) {
			return;
		}
	}
	if (!acknowledged) {
		PlayUnitSound(unit, VoiceAcknowledging);
		acknowledged = 1;
	}
	//Wyrmgus start
//	SendCommandMove(unit, pos, flush);
	SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
	//Wyrmgus end
}

/**
**  Called when right button is pressed
**
**  @param mapPixelPos  map position in pixels.
*/
void DoRightButton(const PixelPos &mapPixelPos)
{
	// No unit selected
	if (Selected.empty()) {
		return;
	}
	const Vec2i pos = CMap::Map.MapPixelPosToTilePos(mapPixelPos, UI.CurrentMapLayer->GetIndex());
	CUnit *dest;            // unit under the cursor if any.

	if (UnitUnderCursor != nullptr && !UnitUnderCursor->GetType()->BoolFlag[DECORATION_INDEX].value) {
		dest = UnitUnderCursor;
	} else {
		dest = nullptr;
	}

	//  Unit selected isn't owned by the player.
	//  You can't select your own units + foreign unit(s)
	//  except if it is neutral and it is a resource.
	if (!CanSelectMultipleUnits(*Selected[0]->GetPlayer())) {
		DoRightButton_ForForeignUnit(dest);
		return;
	}

	if (dest != nullptr && dest->GetType()->CanTransport()) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (CanTransport(*dest, *Selected[i])) {
				// We are clicking on a transporter. We have to:
				// 1) Flush the transporters orders.
				// 2) Tell the transporter to follow the units. We have to queue all
				//    these follow orders, so the transport will go and pick them up
				// 3) Tell all selected land units to go board the transporter.

				// Here we flush the order queue
				SendCommandStopUnit(*dest);
				break;
			}
		}
	}

	int acknowledged = 0; // to play sound
	for (size_t i = 0; i != Selected.size(); ++i) {
		Assert(Selected[i]);
		CUnit &unit = *Selected[i];

		DoRightButton_ForSelectedUnit(unit, dest, pos, acknowledged);
	}
	ShowOrdersCount = GameCycle + Preference.ShowOrders * CYCLES_PER_SECOND;
}

/**
**  Check if the mouse is on a button
**
**  @param screenPos  screen coordinate.
**
**  @return True if mouse is on the button, False otherwise.
*/
bool CUIButton::Contains(const PixelPos &screenPos) const
{
	Assert(this->Style);

	return this->X <= screenPos.x && screenPos.x < this->X + this->Style->Width
		   && this->Y <= screenPos.y && screenPos.y < this->Y + this->Style->Height;
}

/**
**  Set flag on which area is the cursor.
**
**  @param screenPos  screen position.
*/
static void HandleMouseOn(const PixelPos screenPos)
{
	MouseScrollState = ScrollNone;
	ButtonAreaUnderCursor = -1;
	ButtonUnderCursor = -1;

	// BigMapMode is the mode which show only the map (without panel, minimap)
	if (BigMapMode) {
		CursorOn = CursorOnMap;
		//  Scrolling Region Handling.
		HandleMouseScrollArea(screenPos);
		return;
	}

	//  Handle buttons
	if (!IsNetworkGame()) {
		if (UI.MenuButton.X != -1) {
			if (UI.MenuButton.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMenu;
				ButtonUnderCursor = ButtonUnderMenu;
				CursorOn = CursorOnButton;
				return;
			}
		}
	} else {
		//Wyrmgus start
//		if (UI.NetworkMenuButton.X != -1) {
		if (UI.MenuButton.X != -1) {
		//Wyrmgus end
			//Wyrmgus start
//			if (UI.NetworkMenuButton.Contains(screenPos)) {
			if (UI.MenuButton.Contains(screenPos)) {
			//Wyrmgus end
				ButtonAreaUnderCursor = ButtonAreaMenu;
				ButtonUnderCursor = ButtonUnderNetworkMenu;
				CursorOn = CursorOnButton;
				return;
			}
		}
		if (UI.NetworkDiplomacyButton.X != -1) {
			if (UI.NetworkDiplomacyButton.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMenu;
				ButtonUnderCursor = ButtonUnderNetworkDiplomacy;
				CursorOn = CursorOnButton;
				return;
			}
		}
	}
	for (size_t i = 0; i < UI.PlaneButtons.size(); ++i) {
		const CUIButton &button = UI.PlaneButtons[i];

		if (button.X != -1) {
			if (button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMapLayerPlane;
				ButtonUnderCursor = i;
				CursorOn = CursorOnButton;
				return;
			}
		}
	}
	for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
		const CUIButton &button = UI.WorldButtons[i];

		if (button.X != -1) {
			if (button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMapLayerWorld;
				ButtonUnderCursor = i;
				CursorOn = CursorOnButton;
				return;
			}
		}
	}
	for (size_t i = 0; i < UI.SurfaceLayerButtons.size(); ++i) {
		const CUIButton &button = UI.SurfaceLayerButtons[i];

		if (button.X != -1) {
			if (button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMapLayerSurfaceLayer;
				ButtonUnderCursor = i;
				CursorOn = CursorOnButton;
				return;
			}
		}
	}
	for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
		const CUIUserButton &button = UI.UserButtons[i];

		if (button.Button.X != -1) {
			if (button.Button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaUser;
				ButtonUnderCursor = i;
				CursorOn = CursorOnButton;
				return;
			}
		}

	}
	const size_t buttonCount = UI.ButtonPanel.Buttons.size();
	for (unsigned int j = 0; j < buttonCount; ++j) {
		if (UI.ButtonPanel.Buttons[j].Contains(screenPos)) {
			ButtonAreaUnderCursor = ButtonAreaButton;
			if (!CurrentButtons.empty() && CurrentButtons[j].GetPos() != -1) {
				ButtonUnderCursor = j;
				CursorOn = CursorOnButton;
				return;
			}
		}
	}
	if (!Selected.empty()) {
		if (Selected.size() == 1 && Selected[0]->GetType()->CanTransport() && Selected[0]->BoardCount && CurrentButtonLevel == Selected[0]->GetType()->ButtonLevelForTransporter) {
			const size_t size = UI.TransportingButtons.size();

			for (size_t i = std::min<size_t>(Selected[0]->BoardCount, size); i != 0;) {
				--i;
				if (UI.TransportingButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaTransporting;
					ButtonUnderCursor = i;
					CursorOn = CursorOnButton;
					return;
				}
			}
		}
		//Wyrmgus start
		if (Selected.size() == 1 && Selected[0]->HasInventory() && CurrentButtonLevel == CButtonLevel::InventoryButtonLevel) {
			const size_t size = UI.InventoryButtons.size();

			for (size_t i = std::min<size_t>(Selected[0]->InsideCount, size); i != 0;) {
				--i;
				if (UI.InventoryButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaInventory;
					ButtonUnderCursor = i;
					CursorOn = CursorOnButton;
					return;
				}
			}
		}
		//Wyrmgus end
		if (Selected.size() == 1) {
			if (Selected[0]->CurrentAction() == UnitActionTrain) {
				if (Selected[0]->Orders.size() == 1) {
					if (UI.SingleTrainingButton->Contains(screenPos)) {
						ButtonAreaUnderCursor = ButtonAreaTraining;
						ButtonUnderCursor = 0;
						CursorOn = CursorOnButton;
						return;
					}
				} else {
					const size_t size = UI.TrainingButtons.size();
					size_t j = 0;
					for (size_t i = 0; i < Selected[0]->Orders.size() && j < size; ++i) {
						if (Selected[0]->Orders[i]->Action == UnitActionTrain) {
							const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[i]);
							if (i > 0 && j > 0 && Selected[0]->Orders[i - 1]->Action == UnitActionTrain) {
								const COrder_Train &previous_order = *static_cast<COrder_Train *>(Selected[0]->Orders[i - 1]);
								if (previous_order.GetUnitType().GetIndex() == order.GetUnitType().GetIndex()) {
									continue;
								}
							}
							if (Selected[0]->Orders[i]->Action == UnitActionTrain
								&& UI.TrainingButtons[j].Contains(screenPos)) {
								ButtonAreaUnderCursor = ButtonAreaTraining;
								ButtonUnderCursor = j;
								CursorOn = CursorOnButton;
								return;
							}
							++j;
						}
					}
				}
			} else if (Selected[0]->CurrentAction() == UnitActionUpgradeTo) {
				if (UI.UpgradingButton->Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaUpgrading;
					ButtonUnderCursor = 0;
					CursorOn = CursorOnButton;
					return;
				}
			} else if (Selected[0]->CurrentAction() == UnitActionResearch) {
				if (UI.ResearchingButton->Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaResearching;
					ButtonUnderCursor = 0;
					CursorOn = CursorOnButton;
					return;
				}
			}
		}
		if (Selected.size() == 1) {
			if (UI.SingleSelectedButton && UI.SingleSelectedButton->Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaSelected;
				ButtonUnderCursor = 0;
				CursorOn = CursorOnButton;
				return;
			}
		} else {
			const size_t size = UI.SelectedButtons.size();

			for (size_t i = std::min(Selected.size(), size); i != 0;) {
				--i;
				if (UI.SelectedButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaSelected;
					ButtonUnderCursor = i;
					CursorOn = CursorOnButton;
					return;
				}
			}
		}
	}

	//Wyrmgus start
	if (UI.IdleWorkerButton && UI.IdleWorkerButton->Contains(screenPos) && !CPlayer::GetThisPlayer()->FreeWorkers.empty()) {
		ButtonAreaUnderCursor = ButtonAreaIdleWorker;
		ButtonUnderCursor = 0;
		CursorOn = CursorOnButton;
		return;
	}
	
	if (UI.LevelUpUnitButton && UI.LevelUpUnitButton->Contains(screenPos) && !CPlayer::GetThisPlayer()->LevelUpUnits.empty()) {
		ButtonAreaUnderCursor = ButtonAreaLevelUpUnit;
		ButtonUnderCursor = 0;
		CursorOn = CursorOnButton;
		return;
	}
	
	for (size_t i = 0; i < UI.HeroUnitButtons.size() && i < CPlayer::GetThisPlayer()->Heroes.size(); ++i) {
		if (UI.HeroUnitButtons[i].Contains(screenPos)) {
			ButtonAreaUnderCursor = ButtonAreaHeroUnit;
			ButtonUnderCursor = i;
			CursorOn = CursorOnButton;
			return;
		}
	}
	//Wyrmgus end
	
	//  Minimap
	if (UI.Minimap.Contains(screenPos)) {
		CursorOn = CursorOnMinimap;
		return;
	}

	//  On UI graphic
	bool on_ui = false;
	const size_t size = UI.Fillers.size();
	for (unsigned int j = 0; j < size; ++j) {
		if (UI.Fillers[j].OnGraphic(screenPos.x, screenPos.y)) {
			on_ui = true;
			break;
		}
	}

	//  Map
	if (!on_ui && UI.MapArea.Contains(screenPos)) {
		CViewport *vp = GetViewport(screenPos);
		Assert(vp);
		// viewport changed
		if (UI.MouseViewport != vp) {
			UI.MouseViewport = vp;
			DebugPrint("current viewport changed to %ld.\n" _C_
					   static_cast<long int>(vp - UI.Viewports));
		}

		// Note cursor on map can be in scroll area
		CursorOn = CursorOnMap;
	} else {
		CursorOn = CursorOnUnknown;
	}

	//  Scrolling Region Handling.
	HandleMouseScrollArea(screenPos);
}

/**
**  Handle cursor exits the game window (only for some videomodes)
**  @todo FIXME: make it so that the game is partially 'paused'.
**         Game should run (for network play), but not react on or show
**         interactive events.
*/
void HandleMouseExit()
{
	// Disabled
	if (!LeaveStops) {
		return;
	}
	// Denote cursor not on anything in window (used?)
	CursorOn = CursorOnUnknown;

	// Prevent scrolling while out of focus (on other applications) */
	KeyScrollState = MouseScrollState = ScrollNone;

	// Show hour-glass (to denote to the user, the game is waiting)
	// FIXME: couldn't define a hour-glass that easily, so used pointer
	CursorScreenPos.x = Video.Width / 2;
	CursorScreenPos.y = Video.Height / 2;
	GameCursor = UI.Point.Cursor;
}

/**
**  Restrict mouse cursor to viewport.
*/
void RestrictCursorToViewport()
{
	UI.SelectedViewport->Restrict(CursorScreenPos.x, CursorScreenPos.y);
	UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
	CursorOn = CursorOnMap;
}

/**
**  Restrict mouse cursor to minimap
*/
void RestrictCursorToMinimap()
{
	CursorScreenPos.x = std::clamp(CursorScreenPos.x, UI.Minimap.X, UI.Minimap.X + UI.Minimap.W - 1);
	CursorScreenPos.y = std::clamp(CursorScreenPos.y, UI.Minimap.Y, UI.Minimap.Y + UI.Minimap.H - 1);

	UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
	CursorOn = CursorOnMinimap;
}

/**
**  Use the mouse to scroll the map
**
**  @param pos  Screen position.
*/
static void MouseScrollMap(const PixelPos &pos)
{
	const int speed = (KeyModifiers & ModifierControl) ? UI.MouseScrollSpeedControl : UI.MouseScrollSpeedDefault;
	const PixelDiff diff(pos - CursorStartScreenPos);

	UI.MouseViewport->Set(UI.MouseViewport->MapPos, UI.MouseViewport->Offset + speed * diff);
	UI.MouseWarpPos = CursorStartScreenPos;
}

/**
**  Handle movement of the cursor.
**
**  @param cursorPos  Screen X position.
*/
void UIHandleMouseMove(const PixelPos &cursorPos)
{
	enum _cursor_on_ OldCursorOn;

	OldCursorOn = CursorOn;
	//  Selecting units.
	if (CursorState == CursorStateRectangle) {
		// Restrict cursor to viewport.
		UI.SelectedViewport->Restrict(CursorScreenPos.x, CursorScreenPos.y);
		UI.MouseWarpPos = CursorScreenPos;
		return;
	}

	//  Move map.
	if (GameCursor == UI.Scroll.Cursor) {
		MouseScrollMap(cursorPos);
		return;
	}

	UnitUnderCursor = nullptr;
	GameCursor = UI.Point.Cursor;  // Reset
	HandleMouseOn(cursorPos);

	//  Make the piemenu "follow" the mouse
	if (CursorState == CursorStatePieMenu && CursorOn == CursorOnMap) {
		CursorStartScreenPos.x = std::clamp(CursorStartScreenPos.x, CursorScreenPos.x - UI.PieMenu.X[2], CursorScreenPos.x + UI.PieMenu.X[2]);
		CursorStartScreenPos.y = std::clamp(CursorStartScreenPos.y, CursorScreenPos.y - UI.PieMenu.Y[4], CursorScreenPos.y + UI.PieMenu.Y[4]);
		return;
	}

	// Restrict mouse to minimap when dragging
	if (OldCursorOn == CursorOnMinimap && CursorOn != CursorOnMinimap && (MouseButtons & LeftButton)) {
		const Vec2i cursorPos = UI.Minimap.ScreenToTilePos(CursorScreenPos);

		RestrictCursorToMinimap();
		UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(cursorPos, UI.CurrentMapLayer));
		return;
	}

	//  User may be draging with button pressed.
	//Wyrmgus start
//	if (GameMenuButtonClicked || GameDiplomacyButtonClicked) {
	if (UI.MenuButton.Clicked || UI.NetworkDiplomacyButton.Clicked) {
	//Wyrmgus end
		return;
	} else {
		for (size_t i = 0; i < UI.PlaneButtons.size(); ++i) {
			const CUIButton &button = UI.PlaneButtons[i];

			if (button.Clicked) {
				return;
			}
		}
		for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
			const CUIButton &button = UI.WorldButtons[i];

			if (button.Clicked) {
				return;
			}
		}
		for (size_t i = 0; i < UI.SurfaceLayerButtons.size(); ++i) {
			const CUIButton &button = UI.SurfaceLayerButtons[i];

			if (button.Clicked) {
				return;
			}
		}
		for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
			const CUIUserButton &button = UI.UserButtons[i];

			if (button.Clicked) {
				return;
			}
		}
	}

	// This is forbidden for unexplored and not visible space
	// FIXME: This must done new, moving units, scrolling...
	if (UI.MouseViewport == nullptr) {
		fprintf(stderr, "Mouse viewport pointer is null.\n");
	}
	
	if (CursorOn == CursorOnMap && UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
		const CViewport &vp = *UI.MouseViewport;
		const Vec2i tilePos = vp.ScreenToTilePos(cursorPos);

		try {
			if (CursorBuilding && (MouseButtons & LeftButton) && Selected.at(0)
				&& (KeyModifiers & (ModifierAlt | ModifierShift))) {
				const CUnit &unit = *Selected[0];
				const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
				bool explored = CanBuildOnArea(*Selected[0], tilePos);

				// We now need to check if there are another build commands on this build spot
				bool buildable = true;
				for (std::vector<COrderPtr>::const_iterator it = unit.Orders.begin();
					 it != unit.Orders.end(); ++it) {
					COrder &order = **it;
					if (order.Action == UnitActionBuild) {
						COrder_Build &build = dynamic_cast<COrder_Build &>(order);
						if (tilePos.x >= build.GetGoalPos().x
							&& tilePos.x < build.GetGoalPos().x + build.GetUnitType().TileSize.x
							&& tilePos.y >= build.GetGoalPos().y
							&& tilePos.y < build.GetGoalPos().y + build.GetUnitType().TileSize.y) {
							buildable = false;
							break;
						}
					}
				}

				// 0 Test build, don't really build
				//Wyrmgus start
//				if (CanBuildUnitType(Selected[0], *CursorBuilding, tilePos, 0) && buildable && (explored || ReplayRevealMap)) {
				if (CanBuildUnitType(Selected[0], *CursorBuilding, tilePos, 0, false, UI.CurrentMapLayer->GetIndex()) && buildable && (explored || ReplayRevealMap)) {
				//Wyrmgus end
					const int flush = !(KeyModifiers & ModifierShift);
					for (size_t i = 0; i != Selected.size(); ++i) {
						//Wyrmgus start
//						SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush);
						SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush, UI.CurrentMapLayer->GetIndex());
						//Wyrmgus end
					}
					if (!(KeyModifiers & (ModifierAlt | ModifierShift))) {
						CancelBuildingMode();
					}
				}
			}
		} catch (const std::out_of_range &oor) {
			DebugPrint("Selected is empty: %s\n" _C_ oor.what());
		}
		if (Preference.ShowNameDelay) {
			ShowNameDelay = GameCycle + Preference.ShowNameDelay;
			ShowNameTime = GameCycle + Preference.ShowNameDelay + Preference.ShowNameTime;
		}

		bool show = ReplayRevealMap ? true : false;
		if (show == false) {
			CMapField &mf = *UI.CurrentMapLayer->Field(tilePos);
			for (int i = 0; i < PlayerMax; ++i) {
				if (mf.playerInfo.IsTeamExplored(*CPlayer::Players[i])
					//Wyrmgus start
//					&& (i == CPlayer::GetThisPlayer()->GetIndex() || CPlayer::Players[i]->IsBothSharedVision(*CPlayer::GetThisPlayer()))) {
					&& (i == CPlayer::GetThisPlayer()->GetIndex() || CPlayer::Players[i]->IsBothSharedVision(*CPlayer::GetThisPlayer()) || CPlayer::Players[i]->Revealed)) {
					//Wyrmgus end
					show = true;
					break;
				}
			}
		}

		if (show) {
			const PixelPos mapPixelPos = vp.ScreenToMapPixelPos(cursorPos);
			UnitUnderCursor = UnitOnScreen(mapPixelPos.x, mapPixelPos.y);
		}
		
		//Wyrmgus start
		if (show && Selected.size() >= 1 && Selected[0]->GetPlayer() == CPlayer::GetThisPlayer()) {
			bool has_terrain_resource = false;
			const CViewport &vp = *UI.MouseViewport;
			const Vec2i tilePos = vp.ScreenToTilePos(cursorPos);
			for (int res = 0; res < MaxCosts; ++res) {
				if (Selected[0]->GetType()->ResInfo[res]
					//Wyrmgus start
//					&& Selected[0]->GetType()->ResInfo[res]->TerrainHarvester
					//Wyrmgus end
					&& UI.CurrentMapLayer->Field(tilePos)->IsTerrainResourceOnMap(res)
				) {
					has_terrain_resource = true;
				}
			}
			if (has_terrain_resource) {
				GameCursor = UI.YellowHair.Cursor;
			}
		}
		//Wyrmgus end
	} else if (CursorOn == CursorOnMinimap) {
		const Vec2i tilePos = UI.Minimap.ScreenToTilePos(cursorPos);

		if (UI.CurrentMapLayer->Field(tilePos)->playerInfo.IsTeamExplored(*CPlayer::GetThisPlayer()) || ReplayRevealMap) {
			//Wyrmgus start
//			UnitUnderCursor = UnitOnMapTile(tilePos, -1);
			UnitUnderCursor = UnitOnMapTile(tilePos, -1, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
		}
	}

	// NOTE: If unit is not selectable as a goal, you can't get a cursor hint
	if (UnitUnderCursor != nullptr && !UnitUnderCursor->IsVisibleAsGoal(*CPlayer::GetThisPlayer()) &&
		!ReplayRevealMap) {
		UnitUnderCursor = nullptr;
	}

	//  Selecting target.
	if (CursorState == CursorStateSelect) {
		if (CursorOn == CursorOnMap || CursorOn == CursorOnMinimap) {
			if (CustomCursor.length() && CursorByIdent(CustomCursor)) {
				GameCursor = CursorByIdent(CustomCursor);
			} else {
				GameCursor = UI.YellowHair.Cursor;
			}
			if (UnitUnderCursor != nullptr && !UnitUnderCursor->GetType()->BoolFlag[DECORATION_INDEX].value) {
				if (UnitUnderCursor->GetPlayer() == CPlayer::GetThisPlayer() ||
					CPlayer::GetThisPlayer()->IsAllied(*UnitUnderCursor)) {
					if (CustomCursor.length() && CursorByIdent(CustomCursor)) {
						GameCursor = CursorByIdent(CustomCursor);
					} else {
						//Wyrmgus start
//						GameCursor = UI.YellowHair.Cursor;
						GameCursor = UI.GreenHair.Cursor;
						//Wyrmgus end
					}
				//Wyrmgus start
//				} else if (UnitUnderCursor->GetPlayer()->GetIndex() != PlayerNumNeutral) {
				} else if (CPlayer::GetThisPlayer()->IsEnemy(*UnitUnderCursor) || UnitUnderCursor->GetType()->BoolFlag[OBSTACLE_INDEX].value) {
				//Wyrmgus end
					if (CustomCursor.length() && CursorByIdent(CustomCursor)) {
						GameCursor = CursorByIdent(CustomCursor);
					} else {
						//Wyrmgus start
//						GameCursor = UI.YellowHair.Cursor;
						GameCursor = UI.RedHair.Cursor;
						//Wyrmgus end
					}
				}
			}
			if (CursorOn == CursorOnMinimap && (MouseButtons & RightButton)) {
				const Vec2i cursorPos = UI.Minimap.ScreenToTilePos(CursorScreenPos);
				//  Minimap move viewpoint
				UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(cursorPos, UI.CurrentMapLayer));
			}
		}
		// FIXME: must move minimap if right button is down !
		return;
	}

	//  Cursor pointing.
	if (CursorOn == CursorOnMap) {
		//  Map
		if (UnitUnderCursor != nullptr && !UnitUnderCursor->GetType()->BoolFlag[DECORATION_INDEX].value
			&& (UnitUnderCursor->IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap)) {
			//Wyrmgus start
//			GameCursor = UI.Glass.Cursor;
			if (
				Selected.size() >= 1 && Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() && UnitUnderCursor->GetPlayer() != CPlayer::GetThisPlayer()
				&& (Selected[0]->IsEnemy(*UnitUnderCursor) || UnitUnderCursor->GetType()->BoolFlag[OBSTACLE_INDEX].value)
			) {
				GameCursor = UI.RedHair.Cursor;
			} else if (
				Selected.size() >= 1 && Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() &&
				(
					Selected[0]->CanHarvest(UnitUnderCursor, false)
					&& (!Selected[0]->GetCurrentResource() || !UnitUnderCursor->GetType()->CanStore[Selected[0]->GetCurrentResource()] || (Selected[0]->GetCurrentResource() == TradeCost && UnitUnderCursor->GetPlayer() != CPlayer::GetThisPlayer()))
				)
			) {
				GameCursor = UI.YellowHair.Cursor;
			} else {
				GameCursor = UI.Glass.Cursor;
			}
			//Wyrmgus end
		}
		return;
	}

	if (CursorOn == CursorOnMinimap && (MouseButtons & LeftButton)) {
		//  Minimap move viewpoint
		const Vec2i cursorPos = UI.Minimap.ScreenToTilePos(CursorScreenPos);

		UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(cursorPos, UI.CurrentMapLayer));
		CursorStartScreenPos = CursorScreenPos;
		return;
	}
}

//.............................................................................

/**
**  Send selected units to repair
**
**  @param tilePos  tile map position.
*/
//Wyrmgus start
//static int SendRepair(const Vec2i &tilePos)
static int SendRepair(const Vec2i &tilePos, int flush)
//Wyrmgus end
{
	CUnit *dest = UnitUnderCursor;
	int ret = 0;

	// Check if the dest is repairable!
	//Wyrmgus start
//	if (dest && dest->Variable[HP_INDEX].Value < dest->Variable[HP_INDEX].Max
	if (dest && dest->Variable[HP_INDEX].Value < dest->GetModifiedVariable(HP_INDEX, VariableMax)
	//Wyrmgus end
		&& dest->GetType()->GetRepairHP()
		&& (dest->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsAllied(*dest))) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			if (unit->GetType()->RepairRange) {
				//Wyrmgus start
//				const int flush = !(KeyModifiers & ModifierShift);
				//Wyrmgus end

				//Wyrmgus start
//				SendCommandRepair(*unit, tilePos, dest, flush);
				SendCommandRepair(*unit, tilePos, dest, flush, UI.CurrentMapLayer->GetIndex());
				//Wyrmgus end
				ret = 1;
			} else {
				DebugPrint("Non-worker repairs\n");
			}
		}
	}
	return ret;
}

/**
**  Send selected units to point.
**
**  @param tilePos  tile map position.
**
**  @todo To reduce the CPU load for pathfinder, we should check if
**        the destination is reachable and handle nice group movements.
*/
//Wyrmgus start
//static int SendMove(const Vec2i &tilePos)
static int SendMove(const Vec2i &tilePos, int flush)
//Wyrmgus end
{
	CUnit *goal = UnitUnderCursor;
	int ret = 0;
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end

	// Alt makes unit to defend goal
	if (goal && (KeyModifiers & ModifierAlt)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			goal->Blink = 4;
			SendCommandDefend(*unit, *goal, flush);
			ret = 1;
		}
	//Wyrmgus start
	//  Ctrl + click on an empty space moves + stand ground
	} else if (!goal && (KeyModifiers & ModifierControl)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			//Wyrmgus start
//			SendCommandMove(*unit, tilePos, flush);
			SendCommandMove(*unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			SendCommandStandGround(*unit, 0);
			ret = 1;
		}
	//Wyrmgus end
	} else {
		// Move to a transporter.
		if (goal && goal->GetType()->CanTransport()) {
			size_t i;
			for (i = 0; i != Selected.size(); ++i) {
				if (CanTransport(*goal, *Selected[i])) {
					SendCommandStopUnit(*goal);
					ret = 1;
					break;
				}
			}
			if (i == Selected.size()) {
				goal = nullptr;
			}
		} else {
			goal = nullptr;
		}

		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			if (goal && CanTransport(*goal, *unit)) {
				goal->Blink = 4;
				SendCommandFollow(*goal, *unit, 0);
				SendCommandBoard(*unit, *goal, flush);
				ret = 1;
			} else {
				//Wyrmgus start
//				SendCommandMove(*unit, tilePos, flush);
				SendCommandMove(*unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
				//Wyrmgus end
				ret = 1;
			}
		}
	}
	return ret;
}

/**
**  Send the current selected group attacking.
**
**  To empty field:
**    Move to this field attacking all enemy units in reaction range.
**
**  To unit:
**    Move to unit attacking and tracing the unit until dead.
**
**  @param tilePos  tile map position.
**
**  @return 1 if any unit have a new order, 0 else.
**
**  @see Selected
*/
//Wyrmgus start
//static int SendAttack(const Vec2i &tilePos)
static int SendAttack(const Vec2i &tilePos, int flush)
//Wyrmgus end
{
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end
	CUnit *dest = UnitUnderCursor;
	int ret = 0;

	if (dest && dest->GetType()->BoolFlag[DECORATION_INDEX].value) {
		dest = nullptr;
	}
	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];

		//Wyrmgus start
//		if (unit.GetType()->CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			if (!dest || (dest != &unit && CanTarget(*unit.GetType(), *dest->GetType()))) {
				if (dest) {
					dest->Blink = 4;
				}
				//Wyrmgus start
//				SendCommandAttack(unit, tilePos, dest, flush);
				SendCommandAttack(unit, tilePos, dest, flush, UI.CurrentMapLayer->GetIndex());
				//Wyrmgus end
				ret = 1;
			}
		} else {
			if (unit.CanMove()) {
				//Wyrmgus start
//				SendCommandMove(unit, tilePos, flush);
				SendCommandMove(unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
				//Wyrmgus end
				ret = 1;
			}
		}
	}
	return ret;
}

/**
**  Send the current selected group ground attacking.
**
**  @param tilePos  tile map position.
*/
//Wyrmgus start
//static int SendAttackGround(const Vec2i &tilePos)
static int SendAttackGround(const Vec2i &tilePos, int flush)
//Wyrmgus end
{
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end
	int ret = 0;

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];
		//Wyrmgus start
//		if (unit.GetType()->CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			//Wyrmgus start
//			SendCommandAttackGround(unit, tilePos, flush);
			SendCommandAttackGround(unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			ret = 1;
		} else {
			//Wyrmgus start
//			SendCommandMove(unit, tilePos, flush);
			SendCommandMove(unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			ret = 1;
		}
	}
	return ret;
}

/**
**  Let units patrol between current position and the selected.
**
**  @param tilePos  tile map position.
*/
//Wyrmgus start
//static int SendPatrol(const Vec2i &tilePos)
static int SendPatrol(const Vec2i &tilePos, int flush)
//Wyrmgus end
{
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];
		//Wyrmgus start
//		SendCommandPatrol(unit, tilePos, flush);
		SendCommandPatrol(unit, tilePos, flush, UI.CurrentMapLayer->GetIndex());
		//Wyrmgus end
	}
	return Selected.empty() ? 0 : 1;
}

/**
**  Let units harvest wood/mine gold/haul oil
**
**  @param pos  tile map position
**
**  @see Selected
*/
//Wyrmgus start
//static int SendResource(const Vec2i &pos)
static int SendResource(const Vec2i &pos, int flush)
//Wyrmgus end
{
	int res;
	CUnit *dest = UnitUnderCursor;
	int ret = 0;
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end
	const CMapField &mf = *UI.CurrentMapLayer->Field(pos);

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];

		if (unit.GetType()->BoolFlag[HARVESTER_INDEX].value) {
			if (dest
				//Wyrmgus start
//				&& (res = dest->GetType()->GivesResource) != 0
				&& (res = dest->GivesResource) != 0
				//Wyrmgus end
				//Wyrmgus start
//				&& unit.GetType()->ResInfo[res]
				&& unit.CanHarvest(dest)
				//Wyrmgus end
				&& unit.GetResourcesHeld() < unit.GetType()->ResInfo[res]->ResourceCapacity
				//Wyrmgus start
//				&& dest->GetType()->BoolFlag[CANHARVEST_INDEX].value
//				&& (dest->GetPlayer() == unit.GetPlayer() || dest->GetPlayer()->GetIndex() == PlayerMax - 1)) {
				) {
				//Wyrmgus end
				dest->Blink = 4;
				SendCommandResource(unit, *dest, flush);
				ret = 1;
				continue;
			} else {
				for (res = 0; res < MaxCosts; ++res) {
					if (unit.GetType()->ResInfo[res]
						//Wyrmgus start
//						&& unit.GetType()->ResInfo[res]->TerrainHarvester
//						&& mf.playerInfo.IsExplored(*unit.GetPlayer())
						&& mf.playerInfo.IsTeamExplored(*unit.GetPlayer())
						//Wyrmgus end
						&& mf.IsTerrainResourceOnMap(res)
						&& unit.GetResourcesHeld() < unit.GetType()->ResInfo[res]->ResourceCapacity
						&& (unit.GetCurrentResource() != res || unit.GetResourcesHeld() < unit.GetType()->ResInfo[res]->ResourceCapacity)) {
						//Wyrmgus start
//						SendCommandResourceLoc(unit, pos, flush);
						SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
						//Wyrmgus end
						ret = 1;
						break;
					}
				}
				if (res != MaxCosts) {
					continue;
				}
			}
		}
		if (!unit.CanMove()) {
			//Wyrmgus start
//			if (dest && dest->GetType()->GivesResource && dest->GetType()->BoolFlag[CANHARVEST_INDEX].value) {
			if (dest && dest->GivesResource && dest->GetType()->BoolFlag[CANHARVEST_INDEX].value) {
			//Wyrmgus end
				dest->Blink = 4;
				SendCommandResource(unit, *dest, flush);
				ret = 1;
				continue;
			}
			//Wyrmgus start
//			if (mf.playerInfo.IsExplored(*unit.GetPlayer()) && mf.IsTerrainResourceOnMap()) {
			if (mf.playerInfo.IsTeamExplored(*unit.GetPlayer()) && mf.IsTerrainResourceOnMap()) {
			//Wyrmgus end
				//Wyrmgus start
//				SendCommandResourceLoc(unit, pos, flush);
				SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
				//Wyrmgus end
				ret = 1;
				continue;
			}
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->GetIndex());
			//Wyrmgus end
			ret = 1;
			continue;
		}
	}
	return ret;
}

/**
**  Send selected units to unload passengers.
**
**  @param tilePos  tile map position.
*/
//Wyrmgus start
//static int SendUnload(const Vec2i &tilePos)
static int SendUnload(const Vec2i &tilePos, int flush)
//Wyrmgus emd
{
	//Wyrmgus start
//	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end

	for (size_t i = 0; i != Selected.size(); ++i) {
		// FIXME: not only transporter selected?
		//Wyrmgus start
//		SendCommandUnload(*Selected[i], tilePos, NoUnitP, flush);
		SendCommandUnload(*Selected[i], tilePos, NoUnitP, flush, UI.CurrentMapLayer->GetIndex());
		//Wyrmgus end
	}
	return Selected.empty() ? 0 : 1;
}

/**
**  Send the current selected group for spell cast.
**
**  To empty field:
**  To unit:
**    Spell cast on unit or on map spot.
**
**  @param tilePos  tile map position.
**
**  @see Selected
*/
static int SendSpellCast(const Vec2i &tilePos, int flush)
{
	CUnit *dest = UnitUnderCursor;
	int ret = 0;

	/* NOTE: Vladi:
	   This is a high-level function, it sends target spot and unit
	   (if exists). All checks are performed at spell cast handle
	   function which will cancel function if cannot be executed
	 */
	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];
		if (unit.GetType()->Spells.size() == 0) {
			DebugPrint("but unit %d(%s) can't cast spells?\n" _C_
					   UnitNumber(unit) _C_ unit.GetType()->GetName().utf8().get_data());
			// this unit cannot cast spell
			continue;
		}
		if (dest && &unit == dest) {
			// no unit can cast spell on himself
			// n0b0dy: why not?
			continue;
		}
		// CursorValue here holds the spell type id
		const CSpell *spell = CSpell::Spells[CursorValue];
		if (!spell) {
			fprintf(stderr, "unknown spell-id: %d\n", CursorValue);
			ExitFatal(1);
		}
		
		if (std::find(unit.GetType()->Spells.begin(), unit.GetType()->Spells.end(), spell) == unit.GetType()->Spells.end()) {
			continue; //the unit's type cannot cast this spell
		}
		
		if (!unit.CanCastSpell(spell, false)) {
			continue;
		}
		
		SendCommandSpellCast(unit, tilePos, spell->Target == TargetPosition ? nullptr : dest , CursorValue, flush, UI.CurrentMapLayer->GetIndex());
		ret = 1;
		
		//if the spell's effects do not stack on the same target, then only one selected unit should cast it
		if (!spell->Stackable) {
			break;
		}
	}
	return ret;
}

//Wyrmgus start
/**
**  Set point as rally point for selected units.
**
**  @param tilePos  tile map position.
**
**  @todo To reduce the CPU load for pathfinder, we should check if
**        the destination is reachable and handle nice group movements.
*/
static int SendRallyPoint(const Vec2i &tilePos)
{
	int ret = 0;

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit *unit = Selected[i];

		SendCommandRallyPoint(*unit, tilePos, UI.CurrentMapLayer->GetIndex());
		ret = 1;
	}
	
	return ret;
}
//Wyrmgus end

/**
**  Send a command to selected units.
**
**  @param tilePos  tile map position.
*/
static void SendCommand(const Vec2i &tilePos)
{
	int ret = 0;

	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
	
	//Wyrmgus start
	const int flush = !(KeyModifiers & ModifierShift);
	//Wyrmgus end
	
	switch (CursorAction) {
		case ButtonMove:
			//Wyrmgus start
//			ret = SendMove(tilePos);
			ret = SendMove(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonRepair:
			//Wyrmgus start
//			ret = SendRepair(tilePos);
			ret = SendRepair(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonAttack:
			//Wyrmgus start
//			ret = SendAttack(tilePos);
			ret = SendAttack(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonAttackGround:
			//Wyrmgus start
//			ret = SendAttackGround(tilePos);
			ret = SendAttackGround(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonPatrol:
			//Wyrmgus start
//			ret = SendPatrol(tilePos);
			ret = SendPatrol(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonHarvest:
			//Wyrmgus start
//			ret = SendResource(tilePos);
			ret = SendResource(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonUnload:
			//Wyrmgus start
//			ret = SendUnload(tilePos);
			ret = SendUnload(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonSpellCast:
			//Wyrmgus start
//			ret = SendSpellCast(tilePos);
			ret = SendSpellCast(tilePos, flush);
			//Wyrmgus end
			break;
		//Wyrmgus start
		case ButtonRallyPoint:
			ret = SendRallyPoint(tilePos);
			break;
		//Wyrmgus end
		default:
			DebugPrint("Unsupported send action %d\n" _C_ CursorAction);
			break;
	}
	if (ret) {
		// Acknowledge the command with first selected unit.
		for (size_t i = 0; i != Selected.size(); ++i) {
			//Wyrmgus start
//			if (CursorAction == ButtonAttack || CursorAction == ButtonAttackGround || CursorAction == ButtonSpellCast) {
			if (CursorAction == ButtonAttack || CursorAction == ButtonAttackGround) {
			//Wyrmgus end
				//Wyrmgus start
				/*
				if (Selected[i]->GetType()->MapSound.Attack.Sound) {
					PlayUnitSound(*Selected[i], VoiceAttack);
					break;
				} else if (Selected[i]->GetType()->MapSound.Acknowledgement.Sound) {
					PlayUnitSound(*Selected[i], VoiceAcknowledging);
					break;
				}
				*/
				PlayUnitSound(*Selected[i], VoiceAttack);
				break;
				//Wyrmgus end
			//Wyrmgus start
//			} else if (CursorAction == ButtonRepair && Selected[i]->GetType()->MapSound.Repair.Sound) {
			} else if (CursorAction == ButtonRepair) {
			//Wyrmgus end
				PlayUnitSound(*Selected[i], VoiceRepairing);
				break;
			//Wyrmgus start
//			} else if (CursorAction == ButtonBuild && Selected[i]->GetType()->MapSound.Build.Sound) {
			} else if (CursorAction == ButtonBuild) {
			//Wyrmgus end
				PlayUnitSound(*Selected[i], VoiceBuild);
				break;
			//Wyrmgus start
//			} else if (Selected[i]->GetType()->MapSound.Acknowledgement.Sound) {
			} else {
			//Wyrmgus end
				PlayUnitSound(*Selected[i], VoiceAcknowledging);
				break;
			}
		}
		ShowOrdersCount = GameCycle + Preference.ShowOrders * CYCLES_PER_SECOND;
	}
}

//.............................................................................

/**
**  Mouse button press on selection/group area.
**
**  @param num     Button number.
**  @param button  Mouse Button pressed.
*/
static void DoSelectionButtons(int num, unsigned)
{
	if (GameObserve || GamePaused || GameEstablishing) {
		return;
	}

	//Wyrmgus start
//	if (static_cast<size_t>(num) >= Selected.size() || !(MouseButtons & LeftButton)) {
	if (static_cast<size_t>(num) >= Selected.size()) {
	//Wyrmgus end
		return;
	}

	CUnit &unit = *Selected[num];

	if ((KeyModifiers & ModifierControl) || (MouseButtons & (LeftButton << MouseDoubleShift))) {
		if (KeyModifiers & ModifierShift) {
			ToggleUnitsByType(unit);
		} else {
			SelectUnitsByType(unit);
		}
	} else if (KeyModifiers & ModifierAlt) {
		if (KeyModifiers & ModifierShift) {
			AddGroupFromUnitToSelection(unit);
		} else {
			SelectGroupFromUnit(unit);
		}
	} else if (KeyModifiers & ModifierShift) {
		ToggleSelectUnit(unit);
	} else {
		SelectSingleUnit(unit);
	}

	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentButtonLevel = nullptr;
	SelectionChanged();
}

//.............................................................................

/**
**  Handle mouse button pressed in select state.
**
**  Select state is used for target of patrol, attack, move, ....
**
**  @param button  Button pressed down.
*/
static void UISelectStateButtonDown(unsigned)
{
	if (GameObserve || GamePaused || GameEstablishing) {
		return;
	}

	//
	//  Clicking on the map.
	//
	if (CursorOn == CursorOnMap && UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
		CursorState = CursorStatePoint;
		GameCursor = UI.Point.Cursor;
		CustomCursor.clear();
		CurrentButtonLevel = nullptr;
		UI.ButtonPanel.Update();

		if (MouseButtons & LeftButton) {
			const CViewport &vp = *UI.MouseViewport;
			const PixelPos mapPixelPos = vp.ScreenToMapPixelPos(CursorScreenPos);

			if (!ClickMissile.empty()) {
				MakeLocalMissile(*MissileTypeByIdent(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->GetIndex());
			}
			SendCommand(CMap::Map.MapPixelPosToTilePos(mapPixelPos, UI.CurrentMapLayer->GetIndex()));
		}
		return;
	}

	//
	//  Clicking on the minimap.
	//
	if (CursorOn == CursorOnMinimap) {
		const Vec2i cursorTilePos = UI.Minimap.ScreenToTilePos(CursorScreenPos);

		if (MouseButtons & LeftButton) {
			const PixelPos mapPixelPos = CMap::Map.TilePosToMapPixelPos_Center(cursorTilePos, UI.CurrentMapLayer);

			UI.StatusLine.Clear();
			UI.StatusLine.ClearCosts();
			CursorState = CursorStatePoint;
			GameCursor = UI.Point.Cursor;
			CustomCursor.clear();
			CurrentButtonLevel = nullptr;
			UI.ButtonPanel.Update();
			if (!ClickMissile.empty()) {
				MakeLocalMissile(*MissileTypeByIdent(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->GetIndex());
			}
			SendCommand(cursorTilePos);
		} else {
			UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(cursorTilePos, UI.CurrentMapLayer));
		}
		return;
	}

	if (CursorOn == CursorOnButton) {
		// FIXME: other buttons?
		if (ButtonAreaUnderCursor == ButtonAreaButton) {
			OldButtonUnderCursor = ButtonUnderCursor;
			return;
		}
	}

	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CursorState = CursorStatePoint;
	if (CustomCursor.length() && CursorByIdent(CustomCursor)) {
		GameCursor = CursorByIdent(CustomCursor);
	} else {
		GameCursor = UI.YellowHair.Cursor;
	}
	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
}


static void UIHandleButtonDown_OnMap(unsigned button)
{
	Assert(UI.MouseViewport);
	
	if (!UI.MouseViewport) {
		return;
	}

#ifdef USE_TOUCHSCREEN
	// Detect double left click
	const bool doubleLeftButton = MouseButtons & (LeftButton << MouseDoubleShift);
#endif
	if ((MouseButtons & LeftButton) && UI.SelectedViewport != UI.MouseViewport) {
		UI.SelectedViewport = UI.MouseViewport;
		DebugPrint("selected viewport changed to %ld.\n" _C_
				   static_cast<long int>(UI.SelectedViewport - UI.Viewports));
	}

	// to redraw the cursor immediately (and avoid up to 1 sec delay
	if (CursorBuilding) {
#ifdef USE_TOUCHSCREEN
		// On touch screen is building started with double left click
		if (!doubleLeftButton) {
			return;
		}
#endif
		// Possible Selected[0] was removed from map
		// need to make sure there is a unit to build
		if (Selected[0] && (MouseButtons & LeftButton)
			&& UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {// enter select mode
			const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
			bool explored = CanBuildOnArea(*Selected[0], tilePos);

			// 0 Test build, don't really build
			//Wyrmgus start
//			if (CanBuildUnitType(Selected[0], *CursorBuilding, tilePos, 0) && (explored || ReplayRevealMap)) {
			if (CanBuildUnitType(Selected[0], *CursorBuilding, tilePos, 0, false, UI.CurrentMapLayer->GetIndex()) && (explored || ReplayRevealMap)) {
			//Wyrmgus end
				const int flush = !(KeyModifiers & ModifierShift);
				PlayGameSound(GameSounds.PlacementSuccess[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
				PlayUnitSound(*Selected[0], VoiceBuild);
				for (size_t i = 0; i != Selected.size(); ++i) {
					//Wyrmgus start
//					SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush);
					SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush, UI.CurrentMapLayer->GetIndex());
					//Wyrmgus end
				}
				if (!(KeyModifiers & (ModifierAlt | ModifierShift))) {
					CancelBuildingMode();
				}
			} else {
				if (UI.CurrentMapLayer->GetIndex() != CPlayer::GetThisPlayer()->StartMapLayer && (UI.CurrentMapLayer->GetPlane() != CMap::Map.MapLayers[CPlayer::GetThisPlayer()->StartMapLayer]->GetPlane() || UI.CurrentMapLayer->GetWorld() != CMap::Map.MapLayers[CPlayer::GetThisPlayer()->StartMapLayer]->GetWorld())) {
					CPlayer::GetThisPlayer()->Notify("%s", _("Cannot build in another plane or world"));
				}
				PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
			}
		} else {
			CancelBuildingMode();
		}
		return;
	}

	if (MouseButtons & UI.PieMenu.MouseButton) { // enter pie menu
		UnitUnderCursor = nullptr;
		GameCursor = UI.Point.Cursor;  // Reset
		CursorStartScreenPos = CursorScreenPos;
		if (!Selected.empty() && Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() && CursorState == CursorStatePoint) {
			CursorState = CursorStatePieMenu;
		}
#ifdef USE_TOUCHSCREEN
	} else if (doubleLeftButton) {
#else
	} else if (MouseButtons & RightButton) {
#endif
		if (!GameObserve && !GamePaused && !GameEstablishing && UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
			CUnit *unit;
			// FIXME: Rethink the complete chaos of coordinates here
			// FIXME: Johns: Perhaps we should use a pixel map coordinates
			const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);

			if (UnitUnderCursor != nullptr && (unit = UnitOnMapTile(tilePos, -1, UI.CurrentMapLayer->GetIndex()))
				&& !UnitUnderCursor->GetType()->BoolFlag[DECORATION_INDEX].value) {
				unit->Blink = 4;                // if right click on building -- blink
			} else { // if not not click on building -- green cross
				if (!ClickMissile.empty()) {
					const PixelPos mapPixelPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);

					MakeLocalMissile(*MissileTypeByIdent(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->GetIndex());
				}
			}
			const PixelPos mapPixelPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);
			DoRightButton(mapPixelPos);
		}
	} else if (MouseButtons & LeftButton) { // enter select mode
		CursorStartScreenPos = CursorScreenPos;
		CursorStartMapPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);
		GameCursor = UI.Cross.Cursor;
		CursorState = CursorStateRectangle;
	} else if (MouseButtons & MiddleButton) {// enter move map mode
		CursorStartScreenPos = CursorScreenPos;
		GameCursor = UI.Scroll.Cursor;
		//Wyrmgus start
		UnitUnderCursor = nullptr;
		//Wyrmgus end
	}
}

static void UIHandleButtonDown_OnMinimap(unsigned button)
{
	const Vec2i cursorTilePos = UI.Minimap.ScreenToTilePos(CursorScreenPos);

	if (MouseButtons & LeftButton) { // enter move mini-mode
		UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(cursorTilePos, UI.CurrentMapLayer));
	} else if (MouseButtons & RightButton) {
		if (!GameObserve && !GamePaused && !GameEstablishing) {
			const PixelPos mapPixelPos = CMap::Map.TilePosToMapPixelPos_Center(cursorTilePos, UI.CurrentMapLayer);
			if (!ClickMissile.empty()) {
				MakeLocalMissile(*MissileTypeByIdent(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->GetIndex());
			}
			DoRightButton(mapPixelPos);
		}
	}
}

static void UIHandleButtonDown_OnButton(unsigned button)
{
	// clicked on info panel - selection shown
	//Wyrmgus start
	/*
	if (Selected.size() > 1 && ButtonAreaUnderCursor == ButtonAreaSelected) {
		DoSelectionButtons(ButtonUnderCursor, button);
	} else if ((MouseButtons & LeftButton)) {
	*/
	if ((MouseButtons & LeftButton)) {
	//Wyrmgus end
		//  clicked on menu button
		if (ButtonAreaUnderCursor == ButtonAreaMenu) {
			if ((ButtonUnderCursor == ButtonUnderMenu || ButtonUnderCursor == ButtonUnderNetworkMenu)
				//Wyrmgus start
//				&& !GameMenuButtonClicked) {
				&& !UI.MenuButton.Clicked) {
				//Wyrmgus end
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				//Wyrmgus start
//				GameMenuButtonClicked = true;
				UI.MenuButton.Clicked = true;
				//Wyrmgus end
			//Wyrmgus start
//			} else if (ButtonUnderCursor == ButtonUnderNetworkDiplomacy && !GameDiplomacyButtonClicked) {
			} else if (ButtonUnderCursor == ButtonUnderNetworkDiplomacy && !UI.NetworkDiplomacyButton.Clicked) {
			//Wyrmgus end
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				//Wyrmgus start
//				GameDiplomacyButtonClicked = true;
				UI.NetworkDiplomacyButton.Clicked = true;
				//Wyrmgus end
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaMapLayerPlane) {
			for (size_t i = 0; i < UI.PlaneButtons.size(); ++i) {
				CUIButton &button = UI.PlaneButtons[i];

				if (i == size_t(ButtonUnderCursor) && !button.Clicked) {
					PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
					button.Clicked = true;
				}
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaMapLayerWorld) {
			for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
				CUIButton &button = UI.WorldButtons[i];

				if (i == size_t(ButtonUnderCursor) && !button.Clicked) {
					PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
					button.Clicked = true;
				}
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaMapLayerSurfaceLayer) {
			for (size_t i = 0; i < UI.SurfaceLayerButtons.size(); ++i) {
				CUIButton &button = UI.SurfaceLayerButtons[i];

				if (i == size_t(ButtonUnderCursor) && !button.Clicked) {
					PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
					button.Clicked = true;
				}
			}
		//  clicked on user buttons
		} else if (ButtonAreaUnderCursor == ButtonAreaUser) {
			for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
				CUIUserButton &button = UI.UserButtons[i];

				if (i == size_t(ButtonUnderCursor) && !button.Clicked) {
					PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
					button.Clicked = true;
				}
			}
			//  clicked on selected button
		//Wyrmgus start
		/*
		} else if (ButtonAreaUnderCursor == ButtonAreaSelected) {
			//  clicked on single unit shown
			if (ButtonUnderCursor == 0 && Selected.size() == 1) {
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				UI.SelectedViewport->Center(Selected[0]->GetMapPixelPosCenter());
			}
			//  clicked on training button
		} else if (ButtonAreaUnderCursor == ButtonAreaTraining) {
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				if (static_cast<size_t>(ButtonUnderCursor) < Selected[0]->Orders.size()
					&& Selected[0]->Orders[ButtonUnderCursor]->Action == UnitActionTrain) {
					const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[ButtonUnderCursor]);

					DebugPrint("Cancel slot %d %s\n" _C_ ButtonUnderCursor _C_ order.GetUnitType().Ident.c_str());
					SendCommandCancelTraining(*Selected[0], ButtonUnderCursor, &order.GetUnitType());
				}
			}
			//  clicked on upgrading button
		} else if (ButtonAreaUnderCursor == ButtonAreaUpgrading) {
			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel upgrade %s\n" _C_ Selected[0]->GetType()->Ident.c_str());
					SendCommandCancelUpgradeTo(*Selected[0]);
				}
			}
			//  clicked on researching button
		} else if (ButtonAreaUnderCursor == ButtonAreaResearching) {
			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel research %s\n" _C_ Selected[0]->GetType()->Ident.c_str());
					SendCommandCancelResearch(*Selected[0]);
				}
			}
			//  clicked on button panel
		} else if (ButtonAreaUnderCursor == ButtonAreaTransporting) {
			//  for transporter
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->IsAllied(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				if (Selected[0]->BoardCount >= ButtonUnderCursor) {
					CUnit *uins = Selected[0]->UnitInside;
					size_t j = 0;

					for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
						if (!uins->Boarded || j >= UI.TransportingButtons.size() || (Selected[0]->GetPlayer() != CPlayer::GetThisPlayer() && uins->GetPlayer() != CPlayer::GetThisPlayer())) {
							continue;
						}
						if (ButtonAreaUnderCursor == ButtonAreaTransporting
							&& static_cast<size_t>(ButtonUnderCursor) == j) {
								Assert(uins->Boarded);
								const int flush = !(KeyModifiers & ModifierShift);
								if (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || uins->GetPlayer() == CPlayer::GetThisPlayer()) {
									SendCommandUnload(*Selected[0], Selected[0]->GetTilePos(), uins, flush);
								}
						}
						++j;
					}
				}
			}
		*/
		//Wyrmgus end
		} else if (ButtonAreaUnderCursor == ButtonAreaButton) {
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				OldButtonUnderCursor = ButtonUnderCursor;
			}
		}
	} else if ((MouseButtons & MiddleButton)) {
		//Wyrmgus start
		/*
		//  clicked on info panel - single unit shown
		if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0 && Selected.size() == 1) {
			PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
			if (UI.SelectedViewport->Unit == Selected[0]) {
				UI.SelectedViewport->Unit = nullptr;
			} else {
				UI.SelectedViewport->Unit = Selected[0];
			}
		}
		*/
		//Wyrmgus end
	} else if ((MouseButtons & RightButton)) {
	}
}

//Wyrmgus start
static void UIHandleButtonUp_OnButton(unsigned button)
{
	// clicked on info panel - selection shown
	if (Selected.size() > 1 && ButtonAreaUnderCursor == ButtonAreaSelected) {
		DoSelectionButtons(ButtonUnderCursor, button);
	} else {
		if (ButtonAreaUnderCursor == ButtonAreaSelected) {
			//  clicked on single unit shown
			if (ButtonUnderCursor == 0 && Selected.size() == 1) {
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				if ((1 << button) == LeftButton) {
					if (Selected[0]->GetMapLayer() != UI.CurrentMapLayer) {
						ChangeCurrentMapLayer(Selected[0]->GetMapLayer()->GetIndex());
					}
					UI.SelectedViewport->Center(Selected[0]->GetMapPixelPosCenter());
				} else if ((1 << button) == RightButton) {
					std::string encyclopedia_ident = Selected[0]->GetType()->Ident;
					std::string encyclopedia_state = "units";
					if (Selected[0]->Character != nullptr && Selected[0]->Character->GetIcon() != nullptr && !Selected[0]->Character->Custom) {
						encyclopedia_ident = Selected[0]->Character->Ident;
						encyclopedia_state = "heroes";
					} else if (Selected[0]->Unique != nullptr) {
						encyclopedia_ident = Selected[0]->Unique->Ident;
						encyclopedia_state = "unique_items";
					}
					CclCommand("if (OpenEncyclopediaUnitEntry ~= nil) then OpenEncyclopediaUnitEntry(\"" + encyclopedia_ident + "\", \"" + encyclopedia_state + "\") end;");
				} else if ((1 << button) == MiddleButton) {
					//  clicked on info panel - single unit shown
					if (UI.SelectedViewport->Unit == Selected[0]) {
						UI.SelectedViewport->Unit = nullptr;
					} else {
						UI.SelectedViewport->Unit = Selected[0];
					}
				}
			}
			//  clicked on training button
		} else if (ButtonAreaUnderCursor == ButtonAreaTraining) {
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				if (static_cast<size_t>(ButtonUnderCursor) < Selected[0]->Orders.size()) {
					size_t j = 0;
					int order_slot = -1;
					for (size_t i = 0; i < Selected[0]->Orders.size(); ++i) {
						if (Selected[0]->Orders[i]->Action == UnitActionTrain) {
							const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[i]);
							if (i > 0 && j > 0 && Selected[0]->Orders[i - 1]->Action == UnitActionTrain) {
								const COrder_Train &previous_order = *static_cast<COrder_Train *>(Selected[0]->Orders[i - 1]);
								if (previous_order.GetUnitType().GetIndex() == order.GetUnitType().GetIndex()) {
									if (order_slot != -1) {
										order_slot = i; //so that it removes the last training order of that unit type
									}
									continue;
								}
							}
							if (j >= UI.TrainingButtons.size() || order_slot != -1) {
								break;
							} else if (j == static_cast<size_t>(ButtonUnderCursor)) {
								order_slot = i;
							}
							++j;
						}
					}
					if (order_slot != -1) {
						const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[order_slot]);

						DebugPrint("Cancel slot %d %s\n" _C_ order_slot _C_ order.GetUnitType().Ident.c_str());
						SendCommandCancelTraining(*Selected[0], order_slot, &order.GetUnitType());
					}
				}
			}
			//  clicked on upgrading button
		} else if (ButtonAreaUnderCursor == ButtonAreaUpgrading) {
			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel upgrade %s\n" _C_ Selected[0]->GetType()->Ident.c_str());
					SendCommandCancelUpgradeTo(*Selected[0]);
				}
			}
			//  clicked on researching button
		} else if (ButtonAreaUnderCursor == ButtonAreaResearching) {
			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel research %s\n" _C_ Selected[0]->GetType()->Ident.c_str());
					SendCommandCancelResearch(*Selected[0]);
				}
			}
			//  clicked on button panel
		} else if (ButtonAreaUnderCursor == ButtonAreaTransporting) {
			//  for transporter
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->IsAllied(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				if (Selected[0]->BoardCount >= ButtonUnderCursor) {
					CUnit *uins = Selected[0]->UnitInside;
					size_t j = 0;

					for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
						if (!uins->Boarded || j >= UI.TransportingButtons.size() || (Selected[0]->GetPlayer() != CPlayer::GetThisPlayer() && uins->GetPlayer() != CPlayer::GetThisPlayer())) {
							continue;
						}
						if (ButtonAreaUnderCursor == ButtonAreaTransporting
							&& static_cast<size_t>(ButtonUnderCursor) == j) {
								Assert(uins->Boarded);
								const int flush = !(KeyModifiers & ModifierShift);
								if (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || uins->GetPlayer() == CPlayer::GetThisPlayer()) {
									SendCommandUnload(*Selected[0], Selected[0]->GetTilePos(), uins, flush, Selected[0]->GetMapLayer()->GetIndex());
								}
						}
						++j;
					}
				}
			}
		//Wyrmgus start
		} else if (ButtonAreaUnderCursor == ButtonAreaInventory) {
			//  for inventory unit
			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (Selected[0]->InsideCount >= ButtonUnderCursor) {
					CUnit *uins = Selected[0]->UnitInside;
					size_t j = 0;

					for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
						if (!uins->GetType()->BoolFlag[ITEM_INDEX].value || j >= UI.InventoryButtons.size() || (Selected[0]->GetPlayer() != CPlayer::GetThisPlayer() && uins->GetPlayer() != CPlayer::GetThisPlayer())) {
							continue;
						}
						if (ButtonAreaUnderCursor == ButtonAreaInventory
							&& static_cast<size_t>(ButtonUnderCursor) == j) {
								Assert(uins->GetType()->BoolFlag[ITEM_INDEX].value);
								const int flush = !(KeyModifiers & ModifierShift);
								if (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || uins->GetPlayer() == CPlayer::GetThisPlayer()) {
									if ((1 << button) == LeftButton) {
										if  (!uins->Bound) {
											SendCommandUnload(*Selected[0], Selected[0]->GetTilePos(), uins, flush, Selected[0]->GetMapLayer()->GetIndex());
										} else {
											if (Selected[0]->GetPlayer() == CPlayer::GetThisPlayer()) {
												std::string item_name = uins->GetMessageName();
												if (!uins->Unique) {
													item_name = "the " + item_name;
												}
												Selected[0]->GetPlayer()->Notify(NotifyRed, Selected[0]->GetTilePos(), Selected[0]->GetMapLayer()->GetIndex(), _("%s cannot drop %s."), Selected[0]->GetMessageName().c_str(), item_name.c_str());
											}
										}
									} else if ((1 << button) == RightButton) {
										SendCommandUse(*Selected[0], *uins, flush);
									}
								}
						}
						++j;
					}
				}
			}
		//Wyrmgus end
		} else if (ButtonAreaUnderCursor == ButtonAreaButton) {
			//Wyrmgus start
//			if (!GameObserve && !GamePaused && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !GamePaused && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
			//Wyrmgus end
				OldButtonUnderCursor = ButtonUnderCursor;
			}
		//Wyrmgus start
		} else if (ButtonAreaUnderCursor == ButtonAreaIdleWorker) {
			if (ButtonUnderCursor == 0) {
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				UiFindIdleWorker();
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit) {
			if (ButtonUnderCursor == 0) {
				PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				UiFindLevelUpUnit();
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaHeroUnit) {
			PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
			UiFindHeroUnit(ButtonUnderCursor);
		//Wyrmgus end
		}
	}
}
//Wyrmgus end

/**
**  Called if mouse button pressed down.
**
**  @param button  Button pressed down.
*/
void UIHandleButtonDown(unsigned button)
{
	// Detect long left selection click
	const bool longLeftButton = (MouseButtons & ((LeftButton << MouseHoldShift))) != 0;

#ifdef USE_TOUCHSCREEN
	// If we are moving with stylus/finger,
	// left button on touch screen devices is still clicked
	// Ignore handle if left button is long cliked
	if (longLeftButton) {
		return;
	}
#endif

	static bool OldShowSightRange;
	static bool OldShowReactionRange;
	static bool OldShowAttackRange;
	static bool OldValid = false;
	OldButtonUnderCursor = -1;

	// Reset the ShowNameDelay counters
	ShowNameDelay = ShowNameTime = GameCycle;

	if (longLeftButton) {
		if (!OldValid) {
			OldShowSightRange = Preference.ShowSightRange;
			OldShowAttackRange = Preference.ShowAttackRange;
			OldShowReactionRange = Preference.ShowReactionRange;
			OldValid = true;

			Preference.ShowSightRange = true;
			Preference.ShowAttackRange = true;
			Preference.ShowReactionRange = true;
		}
	} else if (OldValid) {
		Preference.ShowSightRange = OldShowSightRange;
		Preference.ShowAttackRange = OldShowAttackRange;
		Preference.ShowReactionRange = OldShowReactionRange;
		OldValid = false;
	}

	// select mode
	if (CursorState == CursorStateRectangle) {
		return;
	}
	// CursorOn should have changed with BigMapMode, so recompute it.
	HandleMouseOn(CursorScreenPos);
	//  Selecting target. (Move,Attack,Patrol,... commands);
	if (CursorState == CursorStateSelect) {
		UISelectStateButtonDown(button);
		return;
	}

	if (CursorState == CursorStatePieMenu) {
		if (CursorOn == CursorOnMap) {
			HandlePieMenuMouseSelection();
			return;
		} else {
			// Pie Menu canceled
			CursorState = CursorStatePoint;
			// Don't return, we might be over another button
		}
	}

	//  Cursor is on the map area
	if (CursorOn == CursorOnMap) {
		UIHandleButtonDown_OnMap(button);
	} else if (CursorOn == CursorOnMinimap) {
		//  Cursor is on the minimap area
		UIHandleButtonDown_OnMinimap(button);
	} else if (CursorOn == CursorOnButton) {
		//  Cursor is on the buttons: group or command
		UIHandleButtonDown_OnButton(button);
	}
}

/**
**  Called if mouse button released.
**
**  @param button  Button released.
*/
void UIHandleButtonUp(unsigned button)
{
	//
	//  Move map.
	//
	if (GameCursor == UI.Scroll.Cursor) {
		GameCursor = UI.Point.Cursor;
		return;
	}

	//
	//  Pie Menu
	//
	if (CursorState == CursorStatePieMenu) {
		// Little threshold
		if (1 < abs(CursorStartScreenPos.x - CursorScreenPos.x)
			|| 1 < abs(CursorStartScreenPos.y - CursorScreenPos.y)) {
			// there was a move, handle the selected button/pie
			HandlePieMenuMouseSelection();
		}
	}

	//Wyrmgus start
//	if ((1 << button) == LeftButton) {
	if (
		(1 << button) == LeftButton
		|| (
			(1 << button) == RightButton
			&& (ButtonAreaUnderCursor == ButtonAreaInventory || (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0 && Selected.size() == 1))
		)
		|| (
			(1 << button) == MiddleButton
			&& (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0 && Selected.size() == 1)
		)
	) {
	//Wyrmgus end
		//
		//  Menu (F10) button
		//
		//Wyrmgus start
//		if (GameMenuButtonClicked) {
		if (UI.MenuButton.Clicked) {
		//Wyrmgus end
			//Wyrmgus start
//			GameMenuButtonClicked = false;
			UI.MenuButton.Clicked = false;
			//Wyrmgus end
			if (ButtonAreaUnderCursor == ButtonAreaMenu) {
				if (ButtonUnderCursor == ButtonUnderMenu || ButtonUnderCursor == ButtonUnderNetworkMenu) {
					// FIXME: Not if, in input mode.
					if (!IsNetworkGame()) {
						GamePaused = true;
						//Wyrmgus start
//						UI.StatusLine.Set(_("Game Paused"));
						//Wyrmgus end
					}
					if (ButtonUnderCursor == ButtonUnderMenu) {
						if (UI.MenuButton.Callback) {
							UI.MenuButton.Callback->action("");
						}
					} else {
						//Wyrmgus start
//						if (UI.NetworkMenuButton.Callback) {
//							UI.NetworkMenuButton.Callback->action("");
//						}
						if (UI.MenuButton.Callback) {
							UI.MenuButton.Callback->action("");
						}
						//Wyrmgus end
					}
					return;
				}
			}
		}

		//
		//  Diplomacy button
		//
		//Wyrmgus start
//		if (GameDiplomacyButtonClicked) {
		if (UI.NetworkDiplomacyButton.Clicked) {
		//Wyrmgus end
			//Wyrmgus start
//			GameDiplomacyButtonClicked = false;
			UI.NetworkDiplomacyButton.Clicked = false;
			//Wyrmgus end
			if (ButtonAreaUnderCursor == ButtonAreaMenu && ButtonUnderCursor == ButtonUnderNetworkDiplomacy) {
				if (UI.NetworkDiplomacyButton.Callback) {
					UI.NetworkDiplomacyButton.Callback->action("");
				}
				return;
			}
		}
		
		//
		//  Plane buttons
		//
		for (size_t i = 0; i < UI.PlaneButtons.size(); ++i) {
			CUIButton &button = UI.PlaneButtons[i];

			if (button.Clicked) {
				button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaMapLayerPlane) {
					CMap::Map.SetCurrentPlane(CPlane::Get(i));
					if (button.Callback) {
						button.Callback->action("");
					}
					return;
				}
			}
		}

		//
		//  World buttons
		//
		for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
			CUIButton &button = UI.WorldButtons[i];

			if (button.Clicked) {
				button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaMapLayerWorld) {
					CMap::Map.SetCurrentWorld(CWorld::Get(i));
					if (button.Callback) {
						button.Callback->action("");
					}
					return;
				}
			}
		}

		//
		//  Surface layer buttons
		//
		for (size_t i = 0; i < UI.SurfaceLayerButtons.size(); ++i) {
			CUIButton &button = UI.SurfaceLayerButtons[i];

			if (button.Clicked) {
				button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaMapLayerSurfaceLayer) {
					CMap::Map.SetCurrentSurfaceLayer(i);
					if (button.Callback) {
						button.Callback->action("");
					}
					return;
				}
			}
		}

		//
		//  User buttons
		//
		for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
			CUIUserButton &button = UI.UserButtons[i];

			if (button.Clicked) {
				button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaUser) {
					if (button.Button.Callback) {
						button.Button.Callback->action("");
					}
					return;
				}
			}
		}
		
		//Wyrmgus start
//		if (!GameObserve && !GamePaused && !GameEstablishing && Selected.empty() == false && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
		if (!GameObserve && !GamePaused && !GameEstablishing && Selected.empty() == false && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer()))) {
		//Wyrmgus end
			if (OldButtonUnderCursor != -1 && OldButtonUnderCursor == ButtonUnderCursor) {
				UI.ButtonPanel.DoClicked(ButtonUnderCursor);
				OldButtonUnderCursor = -1;
				return;
			}
		}
		if (CursorOn == CursorOnButton) {
			// FIXME: other buttons?
			if (ButtonAreaUnderCursor == ButtonAreaButton && OldButtonUnderCursor != -1 && OldButtonUnderCursor == ButtonUnderCursor) {
				UI.ButtonPanel.DoClicked(ButtonUnderCursor);
				return;
			}
			//Wyrmgus start
			UIHandleButtonUp_OnButton(button);
			//Wyrmgus end
		}
	}

	// FIXME: should be completly rewritten
	// FIXME: must selecting!  (lokh: what does this mean? is this done now?)

	// SHIFT toggles select/unselect a single unit and
	// add the content of the rectangle to the selectection
	// ALT takes group of unit
	if (CursorState == CursorStateRectangle && !(MouseButtons & LeftButton)) { // leave select mode
		int num = 0;
		//
		//  Little threshold
		//
		if (CursorStartScreenPos.x < CursorScreenPos.x - 1 || CursorScreenPos.x + 1 < CursorStartScreenPos.x
			|| CursorStartScreenPos.y < CursorScreenPos.y - 1 || CursorScreenPos.y + 1 < CursorStartScreenPos.y) {
			PixelPos pos0 = CursorStartMapPos;
			const PixelPos cursorMapPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);
			PixelPos pos1 = cursorMapPos;

			if (pos0.x > pos1.x) {
				std::swap(pos0.x, pos1.x);
			}
			if (pos0.y > pos1.y) {
				std::swap(pos0.y, pos1.y);
			}
			if (KeyModifiers & ModifierShift) {
				if (KeyModifiers & ModifierAlt) {
					num = AddSelectedGroundUnitsInRectangle(pos0, pos1);
				} else if (KeyModifiers & ModifierControl) {
					num = AddSelectedAirUnitsInRectangle(pos0, pos1);
				} else {
					num = AddSelectedUnitsInRectangle(pos0, pos1);
				}
			} else {
				if (KeyModifiers & ModifierAlt) {
					num = SelectGroundUnitsInRectangle(pos0, pos1);
				} else if (KeyModifiers & ModifierControl) {
					num = SelectAirUnitsInRectangle(pos0, pos1);
				} else {
					num = SelectUnitsInRectangle(pos0, pos1);
				}
			}
#ifdef USE_TOUCHSCREEN
			// On touch screen select single unit only when long click is detected
			// This fix problem with emulating right mouse button as long left click on touch screens
		} else if (button == 0x1000001) {
#else
		} else {
#endif
			//
			// Select single unit
			//
			// cade: cannot select unit on invisible space
			// FIXME: johns: only complete invisibile units
			const Vec2i cursorTilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
			CUnit *unit = nullptr;
			if (ReplayRevealMap || UI.CurrentMapLayer->Field(cursorTilePos)->playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
				const PixelPos cursorMapPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);

				unit = UnitOnScreen(cursorMapPos.x, cursorMapPos.y);
			}
			if (unit) {
				// FIXME: Not nice coded, button number hardcoded!
				if ((KeyModifiers & ModifierControl)
					|| (button & (1 << MouseDoubleShift))) {
					if (KeyModifiers & ModifierShift) {
						num = ToggleUnitsByType(*unit);
					} else {
						num = SelectUnitsByType(*unit);
					}
				} else if ((KeyModifiers & ModifierAlt) && unit->LastGroup) {
					if (KeyModifiers & ModifierShift) {
						num = AddGroupFromUnitToSelection(*unit);
					} else {
						num = SelectGroupFromUnit(*unit);
					}

					// Don't allow to select own and enemy units.
					// Don't allow mixing buildings
				} else if (KeyModifiers & ModifierShift
						   && (unit->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*unit))
						   //Wyrmgus start
//						   && !unit->GetType()->BoolFlag[BUILDING_INDEX].value
						   && (!Selected.size() || UnitCanBeSelectedWith(*Selected[0], *unit))
						   //Wyrmgus end
						   //Wyrmgus start
//						   && (Selected.size() != 1 || !Selected[0]->GetType()->BoolFlag[BUILDING_INDEX].value)
						   //Wyrmgus end
						   && (Selected.size() != 1 || Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]))) {
					num = ToggleSelectUnit(*unit);
					if (!num) {
						SelectionChanged();
					}
				} else {
					SelectSingleUnit(*unit);
					num = 1;
				}
			} else {
				num = 0;
			}
		}

		if (num) {
			UI.StatusLine.Clear();
			UI.StatusLine.ClearCosts();
			CurrentButtonLevel = nullptr;
			SelectionChanged();

			//
			//  Play selecting sound.
			//    Buildings,
			//    This player, or neutral unit (goldmine,critter)
			//    Other clicks.
			//
			if (Selected.size() == 1) {
				if (Selected[0]->CurrentAction() == UnitActionBuilt && Selected[0]->GetPlayer()->GetIndex() == CPlayer::GetThisPlayer()->GetIndex()) {
					PlayUnitSound(*Selected[0], VoiceBuilding);
				} else if (Selected[0]->Burning) {
					// FIXME: use GameSounds.Burning
					PlayGameSound(SoundForName("burning"), MaxSampleVolume);
				} else if (Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])
						   || CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->GetPlayer())) {
					PlayUnitSound(*Selected[0], VoiceSelected);
				} else {
					PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
				}
				//Wyrmgus start
				/*
				//Wyrmgus start
//				if (Selected[0]->GetPlayer() == CPlayer::GetThisPlayer()) {
				if (Selected[0]->GetPlayer() == CPlayer::GetThisPlayer() && !Selected[0]->GetType()->BoolFlag[HERO_INDEX].value) { // don't display this for heroes
				//Wyrmgus end
					char buf[64];
					if (Selected[0]->GetPlayer()->GetUnitTypeCount(Selected[0]->GetType()) > 1) {
						snprintf(buf, sizeof(buf), _("You have ~<%d~> %ss"),
								 Selected[0]->GetPlayer()->GetUnitTypeCount(Selected[0]->GetType()),
								//Wyrmgus start
//								 Selected[0]->GetType()->Name.c_str());
								 Selected[0]->GetTypeName().c_str());
								//Wyrmgus end
					} else {
						snprintf(buf, sizeof(buf), _("You have ~<%d~> %s(s)"),
								 Selected[0]->GetPlayer()->GetUnitTypeCount(Selected[0]->GetType()),
								//Wyrmgus start
//								 Selected[0]->GetType()->Name.c_str());
								 Selected[0]->GetTypeName().c_str());
								//Wyrmgus end
					}
					UI.StatusLine.Set(buf);
				}
				*/
				//Wyrmgus end
			}
		}

		CursorStartScreenPos.x = 0;
		CursorStartScreenPos.y = 0;
		GameCursor = UI.Point.Cursor;
		CursorState = CursorStatePoint;
	}
}

/**
**  Get pie menu under the cursor
**
**  @return  Index of the pie menu under the cursor or -1 for none
*/
static int GetPieUnderCursor()
{
	int x = CursorScreenPos.x - (CursorStartScreenPos.x - ICON_SIZE_X / 2);
	int y = CursorScreenPos.y - (CursorStartScreenPos.y - ICON_SIZE_Y / 2);
	for (int i = 0; i < 9; ++i) {
		if (x > UI.PieMenu.X[i] && x < UI.PieMenu.X[i] + ICON_SIZE_X
			&& y > UI.PieMenu.Y[i] && y < UI.PieMenu.Y[i] + ICON_SIZE_Y) {
			return i;
		}
	}
	return -1; // no pie under cursor
}

/**
**  Draw Pie Menu
*/
void DrawPieMenu()
{
	char buf[2] = "?";

	if (CursorState != CursorStatePieMenu) {
		return;
	}

	if (CurrentButtons.empty()) { // no buttons
		CursorState = CursorStatePoint;
		return;
	}
	std::vector<ButtonAction> &buttons(CurrentButtons);
	CLabel label(GetGameFont());
	CViewport *vp = UI.SelectedViewport;
	PushClipping();
	vp->SetClipping();

	// Draw background
	if (UI.PieMenu.G) {
		UI.PieMenu.G->DrawFrameClip(0,
									CursorStartScreenPos.x - UI.PieMenu.G->Width / 2,
									CursorStartScreenPos.y - UI.PieMenu.G->Height / 2);
	}
	for (int i = 0; i < (int)UI.ButtonPanel.Buttons.size() && i < 9; ++i) {
		if (buttons[i].GetPos() != -1) {
			int x = CursorStartScreenPos.x - ICON_SIZE_X / 2 + UI.PieMenu.X[i];
			int y = CursorStartScreenPos.y - ICON_SIZE_Y / 2 + UI.PieMenu.Y[i];
			const PixelPos pos(x, y);

			bool gray = false;
			for (size_t j = 0; j != Selected.size(); ++j) {
				if (!IsButtonAllowed(*Selected[j], buttons[i])) {
					gray = true;
					break;
				}
			}
			// Draw icon
			if (gray) {
				buttons[i].Icon.Icon->DrawGrayscaleIcon(pos);
			} else {
				buttons[i].Icon.Icon->DrawIcon(pos, CPlayer::GetThisPlayer()->GetIndex());
			}

			// Tutorial show command key in icons
			if (UI.ButtonPanel.ShowCommandKey) {
				const char *text;

				//Wyrmgus start
//				if (buttons[i].Key == 27) {
				if (buttons[i].GetKey() == 27) {
				//Wyrmgus end
					text = "ESC";
				} else {
					//Wyrmgus start
//					buf[0] = toupper(buttons[i].Key);
					buf[0] = toupper(buttons[i].GetKey());
					//Wyrmgus end
					text = (const char *)buf;
				}
				label.DrawClip(x + 4, y + 4, text);
			}
		}
	}

	PopClipping();

	int i = GetPieUnderCursor();
	if (i != -1 && KeyState != KeyStateInput && buttons[i].GetPos() != -1) {
		if (!Preference.NoStatusLineTooltips) {
			UpdateStatusLineForButton(buttons[i]);
		}
		DrawPopup(buttons[i], CursorStartScreenPos.x + UI.PieMenu.X[i], CursorStartScreenPos.y + UI.PieMenu.Y[i]);
	}
}

/**
**  Handle pie menu mouse selection
*/
static void HandlePieMenuMouseSelection()
{
	if (CurrentButtons.empty()) {  // no buttons
		return;
	}

	int pie = GetPieUnderCursor();
	if (pie != -1) {
		const ButtonCmd action = CurrentButtons[pie].GetAction();
		UI.ButtonPanel.DoClicked(pie);
		if (action == ButtonButton) {
			// there is a submenu => stay in piemenu mode
			// and recenter the piemenu around the cursor
			CursorStartScreenPos = CursorScreenPos;
		} else {
			if (CursorState == CursorStatePieMenu) {
				CursorState = CursorStatePoint;
			}
			CursorOn = CursorOnUnknown;
			UIHandleMouseMove(CursorScreenPos); // recompute CursorOn and company
		}
	} else {
		CursorState = CursorStatePoint;
		CursorOn = CursorOnUnknown;
		UIHandleMouseMove(CursorScreenPos); // recompute CursorOn and company
	}
}
