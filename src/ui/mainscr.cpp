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
/**@name mainscr.cpp - The main screen. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Valery Shchedrin,
//      Jimmy Salmon and Andrettin
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

#include "action/action_built.h"
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
#include "age.h"
#ifdef DEBUG
#include "ai/ai_local.h"
#endif
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource_storage_type.h"
#include "engine_interface.h"
#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/minimap_mode.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_feature.h"
#include "map/tile.h"
#include "map/world.h"
#include "menus.h"
#include "network.h"
#include "player/civilization.h"
#include "player/player.h"
#include "player/player_type.h"
#include "quest/objective/quest_objective.h"
#include "quest/player_quest_objective.h"
#include "quest/quest.h"
#include "script.h"
#include "script/trigger.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "spell/spell.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/contenttype.h"
#include "ui/cursor.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/assert_util.h"
#include "util/date_util.h"
#include "util/point_util.h"
#include "util/util.h"
#include "upgrade/upgrade.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  UI BUTTONS
----------------------------------------------------------------------------*/

void DrawUserDefinedButtons(std::vector<std::function<void(renderer *)>> &render_commands)
{
	for (size_t i = 0; i < UI.UserButtons.size(); ++i) {
		const CUIUserButton &button = UI.UserButtons[i];

		if (button.Button.X != -1) {
			DrawUIButton(button.Button.Style,
						 (ButtonAreaUnderCursor == ButtonAreaUser
						  && size_t(ButtonUnderCursor) == i ? MI_FLAGS_ACTIVE : 0) |
						 (button.Clicked ? MI_FLAGS_CLICKED : 0),
						 button.Button.X, button.Button.Y,
						 button.Button.Text, render_commands);
		}
	}
}

/*----------------------------------------------------------------------------
--  Icons
----------------------------------------------------------------------------*/

/**
**  Draw life bar of a unit at x,y.
**  Placed under icons on top-panel.
**
**  @param unit  Pointer to unit.
**  @param x     Screen X position of icon
**  @param y     Screen Y position of icon
*/
static void UiDrawLifeBar(const CUnit &unit, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	// FIXME: add icon borders
	int hBar, hAll;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	//Wyrmgus start
	if (Preference.IconsShift && wyrmgus::defines::get()->get_icon_frame_graphics() != nullptr && wyrmgus::defines::get()->get_pressed_icon_frame_graphics() != nullptr) {
//	if (Preference.IconsShift) {
		hBar = (4 * scale_factor).to_int();
		hAll = (8 * scale_factor).to_int();
		y += (2 * scale_factor).to_int();
	} else if (Preference.IconsShift) {
	//Wyrmgus end
		hBar = (6 * scale_factor).to_int();
		hAll = (10 * scale_factor).to_int();
	} else {
		hBar = (5 * scale_factor).to_int();
		hAll = (7 * scale_factor).to_int();
	}
	y += unit.Type->get_icon()->get_graphics()->Height;
	//Wyrmgus start
	/*
	Video.FillRectangleClip(ColorBlack, x - 4, y + 2,
		unit.Type->Icon.Icon->G->Width + 8, hAll);
	*/
	if (defines::get()->get_bar_frame_graphics() != nullptr) {
		defines::get()->get_bar_frame_graphics()->DrawClip(x + ((-2 - 4) * scale_factor).to_int(), y + ((4 - 4) * scale_factor).to_int(), render_commands);
		Video.FillRectangleClip(ColorBlack, x - (2 * scale_factor).to_int(), y + (4 * scale_factor).to_int(),
			unit.Type->get_icon()->get_graphics()->Width + ((6 - 2) * scale_factor).to_int(), hBar, render_commands);
	} else {
		Video.FillRectangleClip(ColorBlack, x - (4 * scale_factor).to_int(), y + (2 * scale_factor).to_int(),
			unit.Type->get_icon()->get_graphics()->Width + (8 * scale_factor).to_int(), hAll, render_commands);
	}
	//Wyrmgus end

	if (unit.Variable[HP_INDEX].Value) {
		uint32_t color;
		//Wyrmgus start
		uint32_t lighter_color;
//		int f = (100 * unit.Variable[HP_INDEX].Value) / unit.Variable[HP_INDEX].Max;
		int f = (100 * unit.Variable[HP_INDEX].Value) / unit.GetModifiedVariable(HP_INDEX, VariableAttribute::Max);
		//Wyrmgus end

		if (f > 75) {
			color = ColorDarkGreen;
			//Wyrmgus start
			lighter_color = CVideo::MapRGB(67, 137, 8);
			//Wyrmgus end
		} else if (f > 50) {
			color = ColorYellow;
			//Wyrmgus start
			lighter_color = CVideo::MapRGB(255, 255, 210);
			//Wyrmgus end
		} else if (f > 25) {
			color = ColorOrange;
			//Wyrmgus start
			lighter_color = CVideo::MapRGB(255, 180, 90);
			//Wyrmgus end
		} else {
			color = ColorRed;
			//Wyrmgus start
			lighter_color = CVideo::MapRGB(255, 100, 100);
			//Wyrmgus end
		}

		f = (f * (unit.Type->get_icon()->get_graphics()->Width + (6 * scale_factor).to_int())) / 100;
		Video.FillRectangleClip(color, x - (2 * scale_factor).to_int(), y + (4 * scale_factor).to_int(),
			f > 1 ? f - 2 : 0, hBar, render_commands);
		//Wyrmgus start
		Video.FillRectangleClip(lighter_color, x - (2 * scale_factor).to_int(), y + (4 * scale_factor).to_int(),
			f > 1 ? f - 2 : 0, 1, render_commands);
		//Wyrmgus end
	}
}

/**
**  Draw mana bar of a unit at x,y.
**  Placed under icons on top-panel.
**
**  @param unit  Pointer to unit.
**  @param x     Screen X position of icon
**  @param y     Screen Y position of icon
*/
static void UiDrawManaBar(const CUnit &unit, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	// FIXME: add icon borders
	y += unit.Type->get_icon()->get_graphics()->Height;
	Video.FillRectangleClip(ColorBlack, x, y + 3, unit.Type->get_icon()->get_graphics()->Width, 4, render_commands);

	//Wyrmgus start
//	if (unit.Stats->Variables[MANA_INDEX].Max) {
	if (unit.GetModifiedVariable(MANA_INDEX, VariableAttribute::Max)) {
	//Wyrmgus end
		//Wyrmgus start
//		int f = (100 * unit.Variable[MANA_INDEX].Value) / unit.Variable[MANA_INDEX].Max;
		int f = (100 * unit.GetModifiedVariable(MANA_INDEX, VariableAttribute::Value)) / unit.GetModifiedVariable(MANA_INDEX, VariableAttribute::Max);
		//Wyrmgus end
		f = (f * (unit.Type->get_icon()->get_graphics()->Width)) / 100;
		Video.FillRectangleClip(ColorBlue, x + 1, y + 3 + 1, f, 2, render_commands);
	}
}

/**
**  Tell if we can show the content.
**  verify each sub condition for that.
**
**  @param condition   condition to verify.
**  @param unit        unit that certain condition can refer.
**
**  @return            0 if we can't show the content, else 1.
*/
static bool CanShowContent(const ConditionPanel *condition, const CUnit &unit)
{
	if (!condition) {
		return true;
	}
	if ((condition->ShowOnlySelected && !unit.Selected)
		|| (unit.Player->get_type() == player_type::neutral && condition->HideNeutral)
		|| (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->is_enemy_of(unit) && !CPlayer::GetThisPlayer()->is_allied_with(unit) && condition->HideNeutral)
		|| (CPlayer::GetThisPlayer()->is_enemy_of(unit) && !condition->ShowOpponent)
		|| (CPlayer::GetThisPlayer()->is_allied_with(unit) && (unit.Player != CPlayer::GetThisPlayer()) && condition->HideAllied)
		|| (condition->ShowIfCanCastAnySpell && !unit.CanCastAnySpell())
	) {
		return false;
	}
	if (condition->BoolFlags && !unit.Type->CheckUserBoolFlags(condition->BoolFlags.get())) {
		return false;
	}
	//Wyrmgus start
	if (condition->Affixed != CONDITION_TRUE) {
		if ((condition->Affixed == CONDITION_ONLY) ^ (unit.Prefix != nullptr || unit.Suffix != nullptr)) {
			return false;
		}
	}
	if (condition->Unique != CONDITION_TRUE) {
		if ((condition->Unique == CONDITION_ONLY) ^ (unit.get_unique() != nullptr || unit.get_character() != nullptr)) {
			return false;
		}
	}
	if (condition->Replenishment != CONDITION_TRUE) {
		if ((condition->Replenishment == CONDITION_ONLY) ^ (unit.Variable[GIVERESOURCE_INDEX].Increase != 0)) {
			return false;
		}
	}
	//Wyrmgus end
	if (condition->Variables) {
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
				if ((condition->Variables[i] == CONDITION_ONLY) ^ unit.Variable[i].Enable) {
					return false;
				}
			}
		}
	}
	return true;
}

/**
**  Return the value corresponding.
**
**  @param unit   Unit.
**  @param index  Index of the variable.
**  @param e      Component of the variable.
**  @param t      Which var use (0:unit, 1:Type, 2:Stats)
**
**  @return       Value corresponding
*/
UStrInt GetComponent(const CUnit &unit, const int index, const VariableAttribute e, const int t)
{
	UStrInt val{};
	const wyrmgus::unit_variable *var = nullptr;

	assert_throw(static_cast<unsigned int>(index) < UnitTypeVar.GetNumberVariable());
	
	switch (t) {
		case 0: // Unit:
			var = &unit.Variable[index];
			break;
		case 1: // Type:
			var = &unit.Type->MapDefaultStat.Variables[index];
			break;
		case 2: // Stats:
			var = &unit.Stats->Variables[index];
			break;
		default:
			DebugPrint("Bad value for GetComponent: t = %d" _C_ t);
			var = &unit.Variable[index];
			break;
	}

	switch (e) {
		case VariableAttribute::Value:
			val.type = USTRINT_INT;
			val.i = var->Value;
			break;
		case VariableAttribute::Max:
			val.type = USTRINT_INT;
			val.i = var->Max;
			break;
		case VariableAttribute::Increase:
			val.type = USTRINT_INT;
			val.i = var->Increase;
			break;
		case VariableAttribute::Diff:
			val.type = USTRINT_INT;
			val.i = var->Max - var->Value;
			break;
		case VariableAttribute::Percent:
			assert_throw(unit.Variable[index].Max != 0);
			val.type = USTRINT_INT;
			val.i = 100 * var->Value / var->Max;
			break;
		case VariableAttribute::Name:
			if (index == GIVERESOURCE_INDEX) {
				val.type = USTRINT_STR;
				//Wyrmgus start
//				val.i = unit.Type->GivesResource;
//				val.s = DefaultResourceNames[unit.Type->GivesResource].c_str();
				val.i = unit.GivesResource;
				val.s = DefaultResourceNames[unit.GivesResource].c_str();
				//Wyrmgus end
			} else if (index == CARRYRESOURCE_INDEX) {
				val.type = USTRINT_STR;
				val.i = unit.CurrentResource;
				val.s = DefaultResourceNames[unit.CurrentResource].c_str();
			} else {
				val.type = USTRINT_STR;
				val.i = index;
				val.s = UnitTypeVar.VariableNameLookup[index];
			}
			break;
		//Wyrmgus start
		case VariableAttribute::Change:
			val.type = USTRINT_INT;
			if (unit.Container && unit.Container->HasInventory()) {
				val.i = unit.Container->GetItemVariableChange(&unit, index);
			} else if (unit.Work || unit.Elixir) {
				val.i = unit.GetItemVariableChange(&unit, index);
			} else {
				val.i = var->Value;
			}
			break;
		case VariableAttribute::IncreaseChange:
			val.type = USTRINT_INT;
			if (unit.Container && unit.Container->HasInventory()) {
				val.i = unit.Container->GetItemVariableChange(&unit, index, true);
			} else if (unit.Work || unit.Elixir) {
				val.i = unit.GetItemVariableChange(&unit, index, true);
			} else {
				val.i = var->Increase;
			}
			break;
		//Wyrmgus end
		default:
			throw std::runtime_error("Invalid variable attribute: " + std::to_string(static_cast<int>(e)));
	}
	return val;
}

UStrInt GetComponent(const wyrmgus::unit_type &type, const int index, const VariableAttribute e, const int t)
{
	UStrInt val{};
	const wyrmgus::unit_variable *var = nullptr;

	assert_throw(static_cast<unsigned int>(index) < UnitTypeVar.GetNumberVariable());

	switch (t) {
		case 0: // Unit:
			var = &type.Stats[CPlayer::GetThisPlayer()->get_index()].Variables[index];
			break;
		case 1: // Type:
			var = &type.MapDefaultStat.Variables[index];
			break;
		case 2: // Stats:
			var = &type.Stats[CPlayer::GetThisPlayer()->get_index()].Variables[index];
			break;
		default:
			DebugPrint("Bad value for GetComponent: t = %d" _C_ t);
			var = &type.Stats[CPlayer::GetThisPlayer()->get_index()].Variables[index];
			break;
	}

	switch (e) {
		case VariableAttribute::Value:
			val.type = USTRINT_INT;
			val.i = var->Value;
			break;
		case VariableAttribute::Max:
			val.type = USTRINT_INT;
			val.i = var->Max;
			break;
		case VariableAttribute::Increase:
			val.type = USTRINT_INT;
			val.i = var->Increase;
			break;
		case VariableAttribute::Diff:
			val.type = USTRINT_INT;
			val.i = var->Max - var->Value;
			break;
		case VariableAttribute::Percent:
			assert_throw(type.Stats[CPlayer::GetThisPlayer()->get_index()].Variables[index].Max != 0);
			val.type = USTRINT_INT;
			val.i = 100 * var->Value / var->Max;
			break;
		case VariableAttribute::Name:
			if (index == GIVERESOURCE_INDEX) {
				val.type = USTRINT_STR;
				val.i = type.get_given_resource() != nullptr ? type.get_given_resource()->get_index() : 0;
				val.s = DefaultResourceNames[type.get_given_resource()->get_index()].c_str();
			} else {
				val.type = USTRINT_STR;
				val.i = index;
				val.s = UnitTypeVar.VariableNameLookup[index];
			}
			break;
		//Wyrmgus start
		case VariableAttribute::IncreaseChange:
		case VariableAttribute::Change:
			val.type = USTRINT_INT;
			val.i = 0;
			break;
		//Wyrmgus end
		default:
			throw std::runtime_error("Invalid variable attribute: " + std::to_string(static_cast<int>(e)));
	}

	return val;
}

static void DrawUnitInfo_Training(const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (unit.Orders.size() == 1 || unit.Orders[1]->Action != UnitAction::Train) {
		if (!UI.SingleTrainingText.empty()) {
			CLabel label(UI.SingleTrainingFont);
			label.Draw(UI.SingleTrainingTextX, UI.SingleTrainingTextY, UI.SingleTrainingText, render_commands);
		}
		if (UI.SingleTrainingButton) {
			const COrder_Train &order = *static_cast<COrder_Train *>(unit.CurrentOrder());
			const unit_type_variation *variation = order.GetUnitType().GetDefaultVariation(CPlayer::GetThisPlayer());
			const icon *icon = (variation && variation->Icon.Icon) ? variation->Icon.Icon : order.GetUnitType().get_icon();
			//Wyrmgus start
//			const unsigned int flags = (ButtonAreaUnderCursor == ButtonAreaTraining && ButtonUnderCursor == 0) ?
			unsigned int flags = (ButtonAreaUnderCursor == ButtonAreaTraining && ButtonUnderCursor == 0) ?
			//Wyrmgus end
									   (IconActive | (MouseButtons & LeftButton)) : 0;
			//Wyrmgus start
			flags |= IconCommandButton;
			//Wyrmgus end
			const PixelPos pos(UI.SingleTrainingButton->X, UI.SingleTrainingButton->Y);
			icon->DrawUnitIcon(*UI.SingleTrainingButton->Style, flags, pos, "", unit.get_player_color(), render_commands);
		}
	} else {
		if (!UI.TrainingText.empty()) {
			CLabel label(UI.TrainingFont);
			label.Draw(UI.TrainingTextX, UI.TrainingTextY, UI.TrainingText, render_commands);
		}
		if (!UI.TrainingButtons.empty()) {
			size_t j = 0;
			std::vector<int> train_counter;
			for (size_t i = 0; i < unit.Orders.size(); ++i) {
				if (unit.Orders[i]->Action == UnitAction::Train) {
					const COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[i].get());
					if (i > 0 && j > 0 && unit.Orders[i - 1]->Action == UnitAction::Train) {
						const COrder_Train &previous_order = *static_cast<COrder_Train *>(unit.Orders[i - 1].get());
						if (previous_order.GetUnitType().Slot == order.GetUnitType().Slot) {
							train_counter[j - 1]++;
							continue;
						}
					}
					if (j >= UI.TrainingButtons.size()) {
						break;
					}
					const unit_type_variation *variation = order.GetUnitType().GetDefaultVariation(CPlayer::GetThisPlayer());
					const icon *icon = (variation && variation->Icon.Icon) ? variation->Icon.Icon : order.GetUnitType().get_icon();
					//Wyrmgus start
//					const int flag = (ButtonAreaUnderCursor == ButtonAreaTraining
					int flag = (ButtonAreaUnderCursor == ButtonAreaTraining
					//Wyrmgus end
									  && static_cast<size_t>(ButtonUnderCursor) == j) ?
									 (IconActive | (MouseButtons & LeftButton)) : 0;
					const PixelPos pos(UI.TrainingButtons[j].X, UI.TrainingButtons[j].Y);
					//Wyrmgus start
					flag |= IconCommandButton;
					icon->DrawUnitIcon(*UI.TrainingButtons[j].Style, flag, pos, "", unit.get_player_color(), render_commands);
					train_counter.push_back(1);
					++j;
				}
			}
			
			for (size_t i = 0; i < train_counter.size(); ++i) {
				if (train_counter[i] > 1) {
					std::string number_string = std::to_string(train_counter[i]);
					CLabel label(wyrmgus::defines::get()->get_game_font());

					const PixelPos pos(UI.TrainingButtons[i].X, UI.TrainingButtons[i].Y);
					label.Draw(pos.x + (46 * preferences::get()->get_scale_factor()).to_int() - defines::get()->get_game_font()->Width(number_string), pos.y + 0, number_string, render_commands);
				}
			}
		}
	}
}

static void DrawUnitInfo_portrait(const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (UI.SingleSelectedButton) {
		const PixelPos pos(UI.SingleSelectedButton->X, UI.SingleSelectedButton->Y);
		 //Wyrmgus start
//		const int flag = (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) ?
//						 (IconActive | (MouseButtons & LeftButton)) : 0;
		int flag = (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) ?
						 IconActive : 0;
		
		if (flag == IconActive && ((MouseButtons & LeftButton) || (MouseButtons & MiddleButton) || (MouseButtons & RightButton))) {
			flag = IconClicked;
		}
		 //Wyrmgus end

		unit.get_icon()->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", unit.get_player_color(), render_commands);
	}
}

static bool DrawUnitInfo_single_selection(const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	switch (unit.CurrentAction()) {
		case UnitAction::Train: { //  Building training units.
			//Wyrmgus start
			const COrder_Train &order = *static_cast<COrder_Train *>(unit.CurrentOrder());
			if (order.GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0) { //don't show the training button for a quick moment if the time cost is 0
				return false;
			}
			//Wyrmgus end
			DrawUnitInfo_Training(unit, render_commands);
			return true;
		}
		case UnitAction::UpgradeTo: { //  Building upgrading to better type.
			if (UI.UpgradingButton) {
				const COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(unit.CurrentOrder());
				
				//Wyrmgus start
				if (order.GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0) { //don't show the upgrading button for a quick moment if the time cost is 0
					return false;
				}
				//Wyrmgus end

				const icon *icon = order.GetUnitType().get_icon();
				unsigned int flag = (ButtonAreaUnderCursor == ButtonAreaUpgrading
									 && ButtonUnderCursor == 0) ?
									(IconActive | (MouseButtons & LeftButton)) : 0;
				const PixelPos pos(UI.UpgradingButton->X, UI.UpgradingButton->Y);
				flag |= IconCommandButton;
				icon->DrawUnitIcon(*UI.UpgradingButton->Style, flag, pos, "", unit.get_player_color(), render_commands);
			}
			return true;
		}
		case UnitAction::Research: { //  Building research new technology.
			if (UI.ResearchingButton) {
				COrder_Research &order = *static_cast<COrder_Research *>(unit.CurrentOrder());
				
				//Wyrmgus start
				if (order.GetUpgrade().get_time_cost() == 0) {
					//don't show the researching button for a quick moment if the time cost is 0
					return false;
				}
				//Wyrmgus end
				
				wyrmgus::icon &icon = *order.GetUpgrade().get_icon();
				int flag = (ButtonAreaUnderCursor == ButtonAreaResearching
							&& ButtonUnderCursor == 0) ?
						   (IconActive | (MouseButtons & LeftButton)) : 0;
				PixelPos pos(UI.ResearchingButton->X, UI.ResearchingButton->Y);
				flag |= IconCommandButton;
				icon.DrawUnitIcon(*UI.ResearchingButton->Style, flag, pos, "", unit.get_player_color(), render_commands);
			}
			return true;
		}
		default:
			return false;
	}
}

static void DrawUnitInfo_transporter(CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CUnit *uins = unit.UnitInside;
	size_t j = 0;

	for (int i = 0; i < unit.InsideCount; ++i, uins = uins->NextContained) {
		//Wyrmgus start
//		if (!uins->Boarded || j >= UI.TransportingButtons.size()) {
		if (!uins->Boarded || j >= UI.TransportingButtons.size() || (unit.Player != CPlayer::GetThisPlayer() && uins->Player != CPlayer::GetThisPlayer())) {
		//Wyrmgus end
			continue;
		}

		int flag = (ButtonAreaUnderCursor == ButtonAreaTransporting && static_cast<size_t>(ButtonUnderCursor) == j) ?
				   (IconActive | (MouseButtons & LeftButton)) : 0;
		//Wyrmgus start
		flag |= IconCommandButton;
		//Wyrmgus end
		const PixelPos pos(UI.TransportingButtons[j].X, UI.TransportingButtons[j].Y);
		uins->get_icon()->DrawUnitIcon(*UI.TransportingButtons[j].Style, flag, pos, "", uins->get_player_color(), render_commands);
		//Wyrmgus start
//		UiDrawLifeBar(*uins, pos.x, pos.y);
//		if (uins->Type->CanCastSpell && uins->Variable[MANA_INDEX].Max) {
		if (uins->Type->Spells.size() > 0 && uins->Variable[MANA_INDEX].Enable && uins->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max)) {
		//Wyrmgus end
			//Wyrmgus start
//			UiDrawManaBar(*uins, pos.x, pos.y);
			// don't draw the mana bar when within transporters, as there's not enough space for it
			//Wyrmgus end
		}
		++j;
	}
}

//Wyrmgus start
static void DrawUnitInfo_inventory(CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CUnit *uins = unit.UnitInside;
	size_t j = 0;

	for (int i = 0; i < unit.InsideCount; ++i, uins = uins->NextContained) {
		if (!uins->Type->BoolFlag[ITEM_INDEX].value || j >= UI.InventoryButtons.size()) {
			continue;
		}
		
		int flag = (ButtonAreaUnderCursor == ButtonAreaInventory && static_cast<size_t>(ButtonUnderCursor) == j) ?
				   IconActive : 0;
		if (flag && ((MouseButtons & LeftButton) || (MouseButtons & RightButton))) {
			flag |= IconClicked;
		}
		flag |= IconCommandButton;
		if (unit.IsItemEquipped(uins)) {
			flag |= IconSelected;
		}
		const PixelPos pos(UI.InventoryButtons[j].X, UI.InventoryButtons[j].Y);
		uins->get_icon()->DrawUnitIcon(*UI.InventoryButtons[j].Style, flag, pos, "", unit.get_player_color(), render_commands);
		++j;
	}
}
//Wyrmgus end

/**
**  Draw the unit info into top-panel.
**
**  @param unit  Pointer to unit.
*/
static void DrawUnitInfo(CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	UpdateUnitVariables(unit);
	
	for (size_t i = 0; i != UI.InfoPanelContents.size(); ++i) {
		if (CanShowContent(UI.InfoPanelContents[i]->Condition.get(), unit)) {
			for (const std::unique_ptr<CContentType> &content : UI.InfoPanelContents[i]->Contents) {
				if (CanShowContent(content->Condition.get(), unit)) {
					content->Draw(unit, UI.InfoPanelContents[i]->DefaultFont, render_commands);
				}
			}
		}
	}

	assert_throw(unit.Type != nullptr);
	const wyrmgus::unit_type &type = *unit.Type;

	// Draw IconUnit
	DrawUnitInfo_portrait(unit, render_commands);

	//Wyrmgus start
//	if (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->is_allied_with(*unit.Player)) {
	if (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->is_allied_with(*unit.Player) && !CPlayer::GetThisPlayer()->has_building_access(&unit)) {
	//Wyrmgus end
		return;
	}

	//  Show progress if they are selected.
	if (IsOnlySelected(unit)) {
		if (DrawUnitInfo_single_selection(unit, render_commands)) {
			return;
		}
	}

	//  Transporting units.
	if (type.CanTransport() && unit.BoardCount && CurrentButtonLevel == unit.Type->ButtonLevelForTransporter) {
		DrawUnitInfo_transporter(unit, render_commands);
		return;
	}
	
	//Wyrmgus start
	if (unit.HasInventory() && unit.InsideCount && CurrentButtonLevel == wyrmgus::defines::get()->get_inventory_button_level()) {
		DrawUnitInfo_inventory(unit, render_commands);
		return;
	}
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
--  Map Layer Buttons
----------------------------------------------------------------------------*/

/**
**	@brief	Draw the map layer buttons.
*/
void DrawMapLayerButtons(std::vector<std::function<void(renderer *)>> &render_commands)
{
	for (size_t i = 0; i < UI.WorldButtons.size(); ++i) {
		if (UI.WorldButtons[i].X != -1) {
			DrawUIButton(UI.WorldButtons[i].Style,
				(ButtonAreaUnderCursor == ButtonAreaMapLayerWorld && ButtonUnderCursor == static_cast<int>(i) ? MI_FLAGS_ACTIVE : 0)
				| ((UI.WorldButtons[i].Clicked || CMap::get()->GetCurrentWorld() == wyrmgus::world::get_all()[i]) ? MI_FLAGS_CLICKED : 0),
				UI.WorldButtons[i].X, UI.WorldButtons[i].Y,
				UI.WorldButtons[i].Text, render_commands
			);
		}
	}
}

static std::unique_ptr<wyrmgus::button> get_territory_tooltip_button(const CPlayer *player)
{
	auto button = std::make_unique<wyrmgus::button>();
	button->Hint = player->get_full_name();
	button->Action = ButtonCmd::Player;
	button->Popup = "popup_territory";
	button->Value = player->get_index();
	return button;
}

//Wyrmgus start
/**
**	@brief	Draw certain popups if something is being hovered over
*/
void DrawPopups(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (engine_interface::get()->is_modal_dialog_open()) {
		return;
	}

	//
	// Draw unit under the cursor's name popup
	//
	if (CursorOn == cursor_on::map && (!Preference.ShowNameDelay || ShowNameDelay < GameCycle) && (!Preference.ShowNameTime || GameCycle < ShowNameTime)) {
		CViewport *vp = GetViewport(CursorScreenPos);
		if (vp) {
			const Vec2i tilePos = vp->ScreenToTilePos(CursorScreenPos);
			const tile &mf = *UI.CurrentMapLayer->Field(tilePos);
			const bool isMapFieldVisible = mf.player_info->IsTeamVisible(*CPlayer::GetThisPlayer());

			if (UI.MouseViewport && UI.MouseViewport->IsInsideMapArea(CursorScreenPos) && (isMapFieldVisible || ReplayRevealMap) && !(MouseButtons & MiddleButton)) { //don't display if in move map mode
				if (UnitUnderCursor && !UnitUnderCursor->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value && UnitUnderCursor->IsAliveOnMap()) {
					PixelPos unit_center_pos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(UnitUnderCursor->tilePos);
					unit_center_pos = vp->scaled_map_to_screen_pixel_pos(unit_center_pos);
					std::string unit_name;
					if (UnitUnderCursor->get_unique() != nullptr || UnitUnderCursor->Prefix != nullptr || UnitUnderCursor->Suffix != nullptr || UnitUnderCursor->Work != nullptr || UnitUnderCursor->Spell != nullptr || UnitUnderCursor->get_character() != nullptr) {
						if (!UnitUnderCursor->Identified) {
							unit_name = UnitUnderCursor->get_type_name() + " (" + _("Unidentified") + ")";
						} else {
							unit_name = UnitUnderCursor->get_full_name();
						}
					} else if (UnitUnderCursor->get_site() != nullptr && !UnitUnderCursor->get_site()->is_settlement() && !UnitUnderCursor->get_full_name().empty()) {
						unit_name = UnitUnderCursor->get_full_name() + " (" + UnitUnderCursor->get_type_name() + ")";
					} else {
						unit_name = UnitUnderCursor->get_type_name();
					}
					if (UnitUnderCursor->Player->get_index() != PlayerNumNeutral && !UnitUnderCursor->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value) {
						unit_name += " (" + UnitUnderCursor->Player->get_name() + ")";
					}
					//hackish way to make the popup appear correctly for the unit under cursor
					wyrmgus::button ba;
					ba.Hint = unit_name;
					ba.Action = ButtonCmd::Unit;
					ba.Value = UnitNumber(*UnitUnderCursor);
					ba.Popup = "popup_unit_under_cursor";
					DrawPopup(ba, unit_center_pos.x, unit_center_pos.y, render_commands);
					LastDrawnButtonPopup = nullptr;
				} else if (mf.get_terrain_feature() != nullptr || mf.get_world() != nullptr) {
					if (UI.get_tooltip_cycle_count() >= UI.get_tooltip_cycle_threshold()) {
						PixelPos tile_center_pos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(tilePos);
						tile_center_pos = vp->scaled_map_to_screen_pixel_pos(tile_center_pos);

						//hackish way to make the popup appear correctly for the tile under cursor
						wyrmgus::button ba;
						if (mf.get_terrain_feature() != nullptr) {
							const wyrmgus::civilization *tile_owner_civilization = mf.get_owner() ? mf.get_owner()->get_civilization() : nullptr;
							ba.Hint = mf.get_terrain_feature()->get_cultural_name(tile_owner_civilization);
						}
						ba.Action = ButtonCmd::Tile;
						ba.Value = CMap::get()->get_pos_index(tilePos, UI.CurrentMapLayer->ID);
						ba.Popup = "popup_tile_under_cursor";
						DrawPopup(ba, tile_center_pos.x, tile_center_pos.y, render_commands);
						LastDrawnButtonPopup = nullptr;
					} else {
						UI.increment_tooltip_cycle_count();
					}
				}
			}
		}
	}
	
	if (Selected.size() == 1) {
		if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) {
			//hackish way to make the popup appear correctly for the single selected unit
			wyrmgus::button ba;
			ba.Hint = Selected[0]->GetMessageName();
			ba.Action = ButtonCmd::Unit;
			ba.Value = UnitNumber(*Selected[0]);
			ba.Popup = "popup_unit";
			DrawPopup(ba, UI.SingleSelectedButton->X, UI.SingleSelectedButton->Y, render_commands);
			LastDrawnButtonPopup = nullptr;
		}
		
		if (!(Selected[0]->Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->is_allied_with(*Selected[0]->Player) && !CPlayer::GetThisPlayer()->has_building_access(Selected[0])) && Selected[0]->HasInventory() && Selected[0]->InsideCount && CurrentButtonLevel == wyrmgus::defines::get()->get_inventory_button_level()) {
		CUnit *uins = Selected[0]->UnitInside;
		size_t j = 0;

			for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
				if (!uins->Type->BoolFlag[ITEM_INDEX].value || j >= UI.InventoryButtons.size()) {
					continue;
				}
				
				int flag = (ButtonAreaUnderCursor == ButtonAreaInventory && static_cast<size_t>(ButtonUnderCursor) == j) ?
						   IconActive : 0;
				if (flag && ((MouseButtons & LeftButton) || (MouseButtons & RightButton))) {
					flag |= IconClicked;
				}
				flag |= IconCommandButton;
				if (Selected[0]->IsItemEquipped(uins)) {
					flag |= IconSelected;
				}
				const PixelPos pos(UI.InventoryButtons[j].X, UI.InventoryButtons[j].Y);
				uins->get_icon()->DrawUnitIcon(*UI.InventoryButtons[j].Style, flag, pos, "", Selected[0]->get_player_color(), render_commands);
				if (ButtonAreaUnderCursor == ButtonAreaInventory
					&& static_cast<size_t>(ButtonUnderCursor) == j) {
					//hackish way to make the popup appear correctly for the inventory item
					wyrmgus::button ba;
					if (!uins->Name.empty() && uins->Identified) {
						ba.Hint = uins->Name;
					} else {
						ba.Hint = uins->get_type_name();
						if (!uins->Identified) {
							ba.Hint += " (Unidentified)";
						}
					}
					ba.pos = j;
					ba.level = wyrmgus::defines::get()->get_inventory_button_level();
					ba.Action = ButtonCmd::Unit;
					ba.Value = UnitNumber(*uins);
					ba.Popup = "popup_item_inventory";
					DrawPopup(ba, UI.InventoryButtons[j].X, UI.InventoryButtons[j].Y, render_commands);
					LastDrawnButtonPopup = nullptr;
				}
				++j;
			}
		}
	}
	
	if (Selected.size() >= 1) {
		//
		//  Update status line for this button and draw popups
		//
		for (int i = 0; i < (int) UI.ButtonPanel.Buttons.size(); ++i) {
			if (ButtonAreaUnderCursor == ButtonAreaButton &&
				//Wyrmgus start
	//			ButtonUnderCursor == i && !game::get()->is_console_active()) {
				ButtonUnderCursor == i && !game::get()->is_console_active()
				&& CurrentButtons[i]->get_level() == CurrentButtonLevel && IsButtonAllowed(*Selected[0], *CurrentButtons[i])) {
				//Wyrmgus end
					DrawPopup(*CurrentButtons[i], UI.ButtonPanel.Buttons[i].X, UI.ButtonPanel.Buttons[i].Y, render_commands);
			}
		}
	}
	
	if (UI.IdleWorkerButton && !CPlayer::GetThisPlayer()->FreeWorkers.empty()) {
		if (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) { //if the mouse is hovering over the idle worker button, draw a tooltip
			std::string idle_worker_tooltip = _("Find Idle Worker (~!.)");
			DrawGenericPopup(idle_worker_tooltip, UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y, render_commands);
		}
	}
	
	if (UI.LevelUpUnitButton && !CPlayer::GetThisPlayer()->LevelUpUnits.empty()) {
		if (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) { //if the mouse is hovering over the level up unit button, draw a tooltip
			std::string level_up_unit_tooltip = _("Find Unit with Available Level Up");
			DrawGenericPopup(level_up_unit_tooltip, UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y, render_commands);
		}
	}

	for (size_t i = 0; i < UI.HeroUnitButtons.size() && i < CPlayer::GetThisPlayer()->Heroes.size(); ++i) {
		if (ButtonAreaUnderCursor == ButtonAreaHeroUnit && ButtonUnderCursor == static_cast<int>(i)) { //if the mouse is hovering over the level up unit button, draw a tooltip
			std::string custom_hero_unit_tooltip = _("Select");
			custom_hero_unit_tooltip += " " + CPlayer::GetThisPlayer()->Heroes[i]->GetMessageName();
			DrawGenericPopup(custom_hero_unit_tooltip, UI.HeroUnitButtons[i].X, UI.HeroUnitButtons[i].Y, render_commands);
		}
	}
		
	const wyrmgus::time_of_day *time_of_day = UI.SelectedViewport->get_center_tile_time_of_day();
	std::shared_ptr<const CGraphic> time_of_day_icon_graphics;
	if (time_of_day != nullptr) {
		time_of_day_icon_graphics = time_of_day->get_icon()->get_graphics();
	}
	
	if (ButtonAreaUnderCursor == ButtonAreaMapLayerWorld) {
		DrawGenericPopup(wyrmgus::world::get_all()[ButtonUnderCursor]->get_name(), UI.WorldButtons[ButtonUnderCursor].X, UI.WorldButtons[ButtonUnderCursor].Y, render_commands);
	}

	if (CursorOn == cursor_on::minimap) {
		const QPoint minimap_tile_pos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);
		if (CMap::get()->Info->IsPointOnMap(minimap_tile_pos, UI.CurrentMapLayer->ID)) {
			const wyrmgus::tile *tile = UI.CurrentMapLayer->Field(minimap_tile_pos);

			if (tile->player_info->IsTeamExplored(*CPlayer::GetThisPlayer())) {
				//hackish way to make the popup appear correctly
				std::unique_ptr<wyrmgus::button> button;
				const bool is_tile_water = tile->is_water() && !tile->is_river();
				const bool is_tile_space = tile->is_space();
				const bool is_tile_non_land = is_tile_water || is_tile_space;

				switch (UI.get_minimap()->get_mode()) {
					case wyrmgus::minimap_mode::territories:
						if (tile->get_owner() != nullptr && !is_tile_non_land) {
							button = get_territory_tooltip_button(tile->get_owner());
						}
						break;
					case wyrmgus::minimap_mode::territories_with_non_land:
						if (tile->get_owner() != nullptr) {
							button = get_territory_tooltip_button(tile->get_owner());
						}
						break;
					case wyrmgus::minimap_mode::realms:
						if (tile->get_realm_owner() != nullptr && !is_tile_non_land) {
							button = get_territory_tooltip_button(tile->get_realm_owner());
						}
						break;
					case wyrmgus::minimap_mode::realms_with_non_land:
						if (tile->get_realm_owner() != nullptr) {
							button = get_territory_tooltip_button(tile->get_realm_owner());
						}
						break;
					case wyrmgus::minimap_mode::settlements:
					case wyrmgus::minimap_mode::settlements_with_non_land:
					{
						const wyrmgus::site *settlement = tile->get_settlement();
						if (tile->get_settlement() == nullptr) {
							break;
						}

						const wyrmgus::site_game_data *settlement_game_data = settlement->get_game_data();
						const CUnit *site_unit = settlement_game_data->get_site_unit();

						if (site_unit == nullptr) {
							throw std::runtime_error("Settlement \"" + settlement->get_identifier() + "\" has territory on the map, but has no site unit.");
						}

						const wyrmgus::tile *site_center_tile = site_unit->get_center_tile();

						const bool is_same_tile_type_as_settlement = (is_tile_water == (site_center_tile->is_water() && !site_center_tile->is_river()) && is_tile_space == site_center_tile->is_space());

						if (UI.get_minimap()->get_mode() == wyrmgus::minimap_mode::settlements_with_non_land || is_same_tile_type_as_settlement) {
							button = std::make_unique<wyrmgus::button>();
							button->Action = ButtonCmd::None;
							button->Popup = "popup_settlement";
							button->Hint = settlement_game_data->get_current_cultural_name();
						}
						break;
					}
					default:
						break;
				}

				if (button != nullptr) {
					DrawPopup(*button, CursorScreenPos.x, CursorScreenPos.y, render_commands);
					LastDrawnButtonPopup = nullptr;
				}
			}
		}
	}
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  MESSAGE
----------------------------------------------------------------------------*/

static constexpr int MESSAGES_MAX = 10; /// How many can be displayed
//Wyrmgus start
static constexpr int OBJECTIVES_MAX = 32; /// How many objectives be displayed
//Wyrmgus end

static char MessagesEvent[MESSAGES_MAX][256];  /// Array of event messages
static Vec2i MessagesEventPos[MESSAGES_MAX];   /// coordinate of event
static int MessagesEventCount;                 /// Number of event messages
static int MessagesEventIndex;                 /// FIXME: docu

class MessagesDisplay
{
public:
	MessagesDisplay() : show(true)
	{
#ifdef DEBUG
		showBuilList = false;
#endif
		CleanMessages();
		//Wyrmgus start
		CleanObjectives();
		//Wyrmgus end
	}

	void UpdateMessages();
	void AddUniqueMessage(const char *s);
	//Wyrmgus start
	void AddObjective(const char *msg);
	//Wyrmgus end
	void DrawMessages(std::vector<std::function<void(renderer *)>> &render_commands);
	void CleanMessages();
	//Wyrmgus start
	void CleanObjectives();
	//Wyrmgus end
	void ToggleShowMessages() { show = !show; }
#ifdef DEBUG
	void ToggleShowBuilListMessages() { showBuilList = !showBuilList; }
#endif

protected:
	void ShiftMessages();
	void AddMessage(const char *msg);
	bool CheckRepeatMessage(const char *msg);

private:
	char Messages[MESSAGES_MAX][256];         /// Array of messages
	int  MessagesCount;                       /// Number of messages
	int  MessagesScrollY;
	//Wyrmgus start
	char Objectives[OBJECTIVES_MAX][256];         /// Array of objectives
	int  ObjectivesCount;                       /// Number of objectives
	//Wyrmgus end
	unsigned long MessagesFrameTimeout;       /// Frame to expire message
	bool show;
#ifdef DEBUG
	bool showBuilList;
#endif
};

/**
**  Shift messages array by one.
*/
void MessagesDisplay::ShiftMessages()
{
	if (MessagesCount) {
		--MessagesCount;
		for (int z = 0; z < MessagesCount; ++z) {
			strcpy_s(Messages[z], sizeof(Messages[z]), Messages[z + 1]);
		}
	}
}

/**
**  Update messages
**
**  @todo FIXME: make scroll speed configurable.
*/
void MessagesDisplay::UpdateMessages()
{
	if (!MessagesCount) {
		return;
	}

	// Scroll/remove old message line
	const unsigned long ticks = GetTicks();
	if (MessagesFrameTimeout < ticks) {
		++MessagesScrollY;
		if (MessagesScrollY == UI.MessageFont->Height() + (1 * preferences::get()->get_scale_factor()).to_int()) {
			MessagesFrameTimeout = ticks + UI.MessageScrollSpeed * 1000;
			MessagesScrollY = 0;
			ShiftMessages();
		}
	}
}

/**
**  Draw message(s).
**
**  @todo FIXME: make message font configurable.
*/
void MessagesDisplay::DrawMessages(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (show && preferences::get()->is_show_messages_enabled()) {
		CLabel label(UI.MessageFont);

		const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
		// background so the text is easier to read
		if (MessagesCount) {
			int textHeight = MessagesCount * (UI.MessageFont->Height() + (1 * scale_factor).to_int());
			uint32_t color = CVideo::MapRGB(38, 38, 78);
			//Wyrmgus start
			/*
			Video.FillTransRectangleClip(color, UI.MapArea.get_rect().x() + 7, UI.MapArea.get_rect().y() + 7,
										 UI.MapArea.EndX - UI.MapArea.get_rect().x() - 16,
										 textHeight - MessagesScrollY + 1, 0x80);

			Video.DrawRectangle(color, UI.MapArea.get_rect().x() + 6, UI.MapArea.get_rect().y() + 6,
								UI.MapArea.EndX - UI.MapArea.get_rect().x() - 15,
								textHeight - MessagesScrollY + 2);
			*/
			Video.FillTransRectangleClip(color, UI.MapArea.get_rect().x() + (6 * scale_factor).to_int() + 1, UI.MapArea.get_rect().bottom() + ((-16 - 2) * scale_factor).to_int() + 1 - textHeight + MessagesScrollY,
				UI.MapArea.get_rect().right() - UI.MapArea.get_rect().x() - (16 * scale_factor).to_int(),
				textHeight + (1 * scale_factor).to_int(), 0x80, render_commands);

			Video.DrawRectangle(color, UI.MapArea.get_rect().x() + (6 * scale_factor).to_int(), UI.MapArea.get_rect().bottom() + ((-16 - 2) * scale_factor).to_int() - textHeight + MessagesScrollY,
				UI.MapArea.get_rect().right() - UI.MapArea.get_rect().x() - (15 * scale_factor).to_int(),
				textHeight + (2 * scale_factor).to_int(), render_commands);
			//Wyrmgus end
		}

		// Draw message line(s)
		for (int z = 0; z < MessagesCount; ++z) {
			if (z == 0) {
				PushClipping();
				//Wyrmgus start
//					SetClipping(UI.MapArea.get_rect().x() + 8, UI.MapArea.get_rect().y() + 8, Video.Width - 1,
//								Video.Height - 1);
				SetClipping(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int(), Video.Width - 1, UI.MapArea.get_rect().bottom() - (16 * scale_factor).to_int());
				//Wyrmgus end
			}
			/*
			 * Due parallel drawing we have to force message copy due temp
			 * std::string(Messages[z]) creation because
			 * char * pointer may change during text drawing.
			 */
			label.DrawClip(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(),
				//Wyrmgus start
//							   UI.MapArea.get_rect().y() + 8 +
//							   z * (UI.MessageFont->Height() + 1) - MessagesScrollY,
UI.MapArea.get_rect().bottom() - (16 * scale_factor).to_int() - (UI.MessageFont->Height() + (1 * scale_factor).to_int()) +
(z * -1) * (UI.MessageFont->Height() + (1 * scale_factor).to_int()) + MessagesScrollY,
//Wyrmgus end
std::string(Messages[z]), render_commands);
			if (z == 0) {
				PopClipping();
			}
		}

		//Wyrmgus start
		// Draw objectives
		int z = 0;

		for (int i = 0; i < ObjectivesCount; ++i, ++z) {
			if (z == 0) {
				PushClipping();
				SetClipping(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int(), Video.Width - 1, Video.Height - 1);
			}
			/*
			 * Due parallel drawing we have to force message copy due temp
			 * std::string(Objectives[i]) creation because
			 * char * pointer may change during text drawing.
			 */
			label.DrawClip(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int() + z * (UI.MessageFont->Height() + (1 * scale_factor).to_int()), std::string(_(Objectives[i])), render_commands);
			if (z == 0) {
				PopClipping();
			}
		}

		for (const wyrmgus::quest *quest : CPlayer::GetThisPlayer()->get_current_quests()) {
			if (z == 0) {
				PushClipping();
				SetClipping(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int(), Video.Width - 1, Video.Height - 1);
			}
			label.DrawClip(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int() + z * (UI.MessageFont->Height() + (1 * scale_factor).to_int()), std::string(_(quest->get_name().c_str())), render_commands);
			if (z == 0) {
				PopClipping();
			}

			++z;

			for (const auto &objective : CPlayer::GetThisPlayer()->get_quest_objectives()) {
				const wyrmgus::quest_objective *quest_objective = objective->get_quest_objective();
				if (quest_objective->get_quest() != quest) {
					continue;
				}

				std::string objective_string = "- ";
				if (!quest_objective->get_objective_string().empty()) {
					objective_string += quest_objective->get_objective_string();
				} else {
					objective_string += quest_objective->generate_objective_string(CPlayer::GetThisPlayer());
				}
				if (quest_objective->get_quantity() != 0) {
					objective_string += " (" + std::to_string(objective->get_counter()) + "/" + std::to_string(quest_objective->get_quantity()) + ")";
				}

				label.DrawClip(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int() + z * (UI.MessageFont->Height() + (1 * scale_factor).to_int()), objective_string, render_commands);
				++z;
			}
			for (const std::string &objective_string : quest->get_objective_strings()) {
				label.DrawClip(UI.MapArea.get_rect().x() + (8 * scale_factor).to_int(), UI.MapArea.get_rect().y() + (8 * scale_factor).to_int() + z * (UI.MessageFont->Height() + (1 * scale_factor).to_int()), "- " + std::string(_(objective_string.c_str())), render_commands);
				++z;
			}
		}
		//Wyrmgus end
	}
}

/**
**  Adds message to the stack
**
**  @param msg  Message to add.
*/
void MessagesDisplay::AddMessage(const char *msg)
{
	unsigned long ticks = GetTicks();

	if (!MessagesCount) {
		MessagesFrameTimeout = ticks + UI.MessageScrollSpeed * 1000;
	}

	if (MessagesCount == MESSAGES_MAX) {
		// Out of space to store messages, can't scroll smoothly
		ShiftMessages();
		MessagesFrameTimeout = ticks + UI.MessageScrollSpeed * 1000;
		MessagesScrollY = 0;
	}
	char *ptr;
	char *next;
	char *message = Messages[MessagesCount];
	// Split long message into lines
	if (strlen(msg) >= sizeof(Messages[0])) {
		strncpy(message, msg, sizeof(Messages[0]) - 1);
		ptr = message + sizeof(Messages[0]) - 1;
		*ptr-- = '\0';
		next = ptr + 1;
		while (ptr >= message) {
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			}
			--ptr;
		}
		if (ptr < message) {
			ptr = next - 1;
		}
	} else {
		strcpy_s(message, sizeof(Messages[MessagesCount]), msg);
		next = ptr = message + strlen(message);
	}

	while (UI.MessageFont->Width(message) + 8 >= UI.MapArea.get_rect().width()) {
		while (1) {
			--ptr;
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			} else if (ptr == message) {
				break;
			}
		}
		// No space found, wrap in the middle of a word
		if (ptr == message) {
			ptr = next - 1;
			while (UI.MessageFont->Width(message) + 8 >= UI.MapArea.get_rect().width()) {
				*--ptr = '\0';
			}
			next = ptr + 1;
			break;
		}
	}

	++MessagesCount;

	if (strlen(msg) != (size_t)(ptr - message)) {
		AddMessage(msg + (next - message));
	}
}

/**
**  Check if this message repeats
**
**  @param msg  Message to check.
**
**  @return     true to skip this message
*/
bool MessagesDisplay::CheckRepeatMessage(const char *msg)
{
	if (MessagesCount < 1) {
		return false;
	}
	if (!strcmp(msg, Messages[MessagesCount - 1])) {
		return true;
	}
	return false;
}

//Wyrmgus start
/**
**  Adds objective to the stack
**
**  @param msg  Objective to add.
*/
void MessagesDisplay::AddObjective(const char *msg)
{
	char *ptr;
	char *next;
	char *message = Objectives[ObjectivesCount];
	// Split long message into lines
	if (strlen(msg) >= sizeof(Objectives[0])) {
		strncpy(message, msg, sizeof(Objectives[0]) - 1);
		ptr = message + sizeof(Objectives[0]) - 1;
		*ptr-- = '\0';
		next = ptr + 1;
		while (ptr >= message) {
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			}
			--ptr;
		}
		if (ptr < message) {
			ptr = next - 1;
		}
	} else {
		strcpy_s(message, sizeof(Objectives[ObjectivesCount]), msg);
		next = ptr = message + strlen(message);
	}

	while (UI.MessageFont->Width(message) + 8 >= UI.MapArea.get_rect().width()) {
		while (1) {
			--ptr;
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			} else if (ptr == message) {
				break;
			}
		}
		// No space found, wrap in the middle of a word
		if (ptr == message) {
			ptr = next - 1;
			while (UI.MessageFont->Width(message) + 8 >= UI.MapArea.get_rect().width()) {
				*--ptr = '\0';
			}
			next = ptr + 1;
			break;
		}
	}

	++ObjectivesCount;

	if (strlen(msg) != (size_t)(ptr - message)) {
		AddObjective(msg + (next - message));
	}
}
//Wyrmgus end

/**
**  Add a new message to display only if it differs from the preceding one.
*/
void MessagesDisplay::AddUniqueMessage(const char *s)
{
	if (!CheckRepeatMessage(s)) {
		AddMessage(s);
	}
}

/**
**  Clean up messages.
*/
void MessagesDisplay::CleanMessages()
{
	MessagesCount = 0;
	MessagesScrollY = 0;
	MessagesFrameTimeout = 0;
	//Wyrmgus start
	ObjectivesCount = 0;
	//Wyrmgus end

	MessagesEventCount = 0;
	MessagesEventIndex = 0;
}

//Wyrmgus start
/**
**  Clean up objectives.
*/
void MessagesDisplay::CleanObjectives()
{
	ObjectivesCount = 0;
}
//Wyrmgus end

static MessagesDisplay allmessages;

/**
**  Update messages
*/
void UpdateMessages()
{
	allmessages.UpdateMessages();
}

/**
**  Clean messages
*/
void CleanMessages()
{
	allmessages.CleanMessages();
}

//Wyrmgus start
/**
**  Clean messages
*/
void CleanObjectives()
{
	allmessages.CleanObjectives();
}
//Wyrmgus end

/**
**  Draw messages
*/
void DrawMessages(std::vector<std::function<void(renderer *)>> &render_commands)
{
	allmessages.DrawMessages(render_commands);
}

/**
**  Set message to display.
**
**  @param fmt  To be displayed in text overlay.
*/
void SetMessage(const char *fmt, ...)
{
	char temp[512];
	va_list va;

	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	temp[sizeof(temp) - 1] = '\0';
	va_end(va);
	allmessages.AddUniqueMessage(temp);
}

/**
**  Shift messages events array by one.
*/
void ShiftMessagesEvent()
{
	if (MessagesEventCount) {
		--MessagesEventCount;
		for (int z = 0; z < MessagesEventCount; ++z) {
			MessagesEventPos[z] = MessagesEventPos[z + 1];
			strcpy_s(MessagesEvent[z], sizeof(MessagesEvent[z]), MessagesEvent[z + 1]);
		}
	}
}

/**
**  Set message to display.
**
**  @param pos    Message pos map origin.
**  @param fmt  To be displayed in text overlay.
**
**  @note FIXME: vladi: I know this can be just separated func w/o msg but
**               it is handy to stick all in one call, someone?
*/
void SetMessageEvent(const Vec2i &pos, int z, const char *fmt, ...)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	char temp[256];
	va_list va;

	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	temp[sizeof(temp) - 1] = '\0';
	va_end(va);
	allmessages.AddUniqueMessage(temp);

	if (MessagesEventCount == MESSAGES_MAX) {
		ShiftMessagesEvent();
	}

	strcpy_s(MessagesEvent[MessagesEventCount], sizeof(MessagesEvent[MessagesEventCount]), temp);
	MessagesEventPos[MessagesEventCount] = pos;
	MessagesEventIndex = MessagesEventCount;
	++MessagesEventCount;
}

//Wyrmgus start
/**
**  Set objective to display.
**
**  @param fmt  To be displayed in text overlay.
*/
void SetObjective(const char *fmt, ...)
{
	char temp[512];
	va_list va;

	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	temp[sizeof(temp) - 1] = '\0';
	va_end(va);
	allmessages.AddObjective(temp);
}
//Wyrmgus end

/**
**  Goto message origin.
*/
void CenterOnMessage()
{
	if (MessagesEventIndex >= MessagesEventCount) {
		MessagesEventIndex = 0;
	}
	if (MessagesEventCount == 0) {
		return;
	}
	const Vec2i &pos(MessagesEventPos[MessagesEventIndex]);
	UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(pos));
	SetMessage(_("~<Event: %s~>"), MessagesEvent[MessagesEventIndex]);
	++MessagesEventIndex;
}

void ToggleShowMessages()
{
	allmessages.ToggleShowMessages();
}

#ifdef DEBUG
void ToggleShowBuilListMessages()
{
	allmessages.ToggleShowBuilListMessages();
}
#endif

/*----------------------------------------------------------------------------
--  INFO PANEL
----------------------------------------------------------------------------*/

/**
**  Draw info panel background.
**
**  @param frame  frame nr. of the info panel background.
*/
static void DrawInfoPanelBackground(unsigned frame, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (UI.InfoPanel.G) {
		UI.InfoPanel.G->DrawFrame(frame, UI.InfoPanel.X, UI.InfoPanel.Y, render_commands);
	}
}

static void InfoPanel_draw_no_selection(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	DrawInfoPanelBackground(0, render_commands);
	if (UnitUnderCursor && UnitUnderCursor->IsVisible(*CPlayer::GetThisPlayer())
		&& !UnitUnderCursor->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) {
		// FIXME: not correct for enemies units
		DrawUnitInfo(*UnitUnderCursor, render_commands);
	} else {
		// FIXME: need some cool ideas for this.
		int x = UI.InfoPanel.X + (16 * scale_factor).to_int();
		int y = UI.InfoPanel.Y + (8 * scale_factor).to_int();

		CLabel label(defines::get()->get_game_font());
		y += (16 * scale_factor).to_int();
		label.Draw(x, y,  _("Cycle:"), render_commands);
		label.Draw(x + (48 * scale_factor).to_int(), y, GameCycle, render_commands);
		//Wyrmgus start
//		label.Draw(x + 110, y, CYCLES_PER_SECOND * VideoSyncSpeed / 100);
		label.Draw(x + (110 * scale_factor).to_int(), y, _("Speed:"), render_commands);
		label.Draw(x + ((110 + 53) * scale_factor).to_int(), y, preferences::get()->get_game_speed(), render_commands);
		//Wyrmgus end
		y += (20 * scale_factor).to_int();

		std::vector<const CPlayer *> listed_players;

		for (int i = 0; i < PlayerMax - 1; ++i) {
			const CPlayer *player = CPlayer::Players[i].get();
			if (player->get_type() != player_type::nobody && !player->has_neutral_faction_type() && CPlayer::GetThisPlayer()->HasContactWith(*player) && player->GetUnitCount() > 0) {
				listed_players.push_back(player);
			}
		}

		//sort players by order of distance of their start position to the main player's
		std::sort(listed_players.begin(), listed_players.end(), [](const CPlayer *a, const CPlayer *b) {
			return wyrmgus::point::distance_to(CPlayer::GetThisPlayer()->StartPos, a->StartPos) < wyrmgus::point::distance_to(CPlayer::GetThisPlayer()->StartPos, b->StartPos);
		});

		for (const CPlayer *player : listed_players) {
			if (player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->is_allied_with(*player)) {
				label.SetNormalColor(wyrmgus::defines::get()->get_ally_font_color());
			} else if (CPlayer::GetThisPlayer()->is_enemy_of(*player)) {
				label.SetNormalColor(wyrmgus::defines::get()->get_enemy_font_color());
			} else {
				label.SetNormalColor(wyrmgus::defines::get()->get_default_font_color());
			}

			Video.DrawRectangleClip(ColorWhite, x, y, (12 * scale_factor).to_int(), (12 * scale_factor).to_int(), render_commands);
			Video.FillRectangleClip(CVideo::MapRGB(player->get_minimap_color()), x + 1, y + 1, (12 * scale_factor).to_int() - 2, (12 * scale_factor).to_int() - 2, render_commands);

			label.Draw(x + (15 * scale_factor).to_int(), y, _(player->get_full_name().c_str()), render_commands);
			y += (14 * scale_factor).to_int();

			if ((y + (12 * scale_factor).to_int()) > Video.Height) { // if the square would overflow the screen, don't draw the player
				break;
			}
		}
	}
}

static void InfoPanel_draw_single_selection(CUnit *selUnit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CUnit &unit = (selUnit ? *selUnit : *Selected[0]);
	int panelIndex;

	// FIXME: not correct for enemy's units
	if (unit.Player == CPlayer::GetThisPlayer()
		|| CPlayer::GetThisPlayer()->IsTeamed(unit)
		|| CPlayer::GetThisPlayer()->is_allied_with(unit)
		|| ReplayRevealMap) {
		if (unit.Orders[0]->Action == UnitAction::Built
			|| unit.Orders[0]->Action == UnitAction::Research
			|| unit.Orders[0]->Action == UnitAction::UpgradeTo
			|| unit.Orders[0]->Action == UnitAction::Train) {
			panelIndex = 3;
		//Wyrmgus start
//		} else if (unit.Stats->Variables[MANA_INDEX].Max) {
		} else if (unit.GetModifiedVariable(MANA_INDEX, VariableAttribute::Max)) {
		//Wyrmgus end
			panelIndex = 2;
		} else {
			panelIndex = 1;
		}
	} else {
		panelIndex = 0;
	}
	DrawInfoPanelBackground(panelIndex, render_commands);
	//Wyrmgus start
	//draw icon panel frame, if any
	if (
		wyrmgus::defines::get()->get_infopanel_frame_graphics() != nullptr
		&& (unit.CurrentAction() != UnitAction::Train || static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0) //don't stop showing the info panel frame for a quick moment if the time cost is 0
		&& (unit.CurrentAction() != UnitAction::UpgradeTo || static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0)
		&& (unit.CurrentAction() != UnitAction::Research || static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().get_time_cost() == 0)
		&& unit.CurrentAction() != UnitAction::Built
		&& !unit.is_enemy_of(*CPlayer::GetThisPlayer())
		&& (unit.Player->get_type() != player_type::neutral || unit.Type->get_given_resource() != nullptr)
	) {
		defines::get()->get_infopanel_frame_graphics()->DrawClip(UI.InfoPanel.X - (4 * preferences::get()->get_scale_factor()).to_int(), UI.InfoPanel.Y + (93 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}
	//Wyrmgus end	
	DrawUnitInfo(unit, render_commands);
	//Wyrmgus start
	/*
	if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) {
		//Wyrmgus start
//		UI.StatusLine.Set(unit.Type->Name);
		
		//hackish way to make the popup appear correctly for the single selected unit
		wyrmgus::button ba;
		ba.Hint = unit.GetMessageName();
		ba.Action = ButtonUnit;
		ba.Value = UnitNumber(unit);
		ba.Popup = "popup_unit";
		DrawPopup(ba, UI.SingleSelectedButton->X, UI.SingleSelectedButton->Y);
		LastDrawnButtonPopup = nullptr;
		//Wyrmgus end
	}
	*/
	//Wyrmgus end
}

static void InfoPanel_draw_multiple_selection(std::vector<std::function<void(renderer *)>> &render_commands)
{
	//  If there are more units selected draw their pictures and a health bar
	DrawInfoPanelBackground(0, render_commands);
	for (size_t i = 0; i != std::min(Selected.size(), UI.SelectedButtons.size()); ++i) {
		//Wyrmgus start
//		const wyrmgus::icon &icon = *Selected[i]->Type->Icon.Icon;
		//Wyrmgus end
		const PixelPos pos(UI.SelectedButtons[i].X, UI.SelectedButtons[i].Y);
		//Wyrmgus start
//		icon.DrawUnitIcon(*UI.SelectedButtons[i].Style,
		Selected[i]->get_icon()->DrawUnitIcon(*UI.SelectedButtons[i].Style,
		//Wyrmgus end
						  (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == (int)i) ?
						  (IconActive | (MouseButtons & LeftButton)) : 0,
						  pos, "", Selected[i]->get_player_color(), render_commands);

		UiDrawLifeBar(*Selected[i], UI.SelectedButtons[i].X, UI.SelectedButtons[i].Y, render_commands);

		if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == (int) i) {
			const wyrmgus::font_color *text_color = nullptr;
			if (Selected[i]->get_unique() != nullptr || Selected[i]->get_character() != nullptr) {
				text_color = wyrmgus::defines::get()->get_unique_font_color();
			} else if (Selected[i]->Prefix != nullptr || Selected[i]->Suffix != nullptr) {
				text_color = wyrmgus::defines::get()->get_magic_font_color();
			}
			DrawGenericPopup(Selected[i]->GetMessageName(), UI.SelectedButtons[i].X, UI.SelectedButtons[i].Y, text_color, nullptr, render_commands);
			//Wyrmgus end
		}
	}

	if (Selected.size() > UI.SelectedButtons.size()) {
		const std::string number_str = "+" + std::to_string(Selected.size() - UI.SelectedButtons.size());
		CLabel(UI.MaxSelectedFont).Draw(UI.MaxSelectedTextX, UI.MaxSelectedTextY, number_str.c_str(), render_commands);
	}
}

/**
**  Draw info panel.
**
**  Panel:
**    neutral      - neutral or opponent
**    normal       - not 1,3,4
**    magic unit   - magic units
**    construction - under construction
*/
void CInfoPanel::Draw(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (UnitUnderCursor && Selected.empty() && !UnitUnderCursor->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value
		&& (ReplayRevealMap || UnitUnderCursor->IsVisible(*CPlayer::GetThisPlayer()))) {
			InfoPanel_draw_single_selection(UnitUnderCursor, render_commands);
	} else {
		switch (Selected.size()) {
			case 0: { InfoPanel_draw_no_selection(render_commands); break; }
			case 1: { InfoPanel_draw_single_selection(nullptr, render_commands); break; }
			default: { InfoPanel_draw_multiple_selection(render_commands); break; }
		}
	}
}
