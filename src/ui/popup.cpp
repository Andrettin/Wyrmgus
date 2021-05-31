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
/**@name popup.cpp - The popup globals. */
//
//      (c) Copyright 2012-2021 by cybermind, Joris Dauphin and Andrettin
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

#include "ui/popup.h"

#include "database/defines.h"
#include "faction.h"
#include "item/item_class.h"
#include "player.h"
#include "script.h"
#include "script/condition/condition.h"
#include "script/trigger.h"
#include "spell/spell.h"
//Wyrmgus start
#include "translate.h"
//Wyrmgus end
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
//Wyrmgus start
#include "unit/unit.h"
//Wyrmgus end
#include "unit/unit_class.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"

PopupConditionPanel::PopupConditionPanel() : ButtonAction(ButtonCmd::None), item_class(wyrmgus::item_class::none)
{
}

int CPopupContentTypeButtonInfo::GetWidth(const wyrmgus::button &button, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	std::string draw("");
	switch (this->InfoType) {
		case PopupButtonInfo_Hint:
			draw = button.get_hint();
			break;
		case PopupButtonInfo_Description:
			draw = button.Description;
			break;
		case PopupButtonInfo_Conditions:
			draw = PrintConditions(button);
			break;
	}
	int width = 0;
	std::string sub;
	if (draw.length()) {
		//Wyrmgus start
		/*
		if (this->MaxWidth) {
			return std::min((unsigned int)font.getWidth(draw), this->MaxWidth);
		}
		*/
		//Wyrmgus end
		int i = 1;
		while (!(sub = GetLineFont(i++, draw, 0, font)).empty()) {
			width = std::max(width, font->getWidth(sub));
		}
		//Wyrmgus start
		if (this->MaxWidth) {
			width = std::min((unsigned int) width, this->MaxWidth);
		}
		//Wyrmgus end
	}
	return width;
}

int CPopupContentTypeButtonInfo::GetHeight(const wyrmgus::button &button, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	std::string draw;
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	switch (this->InfoType) {
		case PopupButtonInfo_Hint:
			draw = button.get_hint();
			break;
		case PopupButtonInfo_Description:
			draw = button.Description;
			break;
		case PopupButtonInfo_Conditions:
			draw = PrintConditions(button);
			break;
	}
	int height = 0;
	if (draw.length()) {
		int i = 1;
		while ((GetLineFont(i++, draw, this->MaxWidth, font)).length()) {
			height += font->Height() + 2 * scale_factor;
		}
	}
	return height;
}

void CPopupContentTypeButtonInfo::Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	CLabel label(font, this->TextColor, this->HighlightColor);
	std::string draw("");
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	switch (this->InfoType) {
		case PopupButtonInfo_Hint:
			draw = _(button.get_hint().c_str());
			break;
		case PopupButtonInfo_Description:
			draw = button.Description;
			break;
		case PopupButtonInfo_Conditions:
			draw = PrintConditions(button);
			break;
	}
	std::string sub(draw);
	if (draw.length()) {
		int i = 0;
		int y_off = y;
		unsigned int width = this->MaxWidth
							 ? std::min(this->MaxWidth, popupWidth - 2 * popup.MarginX)
							 : 0;
		while ((sub = GetLineFont(++i, draw, width, font)).length()) {
			label.Draw(x, y_off, sub, render_commands);
			y_off += font->Height() + 2 * scale_factor;
		}
		return;
	}
}

void CPopupContentTypeButtonInfo::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1));

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "InfoType")) {
			std::string temp(LuaToString(l, -1));
			if (temp == "Hint") {
				this->InfoType = PopupButtonInfo_Hint;
			} else if (temp == "Description") {
				this->InfoType = PopupButtonInfo_Description;
			} else if (temp == "Conditions") {
				this->InfoType = PopupButtonInfo_Conditions;
			}
		} else if (!strcmp(key, "MaxWidth")) {
			this->MaxWidth = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Font")) {
			this->Font = wyrmgus::font::get(LuaToString(l, -1));
		} else {
			LuaError(l, "'%s' invalid for method 'Name' in DefinePopups" _C_ key);
		}
	}
}

int CPopupContentTypeText::GetWidth(const wyrmgus::button &button, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	//Wyrmgus start
	button.SetTriggerData();
	const int resource = button.Value;
	if (button.Action == ButtonCmd::ProduceResource || button.Action == ButtonCmd::SellResource || button.Action == ButtonCmd::BuyResource) {
		TriggerData.resource = wyrmgus::resource::get_all()[resource];
	}
	const std::string text = EvalString(this->Text.get());
	button.CleanTriggerData();
	//Wyrmgus end
	
	//Wyrmgus start
	/*
	if (this->MaxWidth) {
		//Wyrmgus start
//		return std::min((unsigned int)font.getWidth(this->Text), this->MaxWidth);
		return std::min((unsigned int)font.getWidth(text), this->MaxWidth);
		//Wyrmgus end
	}
	*/
	//Wyrmgus end
	int width = 0;
	std::string sub;
	int i = 1;
	//Wyrmgus start
//	while (!(sub = GetLineFont(i++, this->Text, 0, &font)).empty()) {
	while (!(sub = GetLineFont(i++, text, 0, font)).empty()) {
	//Wyrmgus end
		width = std::max(width, font->getWidth(sub));
	}
	//Wyrmgus start
	if (this->MaxWidth) {
		width = std::min((unsigned int) width, this->MaxWidth);
	}
	//Wyrmgus end
	return width;
}

int CPopupContentTypeText::GetHeight(const wyrmgus::button &button, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	//Wyrmgus start
	button.SetTriggerData();
	const int resource = button.Value;
	if (button.Action == ButtonCmd::ProduceResource || button.Action == ButtonCmd::SellResource || button.Action == ButtonCmd::BuyResource) {
		TriggerData.resource = wyrmgus::resource::get_all()[resource];
	}
	const std::string text = EvalString(this->Text.get());
	button.CleanTriggerData();
	//Wyrmgus end
	int height = 0;
	int i = 1;
	//Wyrmgus start
//	while ((GetLineFont(i++, this->Text, this->MaxWidth, &font)).length()) {
	while ((GetLineFont(i++, text, this->MaxWidth, font)).length()) {
	//Wyrmgus end
		height += font->Height() + 2 * scale_factor;
	}
	return height;
}

CPopupContentTypeText::~CPopupContentTypeText()
{
}

void CPopupContentTypeText::Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	//Wyrmgus start
	button.SetTriggerData();
	const int resource = button.Value;
	if (button.Action == ButtonCmd::ProduceResource || button.Action == ButtonCmd::SellResource || button.Action == ButtonCmd::BuyResource) {
		TriggerData.resource = wyrmgus::resource::get_all()[resource];
	}
	const std::string text = EvalString(this->Text.get());
	button.CleanTriggerData();
	//Wyrmgus end
	CLabel label(font, this->TextColor, this->HighlightColor);
	std::string sub;
	int i = 0;
	int y_off = y;
	unsigned int width = this->MaxWidth
						 ? std::min(this->MaxWidth, popupWidth - 2 * popup.MarginX)
						 : 0;
	//Wyrmgus start
//	while ((sub = GetLineFont(++i, this->Text, width, &font)).length()) {
	while ((sub = GetLineFont(++i, text, width, font)).length()) {
	//Wyrmgus end
		label.Draw(x, y_off, sub, render_commands);
		y_off += font->Height() + 2 * scale_factor;
	}
}

void CPopupContentTypeText::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1));

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Text")) {
			//Wyrmgus start
//			this->Text = LuaToString(l, -1);
			this->Text = CclParseStringDesc(l);
			lua_pushnil(l); // ParseStringDesc eat token
			//Wyrmgus end
		} else if (!strcmp(key, "MaxWidth")) {
			this->MaxWidth = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Font")) {
			this->Font = wyrmgus::font::get(LuaToString(l, -1));
		} else {
			LuaError(l, "'%s' invalid for method 'Text' in DefinePopups" _C_ key);
		}
	}
}

int CPopupContentTypeCosts::GetWidth(const wyrmgus::button &button, int *Costs) const
{
	int popupWidth = 0;
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	for (unsigned int i = 1; i <= MaxCosts; ++i) {
		if (Costs[i]) {
			if (UI.Resources[i].IconWidth != -1)	{
				popupWidth += (UI.Resources[i].IconWidth + 5 * scale_factor);
			} else {
				const wyrmgus::resource_icon *icon = nullptr;
				if (i >= MaxCosts) {
					switch (i) {
						case FoodCost:
							icon = wyrmgus::defines::get()->get_food_icon();
							break;
						case ScoreCost:
							icon = wyrmgus::defines::get()->get_score_icon();
							break;
						case ManaResCost:
							icon = wyrmgus::defines::get()->get_mana_icon();
							break;
						default:
							break;
					}
				} else {
					const wyrmgus::resource *resource = wyrmgus::resource::get_all()[i];
					icon = resource->get_icon();
				}

				if (icon != nullptr) {
					popupWidth += (icon->get_graphics()->Width + 5 * scale_factor);
				}
			}
			popupWidth += (font->Width(Costs[i]) + 5 * scale_factor);
		}
	}

	if (Costs[ManaResCost]) {
		const wyrmgus::resource_icon *icon = wyrmgus::defines::get()->get_mana_icon();
		const wyrmgus::spell *spell = wyrmgus::spell::get_all()[button.Value];

		if (spell->get_mana_cost()) {
			popupWidth = 10;
			if (UI.Resources[ManaResCost].IconWidth != -1) {
				popupWidth += (UI.Resources[ManaResCost].IconWidth + 5 * scale_factor);
			} else {
				if (icon != nullptr) {
					popupWidth += (icon->get_graphics()->Width + 5 * scale_factor);
				}
			}
			popupWidth += font->Width(spell->get_mana_cost());
			popupWidth = std::max<int>(popupWidth, font->Width(spell->get_name()) + 10 * scale_factor);
		} else {
			popupWidth = font->Width(button.get_hint()) + 10 * scale_factor;
		}
		popupWidth = std::max<int>(popupWidth, 100 * scale_factor);
	}
	return popupWidth;
}

int CPopupContentTypeCosts::GetHeight(const wyrmgus::button &button, int *Costs) const
{
	Q_UNUSED(button)

	int popupHeight = 0;
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();

	for (unsigned int i = 1; i <= ManaResCost; ++i) {
		const wyrmgus::resource_icon *icon = nullptr;
		if (i >= MaxCosts) {
			switch (i) {
				case FoodCost:
					icon = wyrmgus::defines::get()->get_food_icon();
					break;
				case ScoreCost:
					icon = wyrmgus::defines::get()->get_score_icon();
					break;
				case ManaResCost:
					icon = wyrmgus::defines::get()->get_mana_icon();
					break;
				default:
					break;
			}
		} else {
			const wyrmgus::resource *resource = wyrmgus::resource::get_all()[i];
			icon = resource->get_icon();
		}

		if (Costs[i] && icon != nullptr) {
			popupHeight = std::max(icon->get_graphics()->Height, popupHeight);
		}
	}
	return std::max(popupHeight, font->Height());
}

void CPopupContentTypeCosts::Draw(int x, int y, const CPopup &, const unsigned int, const wyrmgus::button &button, int *Costs, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	CLabel label(font, this->TextColor, this->HighlightColor);
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	for (unsigned int i = 1; i <= MaxCosts; ++i) {
		if (Costs[i]) {
			int y_offset = 0;

			const wyrmgus::resource_icon *icon = nullptr;
			if (i >= MaxCosts) {
				switch (i) {
					case FoodCost:
						icon = wyrmgus::defines::get()->get_food_icon();
						break;
					case ScoreCost:
						icon = wyrmgus::defines::get()->get_score_icon();
						break;
					case ManaResCost:
						icon = wyrmgus::defines::get()->get_mana_icon();
						break;
					default:
						break;
				}
			} else {
				const wyrmgus::resource *resource = wyrmgus::resource::get_all()[i];
				icon = resource->get_icon();
			}

			if (icon != nullptr) {
				int x_offset = UI.Resources[i].IconWidth;
				const std::shared_ptr<CGraphic> &icon_graphics = icon->get_graphics();
				icon_graphics->DrawFrameClip(icon->get_frame(),	x , y, render_commands);
				x += ((x_offset != -1 ? x_offset : icon_graphics->Width) + 5 * scale_factor);
				y_offset = icon_graphics->Height;
				y_offset -= label.Height();
				y_offset /= 2;
			}
			x += label.Draw(x, y + y_offset, Costs[i], render_commands);
			x += 5 * scale_factor;
		}
	}

	if (Costs[ManaResCost]) {
		const wyrmgus::spell &spell = *wyrmgus::spell::get_all()[button.Value];
		const wyrmgus::resource_icon *icon = wyrmgus::defines::get()->get_mana_icon();
		if (spell.get_mana_cost() != 0) {
			int y_offset = 0;
			if (icon != nullptr) {
				int x_offset =  UI.Resources[ManaResCost].IconWidth;
				x += 5 * scale_factor;
				const std::shared_ptr<CGraphic> &icon_graphics = icon->get_graphics();
				icon_graphics->DrawFrameClip(icon->get_frame(), x, y, render_commands);
				x += ((x_offset != -1 ? x_offset : icon_graphics->Width) + 5 * scale_factor);
				y_offset = icon_graphics->Height;
				y_offset -= font->Height();
				y_offset /= 2;
			}
			label.Draw(x, y + y_offset, spell.get_mana_cost(), render_commands);
		}
	}
}

void CPopupContentTypeCosts::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1) || lua_isnil(l, -1));

	if (!lua_isnil(l, -1)) {
		for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);
			if (!strcmp(key, "Font")) {
				this->Font = wyrmgus::font::get(LuaToString(l, -1));
			} else if (!strcmp(key, "Centered")) {
				this->Centered = LuaToBoolean(l, -1);
			} else {
				LuaError(l, "'%s' invalid for method 'Costs' in DefinePopups" _C_ key);
			}
		}
	}
}

CPopupContentTypeLine::CPopupContentTypeLine() : Color(ColorWhite)
{
}

int CPopupContentTypeLine::GetWidth(const wyrmgus::button &button, int *Costs) const
{
	Q_UNUSED(button)
	Q_UNUSED(Costs)

	return this->Width;
}

int CPopupContentTypeLine::GetHeight(const wyrmgus::button &button, int *Costs) const
{
	Q_UNUSED(button)
	Q_UNUSED(Costs)

	return this->Height;
}

void CPopupContentTypeLine::Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	Q_UNUSED(button)
	Q_UNUSED(Costs)

	Video.FillRectangle(this->Color, x - popup.MarginX - this->MarginX + 1,
						y, this->Width && Width < popupWidth ? Width : popupWidth - 2, Height, render_commands);
}

void CPopupContentTypeLine::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1) || lua_isnil(l, -1));

	if (!lua_isnil(l, -1)) {
		for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);
			if (!strcmp(key, "Width")) {
				this->Width = LuaToNumber(l, -1);
			} else if (!strcmp(key, "Height")) {
				this->Height = LuaToNumber(l, -1);
			} else if (!strcmp(key, "Color")) {
				this->Color = LuaToUnsignedNumber(l, -1);
			} else {
				LuaError(l, "'%s' invalid for method 'Costs' in DefinePopups" _C_ key);
			}
		}
	}
}

int CPopupContentTypeVariable::GetWidth(const wyrmgus::button &button, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	//Wyrmgus start
//	TriggerData.Type = UnitTypes[button.Value];
	button.SetTriggerData();
	const int resource = button.Value;
	if (button.Action == ButtonCmd::ProduceResource || button.Action == ButtonCmd::SellResource || button.Action == ButtonCmd::BuyResource) {
		TriggerData.resource = wyrmgus::resource::get_all()[resource];
	}
	//Wyrmgus end
	const std::string text = EvalString(this->Text.get());
	//Wyrmgus start
//	TriggerData.Type = nullptr;
	button.CleanTriggerData();
	//Wyrmgus end
	return font->getWidth(text);
}

int CPopupContentTypeVariable::GetHeight(const wyrmgus::button &, int *) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font();
	return font->Height();
}

CPopupContentTypeVariable::~CPopupContentTypeVariable()
{
}

void CPopupContentTypeVariable::Draw(int x, int y, const CPopup &, const unsigned int, const wyrmgus::button &button, int *, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	wyrmgus::font *font = this->Font ? this->Font : wyrmgus::defines::get()->get_small_font(); // Font to use.

	Assert(this->Index == -1 || ((unsigned int) this->Index < UnitTypeVar.GetNumberVariable()));

	CLabel label(font, this->TextColor, this->HighlightColor);

	if (this->Text) {
		button.SetTriggerData();
		const int resource = button.Value;
		if (button.Action == ButtonCmd::ProduceResource || button.Action == ButtonCmd::SellResource || button.Action == ButtonCmd::BuyResource) {
			TriggerData.resource = wyrmgus::resource::get_all()[resource];
		}
		const std::string text = EvalString(this->Text.get());
		button.CleanTriggerData();
		if (this->Centered) {
			x += (label.DrawCentered(x, y, text, render_commands) * 2);
		} else {
			x += label.Draw(x, y, text, render_commands);
		}
	}

	if (this->Index != -1) {
		int value;

		const wyrmgus::unit_class *unit_class = nullptr;
		const wyrmgus::unit_type *unit_type = nullptr;
		switch (button.Action) {
			case ButtonCmd::TrainClass:
			case ButtonCmd::BuildClass:
			case ButtonCmd::UpgradeToClass:
				unit_class = wyrmgus::unit_class::get_all()[button.Value];
				if (Selected[0]->Player->get_faction() != nullptr) {
					unit_type = Selected[0]->Player->get_faction()->get_class_unit_type(unit_class);
				}
				break;
			case ButtonCmd::Unit:
			case ButtonCmd::Buy:
				unit_type = wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Type;
				break;
			default:
				unit_type = wyrmgus::unit_type::get_all()[button.Value];
				break;
		}

		switch (button.Action) {
			case ButtonCmd::Unit:
			case ButtonCmd::Buy:
				if (
					unit_type->BoolFlag[ITEM_INDEX].value
					&& this->Index != HITPOINTHEALING_INDEX
					&& this->Index != MANA_RESTORATION_INDEX
					&& unit_manager::get()->GetSlotUnit(button.Value).Container
					&& (unit_manager::get()->GetSlotUnit(button.Value).Container->can_equip_item(&unit_manager::get()->GetSlotUnit(button.Value)) || unit_manager::get()->GetSlotUnit(button.Value).Work != nullptr || unit_manager::get()->GetSlotUnit(button.Value).Elixir != nullptr)
					) {
					value = unit_manager::get()->GetSlotUnit(button.Value).Container->GetItemVariableChange(&unit_manager::get()->GetSlotUnit(button.Value), this->Index);
					if (value >= 0) {
						x += label.Draw(x, y, "+", render_commands);
					}
				} else if (wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Work != nullptr || wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Elixir != nullptr) {
					value = wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).GetItemVariableChange(&wyrmgus::unit_manager::get()->GetSlotUnit(button.Value), this->Index);
					if (value >= 0) {
						x += label.Draw(x, y, "+", render_commands);
					}
				} else {
					value = wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Variable[this->Index].Value;
					if (
						(unit_type->BoolFlag[ITEM_INDEX].value && button.Action == ButtonCmd::Buy)
						|| IsBonusVariable(this->Index)
						) {
						if (value >= 0) {
							x += label.Draw(x, y, "+", render_commands);
						}
					}
				}
				break;
			default:
				value = unit_type->Stats[CPlayer::GetThisPlayer()->Index].Variables[this->Index].Value;
				if (value >= 0 && IsBonusVariable(this->Index)) {
					x += label.Draw(x, y, "+", render_commands);
				}
				break;
		}

		x += label.Draw(x, y, value, render_commands);
		if (IsPercentageVariable(this->Index)) {
			x += label.Draw(x, y, "%", render_commands);
		}
		//Wyrmgus end
	}
}

void CPopupContentTypeVariable::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1) || lua_isstring(l, -1));

	if (lua_isstring(l, -1)) {
		this->Text = CclParseStringDesc(l);
		lua_pushnil(l); // ParseStringDesc eat token
	} else {
		for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);
			if (!strcmp(key, "Text")) {
				this->Text = CclParseStringDesc(l);
				lua_pushnil(l); // ParseStringDesc eat token
			} else if (!strcmp(key, "Font")) {
				this->Font = wyrmgus::font::get(LuaToString(l, -1));
			} else if (!strcmp(key, "Centered")) {
				this->Centered = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "Variable")) {
				const char *const name = LuaToString(l, -1);
				this->Index = UnitTypeVar.VariableNameLookup[name];
				if (this->Index == -1) {
					LuaError(l, "unknown variable '%s'" _C_ LuaToString(l, -1));
				}
			} else {
				LuaError(l, "'%s' invalid for method 'Text' in DefinePopups" _C_ key);
			}
		}
	}
}

/**
**  Parse the popup conditions.
**
**  @param l   Lua State.
*/
static std::unique_ptr<PopupConditionPanel> ParsePopupConditions(lua_State *l)
{
	Assert(lua_istable(l, -1));

	auto condition = std::make_unique<PopupConditionPanel>();
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "HasHint")) {
			condition->HasHint = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "HasDescription")) {
			condition->HasDescription = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "HasConditions")) {
			condition->HasConditions = LuaToBoolean(l, -1);
		//Wyrmgus start
		} else if (!strcmp(key, "Class")) {
			condition->Class = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "Description")) {
			condition->Description = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "Quote")) {
			condition->Quote = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "Encyclopedia")) {
			condition->Encyclopedia = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "SettlementName")) {
			condition->settlement_name = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "CanActiveHarvest")) {
			condition->CanActiveHarvest = LuaToBoolean(l, -1);
		//Wyrmgus end
		} else if (!strcmp(key, "ButtonValue")) {
			condition->ButtonValue = LuaToString(l, -1);
		} else if (!strcmp(key, "ButtonAction")) {
			std::string value = LuaToString(l, -1);
			ButtonCmd button_action_id = GetButtonActionIdByName(value);
			if (button_action_id != ButtonCmd::None) {
				condition->ButtonAction = button_action_id;
			} else {
				LuaError(l, "Unsupported button action: %s" _C_ value.c_str());
			}
			//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(key, "Opponent")) {
			condition->Opponent = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Neutral")) {
			condition->Neutral = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "AutoCast")) {
			condition->AutoCast = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Equipped")) {
			condition->Equipped = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Equippable")) {
			condition->Equippable = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Consumable")) {
			condition->Consumable = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Affixed")) {
			condition->Affixed = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Spell")) {
			condition->Spell = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "CanUse")) {
			condition->CanUse = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Work")) {
			condition->Work = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ReadWork")) {
			condition->ReadWork = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Elixir")) {
			condition->Elixir = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ConsumedElixir")) {
			condition->ConsumedElixir = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Unique")) {
			condition->Unique = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "UniqueSet")) {
			condition->UniqueSet = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Bound")) {
			condition->Bound = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Identified")) {
			condition->Identified = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "UnitTypeType")) {
			const char *unit_type_type = LuaToString(l, -1);
			if (!strcmp(unit_type_type, "land")) {
				condition->UnitTypeType = UnitTypeType::Land;
			} else if (!strcmp(unit_type_type, "fly")) {
				condition->UnitTypeType = UnitTypeType::Fly;
			} else if (!strcmp(unit_type_type, "fly-low")) {
				condition->UnitTypeType = UnitTypeType::FlyLow;
			} else if (!strcmp(unit_type_type, "naval")) {
				condition->UnitTypeType = UnitTypeType::Naval;
			} else if (!strcmp(unit_type_type, "space")) {
				condition->UnitTypeType = UnitTypeType::Space;
			} else {
				LuaError(l, "Unsupported Type: %s" _C_ unit_type_type);
			}
		} else if (!strcmp(key, "UnitTypeClass")) {
			condition->unit_class = wyrmgus::unit_class::get(LuaToString(l, -1));
		} else if (!strcmp(key, "ItemClass")) {
			condition->item_class = wyrmgus::string_to_item_class(LuaToString(l, -1));
		} else if (!strcmp(key, "CanStore")) {
			condition->CanStore = GetResourceIdByName(LuaToString(l, -1));
		} else if (!strcmp(key, "ImproveIncome")) {
			condition->ImproveIncome = GetResourceIdByName(LuaToString(l, -1));
		} else if (!strcmp(key, "Weapon")) {
			condition->Weapon = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Shield")) {
			condition->Shield = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Boots")) {
			condition->Boots = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Arrows")) {
			condition->Arrows = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Regeneration")) {
			condition->Regeneration = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "FactionUpgrade")) {
			condition->FactionUpgrade = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "DynastyUpgrade")) {
			condition->DynastyUpgrade = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "FactionCoreSettlements")) {
			condition->FactionCoreSettlements = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ResearchedUpgrade")) {
			condition->ResearchedUpgrade = CUpgrade::get(LuaToString(l, -1));
		} else if (!strcmp(key, "ResearchedUpgradeClass")) {
			condition->researched_upgrade_class = wyrmgus::upgrade_class::get(LuaToString(l, -1));
		} else if (!strcmp(key, "UpgradeResearched")) {
			condition->UpgradeResearched = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Ability")) {
			condition->Ability = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ChildResources")) {
			condition->ChildResources = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ImproveIncomes")) {
			condition->ImproveIncomes = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "LuxuryResource")) {
			condition->LuxuryResource = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "RequirementsString")) {
			condition->RequirementsString = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "ExperienceRequirementsString")) {
			condition->ExperienceRequirementsString = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "BuildingRulesString")) {
			condition->BuildingRulesString = Ccl2Condition(l, LuaToString(l, -1));
		//Wyrmgus end
		} else if (!strcmp(key, "Overlord")) {
			condition->Overlord = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "TopOverlord")) {
			condition->TopOverlord = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "terrain_feature")) {
			condition->terrain_feature = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "world")) {
			condition->world = Ccl2Condition(l, LuaToString(l, -1));
		} else {
			int index = UnitTypeVar.BoolFlagNameLookup[key];
			if (index != -1) {
				if (!condition->BoolFlags) {
					const size_t new_bool_size = UnitTypeVar.GetNumberBoolFlag();
					condition->BoolFlags = std::make_unique<char[]>(new_bool_size);
					memset(condition->BoolFlags.get(), 0, new_bool_size * sizeof(char));
				}
				condition->BoolFlags[index] = Ccl2Condition(l, LuaToString(l, -1));
				continue;
			}
			index = UnitTypeVar.VariableNameLookup[key];
			if (index != -1) {
				if (!condition->Variables) {
					const size_t new_variables_size = UnitTypeVar.GetNumberVariable();
					condition->Variables = std::make_unique<char[]>(new_variables_size);
					memset(condition->Variables.get(), 0, new_variables_size * sizeof(char));
				}
				condition->Variables[index] = Ccl2Condition(l, LuaToString(l, -1));
				continue;
			}
			LuaError(l, "'%s' invalid for Condition in DefinePopups" _C_ key);
		}
	}
	return condition;
}

std::unique_ptr<CPopupContentType> CPopupContentType::ParsePopupContent(lua_State *l)
{
	Assert(lua_istable(l, -1));

	bool wrap = true;
	int marginX = MARGIN_X * wyrmgus::defines::get()->get_scale_factor();
	int marginY = MARGIN_Y * wyrmgus::defines::get()->get_scale_factor();
	int minWidth = 0;
	int minHeight = 0;
	std::string textColor("white");
	std::string highColor("red");
	std::unique_ptr<CPopupContentType> content;
	std::unique_ptr<PopupConditionPanel> condition;

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Wrap")) {
			wrap = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "TextColor")) {
			textColor = LuaToString(l, -1);
		} else if (!strcmp(key, "HighlightColor")) {
			highColor = LuaToString(l, -1);
		} else if (!strcmp(key, "Margin")) {
			CclGetPos(l, &marginX, &marginY);
		} else if (!strcmp(key, "MinWidth")) {
			minWidth = LuaToNumber(l, -1);
		} else if (!strcmp(key, "MinHeight")) {
			minHeight = LuaToNumber(l, -1);
		} else if (!strcmp(key, "More")) {
			Assert(lua_istable(l, -1));
			key = LuaToString(l, -1, 1); // Method name
			lua_rawgeti(l, -1, 2); // Method data
			if (!strcmp(key, "ButtonInfo")) {
				content = std::make_unique<CPopupContentTypeButtonInfo>();
			} else if (!strcmp(key, "Text")) {
				content = std::make_unique<CPopupContentTypeText>();
			} else if (!strcmp(key, "Costs")) {
				content = std::make_unique<CPopupContentTypeCosts>();
			} else if (!strcmp(key, "Line")) {
				content = std::make_unique<CPopupContentTypeLine>();
			} else if (!strcmp(key, "Variable")) {
				content = std::make_unique<CPopupContentTypeVariable>();
			} else {
				LuaError(l, "Invalid drawing method '%s' in DefinePopups" _C_ key);
			}
			content->Parse(l);
			lua_pop(l, 1); // Pop Variable Method data
		} else if (!strcmp(key, "Condition")) {
			condition = ParsePopupConditions(l);
		} else {
			LuaError(l, "'%s' invalid for Contents in DefinePopups" _C_ key);
		}
	}
	content->Wrap = wrap;
	content->MarginX = marginX;
	content->MarginY = marginY;
	content->minSize.x = minWidth;
	content->minSize.y = minHeight;
	content->Condition = std::move(condition);
	content->TextColor = wyrmgus::font_color::get(textColor);
	content->HighlightColor = wyrmgus::font_color::get(highColor);
	return content;
}

CPopup::CPopup() :
	MarginX(MARGIN_X * wyrmgus::defines::get()->get_scale_factor()), MarginY(MARGIN_Y * wyrmgus::defines::get()->get_scale_factor()),
	BackgroundColor(ColorBlue), BorderColor(ColorWhite)
{}

CPopup::~CPopup()
{
}
