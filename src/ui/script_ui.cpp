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
/**@name script_ui.cpp - The ui ccl functions. */
//
//      (c) Copyright 1999-2021 by Lutz Sammer, Jimmy Salmon, Martin Renold
//      and Andrettin
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

#include "ui/ui.h"

#include "database/defines.h"
#include "map/map.h"
#include "menus.h"
#include "script.h"
#include "spell/spell.h"
#include "title.h"
#include "util/util.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/contenttype.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/popup.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"

#include <QUuid>

std::string ClickMissile;		/// FIXME:docu
std::string DamageMissile;		/// FIXME:docu
std::map<std::string, ButtonStyle *> ButtonStyleHash;

static int HandleCount = 1;		/// Lua handler count

CPreference Preference;

/**
**  Set speed of key scroll
**
**  @param l  Lua state.
*/
static int CclSetKeyScrollSpeed(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.KeyScrollSpeed = LuaToNumber(l, 1);
	return 0;
}

/**
**  Get speed of key scroll
**
**  @param l  Lua state.
*/
static int CclGetKeyScrollSpeed(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, UI.KeyScrollSpeed);
	return 1;
}

/**
**  Set speed of mouse scroll
**
**  @param l  Lua state.
*/
static int CclSetMouseScrollSpeed(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.MouseScrollSpeed = LuaToNumber(l, 1);
	return 0;
}

/**
**  Get speed of mouse scroll
**
**  @param l  Lua state.
*/
static int CclGetMouseScrollSpeed(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, UI.MouseScrollSpeed);
	return 1;
}

/**
**  Set speed of middle-mouse scroll
**
**  @param l  Lua state.
*/
static int CclSetMouseScrollSpeedDefault(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.MouseScrollSpeedDefault = LuaToNumber(l, 1);
	return 0;
}

/**
**  Get speed of middle-mouse scroll
**
**  @param l  Lua state.
*/
static int CclGetMouseScrollSpeedDefault(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, UI.MouseScrollSpeedDefault);
	return 0;
}

/**
**  Set speed of ctrl-middle-mouse scroll
**
**  @param l  Lua state.
*/
static int CclSetMouseScrollSpeedControl(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.MouseScrollSpeedControl = LuaToNumber(l, 1);
	return 0;
}

/**
**  Get speed of ctrl-middle-mouse scroll
**
**  @param l  Lua state.
*/
static int CclGetMouseScrollSpeedControl(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, UI.MouseScrollSpeedControl);
	return 0;
}

/**
**  Set which missile is used for right click
**
**  @param l  Lua state.
*/
static int CclSetClickMissile(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	ClickMissile.clear();
	if (args == 1 && !lua_isnil(l, 1)) {
		ClickMissile = lua_tostring(l, 1);
	}
	return 0;
}

/**
**  Set which missile shows Damage
**
**  @param l  Lua state.
*/
static int CclSetDamageMissile(lua_State *l)
{
	const int args = lua_gettop(l);

	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	DamageMissile.clear();
	if (args == 1 && !lua_isnil(l, 1)) {
		DamageMissile = lua_tostring(l, 1);
	}
	return 0;
}

static int CclSetMaxOpenGLTexture(lua_State *l)
{
	LuaCheckArgs(l, 1);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (CclInConfigFile) {
		GLMaxTextureSizeOverride = LuaToNumber(l, 1);
	}
#endif
	return 0;
}

static int CclSetUseTextureCompression(lua_State *l)
{
	LuaCheckArgs(l, 1);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (CclInConfigFile) {
		UseGLTextureCompression = LuaToBoolean(l, 1);
	}
#endif
	return 0;
}

static int CclSetZoomNoResize(lua_State *l)
{
	LuaCheckArgs(l, 1);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (CclInConfigFile) {
		// May have been set from the command line
		ZoomNoResize = LuaToBoolean(l, 1);
	}
#endif
	return 0;
}

/**
**  Set the video resolution.
**
**  @param l  Lua state.
*/
static int CclSetVideoResolution(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (CclInConfigFile) {
		// May have been set from the command line
		if (!Video.Width || !Video.Height) {
			Video.Width = LuaToNumber(l, 1);
			Video.Height = LuaToNumber(l, 2);
		}
	}
	return 0;
}

/**
**  Get the video resolution.
**
**  @param l  Lua state.
*/
static int CclGetVideoResolution(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, Video.Width);
	lua_pushnumber(l, Video.Height);
	return 2;
}

/**
**  Set the video fullscreen mode.
**
**  @param l  Lua state.
*/
static int CclSetVideoFullScreen(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (CclInConfigFile) {
		// May have been set from the command line
		if (!VideoForceFullScreen) {
			Video.FullScreen = LuaToBoolean(l, 1);
		}
	}
	return 0;
}

/**
**  Get the video fullscreen mode.
**
**  @param l  Lua state.
*/
static int CclGetVideoFullScreen(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, Video.FullScreen);
	return 1;
}

/**
**  Default title screens.
**
**  @param l  Lua state.
*/
static int CclSetTitleScreens(lua_State *l)
{
	TitleScreens.clear();

	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		TitleScreen title_screen;
		title_screen.Iterations = 1;
		lua_pushnil(l);
		while (lua_next(l, j + 1)) {
			const char *value = LuaToString(l, -2);
			if (!strcmp(value, "Image")) {
				title_screen.File = LuaToString(l, -1);
			} else if (!strcmp(value, "Timeout")) {
				title_screen.Timeout = LuaToNumber(l, -1);
			} else if (!strcmp(value, "Iterations")) {
				title_screen.Iterations = LuaToNumber(l, -1);
			} else if (!strcmp(value, "Editor")) {
				title_screen.Editor = LuaToNumber(l, -1);
			} else if (!strcmp(value, "Labels")) {
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				const int subargs = lua_rawlen(l, -1);

				title_screen.Labels.clear();

				for (int k = 0; k < subargs; ++k) {
					lua_rawgeti(l, -1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					TitleScreenLabel label;
					lua_pushnil(l);
					while (lua_next(l, -2)) {
						const char *subvalue = LuaToString(l, -2);
						if (!strcmp(subvalue, "Text")) {
							label.Text = LuaToString(l, -1);
						} else if (!strcmp(subvalue, "Font")) {
							label.Font = wyrmgus::font::get(LuaToString(l, -1));
						} else if (!strcmp(subvalue, "Pos")) {
							CclGetPos(l, &label.Xofs, &label.Yofs);
						} else if (!strcmp(subvalue, "Flags")) {
							if (!lua_istable(l, -1)) {
								LuaError(l, "incorrect argument");
							}
							const int subsubargs = lua_rawlen(l, -1);
							for (int subk = 0; subk < subsubargs; ++subk) {
								const char *subsubvalue = LuaToString(l, -1, subk + 1);
								if (!strcmp(subsubvalue, "center")) {
									label.Flags |= TitleFlagCenter;
								} else {
									LuaError(l, "incorrect flag");
								}
							}
						} else {
							LuaError(l, "Unsupported key: %s" _C_ subvalue);
						}
						lua_pop(l, 1);
					}

					title_screen.Labels.push_back(std::move(label));
					lua_pop(l, 1);
				}
			} else {
				LuaError(l, "Unsupported key: %s" _C_ value);
			}
			lua_pop(l, 1);
		}
		TitleScreens.push_back(std::move(title_screen));
	}
	return 0;
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
VariableAttribute Str2VariableAttribute(lua_State *l, const char *s)
{
	static struct {
		const char *s;
		VariableAttribute e;
	} list[] = {
		{"Value", VariableAttribute::Value},
		{"Max", VariableAttribute::Max},
		{"Increase", VariableAttribute::Increase},
		{"Diff", VariableAttribute::Diff},
		{"Percent", VariableAttribute::Percent},
		{"Name", VariableAttribute::Name},
		//Wyrmgus start
		{"Change", VariableAttribute::Change},
		{"IncreaseChange", VariableAttribute::IncreaseChange},
		//Wyrmgus end
		{0, VariableAttribute::Value}
	}; // List of possible values.

	for (int i = 0; list[i].s; ++i) {
		if (!strcmp(s, list[i].s)) {
			return list[i].e;
		}
	}
	LuaError(l, "'%s' is a invalid variable component" _C_ s);
	return VariableAttribute::Value;
}

/**
**  Parse the condition Panel.
**
**  @param l   Lua State.
*/
static std::unique_ptr<ConditionPanel> ParseConditionPanel(lua_State *l)
{
	Assert(lua_istable(l, -1));

	auto condition = std::make_unique<ConditionPanel>();

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "ShowOnlySelected")) {
			condition->ShowOnlySelected = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "HideNeutral")) {
			condition->HideNeutral = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "HideAllied")) {
			condition->HideAllied = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "ShowOpponent")) {
			condition->ShowOpponent = LuaToBoolean(l, -1);
		} else if (!strcmp(key, "ShowIfCanCastAnySpell")) {
			condition->ShowIfCanCastAnySpell = LuaToBoolean(l, -1);
		//Wyrmgus start
		} else if (!strcmp(key, "Affixed")) {
			condition->Affixed = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Unique")) {
			condition->Unique = Ccl2Condition(l, LuaToString(l, -1));
		} else if (!strcmp(key, "Replenishment")) {
			condition->Replenishment = Ccl2Condition(l, LuaToString(l, -1));
		//Wyrmgus end
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
			LuaError(l, "'%s' invalid for Condition in DefinePanelContents" _C_ key);
		}
	}
	return condition;
}

static std::unique_ptr<CContentType> CclParseContent(lua_State *l)
{
	Assert(lua_istable(l, -1));

	std::unique_ptr<CContentType> content;
	std::unique_ptr<ConditionPanel> condition;
	PixelPos pos(0, 0);
	//Wyrmgus start
	std::string textColor("white");
	std::string highColor("red");
	//Wyrmgus end

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Pos")) {
			CclGetPos(l, &pos.x, &pos.y);
			pos.x *= wyrmgus::defines::get()->get_scale_factor();;
			pos.y *= wyrmgus::defines::get()->get_scale_factor();;
		//Wyrmgus start
		} else if (!strcmp(key, "TextColor")) {
			textColor = LuaToString(l, -1);
		} else if (!strcmp(key, "HighlightColor")) {
			highColor = LuaToString(l, -1);
		//Wyrmgus end
		} else if (!strcmp(key, "More")) {
			Assert(lua_istable(l, -1));
			lua_rawgeti(l, -1, 1); // Method name
			lua_rawgeti(l, -2, 2); // Method data
			key = LuaToString(l, -2);
			if (!strcmp(key, "Text")) {
				content = std::make_unique<CContentTypeText>();
			} else if (!strcmp(key, "FormattedText")) {
				content = std::make_unique<CContentTypeFormattedText>();
			} else if (!strcmp(key, "FormattedText2")) {
				content = std::make_unique<CContentTypeFormattedText2>();
			} else if (!strcmp(key, "Icon")) {
				content = std::make_unique<CContentTypeIcon>();
			} else if (!strcmp(key, "LifeBar")) {
				content = std::make_unique<CContentTypeLifeBar>();
			} else if (!strcmp(key, "CompleteBar")) {
				content = std::make_unique<CContentTypeCompleteBar>();
			} else {
				LuaError(l, "Invalid drawing method '%s' in DefinePanelContents" _C_ key);
			}
			content->Parse(l);
			lua_pop(l, 2); // Pop Variable Name and Method
		} else if (!strcmp(key, "Condition")) {
			condition = ParseConditionPanel(l);
		} else {
			LuaError(l, "'%s' invalid for Contents in DefinePanelContents" _C_ key);
		}
	}
	content->Pos = pos;
	content->Condition = std::move(condition);
	content->TextColor = wyrmgus::font_color::get(textColor);
	content->HighlightColor = wyrmgus::font_color::get(highColor);
	return content;
}


/**
**  Define the Panels.
**  Define what is shown in the panel(text, icon, variables)
**
**  @param l  Lua state.
**  @return   0.
*/
static int CclDefinePanelContents(lua_State *l)
{
	const int nargs = lua_gettop(l);

	for (int i = 0; i < nargs; i++) {
		Assert(lua_istable(l, i + 1));
		auto infopanel = std::make_unique<CUnitInfoPanel>();

		for (lua_pushnil(l); lua_next(l, i + 1); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);

			if (!strcmp(key, "Ident")) {
				infopanel->Name = LuaToString(l, -1);
			} else if (!strcmp(key, "Pos")) {
				CclGetPos(l, &infopanel->PosX, &infopanel->PosY);
			} else if (!strcmp(key, "DefaultFont")) {
				infopanel->DefaultFont = wyrmgus::font::get(LuaToString(l, -1));
			} else if (!strcmp(key, "Condition")) {
				infopanel->Condition = ParseConditionPanel(l);
			} else if (!strcmp(key, "Contents")) {
				Assert(lua_istable(l, -1));
				for (size_t j = 0; j < lua_rawlen(l, -1); j++, lua_pop(l, 1)) {
					lua_rawgeti(l, -1, j + 1);
					infopanel->Contents.push_back(CclParseContent(l));
				}
			} else {
				LuaError(l, "'%s' invalid for DefinePanelContents" _C_ key);
			}
		}
		for (const std::unique_ptr<CContentType> &content : infopanel->Contents) { // Default value for invalid value.
			content->Pos.x += infopanel->PosX;
			content->Pos.y += infopanel->PosY;
		}
		size_t j;
		for (j = 0; j < UI.InfoPanelContents.size(); ++j) {
			if (infopanel->Name == UI.InfoPanelContents[j]->Name) {
				DebugPrint("Redefinition of Panel '%s'\n" _C_ infopanel->Name.c_str());
				UI.InfoPanelContents[j] = std::move(infopanel);
				break;
			}
		}

		if (j == UI.InfoPanelContents.size()) {
			UI.InfoPanelContents.push_back(std::move(infopanel));
		}
	}
	return 0;
}

/**
**  Define the Panels.
**  Define what is shown in the panel(text, icon, variables)
**
**  @param l  Lua state.
**  @return   0.
*/
static int CclDefinePopup(lua_State *l)
{
	Assert(lua_istable(l, 1));

	auto popup = std::make_unique<CPopup>();

	for (lua_pushnil(l); lua_next(l, 1); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Ident")) {
			popup->Ident = LuaToString(l, -1);
		} else if (!strcmp(key, "DefaultFont")) {
			popup->DefaultFont = wyrmgus::font::get(LuaToString(l, -1));
		} else if (!strcmp(key, "BackgroundColor")) {
			popup->BackgroundColor = LuaToUnsignedNumber(l, -1);
		} else if (!strcmp(key, "BorderColor")) {
			popup->BorderColor = LuaToUnsignedNumber(l, -1);
		} else if (!strcmp(key, "Margin")) {
			CclGetPos(l, &popup->MarginX, &popup->MarginY);
		} else if (!strcmp(key, "MinWidth")) {
			popup->MinWidth = LuaToNumber(l, -1);
		} else if (!strcmp(key, "MinHeight")) {
			popup->MinHeight = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Contents")) {
			Assert(lua_istable(l, -1));
			for (size_t j = 0; j < lua_rawlen(l, -1); j++, lua_pop(l, 1)) {
				lua_rawgeti(l, -1, j + 1);
				popup->Contents.push_back(CPopupContentType::ParsePopupContent(l));
			}
		} else {
			LuaError(l, "'%s' invalid for DefinePopups" _C_ key);
		}
	}
	for (size_t j = 0; j < UI.ButtonPopups.size(); ++j) {
		if (popup->Ident == UI.ButtonPopups[j]->Ident) {
			DebugPrint("Redefinition of Popup '%s'\n" _C_ popup->Ident.c_str());
			UI.ButtonPopups[j] = std::move(popup);
			return 0;
		}
	}
	UI.ButtonPopups.push_back(std::move(popup));
	return 0;
}

/**
**  Define the viewports.
**
**  @param l  Lua state.
*/
static int CclDefineViewports(lua_State *l)
{
	int i = 0;
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;
		if (!strcmp(value, "mode")) {
			UI.ViewportMode = (ViewportModeType)LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "viewport")) {
			if (!lua_istable(l, j + 1) && lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			UI.Viewports[i].MapPos.x = LuaToNumber(l, j + 1, 1);
			UI.Viewports[i].MapPos.y = LuaToNumber(l, j + 1, 2);
			const int slot = LuaToNumber(l, j + 1, 3);
			if (slot != -1) {
				UI.Viewports[i].Unit = &wyrmgus::unit_manager::get()->GetSlotUnit(slot);
			}
			++i;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	UI.NumViewports = i;
	return 0;
}

/**
**  Fighter right button attacks as default.
**
**  @param l  Lua state.
*/
static int CclRightButtonAttacks(lua_State *l)
{
	LuaCheckArgs(l, 0);
	RightButtonAttacks = true;
	return 0;
}

/**
**  Fighter right button moves as default.
**
**  @param l  Lua state.
*/
static int CclRightButtonMoves(lua_State *l)
{
	LuaCheckArgs(l, 0);
	RightButtonAttacks = false;
	return 0;
}

/**
**  Find a button style
**
**  @param style  Name of the style to find.
**
**  @return       Button style, null if not found.
*/
ButtonStyle *FindButtonStyle(const std::string &style)
{
	return ButtonStyleHash[style];
}

/**
**  Parse button style properties
**
**  @param l  Lua state.
**  @param p  Properties to fill in.
*/
static void ParseButtonStyleProperties(lua_State *l, ButtonStyleProperties *p)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	std::string file;
	int w = 0;
	int h = 0;

	lua_pushnil(l);
	while (lua_next(l, -2)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "File")) {
			file = LuaToString(l, -1);
		} else if (!strcmp(value, "Size")) {
			CclGetPos(l, &w, &h);
		} else if (!strcmp(value, "Frame")) {
			p->Frame = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Border")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			lua_pushnil(l);
			while (lua_next(l, -2)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Color")) {
					p->BorderColorRGB.Parse(l);
				} else if (!strcmp(value, "Size")) {
					p->BorderSize = LuaToNumber(l, -1);
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "TextPos")) {
			CclGetPos(l, &p->TextPos.x, &p->TextPos.y);
		} else if (!strcmp(value, "TextAlign")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "Center")) {
				p->TextAlign = TextAlignment::Center;
			} else if (!strcmp(value, "Right")) {
				p->TextAlign = TextAlignment::Right;
			} else if (!strcmp(value, "Left")) {
				p->TextAlign = TextAlignment::Left;
			} else {
				LuaError(l, "Invalid text alignment: %s" _C_ value);
			}
		} else if (!strcmp(value, "TextNormalColor")) {
			p->TextNormalColor = wyrmgus::font_color::get(LuaToString(l, -1));
		} else if (!strcmp(value, "TextReverseColor")) {
			p->TextReverseColor = wyrmgus::font_color::get(LuaToString(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	if (!file.empty()) {
		p->Sprite = CGraphic::New(file, w, h);
	}
}

/**
**  Define a button style
**
**  @param l  Lua state.
*/
static int CclDefineButtonStyle(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	const char *style = LuaToString(l, 1);
	ButtonStyle *&b = ButtonStyleHash[style];
	if (!b) {
		b = new ButtonStyle;
		// Set to bogus value to see if it was set later
		b->Default.TextPos.x = b->Hover.TextPos.x = b->Clicked.TextPos.x = 0xFFFFFF;
	}

	lua_pushnil(l);
	while (lua_next(l, 2)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "Size")) {
			CclGetPos(l, &b->Width, &b->Height);
		} else if (!strcmp(value, "Font")) {
			b->Font = wyrmgus::font::get(LuaToString(l, -1));
		} else if (!strcmp(value, "TextNormalColor")) {
			b->TextNormalColor = wyrmgus::font_color::get(LuaToString(l, -1));
		} else if (!strcmp(value, "TextReverseColor")) {
			b->TextReverseColor = wyrmgus::font_color::get(LuaToString(l, -1));
		} else if (!strcmp(value, "TextPos")) {
			CclGetPos(l, &b->TextX, &b->TextY);
		} else if (!strcmp(value, "TextAlign")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "Center")) {
				b->TextAlign = TextAlignment::Center;
			} else if (!strcmp(value, "Right")) {
				b->TextAlign = TextAlignment::Right;
			} else if (!strcmp(value, "Left")) {
				b->TextAlign = TextAlignment::Left;
			} else {
				LuaError(l, "Invalid text alignment: %s" _C_ value);
			}
		} else if (!strcmp(value, "Default")) {
			ParseButtonStyleProperties(l, &b->Default);
		} else if (!strcmp(value, "Hover")) {
			ParseButtonStyleProperties(l, &b->Hover);
		} else if (!strcmp(value, "Clicked")) {
			ParseButtonStyleProperties(l, &b->Clicked);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	if (b->Default.TextPos.x == 0xFFFFFF) {
		b->Default.TextPos.x = b->TextX;
		b->Default.TextPos.y = b->TextY;
	}
	if (b->Hover.TextPos.x == 0xFFFFFF) {
		b->Hover.TextPos.x = b->TextX;
		b->Hover.TextPos.y = b->TextY;
	}
	if (b->Clicked.TextPos.x == 0xFFFFFF) {
		b->Clicked.TextPos.x = b->TextX;
		b->Clicked.TextPos.y = b->TextY;
	}

	if (b->Default.TextAlign == TextAlignment::Undefined) {
		b->Default.TextAlign = b->TextAlign;
	}
	if (b->Hover.TextAlign == TextAlignment::Undefined) {
		b->Hover.TextAlign = b->TextAlign;
	}
	if (b->Clicked.TextAlign == TextAlignment::Undefined) {
		b->Clicked.TextAlign = b->TextAlign;
	}
	return 0;
}

/**
**  Add a Lua handler
**  FIXME: when should these be freed?
*/
int AddHandler(lua_State *l)
{
	lua_getglobal(l, "_handlers_");
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_newtable(l);
		lua_setglobal(l, "_handlers_");
		lua_getglobal(l, "_handlers_");
	}
	lua_pushvalue(l, -2);
	lua_rawseti(l, -2, HandleCount);
	lua_pop(l, 1);

	return HandleCount++;
}

/**
**  Call a Lua handler
*/
void CallHandler(unsigned int handle, int value)
{
	lua_getglobal(Lua, "_handlers_");
	lua_rawgeti(Lua, -1, handle);
	lua_pushnumber(Lua, value);
	LuaCall(1, 1);
	lua_pop(Lua, 1);
}

/**
**  Define a button.
**
**  @param l  Lua state.
*/
static int CclDefineButton(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	//generate UUID as an identifier for the Lua-defined button
	const QUuid uuid = QUuid::createUuid();
	const std::string identifier = uuid.toString(QUuid::WithoutBraces).toStdString();

	wyrmgus::button *button = wyrmgus::button::add(identifier, nullptr);

	//
	// Parse the arguments
	//
	lua_pushnil(l);
	while (lua_next(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Pos")) {
			button->pos = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Level")) {
			button->level = wyrmgus::button_level::get(LuaToString(l, -1));
		} else if (!strcmp(value, "AlwaysShow")) {
			button->always_show = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			button->Icon.Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Action")) {
			value = LuaToString(l, -1);
			const ButtonCmd button_action_id = GetButtonActionIdByName(std::string(value));
			if (button_action_id != ButtonCmd::None) {
				button->Action = button_action_id;
			} else {
				LuaError(l, "Unsupported button action: %s" _C_ value);
			}
			//Wyrmgus end
		} else if (!strcmp(value, "Value")) {
			if (!lua_isnumber(l, -1) && !lua_isstring(l, -1) && !lua_isfunction(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			if (lua_isfunction(l, -1)) {
				button->Payload = new LuaCallback(l, -1);
			} else {
				char buf[64];
				const char *s2;

				if (lua_isnumber(l, -1)) {
					snprintf(buf, sizeof(buf), "%ld", (long int)lua_tonumber(l, -1));
					s2 = buf;
				} else {
					s2 = lua_tostring(l, -1);
				}
				button->ValueStr = s2;
			}
		} else if (!strcmp(value, "Allowed")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "check-true")) {
				button->Allowed = ButtonCheckTrue;
			} else if (!strcmp(value, "check-false")) {
				button->Allowed = ButtonCheckFalse;
			} else if (!strcmp(value, "check-upgrade")) {
				button->Allowed = ButtonCheckUpgrade;
			} else if (!strcmp(value, "check-upgrade-not")) {
				button->Allowed = ButtonCheckUpgradeNot;
			} else if (!strcmp(value, "check-upgrade-or")) {
				button->Allowed = ButtonCheckUpgradeOr;
			} else if (!strcmp(value, "check-individual-upgrade")) {
				button->Allowed = ButtonCheckIndividualUpgrade;
			} else if (!strcmp(value, "check-individual-upgrade-or")) {
				button->Allowed = ButtonCheckIndividualUpgradeOr;
			} else if (!strcmp(value, "check-unit-variable")) {
				button->Allowed = ButtonCheckUnitVariable;
			} else if (!strcmp(value, "check-units-or")) {
				button->Allowed = ButtonCheckUnitsOr;
			} else if (!strcmp(value, "check-units-and")) {
				button->Allowed = ButtonCheckUnitsAnd;
			} else if (!strcmp(value, "check-units-not")) {
				button->Allowed = ButtonCheckUnitsNot;
			} else if (!strcmp(value, "check-network")) {
				button->Allowed = ButtonCheckNetwork;
			} else if (!strcmp(value, "check-no-network")) {
				button->Allowed = ButtonCheckNoNetwork;
			} else if (!strcmp(value, "check-no-work")) {
				button->Allowed = ButtonCheckNoWork;
			} else if (!strcmp(value, "check-no-research")) {
				button->Allowed = ButtonCheckNoResearch;
			} else if (!strcmp(value, "check-attack")) {
				button->Allowed = ButtonCheckAttack;
			} else if (!strcmp(value, "check-upgrade-to")) {
				button->Allowed = ButtonCheckUpgradeTo;
			} else if (!strcmp(value, "check-research")) {
				button->Allowed = ButtonCheckResearch;
			} else if (!strcmp(value, "check-single-research")) {
				button->Allowed = ButtonCheckSingleResearch;
			} else if (!strcmp(value, "check-has-inventory")) {
				button->Allowed = ButtonCheckHasInventory;
			} else if (!strcmp(value, "check-has-sub-buttons")) {
				button->Allowed = ButtonCheckHasSubButtons;
			} else {
				LuaError(l, "Unsupported action: %s" _C_ value);
			}
		} else if (!strcmp(value, "AllowArg")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}

			const unsigned int subargs = lua_rawlen(l, -1);

			button->allow_strings.clear();
			for (unsigned int k = 0; k < subargs; ++k) {
				button->allow_strings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Key")) {
			std::string key(LuaToString(l, -1));
			button->Key = GetHotKey(key);
		} else if (!strcmp(value, "Hint")) {
			button->Hint = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			button->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "CommentSound")) {
			button->CommentSound.Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Popup")) {
			button->Popup = LuaToString(l, -1);
		} else if (!strcmp(value, "ForUnit")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			// FIXME: button->UnitMask shouldn't be a string
			std::string umask = ",";
			const unsigned subargs = lua_rawlen(l, -1);
			for (unsigned int k = 0; k < subargs; ++k) {
				const char *s2 = LuaToString(l, -1, k + 1);
				umask += s2;
				umask += ",";
			}
			button->UnitMask = umask;
			if (!strncmp(button->UnitMask.c_str(), ",*,", 3)) {
				button->UnitMask = "*";
			}
		//Wyrmgus start
		} else if (!strcmp(value, "Mod")) {
			button->Mod = LuaToString(l, -1);
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	return 0;
}

/**
**  Run the set-selection-changed-hook.
*/
void SelectionChanged()
{
	// We Changed out selection, anything pending buttonwise must be cleared
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentButtonLevel = nullptr;
	LastDrawnButtonPopup = nullptr;

	UI.ButtonPanel.Update();
	GameCursor = UI.get_cursor(wyrmgus::cursor_type::point);
	CursorBuilding = nullptr;
	CurrentCursorState = CursorState::Point;
	UI.ButtonPanel.Update();
}

/**
**  The selected unit has been altered.
*/
void SelectedUnitChanged()
{
	UI.ButtonPanel.Update();
}

/**
**  Set selection style.
**
**  @param l  Lua state.
*/
static int CclSetSelectionStyle(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const char *style = LuaToString(l, 1);
	if (!strcmp(style, "rectangle")) {
		DrawSelection = DrawSelectionRectangle;
	} else if (!strcmp(style, "alpha-rectangle")) {
		DrawSelection = DrawSelectionRectangleWithTrans;
	} else if (!strcmp(style, "circle")) {
		DrawSelection = DrawSelectionCircle;
	} else if (!strcmp(style, "alpha-circle")) {
		DrawSelection = DrawSelectionCircleWithTrans;
	} else if (!strcmp(style, "corners")) {
		DrawSelection = DrawSelectionCorners;
	} else {
		LuaError(l, "Unsupported selection style");
	}
	return 0;
}

/**
**  Add a new message.
**
**  @param l  Lua state.
*/
static int CclAddMessage(lua_State *l)
{
	LuaCheckArgs(l, 1);
	SetMessage("%s", LuaToString(l, 1));
	return 0;
}

//Wyrmgus start
/**
**  Add a new objective.
**
**  @param l  Lua state.
*/
static int CclAddObjective(lua_State *l)
{
	LuaCheckArgs(l, 1);
	SetObjective("%s", LuaToString(l, 1));
	return 0;
}

/**
**  Clean objectives.
**
**  @param l  Lua state.
*/
static int CclClearObjectives(lua_State *l)
{
	LuaCheckArgs(l, 0);
	CleanObjectives();
	return 0;
}
//Wyrmgus end

/**
**  Set the keys which are use for grouping units, helpful for other keyboards
**
**  @param l  Lua state.
*/
static int CclSetGroupKeys(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UiGroupKeys = LuaToString(l, 1);
	return 0;
}

/**
** Set basic map caracteristics.
**
**  @param l  Lua state.
*/
static int CclPresentMap(lua_State *l)
{
	//Wyrmgus start
	/*
	LuaCheckArgs(l, 5);

	Map.Info.Description = LuaToString(l, 1);
	// Number of players in LuaToNumber(l, 3); // Not used yet.
	Map.Info.MapWidth = LuaToNumber(l, 3);
	Map.Info.MapHeight = LuaToNumber(l, 4);
	Map.Info.MapUID = LuaToNumber(l, 5);
	*/

	CMap::Map.Info.Description = LuaToString(l, 1);
	
	if (lua_gettop(l) > 1) {
		LuaCheckArgs(l, 5);
		
		// Number of players in LuaToNumber(l, 3); // Not used yet.
		CMap::Map.Info.MapWidth = LuaToNumber(l, 3);
		CMap::Map.Info.MapHeight = LuaToNumber(l, 4);
		CMap::Map.Info.MapUID = LuaToNumber(l, 5);
	}
	//Wyrmgus end

	return 0;
}

/**
** Define the lua file that will build the map
**
**  @param l  Lua state.
*/
static int CclDefineMapSetup(lua_State *l)
{
	LuaCheckArgs(l, 1);
	CMap::Map.Info.Filename = LuaToString(l, 1);

	return 0;
}


/**
**  Define an icon.
**
**  @param l  Lua state.
*/
static int CclDefineIcon(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string ident;
	std::string file;
	Vec2i size(0, 0);
	int frame = 0;

	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 1); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "Name")) {
			ident = LuaToString(l, -1);
		} else if (!strcmp(value, "File")) {
			file = LuaToString(l, -1);
		} else if (!strcmp(value, "Size")) {
			CclGetPos(l, &size.x, &size.y);
		} else if (!strcmp(value, "Frame")) {
			frame = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	wyrmgus::icon *icon = wyrmgus::icon::add(ident, nullptr);
	icon->file = file;
	icon->set_frame(frame);
	icon->set_graphics(CPlayerColorGraphic::New(icon->get_file().string(), size, nullptr));

	return 0;
}

static int CclGetIconData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}

	const std::string icon_ident = LuaToString(l, 1);
	const wyrmgus::icon *icon = wyrmgus::icon::get(icon_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "File")) {
		if (!icon->is_loaded()) {
			//ensure that the icon is loaded if its graphics will be used by a widget
			icon->load();
		}

		lua_pushstring(l, icon->get_file().string().c_str());
		return 1;
	} if (!strcmp(data, "Frame")) {
		lua_pushnumber(l, icon->get_frame());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetIcons(lua_State *l)
{
	std::vector<std::string> icons;
	for (const wyrmgus::icon *icon : wyrmgus::icon::get_all()) {
		icons.push_back(icon->get_identifier());
	}
		
	lua_createtable(l, icons.size(), 0);
	for (size_t i = 1; i <= icons.size(); ++i)
	{
		lua_pushstring(l, icons[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Register CCL features for UI.
*/
void UserInterfaceCclRegister()
{
	lua_register(Lua, "AddMessage", CclAddMessage);
	//Wyrmgus start
	lua_register(Lua, "AddObjective", CclAddObjective);
	lua_register(Lua, "ClearObjectives", CclClearObjectives);
	//Wyrmgus end

	lua_register(Lua, "SetKeyScrollSpeed", CclSetKeyScrollSpeed);
	lua_register(Lua, "GetKeyScrollSpeed", CclGetKeyScrollSpeed);
	lua_register(Lua, "SetMouseScrollSpeed", CclSetMouseScrollSpeed);
	lua_register(Lua, "GetMouseScrollSpeed", CclGetMouseScrollSpeed);
	lua_register(Lua, "SetMouseScrollSpeedDefault", CclSetMouseScrollSpeedDefault);
	lua_register(Lua, "GetMouseScrollSpeedDefault", CclGetMouseScrollSpeedDefault);
	lua_register(Lua, "SetMouseScrollSpeedControl", CclSetMouseScrollSpeedControl);
	lua_register(Lua, "GetMouseScrollSpeedControl", CclGetMouseScrollSpeedControl);

	lua_register(Lua, "SetClickMissile", CclSetClickMissile);
	lua_register(Lua, "SetDamageMissile", CclSetDamageMissile);

	lua_register(Lua, "SetMaxOpenGLTexture", CclSetMaxOpenGLTexture);
	lua_register(Lua, "SetUseTextureCompression", CclSetUseTextureCompression);
	lua_register(Lua, "SetZoomNoResize", CclSetZoomNoResize);
	lua_register(Lua, "SetVideoResolution", CclSetVideoResolution);
	lua_register(Lua, "GetVideoResolution", CclGetVideoResolution);
	lua_register(Lua, "SetVideoFullScreen", CclSetVideoFullScreen);
	lua_register(Lua, "GetVideoFullScreen", CclGetVideoFullScreen);

	lua_register(Lua, "SetTitleScreens", CclSetTitleScreens);

	lua_register(Lua, "DefinePanelContents", CclDefinePanelContents);
	lua_register(Lua, "DefinePopup", CclDefinePopup);
	lua_register(Lua, "DefineViewports", CclDefineViewports);

	lua_register(Lua, "RightButtonAttacks", CclRightButtonAttacks);
	lua_register(Lua, "RightButtonMoves", CclRightButtonMoves);

	lua_register(Lua, "DefineButton", CclDefineButton);

	lua_register(Lua, "DefineButtonStyle", CclDefineButtonStyle);

	lua_register(Lua, "PresentMap", CclPresentMap);
	lua_register(Lua, "DefineMapSetup", CclDefineMapSetup);

	//
	// Look and feel of units
	//
	lua_register(Lua, "SetSelectionStyle", CclSetSelectionStyle);

	lua_register(Lua, "SetGroupKeys", CclSetGroupKeys);
	
	lua_register(Lua, "DefineIcon", CclDefineIcon);
	lua_register(Lua, "GetIconData", CclGetIconData);
	lua_register(Lua, "GetIcons", CclGetIcons);
}
