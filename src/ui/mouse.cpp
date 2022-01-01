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

#include "stratagus.h"

#include "ui/ui.h"

#include "action/action_build.h"
#include "action/action_train.h"
#include "actions.h"
//Wyrmgus start
#include "ai/ai_local.h" //for using AiHelpers
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "engine_interface.h"
#include "game/game.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "menus.h"
#include "missile.h"
#include "network.h"
#include "player/player.h"
#include "player/player_type.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "religion/deity.h"
#include "script.h"
#include "script/condition/condition.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "sound/unit_sound_type.h"
#include "spell/spell.h"
#include "spell/spell_target_type.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

#define ICON_SIZE_X (UI.ButtonPanel.Buttons[0].Style->Width)
#define ICON_SIZE_Y (UI.ButtonPanel.Buttons[0].Style->Height)

int MouseButtons;                            /// Current pressed mouse buttons

int KeyModifiers = 0;                        /// Current keyboard modifiers

CUnit *UnitUnderCursor;                      /// Unit under cursor
int ButtonAreaUnderCursor = -1;              /// Button area under cursor
int ButtonUnderCursor = -1;                  /// Button under cursor
int OldButtonUnderCursor = -1;               /// Button under cursor
bool LeaveStops;                             /// Mouse leaves windows stops scroll

cursor_on CursorOn = cursor_on::unknown; /// Cursor on field

CUnit *GetUnitUnderCursor()
{
	return UnitUnderCursor;
}

int GetUnitUnderCursorNumber()
{
	const CUnit *unit = GetUnitUnderCursor();

	if (unit == nullptr) {
		return -1;
	}

	return UnitNumber(*unit);
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
	for (int j = 0; j < unit.Type->get_tile_height(); ++j) {
		for (int i = 0; i < unit.Type->get_tile_width(); ++i) {
			const Vec2i tempPos(i, j);
			if (!UI.CurrentMapLayer->Field(pos + tempPos)->player_info->IsTeamExplored(*CPlayer::GetThisPlayer())) {
				return false;
			}
		}
	}
	return true;
}

static void DoRightButton_ForForeignUnit(CUnit *dest, const Qt::KeyboardModifiers key_modifiers)
{
	CUnit &unit = *Selected[0];

	if (unit.Player->get_index() != PlayerNumNeutral || dest == nullptr
		|| !(dest->Player == CPlayer::GetThisPlayer() || dest->IsTeamed(*CPlayer::GetThisPlayer()))) {
		return;
	}
	// tell to go and harvest from a unit
	//Wyrmgus start
//	const int res = unit.Type->GivesResource;
	const resource *res = unit.get_given_resource();
	//Wyrmgus end

	//Wyrmgus start
//	if (res
//		&& dest->Type->BoolFlag[HARVESTER_INDEX].value
//		&& dest->Type->ResInfo[res]
//		&& dest->ResourcesHeld < dest->Type->ResInfo[res]->ResourceCapacity
//		&& unit.Type->BoolFlag[CANHARVEST_INDEX].value) {
	if (
		dest->can_harvest(&unit)
		&& dest->ResourcesHeld < dest->Type->get_resource_info(res)->ResourceCapacity
	) {
	//Wyrmgus end
		unit.Blink = 4;
		//  Right mouse with SHIFT appends command to old commands.
		const int flush = !(key_modifiers & Qt::ShiftModifier);
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
	if (dest->Type->CanTransport()) {
		// Let the transporter move to the unit. And QUEUE!!!
		if (dest->CanMove() && CanTransport(*dest, unit) && dest->Player == CPlayer::GetThisPlayer()) {
			DebugPrint("Send command follow\n");
			// is flush value correct ?
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
				acknowledged = 1;
			}
			SendCommandFollow(*dest, unit, 0);
		}
		// FIXME : manage correctly production units.
		if ((!unit.CanMove() && dest->Player == CPlayer::GetThisPlayer()) || CanTransport(*dest, unit)) {
			dest->Blink = 4;
			DebugPrint("Board transporter\n");
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
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
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
				acknowledged = 1;
			}
			SendCommandFollow(unit, *dest, 0);
		} else if (!dest->CanMove()) {
			DebugPrint("Want to transport but no unit can move\n");
			return true;
		}
		if (dest->Player == CPlayer::GetThisPlayer()) {
			dest->Blink = 4;
			DebugPrint("Board transporter\n");
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
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
	
	for (const wyrmgus::spell *spell : unit.get_autocast_spells()) {
		if (unit.CanAutoCastSpell(spell)) {
			const AutoCastInfo *autocast = spell->get_autocast_info(unit.Player->AiEnabled);

			if (!spell->CheckAutoCastGenericConditions(unit, autocast, true)) {
				continue;
			}

			if (spell->IsUnitValidAutoCastTarget(dest, unit, autocast)) {
				dest->Blink = 4;
				if (!acknowledged) {
					PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
					acknowledged = 1;
				}

				SendCommandSpellCast(unit, pos, dest, spell->Slot, flush, UI.CurrentMapLayer->ID);

				return true;
			}
		}
	}

	return false;
}

static bool DoRightButton_Harvest_Unit(CUnit &unit, CUnit &dest, int flush, int &acknowledged)
{
	// Return a loaded harvester to deposit
	if (unit.ResourcesHeld > 0
	//Wyrmgus start
//		&& dest.Type->CanStore[unit.CurrentResource]
//		&& (dest.Player == unit.Player
//			|| (dest.Player->is_allied_with(*unit.Player) && unit.Player->is_allied_with(*dest.Player)))) {
		&& unit.can_return_goods_to(&dest)) {
	//Wyrmgus end
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandReturnGoods(unit, &dest, flush);
		return true;
	}
	// Go and harvest from a unit
	//Wyrmgus start
//	const int res = dest.Type->GivesResource;
	const resource *res = dest.get_given_resource();
	//Wyrmgus end
	const wyrmgus::unit_type &type = *unit.Type;
	//Wyrmgus start
//	if (res && type.ResInfo[res] && dest.Type->BoolFlag[CANHARVEST_INDEX].value
//		&& (dest.Player == unit.Player || dest.Player->get_index() == PlayerNumNeutral)) {
	if (unit.can_harvest(&dest)) {
	//Wyrmgus end
			//Wyrmgus start
//			if (unit.ResourcesHeld < type.ResInfo[res]->ResourceCapacity) {
			if (unit.get_current_resource() != res || unit.ResourcesHeld < type.get_resource_info(res)->ResourceCapacity) {
			//Wyrmgus end
				dest.Blink = 4;
				SendCommandResource(unit, dest, flush);
				if (!acknowledged) {
					PlayUnitSound(unit, wyrmgus::unit_sound_type::harvesting);
					acknowledged = 1;
				}
				return true;
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.get_current_resource());
				if (depot) {
					dest.Blink = 4;
					if (!acknowledged) {
						PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
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
	} else if (res && type.get_resource_info(res) != nullptr && !dest.Type->BoolFlag[CANHARVEST_INDEX].value
		&& (dest.Player == unit.Player || dest.Player->get_index() == PlayerNumNeutral)) {
			//Wyrmgus start
//			if (unit.ResourcesHeld < type.ResInfo[res]->ResourceCapacity) {
			if (unit.get_current_resource() != res || unit.ResourcesHeld < type.get_resource_info(res)->ResourceCapacity) {
			//Wyrmgus end
				for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
					if (unit_type->is_template()) {
						continue;
					}

					if (unit_type->get_given_resource() != nullptr && unit_type->get_given_resource() == res && unit_type->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(&unit, *unit_type, dest.tilePos, 1, false, dest.MapLayer->ID)) {
						if (check_conditions(unit_type, unit.Player)) {
							if (wyrmgus::vector::contains(AiHelpers.get_builders(unit_type), unit.Type) || wyrmgus::vector::contains(AiHelpers.get_builder_classes(unit_type->get_unit_class()), unit.Type->get_unit_class())) {
								dest.Blink = 4;
								SendCommandBuildBuilding(unit, dest.tilePos, *unit_type, flush, dest.MapLayer->ID);
								if (!acknowledged) {
									PlayUnitSound(unit, wyrmgus::unit_sound_type::build);
									acknowledged = 1;
								}
								break;
							}
						} else if (check_conditions<true>(unit_type, unit.Player, false)) { //passes predependency check, even though didn't pass dependency check before, so give a message about the requirements
							CPlayer::GetThisPlayer()->Notify(NotifyYellow, dest.tilePos, dest.MapLayer->ID, "%s", _("The requirements have not been fulfilled"));
							break;
						}
					}
				}
				return true;
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.get_current_resource());
				if (depot) {
					dest.Blink = 4;
					if (!acknowledged) {
						PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
						acknowledged = 1;
					}
					SendCommandReturnGoods(unit, depot, flush);
					//Wyrmgus start
					for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
						if (unit_type->is_template()) {
							continue;
						}

						if (unit_type->get_given_resource() != nullptr && unit_type->get_given_resource() == res && unit_type->BoolFlag[CANHARVEST_INDEX].value && CanBuildUnitType(&unit, *unit_type, dest.tilePos, 1, false, dest.MapLayer->ID)) {
							if (check_conditions(unit_type, unit.Player)) {
								SendCommandBuildBuilding(unit, dest.tilePos, *unit_type, 0, dest.MapLayer->ID);
								break;
							} else if (check_conditions<true>(unit_type, unit.Player, false)) { //passes predependency check, even though didn't pass dependency check before, so give a message about the requirements
								CPlayer::GetThisPlayer()->Notify(NotifyYellow, dest.tilePos, dest.MapLayer->ID, "%s", _("The requirements have not been fulfilled"));
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
	if (!UI.CurrentMapLayer->Field(pos)->player_info->IsTeamExplored(*unit.Player)) {
		return false;
	}
	const wyrmgus::unit_type &type = *unit.Type;
	// FIXME: support harvesting more types of terrain.
	for (const wyrmgus::resource *resource : wyrmgus::resource::get_all()) {
		if (type.get_resource_info(resource) != nullptr
			//Wyrmgus start
//			&& type.ResInfo[res]->TerrainHarvester
			&& UI.CurrentMapLayer->Field(pos)->get_resource() == resource
			//Wyrmgus end
			//Wyrmgus start
//			&& ((unit.CurrentResource != res)
//				|| (unit.ResourcesHeld < type.ResInfo[res]->ResourceCapacity))) {
			) {
			//Wyrmgus end
			//Wyrmgus start
			/*
			SendCommandResourceLoc(unit, pos, flush);
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::harvesting);
				acknowledged = 1;
			}
			*/
			if (unit.CurrentResource != resource->get_index() || unit.ResourcesHeld < type.get_resource_info(resource)->ResourceCapacity) {
				SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->ID);
				if (!acknowledged) {
					PlayUnitSound(unit, wyrmgus::unit_sound_type::harvesting);
					acknowledged = 1;
				}
			} else {
				CUnit *depot = FindDeposit(unit, 1000, unit.get_current_resource());
				if (depot) {
					if (!acknowledged) {
						PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
						acknowledged = 1;
					}
					SendCommandReturnGoods(unit, depot, flush);
					SendCommandResourceLoc(unit, pos, 0, UI.CurrentMapLayer->ID);
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
	const wyrmgus::unit_type &type = *unit.Type;

	// Go and repair
	if (type.can_repair() && dest != nullptr
		&& dest->Type->get_repair_hp() != 0
		//Wyrmgus start
//		&& dest->Variable[HP_INDEX].Value < dest->Variable[HP_INDEX].Max
		&& dest->Variable[HP_INDEX].Value < dest->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)
		//Wyrmgus end
		&& (dest->Player == unit.Player || unit.is_allied_with(*dest))) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::repairing);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandRepair(unit, pos, dest, flush);
		SendCommandRepair(unit, pos, dest, flush, UI.CurrentMapLayer->ID);
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
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, *dest, flush);
		return true;
	}
	
	// Go through a connector
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && unit.CanUseItem(dest)) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, *dest, flush);
		return true;
	}
	
	//Wyrmgus start
	//if the clicked unit is a settlement site, build on it
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && dest->Type == settlement_site_unit_type && (dest->Player->get_index() == PlayerNumNeutral || dest->Player == unit.Player)) {
		wyrmgus::unit_type *town_hall_type = unit.Player->get_class_unit_type(wyrmgus::defines::get()->get_town_hall_class());
		if (town_hall_type != nullptr && check_conditions(town_hall_type, unit.Player) && CanBuildUnitType(&unit, *town_hall_type, dest->tilePos, 1, false, dest->MapLayer->ID)) {
			if (wyrmgus::vector::contains(AiHelpers.get_builders(town_hall_type), unit.Type) || wyrmgus::vector::contains(AiHelpers.get_builder_classes(town_hall_type->get_unit_class()), unit.Type->get_unit_class())) {
				dest->Blink = 4;
				SendCommandBuildBuilding(unit, dest->tilePos, *town_hall_type, flush, dest->MapLayer->ID);
				if (!acknowledged) {
					PlayUnitSound(unit, wyrmgus::unit_sound_type::build);
					acknowledged = 1;
				}
				return true;
			}
		}
	}
	//Wyrmgus end
	// Follow another unit
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit
		//Wyrmgus start
//		&& (dest->Player == unit.Player || unit.is_allied_with(*dest) || dest->Player->get_index() == PlayerNumNeutral)) {
		&& (dest->Player == unit.Player || unit.is_allied_with(*dest) || (dest->Player->get_index() == PlayerNumNeutral && !unit.is_enemy_of(*dest) && !dest->Type->BoolFlag[OBSTACLE_INDEX].value))) {
		//Wyrmgus end
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}

		if (dest->Type->CanMove() == false && !dest->Type->BoolFlag[TELEPORTER_INDEX].value) {
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
		} else {
			SendCommandFollow(unit, *dest, flush);
		}
		return true;
	}
	//Wyrmgus start
	// make workers attack enemy units if those are right-clicked upon
	if (UnitUnderCursor != nullptr && dest != nullptr && dest != &unit && unit.CurrentAction() != UnitAction::Built && (unit.is_enemy_of(*dest) || dest->Type->BoolFlag[OBSTACLE_INDEX].value)) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
			acknowledged = 1;
		}
		if (type.can_target(dest)) {
			//Wyrmgus start
//			SendCommandAttack(unit, pos, dest, flush);
			SendCommandAttack(unit, pos, dest, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
		} else { // No valid target
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
		}
		return true;
	}
	//Wyrmgus end
	// Move
	if (!acknowledged) {
		PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
		acknowledged = 1;
	}
	//Wyrmgus start
//	SendCommandMove(unit, pos, flush);
	SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	return true;
}

static bool DoRightButton_AttackUnit(CUnit &unit, CUnit &dest, const Vec2i &pos, int flush, int &acknowledged)
{
	const wyrmgus::unit_type &type = *unit.Type;
	const int action = type.MouseAction;

	//Wyrmgus start
//	if (action == MouseActionSpellCast || unit.is_enemy_of(dest)) {
	if (action == MouseActionSpellCast || unit.is_enemy_of(dest) || dest.Type->BoolFlag[OBSTACLE_INDEX].value) {
	//Wyrmgus end
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
			acknowledged = 1;
		}
		if (action == MouseActionSpellCast && unit.Type->Spells.size() > 0) {
			// This is for demolition squads and such
			assert_throw(unit.Type->Spells.size() > 0);
			int spellnum = type.Spells[0]->Slot;
			SendCommandSpellCast(unit, pos, &dest, spellnum, flush, UI.CurrentMapLayer->ID);
		} else {
			if (type.can_target(&dest)) {
				SendCommandAttack(unit, pos, &dest, flush, UI.CurrentMapLayer->ID);
			} else { // No valid target
				SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->ID);
			}
		}
		return true;
	}
	//Wyrmgus start
	// Pick up an item
	if (&dest != &unit && CanPickUp(unit, dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, dest, flush);
		return true;
	}
	
	// Go through a connector
	if (&dest != &unit && unit.CanUseItem(&dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, dest, flush);
		return true;
	}
	//Wyrmgus end
	if ((dest.Player == unit.Player || unit.is_allied_with(dest) || dest.Player->get_index() == PlayerNumNeutral) && &dest != &unit) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}

		if (!dest.Type->CanMove() && !dest.Type->BoolFlag[TELEPORTER_INDEX].value) {
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
		} else {
			SendCommandFollow(unit, dest, flush);
		}

		return true;
	}
	return false;
}

static void DoRightButton_Attack(CUnit &unit, CUnit *dest, const Vec2i &pos, int flush, int &acknowledged, const Qt::KeyboardModifiers key_modifiers)
{
	if (dest != nullptr && unit.CurrentAction() != UnitAction::Built) {
		if (DoRightButton_AttackUnit(unit, *dest, pos, flush, acknowledged)) {
			return;
		}
	}
	//Wyrmgus start
	/*
	if (Map.WallOnMap(pos)) {
		if (unit.Player->Race == PlayerRaceHuman && Map.OrcWallOnMap(pos)) {
			SendCommandAttack(unit, pos, NoUnitP, flush);
			return;
		}
		if (unit.Player->Race == PlayerRaceOrc && Map.HumanWallOnMap(pos)) {
			SendCommandAttack(unit, pos, NoUnitP, flush);
			return;
		}
	}
	*/
	if (CMap::get()->WallOnMap(pos, UI.CurrentMapLayer->ID) && (UI.CurrentMapLayer->Field(pos)->get_owner() == nullptr || CPlayer::GetThisPlayer()->is_enemy_of(*UI.CurrentMapLayer->Field(pos)->get_owner()))) {
		if (!UI.CurrentMapLayer->Field(pos)->get_overlay_terrain()->UnitType->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->ID);
			return;
		}
	}
	//Wyrmgus end
	// empty space
	if (key_modifiers & Qt::ControlModifier) {
		if (RightButtonAttacks) {
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
				acknowledged = 1;
			}
		} else {
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
		}
	} else {
		if (RightButtonAttacks) {
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttack(unit, pos, NoUnitP, flush);
			SendCommandAttack(unit, pos, NoUnitP, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
		} else {
			// Note: move is correct here, right default is move
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
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
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandPickUp(unit, dest, flush);
		return true;
	}
	
	// Go through a connector
	if (unit.CanUseItem(&dest)) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandUse(unit, dest, flush);
		return true;
	}
	//Wyrmgus end
	if (dest.Player == unit.Player || unit.is_allied_with(dest) || dest.Player->get_index() == PlayerNumNeutral) {
		dest.Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}

		if (dest.Type->CanMove() == false && !dest.Type->BoolFlag[TELEPORTER_INDEX].value) {
			//Wyrmgus start
//			SendCommandMove(unit, dest.tilePos, flush);
			SendCommandMove(unit, dest.tilePos, flush, UI.CurrentMapLayer->ID);
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
	// tell to return a loaded harvester to deposit
	if (dest.ResourcesHeld > 0 && dest.can_return_goods_to(&unit)
		&& dest.Player == unit.Player) {
		dest.Blink = 4;
		SendCommandReturnGoods(dest, &unit, flush);
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		return true;
	}

	// tell to go and harvest from a building
	const resource *res = unit.get_given_resource();

	if (dest.can_harvest(&unit) && dest.ResourcesHeld < dest.Type->get_resource_info(res)->ResourceCapacity && dest.Player == unit.Player) {
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
//	if (dest != nullptr && dest->Type->GivesResource && dest->Type->BoolFlag[CANHARVEST_INDEX].value
//		&& (dest->Player == unit.Player || dest->Player->get_index() == PlayerNumNeutral)) {
	if (unit.can_harvest(dest)) {
		//Wyrmgus end
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandResource(unit, *dest, flush);
		return true;
	}
	// FIXME: support harvesting more types of terrain.
	const wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);
	if (mf.player_info->IsTeamExplored(*unit.Player) && mf.get_resource() != nullptr) {
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandResourceLoc(unit, pos, flush);
		SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->ID);
		//Wyrmgus end
		return true;
	}
	return false;
}

static void DoRightButton_ForSelectedUnit(CUnit &unit, CUnit *dest, const Vec2i &pos, int &acknowledged, const Qt::KeyboardModifiers key_modifiers)
{
	// don't self targeting.
	if (dest == &unit) {
		return;
	}
	if (unit.Removed) {
		return;
	}
	const wyrmgus::unit_type &type = *unit.Type;
	const int action = type.MouseAction;
	//  Right mouse with SHIFT appends command to old commands.
	const int flush = !(key_modifiers & Qt::ShiftModifier);

	//Wyrmgus start
	if (action == MouseActionRallyPoint) {
		SendCommandRallyPoint(unit, pos, UI.CurrentMapLayer->ID);
		return;
	}
	//Wyrmgus end
	
	//  Control + alt click - ground attack
	if ((key_modifiers & Qt::ControlModifier) && (key_modifiers & Qt::AltModifier)) {
		if (unit.Type->BoolFlag[GROUNDATTACK_INDEX].value) {
			if (!acknowledged) {
				PlayUnitSound(unit, wyrmgus::unit_sound_type::attack);
				acknowledged = 1;
			}
			//Wyrmgus start
//			SendCommandAttackGround(unit, pos, flush);
			SendCommandAttackGround(unit, pos, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
			return;
		}
	}
	//  Control + right click on unit is follow anything.
	if ((key_modifiers & Qt::ControlModifier) && dest) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandFollow(unit, *dest, flush);
		return;
	}

	//  Alt + right click on unit is defend anything.
	if ((key_modifiers & Qt::AltModifier) && dest) {
		dest->Blink = 4;
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		SendCommandDefend(unit, *dest, flush);
		return;
	}

	//Wyrmgus start
	//  Ctrl + right click on an empty space moves + stand ground
	if ((key_modifiers & Qt::ControlModifier) && !dest) {
		if (!acknowledged) {
			PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
			acknowledged = 1;
		}
		//Wyrmgus start
//		SendCommandMove(unit, pos, flush);
		SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
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
	if (action == MouseActionHarvest && (!dest || !dest->Type->CanTransport() || dest->Player == CPlayer::GetThisPlayer() || dest->Player->get_type() != player_type::neutral)) { //don't harvest neutral garrisonable units (i.e. tree stumps) by right-clicking
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
		DoRightButton_Attack(unit, dest, pos, flush, acknowledged, key_modifiers);
		return;
	}

	// FIXME: attack/follow/board ...
	if (dest != nullptr && (action == MouseActionMove || action == MouseActionSail)) {
		if (DoRightButton_Follow(unit, *dest, flush, acknowledged)) {
			return;
		}
	}

	// Manage harvester from the destination side.
	if (dest != nullptr && dest->Type->BoolFlag[HARVESTER_INDEX].value) {
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
		PlayUnitSound(unit, wyrmgus::unit_sound_type::acknowledging);
		acknowledged = 1;
	}
	//Wyrmgus start
//	SendCommandMove(unit, pos, flush);
	SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
	//Wyrmgus end
}

/**
**  Called when right button is pressed
**
**  @param mapPixelPos  map position in pixels.
*/
static void DoRightButton(const PixelPos &mapPixelPos, const Qt::KeyboardModifiers key_modifiers)
{
	// No unit selected
	if (Selected.empty()) {
		return;
	}
	const Vec2i pos = CMap::get()->scaled_map_pixel_pos_to_tile_pos(mapPixelPos);
	CUnit *dest;            // unit under the cursor if any.

	if (UnitUnderCursor != nullptr && !UnitUnderCursor->Type->BoolFlag[DECORATION_INDEX].value) {
		dest = UnitUnderCursor;
	} else {
		dest = nullptr;
	}

	//  Unit selected isn't owned by the player.
	//  You can't select your own units + foreign unit(s)
	//  except if it is neutral and it is a resource.
	if (!CanSelectMultipleUnits(*Selected[0]->Player)) {
		DoRightButton_ForForeignUnit(dest, key_modifiers);
		return;
	}

	if (dest != nullptr && dest->Type->CanTransport()) {
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
		assert_throw(Selected[i] != nullptr);
		CUnit &unit = *Selected[i];

		DoRightButton_ForSelectedUnit(unit, dest, pos, acknowledged, key_modifiers);
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
	assert_throw(this->Style != nullptr);

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

	//  Handle buttons

	for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
		const CUIButton &button = UI.WorldButtons[i];

		if (button.X != -1) {
			if (button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaMapLayerWorld;
				ButtonUnderCursor = i;
				CursorOn = cursor_on::button;
				return;
			}
		}
	}

	for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
		const CUIUserButton &user_button = UI.UserButtons[i];

		if (user_button.Button.X != -1) {
			if (user_button.Button.Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaUser;
				ButtonUnderCursor = i;
				CursorOn = cursor_on::button;
				return;
			}
		}

	}
	const size_t buttonCount = UI.ButtonPanel.Buttons.size();
	for (unsigned int j = 0; j < buttonCount; ++j) {
		if (UI.ButtonPanel.Buttons[j].Contains(screenPos)) {
			ButtonAreaUnderCursor = ButtonAreaButton;
			if (!CurrentButtons.empty() && CurrentButtons[j]->get_pos() != -1) {
				ButtonUnderCursor = j;
				CursorOn = cursor_on::button;
				return;
			}
		}
	}
	if (!Selected.empty()) {
		if (Selected.size() == 1 && Selected[0]->Type->CanTransport() && Selected[0]->BoardCount && CurrentButtonLevel == Selected[0]->Type->ButtonLevelForTransporter) {
			const size_t size = UI.TransportingButtons.size();

			for (size_t i = std::min<size_t>(Selected[0]->BoardCount, size); i != 0;) {
				--i;
				if (UI.TransportingButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaTransporting;
					ButtonUnderCursor = i;
					CursorOn = cursor_on::button;
					return;
				}
			}
		}
		//Wyrmgus start
		if (Selected.size() == 1 && Selected[0]->HasInventory() && CurrentButtonLevel == wyrmgus::defines::get()->get_inventory_button_level()) {
			const size_t size = UI.InventoryButtons.size();

			for (size_t i = std::min<size_t>(Selected[0]->InsideCount, size); i != 0;) {
				--i;
				if (UI.InventoryButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaInventory;
					ButtonUnderCursor = i;
					CursorOn = cursor_on::button;
					return;
				}
			}
		}
		//Wyrmgus end
		if (Selected.size() == 1) {
			if (Selected[0]->CurrentAction() == UnitAction::Train) {
				if (Selected[0]->Orders.size() == 1) {
					if (UI.SingleTrainingButton->Contains(screenPos)) {
						ButtonAreaUnderCursor = ButtonAreaTraining;
						ButtonUnderCursor = 0;
						CursorOn = cursor_on::button;
						return;
					}
				} else {
					const size_t size = UI.TrainingButtons.size();
					size_t j = 0;
					for (size_t i = 0; i < Selected[0]->Orders.size() && j < size; ++i) {
						if (Selected[0]->Orders[i]->Action == UnitAction::Train) {
							const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[i].get());
							if (i > 0 && j > 0 && Selected[0]->Orders[i - 1]->Action == UnitAction::Train) {
								const COrder_Train &previous_order = *static_cast<COrder_Train *>(Selected[0]->Orders[i - 1].get());
								if (previous_order.GetUnitType().Slot == order.GetUnitType().Slot) {
									continue;
								}
							}
							if (Selected[0]->Orders[i]->Action == UnitAction::Train
								&& UI.TrainingButtons[j].Contains(screenPos)) {
								ButtonAreaUnderCursor = ButtonAreaTraining;
								ButtonUnderCursor = j;
								CursorOn = cursor_on::button;
								return;
							}
							++j;
						}
					}
				}
			} else if (Selected[0]->CurrentAction() == UnitAction::UpgradeTo) {
				if (UI.UpgradingButton->Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaUpgrading;
					ButtonUnderCursor = 0;
					CursorOn = cursor_on::button;
					return;
				}
			} else if (Selected[0]->CurrentAction() == UnitAction::Research) {
				if (UI.ResearchingButton->Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaResearching;
					ButtonUnderCursor = 0;
					CursorOn = cursor_on::button;
					return;
				}
			}
		}
		if (Selected.size() == 1) {
			if (UI.SingleSelectedButton && UI.SingleSelectedButton->Contains(screenPos)) {
				ButtonAreaUnderCursor = ButtonAreaSelected;
				ButtonUnderCursor = 0;
				CursorOn = cursor_on::button;
				return;
			}
		} else {
			const size_t size = UI.SelectedButtons.size();

			for (size_t i = std::min(Selected.size(), size); i != 0;) {
				--i;
				if (UI.SelectedButtons[i].Contains(screenPos)) {
					ButtonAreaUnderCursor = ButtonAreaSelected;
					ButtonUnderCursor = i;
					CursorOn = cursor_on::button;
					return;
				}
			}
		}
	}

	//Wyrmgus start
	if (UI.IdleWorkerButton && UI.IdleWorkerButton->Contains(screenPos) && !CPlayer::GetThisPlayer()->FreeWorkers.empty()) {
		ButtonAreaUnderCursor = ButtonAreaIdleWorker;
		ButtonUnderCursor = 0;
		CursorOn = cursor_on::button;
		return;
	}
	
	if (UI.LevelUpUnitButton && UI.LevelUpUnitButton->Contains(screenPos) && !CPlayer::GetThisPlayer()->LevelUpUnits.empty()) {
		ButtonAreaUnderCursor = ButtonAreaLevelUpUnit;
		ButtonUnderCursor = 0;
		CursorOn = cursor_on::button;
		return;
	}
	
	for (size_t i = 0; i < UI.HeroUnitButtons.size() && i < CPlayer::GetThisPlayer()->Heroes.size(); ++i) {
		if (UI.HeroUnitButtons[i].Contains(screenPos)) {
			ButtonAreaUnderCursor = ButtonAreaHeroUnit;
			ButtonUnderCursor = i;
			CursorOn = cursor_on::button;
			return;
		}
	}
	//Wyrmgus end
	
	//  Minimap
	if (UI.get_minimap()->Contains(screenPos)) {
		CursorOn = cursor_on::minimap;
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
	if (!on_ui && UI.MapArea.contains(screenPos)) {
		CViewport *vp = GetViewport(screenPos);

		if (vp == nullptr) {
			log::log_error("Screen position " + point::to_string(screenPos) + " is contained in the map area, but has no viewport.");
		}

		// viewport changed
		if (UI.MouseViewport != vp) {
			UI.MouseViewport = vp;
			DebugPrint("current viewport changed to %ld.\n" _C_
					   static_cast<long int>(vp - UI.Viewports));
		}

		// Note cursor on map can be in scroll area
		CursorOn = cursor_on::map;
	} else {
		CursorOn = cursor_on::unknown;
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
	CursorOn = cursor_on::unknown;

	// Prevent scrolling while out of focus (on other applications) */
	KeyScrollState = MouseScrollState = ScrollNone;

	// Show hour-glass (to denote to the user, the game is waiting)
	// FIXME: couldn't define a hour-glass that easily, so used pointer
	CursorScreenPos.x = Video.Width / 2;
	CursorScreenPos.y = Video.Height / 2;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
}

/**
**  Restrict mouse cursor to viewport.
*/
void RestrictCursorToViewport()
{
	UI.SelectedViewport->Restrict(CursorScreenPos.x, CursorScreenPos.y);
	UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
	CursorOn = cursor_on::map;
}

/**
**  Restrict mouse cursor to minimap
*/
void RestrictCursorToMinimap()
{
	CursorScreenPos.x = std::clamp(CursorScreenPos.x, UI.get_minimap()->X, UI.get_minimap()->X + UI.get_minimap()->W - 1);
	CursorScreenPos.y = std::clamp(CursorScreenPos.y, UI.get_minimap()->Y, UI.get_minimap()->Y + UI.get_minimap()->H - 1);

	UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
	CursorOn = cursor_on::minimap;
}

/**
**  Use the mouse to scroll the map
**
**  @param pos  Screen position.
*/
void MouseScrollMap(const PixelPos &pos, const Qt::KeyboardModifiers key_modifiers)
{
	if (pos == CursorStartScreenPos) {
		return;
	}

	const QPoint scroll_start_pos = cursor::get_last_scroll_pos() != QPoint(-1, -1) ? cursor::get_last_scroll_pos() : QPoint(CursorStartScreenPos);

	int speed = (key_modifiers & Qt::ControlModifier) ? UI.MouseScrollSpeedControl : UI.MouseScrollSpeedDefault;

	if (preferences::get()->is_reverse_mousewheel_scrolling_enabled()) {
		speed *= -1;
	}

	const PixelDiff diff(pos - scroll_start_pos);

	UI.MouseViewport->Set(UI.MouseViewport->MapPos, UI.MouseViewport->Offset + speed * diff);

	UI.MouseWarpPos = CursorStartScreenPos;
	cursor::set_last_scroll_pos(pos);
}

static void handle_mouse_move_on_map(const PixelPos &cursor_pos, const Qt::KeyboardModifiers key_modifiers)
{
	if (UI.MouseViewport == nullptr) {
		log::log_error("Mouse viewport pointer is null.");
		return;
	}

	if (!UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
		return;
	}

	const CViewport &vp = *UI.MouseViewport;
	const Vec2i tilePos = vp.ScreenToTilePos(cursor_pos);

	try {
		if (CursorBuilding && (MouseButtons & LeftButton) && Selected.at(0)
			&& (key_modifiers & (Qt::AltModifier | Qt::ShiftModifier))) {
			const CUnit &unit = *Selected[0];
			const Vec2i cursor_tile_pos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
			bool explored = CanBuildOnArea(*Selected[0], cursor_tile_pos);

			// We now need to check if there are another build commands on this build spot
			bool buildable = true;
			for (const std::unique_ptr<COrder> &order : unit.Orders) {
				if (order->Action == UnitAction::Build) {
					const COrder_Build *build = dynamic_cast<const COrder_Build *>(order.get());
					if (cursor_tile_pos.x >= build->GetGoalPos().x
						&& cursor_tile_pos.x < build->GetGoalPos().x + build->GetUnitType().get_tile_width()
						&& cursor_tile_pos.y >= build->GetGoalPos().y
						&& cursor_tile_pos.y < build->GetGoalPos().y + build->GetUnitType().get_tile_height()) {
						buildable = false;
						break;
					}
				}
			}

			// 0 Test build, don't really build
			//Wyrmgus start
//				if (CanBuildUnitType(Selected[0], *CursorBuilding, cursor_tile_pos, 0) && buildable && (explored || ReplayRevealMap)) {
			if (CanBuildUnitType(Selected[0], *CursorBuilding, cursor_tile_pos, 0, false, UI.CurrentMapLayer->ID) && buildable && (explored || ReplayRevealMap)) {
				//Wyrmgus end
				const int flush = !(key_modifiers & Qt::ShiftModifier);
				for (size_t i = 0; i != Selected.size(); ++i) {
					//Wyrmgus start
//						SendCommandBuildBuilding(*Selected[i], cursor_tile_pos, *CursorBuilding, flush);
					SendCommandBuildBuilding(*Selected[i], cursor_tile_pos, *CursorBuilding, flush, UI.CurrentMapLayer->ID);
					//Wyrmgus end
				}
				if (!(key_modifiers & (Qt::AltModifier | Qt::ShiftModifier))) {
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
		const wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(tilePos);
		for (int i = 0; i < PlayerMax; ++i) {
			if (mf.player_info->IsTeamExplored(*CPlayer::Players[i])
				&& (i == CPlayer::GetThisPlayer()->get_index() || CPlayer::Players[i]->has_mutual_shared_vision_with(CPlayer::GetThisPlayer()) || CPlayer::Players[i]->is_revealed())) {
				show = true;
				break;
			}
		}
	}

	if (show) {
		const PixelPos mapPixelPos = vp.screen_to_scaled_map_pixel_pos(cursor_pos);
		UnitUnderCursor = UnitOnScreen(mapPixelPos.x, mapPixelPos.y);
	}

	//Wyrmgus start
	if (show && Selected.size() >= 1 && Selected[0]->Player == CPlayer::GetThisPlayer()) {
		bool has_terrain_resource = false;
		const CViewport &cursor_vp = *UI.MouseViewport;
		const Vec2i cursor_tile_pos = cursor_vp.ScreenToTilePos(cursor_pos);
		for (const wyrmgus::resource *resource : wyrmgus::resource::get_all()) {
			if (Selected[0]->Type->get_resource_info(resource) != nullptr
				//Wyrmgus start
//					&& Selected[0]->Type->ResInfo[res]->TerrainHarvester
					//Wyrmgus end
				&& UI.CurrentMapLayer->Field(cursor_tile_pos)->get_resource() == resource
				) {
				has_terrain_resource = true;
			}
		}
		if (has_terrain_resource) {
			cursor::set_current_cursor(UI.get_cursor(cursor_type::yellow_hair), false);
		}
	}
	//Wyrmgus end
}

static void handle_mouse_move_on_minimap(const PixelPos &cursor_pos)
{
	const Vec2i tile_pos = UI.get_minimap()->screen_to_tile_pos(cursor_pos);

	if (UI.get_minimap()->are_units_visible()) {
		if (UI.CurrentMapLayer->Field(tile_pos)->player_info->IsTeamExplored(*CPlayer::GetThisPlayer()) || ReplayRevealMap) {
			UnitUnderCursor = UnitOnMapTile(tile_pos, unit_domain::none, UI.CurrentMapLayer->ID);
		}
	}
}

/**
**  Handle movement of the cursor.
**
**  @param cursorPos  Screen X position.
*/
void UIHandleMouseMove(const PixelPos &cursorPos, const Qt::KeyboardModifiers key_modifiers)
{
	cursor_on OldCursorOn;

	OldCursorOn = CursorOn;

	UI.reset_tooltip_cycle_count();

	//  Selecting units.
	if (CurrentCursorState == CursorState::Rectangle) {
		// Restrict cursor to viewport.
		UI.SelectedViewport->Restrict(CursorScreenPos.x, CursorScreenPos.y);
		UI.MouseWarpPos = CursorScreenPos;
		return;
	}

	//  Move map.
	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		MouseScrollMap(cursorPos, key_modifiers);
		return;
	}

	UnitUnderCursor = nullptr;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);  // Reset
	HandleMouseOn(cursorPos);

	// Restrict mouse to minimap when dragging
	if (OldCursorOn == cursor_on::minimap && CursorOn != cursor_on::minimap && (MouseButtons & LeftButton)) {
		const Vec2i cursor_tile_pos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);

		RestrictCursorToMinimap();
		UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(cursor_tile_pos));
		return;
	}

	//  User may be dragging with button pressed.
	for (const CUIButton &button : UI.WorldButtons) {
		if (button.Clicked) {
			return;
		}
	}
	for (const CUIUserButton &button : UI.UserButtons) {
		if (button.Clicked) {
			return;
		}
	}

	// This is forbidden for unexplored and not visible space
	// FIXME: This must done new, moving units, scrolling...
	switch (CursorOn) {
		case cursor_on::map:
			handle_mouse_move_on_map(cursorPos, key_modifiers);
			break;
		case cursor_on::minimap:
			handle_mouse_move_on_minimap(cursorPos);
			break;
		default:
			break;
	}

	// NOTE: If unit is not selectable as a goal, you can't get a cursor hint
	if (UnitUnderCursor != nullptr && !UnitUnderCursor->IsVisibleAsGoal(*CPlayer::GetThisPlayer()) &&
		!ReplayRevealMap) {
		UnitUnderCursor = nullptr;
	}

	//  Selecting target.
	if (CurrentCursorState == CursorState::Select) {
		if (CursorOn == cursor_on::map || CursorOn == cursor_on::minimap) {
			cursor::set_current_cursor(UI.get_cursor(cursor_type::yellow_hair), false);
			if (UnitUnderCursor != nullptr && !UnitUnderCursor->Type->BoolFlag[DECORATION_INDEX].value) {
				if (UnitUnderCursor->Player == CPlayer::GetThisPlayer() ||
					CPlayer::GetThisPlayer()->is_allied_with(*UnitUnderCursor)) {
					cursor::set_current_cursor(UI.get_cursor(cursor_type::green_hair), false);
				//Wyrmgus start
//				} else if (UnitUnderCursor->Player->get_index() != PlayerNumNeutral) {
				} else if (CPlayer::GetThisPlayer()->is_enemy_of(*UnitUnderCursor) || UnitUnderCursor->Type->BoolFlag[OBSTACLE_INDEX].value) {
				//Wyrmgus end
					cursor::set_current_cursor(UI.get_cursor(cursor_type::red_hair), false);
				}
			}
			if (CursorOn == cursor_on::minimap && (MouseButtons & RightButton)) {
				const Vec2i cursor_tile_pos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);
				//  Minimap move viewpoint
				UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(cursor_tile_pos));
			}
		}
		// FIXME: must move minimap if right button is down !
		return;
	}

	//  Cursor pointing.
	if (CursorOn == cursor_on::map) {
		//  Map
		if (UnitUnderCursor != nullptr && !UnitUnderCursor->Type->BoolFlag[DECORATION_INDEX].value
			&& (UnitUnderCursor->IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap)) {
			//Wyrmgus start
//			cursor::set_current_cursor(UI.Glass.Cursor);
			if (
				Selected.size() >= 1 && Selected[0]->Player == CPlayer::GetThisPlayer() && UnitUnderCursor->Player != CPlayer::GetThisPlayer()
				&& (Selected[0]->is_enemy_of(*UnitUnderCursor) || UnitUnderCursor->Type->BoolFlag[OBSTACLE_INDEX].value)
			) {
				cursor::set_current_cursor(UI.get_cursor(cursor_type::red_hair), false);
			} else if (
				Selected.size() >= 1 && Selected[0]->Player == CPlayer::GetThisPlayer() &&
				(
					Selected[0]->can_harvest(UnitUnderCursor, false)
					&& (!Selected[0]->CurrentResource || !UnitUnderCursor->Type->can_store(Selected[0]->get_current_resource()) || (Selected[0]->CurrentResource == TradeCost && UnitUnderCursor->Player != CPlayer::GetThisPlayer()))
				)
			) {
				cursor::set_current_cursor(UI.get_cursor(cursor_type::yellow_hair), false);
			} else {
				cursor::set_current_cursor(UI.get_cursor(cursor_type::magnifying_glass), false);
			}
			//Wyrmgus end
		}
		return;
	}

	if (CursorOn == cursor_on::minimap && (MouseButtons & LeftButton)) {
		//  Minimap move viewpoint
		const Vec2i tile_pos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);

		UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(tile_pos));

		//if clicking the minimap made the hovered tile change (e.g. because the minimap is in zoomed mode), then set the cursor's position to that of the old tile pos
		if (tile_pos != UI.get_minimap()->screen_to_tile_pos(CursorScreenPos)) {
			CursorScreenPos = UI.get_minimap()->tile_to_screen_pos(tile_pos);
			UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
		} else {
			CursorStartScreenPos = CursorScreenPos;
		}

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
	if (dest && dest->Variable[HP_INDEX].Value < dest->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)
	//Wyrmgus end
		&& dest->Type->get_repair_hp() != 0
		&& (dest->Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->is_allied_with(*dest))) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			if (unit->can_repair()) {
				SendCommandRepair(*unit, tilePos, dest, flush, UI.CurrentMapLayer->ID);
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
static int SendMove(const Vec2i &tilePos, int flush, const Qt::KeyboardModifiers key_modifiers)
//Wyrmgus end
{
	CUnit *goal = UnitUnderCursor;
	int ret = 0;

	// Alt makes unit to defend goal
	if (goal && (key_modifiers & Qt::AltModifier)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			goal->Blink = 4;
			SendCommandDefend(*unit, *goal, flush);
			ret = 1;
		}
	//Wyrmgus start
	//  Ctrl + click on an empty space moves + stand ground
	} else if (!goal && (key_modifiers & Qt::ControlModifier)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit *unit = Selected[i];

			//Wyrmgus start
//			SendCommandMove(*unit, tilePos, flush);
			SendCommandMove(*unit, tilePos, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
			SendCommandStandGround(*unit, 0);
			ret = 1;
		}
	//Wyrmgus end
	} else {
		// Move to a transporter.
		if (goal && goal->Type->CanTransport()) {
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
				SendCommandMove(*unit, tilePos, flush, UI.CurrentMapLayer->ID);
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
	CUnit *dest = UnitUnderCursor;
	int ret = 0;

	if (dest && dest->Type->BoolFlag[DECORATION_INDEX].value) {
		dest = nullptr;
	}
	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];

		//Wyrmgus start
//		if (unit.Type->CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			if (!dest || (dest != &unit && unit.Type->can_target(dest))) {
				if (dest) {
					dest->Blink = 4;
				}
				//Wyrmgus start
//				SendCommandAttack(unit, tilePos, dest, flush);
				SendCommandAttack(unit, tilePos, dest, flush, UI.CurrentMapLayer->ID);
				//Wyrmgus end
				ret = 1;
			}
		} else {
			if (unit.CanMove()) {
				//Wyrmgus start
//				SendCommandMove(unit, tilePos, flush);
				SendCommandMove(unit, tilePos, flush, UI.CurrentMapLayer->ID);
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
	int ret = 0;

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];
		//Wyrmgus start
//		if (unit.Type->CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			//Wyrmgus start
//			SendCommandAttackGround(unit, tilePos, flush);
			SendCommandAttackGround(unit, tilePos, flush, UI.CurrentMapLayer->ID);
			//Wyrmgus end
			ret = 1;
		} else {
			//Wyrmgus start
//			SendCommandMove(unit, tilePos, flush);
			SendCommandMove(unit, tilePos, flush, UI.CurrentMapLayer->ID);
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
	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];
		//Wyrmgus start
//		SendCommandPatrol(unit, tilePos, flush);
		SendCommandPatrol(unit, tilePos, flush, UI.CurrentMapLayer->ID);
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
	CUnit *dest = UnitUnderCursor;
	int ret = 0;
	const wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);

	for (size_t i = 0; i != Selected.size(); ++i) {
		CUnit &unit = *Selected[i];

		if (unit.Type->BoolFlag[HARVESTER_INDEX].value) {
			if (dest
				//Wyrmgus start
//				&& (res = dest->Type->GivesResource) != 0
				&& dest->get_given_resource() != nullptr
				//Wyrmgus end
				//Wyrmgus start
//				&& unit.Type->ResInfo[res]
				&& unit.can_harvest(dest)
				//Wyrmgus end
				&& unit.ResourcesHeld < unit.Type->get_resource_info(dest->get_given_resource())->ResourceCapacity
				//Wyrmgus start
//				&& dest->Type->BoolFlag[CANHARVEST_INDEX].value
//				&& (dest->Player == unit.Player || dest->Player->get_index() == PlayerMax - 1)) {
				) {
				//Wyrmgus end
				dest->Blink = 4;
				SendCommandResource(unit, *dest, flush);
				ret = 1;
				continue;
			} else {
				bool resource_found = false;
				for (const resource *resource : resource::get_all()) {
					if (unit.Type->get_resource_info(resource) != nullptr
						//Wyrmgus start
//						&& unit.Type->ResInfo[res]->TerrainHarvester
//						&& mf.player_info->is_explored(*unit.Player)
						&& mf.player_info->IsTeamExplored(*unit.Player)
						//Wyrmgus end
						&& mf.get_resource() == resource
						&& unit.ResourcesHeld < unit.Type->get_resource_info(resource)->ResourceCapacity
						&& (unit.get_current_resource() != resource || unit.ResourcesHeld < unit.Type->get_resource_info(resource)->ResourceCapacity)) {
						//Wyrmgus start
//						SendCommandResourceLoc(unit, pos, flush);
						SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->ID);
						//Wyrmgus end
						ret = 1;
						resource_found = true;
						break;
					}
				}
				if (resource_found) {
					continue;
				}
			}
		}
		if (!unit.CanMove()) {
			//Wyrmgus start
//			if (dest && dest->Type->GivesResource && dest->Type->BoolFlag[CANHARVEST_INDEX].value) {
			if (dest && dest->GivesResource && dest->Type->BoolFlag[CANHARVEST_INDEX].value) {
			//Wyrmgus end
				dest->Blink = 4;
				SendCommandResource(unit, *dest, flush);
				ret = 1;
				continue;
			}
			if (mf.player_info->IsTeamExplored(*unit.Player) && mf.get_resource() != nullptr) {
				//Wyrmgus start
//				SendCommandResourceLoc(unit, pos, flush);
				SendCommandResourceLoc(unit, pos, flush, UI.CurrentMapLayer->ID);
				//Wyrmgus end
				ret = 1;
				continue;
			}
			//Wyrmgus start
//			SendCommandMove(unit, pos, flush);
			SendCommandMove(unit, pos, flush, UI.CurrentMapLayer->ID);
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
	for (size_t i = 0; i != Selected.size(); ++i) {
		// FIXME: not only transporter selected?
		//Wyrmgus start
//		SendCommandUnload(*Selected[i], tilePos, NoUnitP, flush);
		SendCommandUnload(*Selected[i], tilePos, NoUnitP, flush, UI.CurrentMapLayer->ID);
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
		if (unit.Type->Spells.size() == 0) {
			DebugPrint("but unit %d(%s) can't cast spells?\n" _C_
					   UnitNumber(unit) _C_ unit.Type->get_name().c_str());
			// this unit cannot cast spell
			continue;
		}
		if (dest && &unit == dest) {
			// no unit can cast spell on himself
			// n0b0dy: why not?
			continue;
		}
		// CursorValue here holds the spell type id
		const wyrmgus::spell *spell = wyrmgus::spell::get_all().at(CursorValue);
		if (!spell) {
			throw std::runtime_error("Unknown spell-id: " + std::to_string(CursorValue));
		}
		
		if (std::find(unit.Type->Spells.begin(), unit.Type->Spells.end(), spell) == unit.Type->Spells.end()) {
			continue; //the unit's type cannot cast this spell
		}
		
		if (!unit.CanCastSpell(spell, false)) {
			continue;
		}
		
		SendCommandSpellCast(unit, tilePos, spell->get_target() == wyrmgus::spell_target_type::position ? nullptr : dest , CursorValue, flush, UI.CurrentMapLayer->ID);
		ret = 1;
		
		//if the spell's effects do not stack on the same target, then only one selected unit should cast it
		if (!spell->is_stackable()) {
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

		SendCommandRallyPoint(*unit, tilePos, UI.CurrentMapLayer->ID);
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
static void SendCommand(const Vec2i &tilePos, const Qt::KeyboardModifiers key_modifiers)
{
	int ret = 0;

	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
	
	const int flush = !(key_modifiers & Qt::ShiftModifier);
	
	switch (CursorAction) {
		case ButtonCmd::Move:
			//Wyrmgus start
//			ret = SendMove(tilePos);
			ret = SendMove(tilePos, flush, key_modifiers);
			//Wyrmgus end
			break;
		case ButtonCmd::Repair:
			//Wyrmgus start
//			ret = SendRepair(tilePos);
			ret = SendRepair(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::Attack:
			//Wyrmgus start
//			ret = SendAttack(tilePos);
			ret = SendAttack(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::AttackGround:
			//Wyrmgus start
//			ret = SendAttackGround(tilePos);
			ret = SendAttackGround(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::Patrol:
			//Wyrmgus start
//			ret = SendPatrol(tilePos);
			ret = SendPatrol(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::Harvest:
			//Wyrmgus start
//			ret = SendResource(tilePos);
			ret = SendResource(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::Unload:
			//Wyrmgus start
//			ret = SendUnload(tilePos);
			ret = SendUnload(tilePos, flush);
			//Wyrmgus end
			break;
		case ButtonCmd::SpellCast:
			//Wyrmgus start
//			ret = SendSpellCast(tilePos);
			ret = SendSpellCast(tilePos, flush);
			//Wyrmgus end
			break;
		//Wyrmgus start
		case ButtonCmd::RallyPoint:
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
//			if (CursorAction == ButtonCmd::Attack || CursorAction == ButtonCmd::AttackGround || CursorAction == ButtonCmd::SpellCast) {
			if (CursorAction == ButtonCmd::Attack || CursorAction == ButtonCmd::AttackGround) {
			//Wyrmgus end
				//Wyrmgus start
				/*
				if (Selected[i]->Type->MapSound->Attack.Sound) {
					PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::attack);
					break;
				} else if (Selected[i]->Type->MapSound->Acknowledgement.Sound) {
					PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::acknowledging);
					break;
				}
				*/
				PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::attack);
				break;
				//Wyrmgus end
			} else if (CursorAction == ButtonCmd::Repair) {
				PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::repairing);
				break;
			} else if (CursorAction == ButtonCmd::Build || CursorAction == ButtonCmd::BuildClass) {
				PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::build);
				break;
			} else {
				PlayUnitSound(*Selected[i], wyrmgus::unit_sound_type::acknowledging);
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
static void DoSelectionButtons(int num, unsigned, const Qt::KeyboardModifiers key_modifiers)
{
	if (GameObserve || game::get()->is_paused() || GameEstablishing) {
		return;
	}

	//Wyrmgus start
//	if (static_cast<size_t>(num) >= Selected.size() || !(MouseButtons & LeftButton)) {
	if (static_cast<size_t>(num) >= Selected.size()) {
	//Wyrmgus end
		return;
	}

	CUnit &unit = *Selected[num];

	if ((key_modifiers & Qt::ControlModifier) || (MouseButtons & (LeftButton << MouseDoubleShift))) {
		if (key_modifiers & Qt::ShiftModifier) {
			ToggleUnitsByType(unit);
		} else {
			SelectUnitsByType(unit);
		}
	} else if (key_modifiers & Qt::AltModifier) {
		if (key_modifiers & Qt::ShiftModifier) {
			AddGroupFromUnitToSelection(unit);
		} else {
			SelectGroupFromUnit(unit);
		}
	} else if (key_modifiers & Qt::ShiftModifier) {
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
static void UISelectStateButtonDown(unsigned, const Qt::KeyboardModifiers key_modifiers)
{
	if (GameObserve || game::get()->is_paused() || GameEstablishing) {
		return;
	}

	//
	//  Clicking on the map.
	//
	if (CursorOn == cursor_on::map && UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
		CurrentCursorState = CursorState::Point;
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
		CustomCursor.clear();
		CurrentButtonLevel = nullptr;
		UI.ButtonPanel.Update();

		if (MouseButtons & LeftButton) {
			const CViewport &vp = *UI.MouseViewport;
			const PixelPos mapPixelPos = vp.screen_to_map_pixel_pos(CursorScreenPos);

			if (!ClickMissile.empty()) {
				MakeLocalMissile(*wyrmgus::missile_type::get(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->ID);
			}
			SendCommand(CMap::get()->map_pixel_pos_to_tile_pos(mapPixelPos), key_modifiers);
		}
		return;
	}

	//
	//  Clicking on the minimap.
	//
	if (CursorOn == cursor_on::minimap) {
		const Vec2i cursorTilePos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);

		if (MouseButtons & LeftButton) {
			const PixelPos mapPixelPos = CMap::get()->tile_pos_to_map_pixel_pos_center(cursorTilePos);

			UI.StatusLine.Clear();
			UI.StatusLine.ClearCosts();
			CurrentCursorState = CursorState::Point;
			cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
			CustomCursor.clear();
			CurrentButtonLevel = nullptr;
			UI.ButtonPanel.Update();
			if (!ClickMissile.empty()) {
				MakeLocalMissile(*wyrmgus::missile_type::get(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->ID);
			}
			SendCommand(cursorTilePos, key_modifiers);
		} else {
			UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(cursorTilePos));
		}
		return;
	}

	if (CursorOn == cursor_on::button) {
		// FIXME: other buttons?
		if (ButtonAreaUnderCursor == ButtonAreaButton) {
			OldButtonUnderCursor = ButtonUnderCursor;
			return;
		}
	}

	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentCursorState = CursorState::Point;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::yellow_hair), false);
	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
}

static void UIHandleButtonDown_OnMap(const Qt::KeyboardModifiers key_modifiers)
{
	assert_throw(UI.MouseViewport != nullptr);
	
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
			if (CanBuildUnitType(Selected[0], *CursorBuilding, tilePos, 0, false, UI.CurrentMapLayer->ID) && (explored || ReplayRevealMap)) {
			//Wyrmgus end
				const int flush = !(key_modifiers & Qt::ShiftModifier);
				PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_success_sound(), MaxSampleVolume);
				PlayUnitSound(*Selected[0], wyrmgus::unit_sound_type::build);
				for (size_t i = 0; i != Selected.size(); ++i) {
					//Wyrmgus start
//					SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush);
					SendCommandBuildBuilding(*Selected[i], tilePos, *CursorBuilding, flush, UI.CurrentMapLayer->ID);
					//Wyrmgus end
				}
				if (!(key_modifiers & (Qt::AltModifier | Qt::ShiftModifier))) {
					CancelBuildingMode();
				}
			} else {
				if (UI.CurrentMapLayer->ID != CPlayer::GetThisPlayer()->StartMapLayer && UI.CurrentMapLayer->world != CMap::get()->MapLayers[CPlayer::GetThisPlayer()->StartMapLayer]->world) {
					CPlayer::GetThisPlayer()->Notify("%s", _("Cannot build in another world"));
				}
				PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
			}
		} else {
			CancelBuildingMode();
		}
		return;
	}

#ifdef USE_TOUCHSCREEN
	if (doubleLeftButton) {
#else
	if (MouseButtons & RightButton) {
#endif
		if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos)) {
			CUnit *unit;
			// FIXME: Rethink the complete chaos of coordinates here
			// FIXME: Johns: Perhaps we should use a pixel map coordinates
			const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);

			if (UnitUnderCursor != nullptr && (unit = UnitOnMapTile(tilePos, unit_domain::none, UI.CurrentMapLayer->ID))
				&& !UnitUnderCursor->Type->BoolFlag[DECORATION_INDEX].value) {
				unit->Blink = 4;                // if right click on building -- blink
			} else { // if not not click on building -- green cross
				if (!ClickMissile.empty()) {
					const PixelPos mapPixelPos = UI.MouseViewport->screen_to_map_pixel_pos(CursorScreenPos);

					MakeLocalMissile(*wyrmgus::missile_type::get(ClickMissile), mapPixelPos, mapPixelPos, UI.CurrentMapLayer->ID);
				}
			}
			const PixelPos mapPixelPos = UI.MouseViewport->screen_to_scaled_map_pixel_pos(CursorScreenPos);
			DoRightButton(mapPixelPos, key_modifiers);
		}
	} else if (MouseButtons & LeftButton) { // enter select mode
		CursorStartScreenPos = CursorScreenPos;
		CursorStartMapPos = UI.MouseViewport->screen_to_scaled_map_pixel_pos(CursorScreenPos);
		cursor::set_current_cursor(UI.get_cursor(cursor_type::cross), false);
		CurrentCursorState = CursorState::Rectangle;
	} else if (MouseButtons & MiddleButton) {// enter move map mode
		CursorStartScreenPos = CursorScreenPos;
		cursor::set_current_cursor(UI.get_cursor(cursor_type::scroll), false);
		//Wyrmgus start
		UnitUnderCursor = nullptr;
		//Wyrmgus end
	}
}

static void UIHandleButtonDown_OnMinimap(const Qt::KeyboardModifiers key_modifiers)
{
	const Vec2i cursor_tile_pos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);

	if (MouseButtons & LeftButton) { // enter move mini-mode
		UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(cursor_tile_pos));
		if (cursor_tile_pos != UI.get_minimap()->screen_to_tile_pos(CursorScreenPos)) {
			CursorScreenPos = UI.get_minimap()->tile_to_screen_pos(cursor_tile_pos);
			UI.MouseWarpPos = CursorStartScreenPos = CursorScreenPos;
		}
	} else if (MouseButtons & RightButton) {
		if (!GameObserve && !game::get()->is_paused() && !GameEstablishing) {
			if (!ClickMissile.empty()) {
				const PixelPos map_pixel_pos = CMap::get()->tile_pos_to_map_pixel_pos_center(cursor_tile_pos);
				MakeLocalMissile(*wyrmgus::missile_type::get(ClickMissile), map_pixel_pos, map_pixel_pos, UI.CurrentMapLayer->ID);
			}
			const PixelPos scaled_map_pixel_pos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(cursor_tile_pos);
			DoRightButton(scaled_map_pixel_pos, key_modifiers);
		}
	}
}

static void UIHandleButtonDown_OnButton(const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(key_modifiers)

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
		if (ButtonAreaUnderCursor == ButtonAreaMapLayerWorld) {
			for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
				CUIButton &world_button = UI.WorldButtons[i];

				if (i == static_cast<size_t>(ButtonUnderCursor) && !world_button.Clicked) {
					PlayGameSound(game_sound_set::get()->get_click_sound(), MaxSampleVolume);
					world_button.Clicked = true;
				}
			}
		//  clicked on user buttons
		} else if (ButtonAreaUnderCursor == ButtonAreaUser) {
			for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
				CUIUserButton &user_button = UI.UserButtons[i];

				if (i == static_cast<size_t>(ButtonUnderCursor) && !user_button.Clicked) {
					PlayGameSound(game_sound_set::get()->get_click_sound(), MaxSampleVolume);
					user_button.Clicked = true;
				}
			}
			//  clicked on selected button
		} else if (ButtonAreaUnderCursor == ButtonAreaButton) {
			//Wyrmgus start
//			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]))) {
			//Wyrmgus end
				OldButtonUnderCursor = ButtonUnderCursor;
			}
		}
	} else if ((MouseButtons & MiddleButton)) {
		//Wyrmgus start
		/*
		//  clicked on info panel - single unit shown
		if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0 && Selected.size() == 1) {
			PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
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
static void UIHandleButtonUp_OnButton(unsigned button, const Qt::KeyboardModifiers key_modifiers)
{
	// clicked on info panel - selection shown
	if (Selected.size() > 1 && ButtonAreaUnderCursor == ButtonAreaSelected) {
		DoSelectionButtons(ButtonUnderCursor, button, key_modifiers);
	} else {
		if (ButtonAreaUnderCursor == ButtonAreaSelected) {
			//  clicked on single unit shown
			if (ButtonUnderCursor == 0 && Selected.size() == 1) {
				PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
				if ((1 << button) == LeftButton) {
					if (Selected[0]->MapLayer != UI.CurrentMapLayer) {
						ChangeCurrentMapLayer(Selected[0]->MapLayer->ID);
					}
					UI.SelectedViewport->Center(Selected[0]->get_scaled_map_pixel_pos_center());
				} else if ((1 << button) == RightButton) {
					QString encyclopedia_type;
					QString encyclopedia_identifier;

					if (Selected[0]->get_character() != nullptr && !Selected[0]->get_character()->is_custom()) {
						if (Selected[0]->get_character()->is_deity()) {
							encyclopedia_type = "deity";
							encyclopedia_identifier = Selected[0]->get_character()->get_deity()->get_identifier_qstring();
						} else {
							encyclopedia_type = "character";
							encyclopedia_identifier = Selected[0]->get_character()->get_identifier_qstring();
						}
					} else if (Selected[0]->get_unique() != nullptr) {
						encyclopedia_type = "unique_item";
						encyclopedia_identifier = Selected[0]->get_unique()->get_identifier_qstring();
					} else {
						encyclopedia_type = "unit_type";
						const unit_type *unit_type = Selected[0]->Type;

						while (unit_type != nullptr && !unit_type->has_encyclopedia_entry()) {
							unit_type = unit_type->Parent;
						}

						if (unit_type != nullptr) {
							encyclopedia_identifier = unit_type->get_identifier_qstring();
						}
					}

					if (!encyclopedia_identifier.isEmpty()) {
						emit engine_interface::get()->encyclopediaEntryOpened(encyclopedia_type + ":" + encyclopedia_identifier);
					}
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
//			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && ThisPlayer->IsTeamed(*Selected[0])) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]))) {
			//Wyrmgus end
				if (static_cast<size_t>(ButtonUnderCursor) < Selected[0]->Orders.size()) {
					size_t j = 0;
					int order_slot = -1;
					for (size_t i = 0; i < Selected[0]->Orders.size(); ++i) {
						if (Selected[0]->Orders[i]->Action == UnitAction::Train) {
							const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[i].get());
							if (i > 0 && j > 0 && Selected[0]->Orders[i - 1]->Action == UnitAction::Train) {
								const COrder_Train &previous_order = *static_cast<COrder_Train *>(Selected[0]->Orders[i - 1].get());
								if (previous_order.GetUnitType().Slot == order.GetUnitType().Slot) {
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
						const COrder_Train &order = *static_cast<COrder_Train *>(Selected[0]->Orders[order_slot].get());

						DebugPrint("Cancel slot %d %s\n" _C_ order_slot _C_ order.GetUnitType().Ident.c_str());
						SendCommandCancelTraining(*Selected[0], order_slot, &order.GetUnitType());
					}
				}
			}
			//  clicked on upgrading button
		} else if (ButtonAreaUnderCursor == ButtonAreaUpgrading) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel upgrade %s\n" _C_ Selected[0]->Type->Ident.c_str());
					SendCommandCancelUpgradeTo(*Selected[0]);
				}
			}
			//  clicked on researching button
		} else if (ButtonAreaUnderCursor == ButtonAreaResearching) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (ButtonUnderCursor == 0 && Selected.size() == 1) {
					DebugPrint("Cancel research %s\n" _C_ Selected[0]->Type->Ident.c_str());
					SendCommandCancelResearch(*Selected[0]);
				}
			}
			//  clicked on button panel
		} else if (ButtonAreaUnderCursor == ButtonAreaTransporting) {
			//  for transporter
			//Wyrmgus start
//			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && ThisPlayer->IsTeamed(*Selected[0])) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->is_allied_with(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]))) {
			//Wyrmgus end
				if (Selected[0]->BoardCount >= ButtonUnderCursor) {
					CUnit *uins = Selected[0]->UnitInside;
					size_t j = 0;

					for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
						if (!uins->Boarded || j >= UI.TransportingButtons.size() || (Selected[0]->Player != CPlayer::GetThisPlayer() && uins->Player != CPlayer::GetThisPlayer())) {
							continue;
						}
						if (ButtonAreaUnderCursor == ButtonAreaTransporting && static_cast<size_t>(ButtonUnderCursor) == j) {
							assert_throw(uins->Boarded);
							const int flush = !(key_modifiers & Qt::ShiftModifier);
							if (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || uins->Player == CPlayer::GetThisPlayer()) {
								SendCommandUnload(*Selected[0], Selected[0]->tilePos, uins, flush, Selected[0]->MapLayer->ID);
							}
						}
						++j;
					}
				}
			}
		//Wyrmgus start
		} else if (ButtonAreaUnderCursor == ButtonAreaInventory) {
			//  for inventory unit
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])) {
				if (Selected[0]->InsideCount >= ButtonUnderCursor) {
					CUnit *uins = Selected[0]->UnitInside;
					size_t j = 0;

					for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
						if (!uins->Type->BoolFlag[ITEM_INDEX].value || j >= UI.InventoryButtons.size() || (Selected[0]->Player != CPlayer::GetThisPlayer() && uins->Player != CPlayer::GetThisPlayer())) {
							continue;
						}
						if (ButtonAreaUnderCursor == ButtonAreaInventory && static_cast<size_t>(ButtonUnderCursor) == j) {
							assert_throw(uins->Type->BoolFlag[ITEM_INDEX].value);
							const int flush = !(key_modifiers & Qt::ShiftModifier);
							if (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || uins->Player == CPlayer::GetThisPlayer()) {
								if ((1 << button) == LeftButton) {
									if  (!uins->Bound) {
										SendCommandUnload(*Selected[0], Selected[0]->tilePos, uins, flush, Selected[0]->MapLayer->ID);
									} else {
										if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
											std::string item_name = uins->GetMessageName();
											if (uins->get_unique() == nullptr) {
												item_name = "the " + item_name;
											}
											Selected[0]->Player->Notify(NotifyRed, Selected[0]->tilePos, Selected[0]->MapLayer->ID, _("%s cannot drop %s."), Selected[0]->GetMessageName().c_str(), item_name.c_str());
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
//			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && ThisPlayer->IsTeamed(*Selected[0])) {
			if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]))) {
			//Wyrmgus end
				OldButtonUnderCursor = ButtonUnderCursor;
			}
		//Wyrmgus start
		} else if (ButtonAreaUnderCursor == ButtonAreaIdleWorker) {
			if (ButtonUnderCursor == 0) {
				PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
				UiFindIdleWorker();
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit) {
			if (ButtonUnderCursor == 0) {
				PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
				UiFindLevelUpUnit();
			}
		} else if (ButtonAreaUnderCursor == ButtonAreaHeroUnit) {
			PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
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
void UIHandleButtonDown(unsigned button, const Qt::KeyboardModifiers key_modifiers)
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
	if (CurrentCursorState == CursorState::Rectangle) {
		return;
	}
	// CursorOn should have changed with BigMapMode, so recompute it.
	HandleMouseOn(CursorScreenPos);
	//  Selecting target. (Move,Attack,Patrol,... commands);
	if (CurrentCursorState == CursorState::Select) {
		UISelectStateButtonDown(button, key_modifiers);
		return;
	}

	//  Cursor is on the map area
	if (CursorOn == cursor_on::map) {
		UIHandleButtonDown_OnMap(key_modifiers);
	} else if (CursorOn == cursor_on::minimap) {
		//  Cursor is on the minimap area
		UIHandleButtonDown_OnMinimap(key_modifiers);
	} else if (CursorOn == cursor_on::button) {
		//  Cursor is on the buttons: group or command
		UIHandleButtonDown_OnButton(key_modifiers);
	}
}

/**
**  Called if mouse button released.
**
**  @param button  Button released.
*/
void UIHandleButtonUp(unsigned button, const Qt::KeyboardModifiers key_modifiers)
{
	//
	//  Move map.
	//
	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
		return;
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
		//  World buttons
		//
		for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
			CUIButton &world_button = UI.WorldButtons[i];

			if (world_button.Clicked) {
				world_button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaMapLayerWorld) {
					CMap::get()->SetCurrentWorld(wyrmgus::world::get_all()[i]);
					if (world_button.Callback) {
						world_button.Callback->action("");
					}
					return;
				}
			}
		}

		//
		//  User buttons
		//
		for (CUIUserButton &user_button : UI.UserButtons) {
			if (user_button.Clicked) {
				user_button.Clicked = false;
				if (ButtonAreaUnderCursor == ButtonAreaUser) {
					if (user_button.Button.Callback) {
						user_button.Button.Callback->action("");
					}
					return;
				}
			}
		}
		
		//Wyrmgus start
//		if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && Selected.empty() == false && ThisPlayer->IsTeamed(*Selected[0])) {
		if (!GameObserve && !game::get()->is_paused() && !GameEstablishing && Selected.empty() == false && (CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]))) {
		//Wyrmgus end
			if (OldButtonUnderCursor != -1 && OldButtonUnderCursor == ButtonUnderCursor) {
				UI.ButtonPanel.DoClicked(ButtonUnderCursor, key_modifiers);
				OldButtonUnderCursor = -1;
				return;
			}
		}
		if (CursorOn == cursor_on::button) {
			// FIXME: other buttons?
			if (ButtonAreaUnderCursor == ButtonAreaButton && OldButtonUnderCursor != -1 && OldButtonUnderCursor == ButtonUnderCursor) {
				UI.ButtonPanel.DoClicked(ButtonUnderCursor, key_modifiers);
				return;
			}
			//Wyrmgus start
			UIHandleButtonUp_OnButton(button, key_modifiers);
			//Wyrmgus end
		}
	}

	// FIXME: should be completly rewritten
	// FIXME: must selecting!  (lokh: what does this mean? is this done now?)

	// SHIFT toggles select/unselect a single unit and
	// add the content of the rectangle to the selectection
	// ALT takes group of unit
	if (CurrentCursorState == CursorState::Rectangle && !(MouseButtons & LeftButton)) { // leave select mode
		int num = 0;
		//
		//  Little threshold
		//
		if (CursorStartScreenPos.x < CursorScreenPos.x - 1 || CursorScreenPos.x + 1 < CursorStartScreenPos.x
			|| CursorStartScreenPos.y < CursorScreenPos.y - 1 || CursorScreenPos.y + 1 < CursorStartScreenPos.y) {
			PixelPos pos0 = CursorStartMapPos;
			const PixelPos cursorMapPos = UI.MouseViewport->screen_to_scaled_map_pixel_pos(CursorScreenPos);
			PixelPos pos1 = cursorMapPos;

			if (pos0.x > pos1.x) {
				std::swap(pos0.x, pos1.x);
			}
			if (pos0.y > pos1.y) {
				std::swap(pos0.y, pos1.y);
			}
			if (key_modifiers & Qt::ShiftModifier) {
				if (key_modifiers & Qt::AltModifier) {
					num = AddSelectedGroundUnitsInRectangle(pos0, pos1);
				} else if (key_modifiers & Qt::ControlModifier) {
					num = AddSelectedAirUnitsInRectangle(pos0, pos1);
				} else {
					num = AddSelectedUnitsInRectangle(pos0, pos1);
				}
			} else {
				if (key_modifiers & Qt::AltModifier) {
					num = SelectGroundUnitsInRectangle(pos0, pos1);
				} else if (key_modifiers & Qt::ControlModifier) {
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
			if (ReplayRevealMap || UI.CurrentMapLayer->Field(cursorTilePos)->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
				const PixelPos cursorMapPos = UI.MouseViewport->screen_to_scaled_map_pixel_pos(CursorScreenPos);

				unit = UnitOnScreen(cursorMapPos.x, cursorMapPos.y);
			}
			if (unit) {
				// FIXME: Not nice coded, button number hardcoded!
				if ((key_modifiers & Qt::ControlModifier)
					|| (button & (1 << MouseDoubleShift))) {
					if (key_modifiers & Qt::ShiftModifier) {
						num = ToggleUnitsByType(*unit);
					} else {
						num = SelectUnitsByType(*unit);
					}
				} else if ((key_modifiers & Qt::AltModifier) && unit->LastGroup) {
					if (key_modifiers & Qt::ShiftModifier) {
						num = AddGroupFromUnitToSelection(*unit);
					} else {
						num = SelectGroupFromUnit(*unit);
					}

					// Don't allow to select own and enemy units.
					// Don't allow mixing buildings
				} else if (key_modifiers & Qt::ShiftModifier
						   && (unit->Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*unit))
						   //Wyrmgus start
//						   && !unit->Type->BoolFlag[BUILDING_INDEX].value
						   && (!Selected.size() || UnitCanBeSelectedWith(*Selected[0], *unit))
						   //Wyrmgus end
						   //Wyrmgus start
//						   && (Selected.size() != 1 || !Selected[0]->Type->BoolFlag[BUILDING_INDEX].value)
						   //Wyrmgus end
						   && (Selected.size() != 1 || Selected[0]->Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]))) {
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
				if (Selected[0]->CurrentAction() == UnitAction::Built && Selected[0]->Player == CPlayer::GetThisPlayer()) {
					PlayUnitSound(*Selected[0], wyrmgus::unit_sound_type::construction);
				} else if (Selected[0]->Burning) {
					// FIXME: use GameSounds.Burning
					PlayGameSound(wyrmgus::sound::get("burning"), MaxSampleVolume);
				} else if (Selected[0]->Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) || CPlayer::GetThisPlayer()->has_building_access(Selected[0]) || (Selected[0]->Player->get_index() == PlayerNumNeutral && !Selected[0]->Type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value)) {
					PlayUnitSound(*Selected[0], wyrmgus::unit_sound_type::selected);
				} else {
					PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
				}
				//Wyrmgus start
				/*
				//Wyrmgus start
//				if (Selected[0]->Player == ThisPlayer) {
				if (Selected[0]->Player == ThisPlayer && !Selected[0]->Type->BoolFlag[HERO_INDEX].value) { // don't display this for heroes
				//Wyrmgus end
					char buf[64];
					if (Selected[0]->Player->GetUnitTypeCount(Selected[0]->Type) > 1) {
						snprintf(buf, sizeof(buf), _("You have ~<%d~> %ss"),
								 Selected[0]->Player->GetUnitTypeCount(Selected[0]->Type),
								//Wyrmgus start
//								 Selected[0]->Type->Name.c_str());
								 Selected[0]->get_type_name().c_str());
								//Wyrmgus end
					} else {
						snprintf(buf, sizeof(buf), _("You have ~<%d~> %s(s)"),
								 Selected[0]->Player->GetUnitTypeCount(Selected[0]->Type),
								//Wyrmgus start
//								 Selected[0]->Type->Name.c_str());
								 Selected[0]->get_type_name().c_str());
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
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
		CurrentCursorState = CursorState::Point;
	}
}
