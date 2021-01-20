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
/**@name contenttype.cpp - . */
//
//      (c) Copyright 1999-2021 by Joris Dauphin and Andrettin
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

#include "stratagus.h"

#include "ui/contenttype.h"

#include "actions.h"
#include "action/action_built.h"
#include "database/defines.h"
#include "script.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "video/font.h"
#include "video/video.h"

enum UStrIntType {
	USTRINT_STR, USTRINT_INT
};
struct UStrInt {
	union {const char *s; int i;};
	UStrIntType type;
};

extern UStrInt GetComponent(const CUnit &unit, int index, VariableAttribute e, int t);
extern UStrInt GetComponent(const wyrmgus::unit_type &type, int index, VariableAttribute e);

CContentType::~CContentType()
{
}

CContentTypeText::CContentTypeText() : Component(VariableAttribute::Value)
{
}

CContentTypeText::~CContentTypeText()
{
}

/**
**  Draw text with variable.
**
**  @param unit         unit with variable to show.
**  @param defaultfont  default font if no specific font in extra data.
*/
void CContentTypeText::Draw(const CUnit &unit, wyrmgus::font *defaultfont) const
{
	std::string text;       // Optional text to display.
	int x = this->Pos.x;
	int y = this->Pos.y;
	wyrmgus::font *font = this->Font ? this->Font : defaultfont;

	Assert(font);
	Assert(this->Index == -1 || ((unsigned int) this->Index < UnitTypeVar.GetNumberVariable()));

	//Wyrmgus start
//	CLabel label(font);
	CLabel label(font, this->TextColor, this->HighlightColor);
	//Wyrmgus end

	if (this->Text != nullptr) {
		text = EvalString(this->Text.get());
		std::string::size_type pos;
		if ((pos = text.find("~|")) != std::string::npos) {
			x += (label.Draw(x - font->getWidth(text.substr(0, pos)), y, text) - font->getWidth(text.substr(0, pos)));
		} else if (this->Centered) {
			x += (label.DrawCentered(x, y, text) * 2);
		} else {
			x += label.Draw(x, y, text);
		}
	}

	if (this->ShowName) {
		//Wyrmgus start
//		label.DrawCentered(x, y, unit.Type->Name);
		label.DrawCentered(x, y, unit.get_type_name());
		//Wyrmgus end
		return;
	}

	if (this->Index != -1) {
		if (!this->Stat) {
			const VariableAttribute component = this->Component;
			switch (component) {
				case VariableAttribute::Value:
				case VariableAttribute::Max:
				case VariableAttribute::Increase:
				case VariableAttribute::Diff:
				case VariableAttribute::Percent:
					label.Draw(x, y, GetComponent(unit, this->Index, component, 0).i);
					break;
				case VariableAttribute::Name:
					label.Draw(x, y, GetComponent(unit, this->Index, component, 0).s);
					break;
				default:
					Assert(0);
			}
		} else {
			int value = unit.Type->MapDefaultStat.Variables[this->Index].Value;
			int diff = unit.Stats->Variables[this->Index].Value - value;

			if (!diff) {
				label.Draw(x, y, value);
			} else {
				char buf[64];
				snprintf(buf, sizeof(buf), diff > 0 ? "%d~<+%d~>" : "%d~<-%d~>", value, diff);
				label.Draw(x, y, buf);
			}
		}
	}
}

CContentTypeFormattedText::CContentTypeFormattedText() : Component(VariableAttribute::Value)
{
}

/**
**  Draw formatted text with variable value.
**
**  @param unit         unit with variable to show.
**  @param defaultfont  default font if no specific font in extra data.
**
**  @note text is limited to 256 chars. (enough?)
**  @note text must have exactly 1 %d.
**  @bug if text format is incorrect.
*/
void CContentTypeFormattedText::Draw(const CUnit &unit, wyrmgus::font *defaultfont) const
{
	char buf[256];
	UStrInt usi1;

	wyrmgus::font *font = this->Font ? this->Font : defaultfont;
	Assert(font);

	//Wyrmgus start
//	CLabel label(font);
	CLabel label(font, this->TextColor, this->HighlightColor);
	//Wyrmgus end

	Assert((unsigned int) this->Index < UnitTypeVar.GetNumberVariable());
	usi1 = GetComponent(unit, this->Index, this->Component, 0);
	if (usi1.type == USTRINT_STR) {
		snprintf(buf, sizeof(buf), this->Format.c_str(), _(usi1.s));
	} else {
		snprintf(buf, sizeof(buf), this->Format.c_str(), usi1.i);
	}

	char *pos;
	if ((pos = strstr(buf, "~|")) != nullptr) {
		std::string buf2(buf);
		label.Draw(this->Pos.x - font->getWidth(buf2.substr(0, pos - buf)), this->Pos.y, buf);
	} else if (this->Centered) {
		label.DrawCentered(this->Pos.x, this->Pos.y, buf);
	} else {
		label.Draw(this->Pos.x, this->Pos.y, buf);
	}
}

CContentTypeFormattedText2::CContentTypeFormattedText2() : Component1(VariableAttribute::Value), Component2(VariableAttribute::Value)
{
}

/**
**  Draw formatted text with variable value.
**
**  @param unit         unit with variable to show.
**  @param defaultfont  default font if no specific font in extra data.
**
**  @note text is limited to 256 chars. (enough?)
**  @note text must have exactly 2 %d.
**  @bug if text format is incorrect.
*/
void CContentTypeFormattedText2::Draw(const CUnit &unit, wyrmgus::font *defaultfont) const
{
	char buf[256];
	UStrInt usi1, usi2;

	wyrmgus::font *font = this->Font ? this->Font : defaultfont;
	Assert(font);
	//Wyrmgus start
//	CLabel label(font);
	CLabel label(font, this->TextColor, this->HighlightColor);
	//Wyrmgus end

	usi1 = GetComponent(unit, this->Index1, this->Component1, 0);
	usi2 = GetComponent(unit, this->Index2, this->Component2, 0);
	if (usi1.type == USTRINT_STR) {
		if (usi2.type == USTRINT_STR) {
			snprintf(buf, sizeof(buf), this->Format.c_str(), _(usi1.s), _(usi2.s));
		} else {
			snprintf(buf, sizeof(buf), this->Format.c_str(), _(usi1.s), usi2.i);
		}
	} else {
		if (usi2.type == USTRINT_STR) {
			snprintf(buf, sizeof(buf), this->Format.c_str(), usi1.i, _(usi2.s));
		} else {
			snprintf(buf, sizeof(buf), this->Format.c_str(), usi1.i, usi2.i);
		}
	}
	char *pos;
	if ((pos = strstr(buf, "~|")) != nullptr) {
		std::string buf2(buf);
		label.Draw(this->Pos.x - font->getWidth(buf2.substr(0, pos - buf)), this->Pos.y, buf);
	} else if (this->Centered) {
		label.DrawCentered(this->Pos.x, this->Pos.y, buf);
	} else {
		label.Draw(this->Pos.x, this->Pos.y, buf);
	}
}


/**
**  Get unit from a unit depending of the relation.
**
**  @param unit  unit reference.
**  @param e     relation with unit.
**
**  @return      The desired unit.
*/
static const CUnit *GetUnitRef(const CUnit &unit, EnumUnit e)
{
	switch (e) {
		case UnitRefItSelf:
			return &unit;
		case UnitRefInside:
			return unit.UnitInside;
		case UnitRefContainer:
			return unit.Container;
		case UnitRefWorker :
			//Wyrmgus start
//			if (unit.CurrentAction() == UnitAction::Built) {
			if (unit.CurrentAction() == UnitAction::Built && !unit.Type->BoolFlag[BUILDEROUTSIDE_INDEX].value) {
			//Wyrmgus end
				COrder_Built &order = *static_cast<COrder_Built *>(unit.CurrentOrder());

				return order.get_worker();
			} else {
				return nullptr;
			}
		case UnitRefGoal:
			return unit.Goal;
		default:
			Assert(0);
	}
	return nullptr;
}

/**
**  Draw icon for unit.
**
**  @param unit         unit with icon to show.
**  @param defaultfont  unused.
*/
/* virtual */ void CContentTypeIcon::Draw(const CUnit &unit, wyrmgus::font *) const
{
	const CUnit *unitToDraw = GetUnitRef(unit, this->UnitRef);

	if (unitToDraw && unitToDraw->get_icon() != nullptr) {
		unitToDraw->get_icon()->DrawUnitIcon(*UI.SingleSelectedButton->Style, 0, this->Pos, "",
			unitToDraw->get_player_color());
	}
}

/**
**  Draw life bar of a unit using selected variable.
**  Placed under icons on top-panel.
**
**  @param unit         Pointer to unit.
**  @param defaultfont  FIXME: docu
**
**  @todo Color and percent value Parametrisation.
*/
/* virtual */ void CContentTypeLifeBar::Draw(const CUnit &unit, wyrmgus::font *) const
{
	Assert((unsigned int) this->Index < UnitTypeVar.GetNumberVariable());
	//Wyrmgus start
	int max = 0;
	if (this->Index == XP_INDEX) {
		max = unit.Variable[XPREQUIRED_INDEX].Value;
	} else {
		max = unit.GetModifiedVariable(this->Index, VariableAttribute::Max);
	}
//	if (!unit.Variable[this->Index].Max) {
	if (!max) {
	//Wyrmgus end
		return;
	}

	uint32_t color;
	//Wyrmgus start
	uint32_t lighter_color;
//	int f = (100 * unit.Variable[this->Index].Value) / unit.Variable[this->Index].Max;
	int f = (100 * unit.Variable[this->Index].Value) / max;
	//Wyrmgus end

	//Wyrmgus start
	/*
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
	*/

	if (this->Index == HP_INDEX) {
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
	} else if (this->Index == MANA_INDEX) {
		color = CVideo::MapRGB(4, 70, 100);
		lighter_color = CVideo::MapRGB(8, 97, 137);
	} else if (this->Index == XP_INDEX) {
		color = CVideo::MapRGB(97, 103, 0);
		lighter_color = CVideo::MapRGB(132, 141, 3);
	} else {
		color = ColorDarkGreen;
		lighter_color = CVideo::MapRGB(67, 137, 8);
	}
	//Wyrmgus end

	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	// Border
	//Wyrmgus start
//	Video.FillRectangleClip(ColorBlack, this->Pos.x - 2, this->Pos.y - 2,
//							this->Width + 3, this->Height + 3);
	if (wyrmgus::defines::get()->get_bar_frame_graphics() != nullptr) {
		wyrmgus::defines::get()->get_bar_frame_graphics()->DrawClip(this->Pos.x + (-1 - 4) * scale_factor, this->Pos.y + (-1 - 4) * scale_factor);
		Video.FillRectangleClip(ColorBlack, this->Pos.x - 1 * scale_factor, this->Pos.y - 1 * scale_factor,
								this->Width * scale_factor, this->Height * scale_factor);
	} else {
		Video.FillRectangleClip(ColorBlack, this->Pos.x - 3 * scale_factor, this->Pos.y - 3 * scale_factor,
								(this->Width + 4) * scale_factor, (this->Height + 4) * scale_factor);
	}
	//Wyrmgus end

	Video.FillRectangleClip(color, this->Pos.x - 1 * scale_factor, this->Pos.y - 1 * scale_factor,
							(this->Width * scale_factor * f) / 100, this->Height * scale_factor);
	//Wyrmgus start
	Video.FillRectangleClip(lighter_color, this->Pos.x - 1 * scale_factor, this->Pos.y - 1 * scale_factor,
							(this->Width * scale_factor * f) / 100, 1 * scale_factor);
	//Wyrmgus end
}

/**
**  Draw life bar of a unit using selected variable.
**  Placed under icons on top-panel.
**
**  @param unit         Pointer to unit.
**  @param defaultfont  FIXME: docu
**
**  @todo Color and percent value Parametrisation.
*/
void CContentTypeCompleteBar::Draw(const CUnit &unit, wyrmgus::font *) const
{
	Assert((unsigned int) this->varIndex < UnitTypeVar.GetNumberVariable());
	//Wyrmgus start
//	if (!unit.Variable[this->varIndex].Max) {
	if (!unit.GetModifiedVariable(this->varIndex, VariableAttribute::Max)) {
	//Wyrmgus end
		return;
	}

	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	int x = this->Pos.x;
	int y = this->Pos.y;
	int w = this->width * scale_factor;
	int h = this->height * scale_factor;
	Assert(w > 0);
	Assert(h > 4);
	const std::array<uint32_t, 12> colors = {ColorRed, ColorYellow, ColorGreen, ColorLightGray,
							 ColorGray, ColorDarkGray, ColorWhite, ColorOrange,
							 ColorLightBlue, ColorBlue, ColorDarkGreen, ColorBlack
							};
	const uint32_t color = (colorIndex != -1) ? colors[colorIndex] : UI.CompletedBarColor;
	//Wyrmgus start
//	const int f = (100 * unit.Variable[this->varIndex].Value) / unit.Variable[this->varIndex].Max;
	const int f = (100 * unit.GetModifiedVariable(this->varIndex, VariableAttribute::Value)) / unit.GetModifiedVariable(this->varIndex, VariableAttribute::Max);
	//Wyrmgus end

	if (!this->hasBorder) {
		//Wyrmgus start
		if (wyrmgus::defines::get()->get_progress_bar_graphics() != nullptr) {
			wyrmgus::defines::get()->get_progress_bar_graphics()->DrawClip(this->Pos.x - 4 * scale_factor, this->Pos.y - 5 * scale_factor);
		}
		//Wyrmgus end
		Video.FillRectangleClip(color, x, y, f * w / 100, h);
		if (UI.CompletedBarShadow) {
			// Shadow
			Video.DrawVLine(ColorGray, x + f * w / 100, y, h);
			Video.DrawHLine(ColorGray, x, y + h, f * w / 100);

			// |~  Light
			Video.DrawVLine(ColorWhite, x, y, h);
			Video.DrawHLine(ColorWhite, x, y, f * w / 100);
		}
	} else {
		Video.DrawRectangleClip(ColorWhite, x, y, w + 4 * scale_factor, h);
		Video.DrawRectangleClip(ColorBlack, x + 1 * scale_factor, y + 1 * scale_factor, w + 2 * scale_factor, h - 2 * scale_factor);
		Video.FillRectangleClip(color, x + 2 * scale_factor, y + 2 * scale_factor, f * w / 100, h - 4 * scale_factor);
	}
}

void CContentTypeText::Parse(lua_State *l)
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
			} else if (!strcmp(key, "Component")) {
				this->Component = Str2VariableAttribute(l, LuaToString(l, -1));
			} else if (!strcmp(key, "Stat")) {
				this->Stat = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ShowName")) {
				this->ShowName = LuaToBoolean(l, -1);
			} else {
				LuaError(l, "'%s' invalid for method 'Text' in DefinePanelContents" _C_ key);
			}
		}
	}
}

/* virtual */ void CContentTypeFormattedText::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1));

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Format")) {
			this->Format = LuaToString(l, -1);
		} else if (!strcmp(key, "Font")) {
			this->Font = wyrmgus::font::get(LuaToString(l, -1));
		} else if (!strcmp(key, "Variable")) {
			const char *const name = LuaToString(l, -1);
			this->Index = UnitTypeVar.VariableNameLookup[name];
			if (this->Index == -1) {
				LuaError(l, "unknown variable '%s'" _C_ name);
			}
		} else if (!strcmp(key, "Component")) {
			this->Component = Str2VariableAttribute(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Centered")) {
			this->Centered = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "'%s' invalid for method 'FormattedText' in DefinePanelContents" _C_ key);
		}
	}
}

/* virtual */ void CContentTypeFormattedText2::Parse(lua_State *l)
{
	Assert(lua_istable(l, -1));
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Format")) {
			this->Format = LuaToString(l, -1);
		} else if (!strcmp(key, "Font")) {
			this->Font = wyrmgus::font::get(LuaToString(l, -1));
		} else if (!strcmp(key, "Variable")) {
			const char *const name = LuaToString(l, -1);
			this->Index1 = UnitTypeVar.VariableNameLookup[name];
			this->Index2 = this->Index1;
			if (this->Index1 == -1) {
				LuaError(l, "unknown variable '%s'" _C_ name);
			}
		} else if (!strcmp(key, "Component")) {
			this->Component1 = Str2VariableAttribute(l, LuaToString(l, -1));
			this->Component2 = Str2VariableAttribute(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Variable1")) {
			const char *const name = LuaToString(l, -1);
			this->Index1 = UnitTypeVar.VariableNameLookup[name];
			if (this->Index1 == -1) {
				LuaError(l, "unknown variable '%s'" _C_ name);
			}
		} else if (!strcmp(key, "Component1")) {
			this->Component1 = Str2VariableAttribute(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Variable2")) {
			const char *const name = LuaToString(l, -1);
			this->Index2 = UnitTypeVar.VariableNameLookup[name];
			if (this->Index2 == -1) {
				LuaError(l, "unknown variable '%s'" _C_ LuaToString(l, -1));
			}
		} else if (!strcmp(key, "Component2")) {
			this->Component2 = Str2VariableAttribute(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Centered")) {
			this->Centered = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "'%s' invalid for method 'FormattedText2' in DefinePanelContents" _C_ key);
		}
	}
}

/**
**  Return enum from string about variable component.
**
**  @param l Lua State.
**  @param s string to convert.
**
**  @return  Corresponding value.
**  @note    Stop on error.
*/
static EnumUnit Str2EnumUnit(lua_State *l, const char *s)
{
	static struct {
		const char *s;
		EnumUnit e;
	} list[] = {
		{"ItSelf", UnitRefItSelf},
		{"Inside", UnitRefInside},
		{"Container", UnitRefContainer},
		{"Worker", UnitRefWorker},
		{"Goal", UnitRefGoal},
		{0, UnitRefItSelf}
	}; // List of possible values.

	for (int i = 0; list[i].s; ++i) {
		if (!strcmp(s, list[i].s)) {
			return list[i].e;
		}
	}
	LuaError(l, "'%s' is a invalid Unit reference" _C_ s);
	return UnitRefItSelf;
}

/* virtual */ void CContentTypeIcon::Parse(lua_State *l)
{
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Unit")) {
			this->UnitRef = Str2EnumUnit(l, LuaToString(l, -1));
		} else {
			LuaError(l, "'%s' invalid for method 'Icon' in DefinePanelContents" _C_ key);
		}
	}
}

/* virtual */ void CContentTypeLifeBar::Parse(lua_State *l)
{
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Variable")) {
			const char *const name = LuaToString(l, -1);
			this->Index = UnitTypeVar.VariableNameLookup[name];
			if (this->Index == -1) {
				LuaError(l, "unknown variable '%s'" _C_ name);
			}
		} else if (!strcmp(key, "Height")) {
			this->Height = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Width")) {
			this->Width = LuaToNumber(l, -1);
		} else {
			LuaError(l, "'%s' invalid for method 'LifeBar' in DefinePanelContents" _C_ key);
		}
	}
	// Default value and checking errors.
	if (this->Height <= 0) {
		this->Height = 5; // Default value.
	}
	if (this->Width <= 0) {
		this->Width = 50; // Default value.
	}
	if (this->Index == -1) {
		LuaError(l, "variable undefined for LifeBar");
	}
}

static int GetColorIndexByName(const char *colorName)
{
	//FIXME: need more general way
	static constexpr std::array<const char *, 12> names = {
		"red", "yellow", "green", "light-gray", "gray", "dark-gray",
		"white", "orange", "light-blue", "blue", "dark-green", "black"
	};

	for (size_t i = 0; i < names.size(); ++i) {
		if (!strcmp(colorName, names[i])) {
			return i;
		}
	}
	return -1;
}

/* virtual */ void CContentTypeCompleteBar::Parse(lua_State *l)
{
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Variable")) {
			const char *const name = LuaToString(l, -1);
			this->varIndex = UnitTypeVar.VariableNameLookup[name];
			if (this->varIndex == -1) {
				LuaError(l, "unknown variable '%s'" _C_ name);
			}
		} else if (!strcmp(key, "Height")) {
			this->height = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Width")) {
			this->width = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Border")) {
			this->hasBorder = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "Color")) {
			const char *const colorName = LuaToString(l, -1);
			this->colorIndex = GetColorIndexByName(colorName);
			if (colorIndex == -1) {
				LuaError(l, "incorrect color: '%s' " _C_ colorName);
			}
		} else {
			LuaError(l, "'%s' invalid for method 'CompleteBar' in DefinePanelContents" _C_ key);
		}
	}
	// Default value and checking errors.
	if (this->height <= 0) {
		this->height = 5; // Default value.
	}
	if (this->width <= 0) {
		this->width = 50; // Default value.
	}
	if (this->varIndex == -1) {
		LuaError(l, "variable undefined for CompleteBar");
	}
}
