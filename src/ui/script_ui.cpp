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
//      (c) Copyright 1999-2019 by Lutz Sammer, Jimmy Salmon, Martin Renold
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ui/ui.h"

#include "map/map.h"
#include "menus.h"
#include "script.h"
#include "spell/spells.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/contenttype.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/popup.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "util.h"
#include "video/font.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::string ClickMissile;		/// FIXME:docu
std::string DamageMissile;		/// FIXME:docu
std::map<std::string, ButtonStyle *> ButtonStyleHash;

static int HandleCount = 1;		/// Lua handler count

CPreference Preference;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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

static int CclSetUseOpenGL(lua_State *l)
{
	LuaCheckArgs(l, 1);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (CclInConfigFile) {
		// May have been set from the command line
		if (!ForceUseOpenGL) {
			UseOpenGL = LuaToBoolean(l, 1);
		}
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
		if (!ForceUseOpenGL) {
			ZoomNoResize = LuaToBoolean(l, 1);
			if (ZoomNoResize) {
				UseOpenGL = true;
			}
		}
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
**  Return enum from string about variable component.
**
**  @param l Lua State.
**  @param s string to convert.
**
**  @return  Corresponding value.
**  @note    Stop on error.
*/
EnumVariable Str2EnumVariable(lua_State *l, const char *s)
{
	static struct {
		const char *s;
		EnumVariable e;
	} list[] = {
		{"Value", VariableValue},
		{"Max", VariableMax},
		{"Increase", VariableIncrease},
		{"Diff", VariableDiff},
		{"Percent", VariablePercent},
		{"Name", VariableName},
		//Wyrmgus start
		{"Change", VariableChange},
		{"IncreaseChange", VariableIncreaseChange},
		//Wyrmgus end
		{0, VariableValue}
	}; // List of possible values.

	for (int i = 0; list[i].s; ++i) {
		if (!strcmp(s, list[i].s)) {
			return list[i].e;
		}
	}
	LuaError(l, "'%s' is a invalid variable component" _C_ s);
	return VariableValue;
}

/**
**  Parse the condition Panel.
**
**  @param l   Lua State.
*/
static ConditionPanel *ParseConditionPanel(lua_State *l)
{
	Assert(lua_istable(l, -1));

	ConditionPanel *condition = new ConditionPanel;

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
					size_t new_bool_size = UnitTypeVar.GetNumberBoolFlag();
					condition->BoolFlags = new char[new_bool_size];
					memset(condition->BoolFlags, 0, new_bool_size * sizeof(char));
				}
				condition->BoolFlags[index] = Ccl2Condition(l, LuaToString(l, -1));
				continue;
			}
			index = UnitTypeVar.VariableNameLookup[key];
			if (index != -1) {
				if (!condition->Variables) {
					size_t new_variables_size = UnitTypeVar.GetNumberVariable();
					condition->Variables = new char[new_variables_size];
					memset(condition->Variables, 0, new_variables_size * sizeof(char));
				}
				condition->Variables[index] = Ccl2Condition(l, LuaToString(l, -1));
				continue;
			}
			LuaError(l, "'%s' invalid for Condition in DefinePanelContents" _C_ key);
		}
	}
	return condition;
}

static CContentType *CclParseContent(lua_State *l)
{
	Assert(lua_istable(l, -1));

	CContentType *content = nullptr;
	ConditionPanel *condition = nullptr;
	PixelPos pos(0, 0);
	//Wyrmgus start
	std::string textColor("white");
	std::string highColor("red");
	//Wyrmgus end

	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);
		if (!strcmp(key, "Pos")) {
			CclGetPos(l, &pos.x, &pos.y);
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
				content = new CContentTypeText;
			} else if (!strcmp(key, "FormattedText")) {
				content = new CContentTypeFormattedText;
			} else if (!strcmp(key, "FormattedText2")) {
				content = new CContentTypeFormattedText2;
			} else if (!strcmp(key, "Icon")) {
				content = new CContentTypeIcon;
			} else if (!strcmp(key, "LifeBar")) {
				content = new CContentTypeLifeBar;
			} else if (!strcmp(key, "CompleteBar")) {
				content = new CContentTypeCompleteBar;
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
	content->Condition = condition;
	//Wyrmgus start
	content->TextColor = textColor;
	content->HighlightColor = highColor;
	//Wyrmgus end
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
		CUnitInfoPanel *infopanel = new CUnitInfoPanel;

		for (lua_pushnil(l); lua_next(l, i + 1); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);

			if (!strcmp(key, "Ident")) {
				infopanel->Name = LuaToString(l, -1);
			} else if (!strcmp(key, "Pos")) {
				CclGetPos(l, &infopanel->PosX, &infopanel->PosY);
			} else if (!strcmp(key, "DefaultFont")) {
				infopanel->DefaultFont = CFont::Get(LuaToString(l, -1));
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
		for (std::vector<CContentType *>::iterator content = infopanel->Contents.begin();
			 content != infopanel->Contents.end(); ++content) { // Default value for invalid value.
			(*content)->Pos.x += infopanel->PosX;
			(*content)->Pos.y += infopanel->PosY;
		}
		size_t j;
		for (j = 0; j < UI.InfoPanelContents.size(); ++j) {
			if (infopanel->Name == UI.InfoPanelContents[j]->Name) {
				DebugPrint("Redefinition of Panel '%s'\n" _C_ infopanel->Name.c_str());
				delete UI.InfoPanelContents[j];
				UI.InfoPanelContents[j] = infopanel;
				break;
			}
		}
		if (j == UI.InfoPanelContents.size()) {
			UI.InfoPanelContents.push_back(infopanel);
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

	CPopup *popup = new CPopup;

	for (lua_pushnil(l); lua_next(l, 1); lua_pop(l, 1)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Ident")) {
			popup->Ident = LuaToString(l, -1);
		} else if (!strcmp(key, "DefaultFont")) {
			popup->DefaultFont = CFont::Get(LuaToString(l, -1));
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
			delete UI.ButtonPopups[j];
			UI.ButtonPopups[j] = popup;
			return 0;
		}
	}
	UI.ButtonPopups.push_back(popup);
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
				UI.Viewports[i].Unit = &UnitManager.GetSlotUnit(slot);
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
**  Enable/disable the fancy buildings.
**
**  @param l  Lua state.
*/
static int CclSetFancyBuildings(lua_State *l)
{
	LuaCheckArgs(l, 1);
	FancyBuildings = LuaToBoolean(l, 1);
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
				p->TextAlign = TextAlignCenter;
			} else if (!strcmp(value, "Right")) {
				p->TextAlign = TextAlignRight;
			} else if (!strcmp(value, "Left")) {
				p->TextAlign = TextAlignLeft;
			} else {
				LuaError(l, "Invalid text alignment: %s" _C_ value);
			}
		} else if (!strcmp(value, "TextNormalColor")) {
			p->TextNormalColor = LuaToString(l, -1);
		} else if (!strcmp(value, "TextReverseColor")) {
			p->TextReverseColor = LuaToString(l, -1);
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
			b->Font = CFont::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "TextNormalColor")) {
			b->TextNormalColor = LuaToString(l, -1);
		} else if (!strcmp(value, "TextReverseColor")) {
			b->TextReverseColor = LuaToString(l, -1);
		} else if (!strcmp(value, "TextPos")) {
			CclGetPos(l, &b->TextX, &b->TextY);
		} else if (!strcmp(value, "TextAlign")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "Center")) {
				b->TextAlign = TextAlignCenter;
			} else if (!strcmp(value, "Right")) {
				b->TextAlign = TextAlignRight;
			} else if (!strcmp(value, "Left")) {
				b->TextAlign = TextAlignLeft;
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

	if (b->Default.TextAlign == TextAlignUndefined) {
		b->Default.TextAlign = b->TextAlign;
	}
	if (b->Hover.TextAlign == TextAlignUndefined) {
		b->Hover.TextAlign = b->TextAlign;
	}
	if (b->Clicked.TextAlign == TextAlignUndefined) {
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
	ButtonAction ba;

	//
	// Parse the arguments
	//
	lua_pushnil(l);
	while (lua_next(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Pos")) {
			ba.Pos = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Level")) {
			ba.Level = CButtonLevel::GetButtonLevel(LuaToString(l, -1));
		} else if (!strcmp(value, "AlwaysShow")) {
			ba.AlwaysShow = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			ba.Icon.Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Action")) {
			value = LuaToString(l, -1);
			int button_action_id = GetButtonActionIdByName(std::string(value));
			if (button_action_id != -1) {
				ba.Action = ButtonCmd(button_action_id);
			} else {
				LuaError(l, "Unsupported button action: %s" _C_ value);
			}
			//Wyrmgus end
		} else if (!strcmp(value, "Value")) {
			if (!lua_isnumber(l, -1) && !lua_isstring(l, -1) && !lua_isfunction(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			char buf[64];
			const char *s2;

			if (lua_isnumber(l, -1)) {
				snprintf(buf, sizeof(buf), "%ld", (long int)lua_tonumber(l, -1));
				s2 = buf;
			} else {
				s2 = lua_tostring(l, -1);
			}
			ba.ValueStr = s2;
		} else if (!strcmp(value, "Allowed")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "check-true")) {
				ba.Allowed = ButtonCheckTrue;
			} else if (!strcmp(value, "check-false")) {
				ba.Allowed = ButtonCheckFalse;
			} else if (!strcmp(value, "check-upgrade")) {
				ba.Allowed = ButtonCheckUpgrade;
			} else if (!strcmp(value, "check-upgrade-not")) {
				ba.Allowed = ButtonCheckUpgradeNot;
			} else if (!strcmp(value, "check-upgrade-or")) {
				ba.Allowed = ButtonCheckUpgradeOr;
			} else if (!strcmp(value, "check-individual-upgrade")) {
				ba.Allowed = ButtonCheckIndividualUpgrade;
			} else if (!strcmp(value, "check-individual-upgrade-or")) {
				ba.Allowed = ButtonCheckIndividualUpgradeOr;
			} else if (!strcmp(value, "check-unit-variable")) {
				ba.Allowed = ButtonCheckUnitVariable;
			} else if (!strcmp(value, "check-units-or")) {
				ba.Allowed = ButtonCheckUnitsOr;
			} else if (!strcmp(value, "check-units-and")) {
				ba.Allowed = ButtonCheckUnitsAnd;
			} else if (!strcmp(value, "check-units-not")) {
				ba.Allowed = ButtonCheckUnitsNot;
			} else if (!strcmp(value, "check-network")) {
				ba.Allowed = ButtonCheckNetwork;
			} else if (!strcmp(value, "check-no-network")) {
				ba.Allowed = ButtonCheckNoNetwork;
			} else if (!strcmp(value, "check-no-work")) {
				ba.Allowed = ButtonCheckNoWork;
			} else if (!strcmp(value, "check-no-research")) {
				ba.Allowed = ButtonCheckNoResearch;
			} else if (!strcmp(value, "check-attack")) {
				ba.Allowed = ButtonCheckAttack;
			} else if (!strcmp(value, "check-upgrade-to")) {
				ba.Allowed = ButtonCheckUpgradeTo;
			} else if (!strcmp(value, "check-research")) {
				ba.Allowed = ButtonCheckResearch;
			} else if (!strcmp(value, "check-single-research")) {
				ba.Allowed = ButtonCheckSingleResearch;
			} else if (!strcmp(value, "check-has-inventory")) {
				ba.Allowed = ButtonCheckHasInventory;
			} else if (!strcmp(value, "check-has-sub-buttons")) {
				ba.Allowed = ButtonCheckHasSubButtons;
			} else {
				LuaError(l, "Unsupported action: %s" _C_ value);
			}
		} else if (!strcmp(value, "AllowArg")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			std::string allowstr;
			const unsigned int subargs = lua_rawlen(l, -1);

			for (unsigned int k = 0; k < subargs; ++k) {
				const char *s2 = LuaToString(l, -1, k + 1);
				allowstr += s2;
				if (k != subargs - 1) {
					allowstr += ",";
				}
			}
			ba.AllowStr = allowstr;
		} else if (!strcmp(value, "Key")) {
			std::string key(LuaToString(l, -1));
			ba.Key = GetHotKey(key);
		} else if (!strcmp(value, "Hint")) {
			ba.Hint = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			ba.Description = LuaToString(l, -1);
		} else if (!strcmp(value, "CommentSound")) {
			ba.CommentSound.Name = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonCursor")) {
			ba.ButtonCursor = LuaToString(l, -1);
		} else if (!strcmp(value, "Popup")) {
			ba.Popup = LuaToString(l, -1);
		} else if (!strcmp(value, "ForUnit")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			// FIXME: ba.UnitMask shouldn't be a string
			std::string umask = ",";
			const unsigned subargs = lua_rawlen(l, -1);
			for (unsigned int k = 0; k < subargs; ++k) {
				const char *s2 = LuaToString(l, -1, k + 1);
				umask += s2;
				umask += ",";
			}
			ba.UnitMask = umask;
			if (!strncmp(ba.UnitMask.c_str(), ",*,", 3)) {
				ba.UnitMask = "*";
			}
		//Wyrmgus start
		} else if (!strcmp(value, "Mod")) {
			ba.Mod = LuaToString(l, -1);
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}
	AddButton(ba.Pos, ba.Level, ba.Icon.Name, ba.Action, ba.ValueStr,
			  ba.Allowed, ba.AllowStr, ba.Key, ba.Hint, ba.Description, ba.CommentSound.Name,
			  //Wyrmgus start
//			  ba.ButtonCursor, ba.UnitMask, ba.Popup, ba.AlwaysShow);
			  ba.ButtonCursor, ba.UnitMask, ba.Popup, ba.AlwaysShow, ba.Mod);
			  //Wyrmgus end
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
	GameCursor = UI.Point.Cursor;
	CursorBuilding = nullptr;
	CursorState = CursorStatePoint;
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

	CMap::Map.Info.Description = LuaToString(l, 1);
	// Number of players in LuaToNumber(l, 3); // Not used yet.
	CMap::Map.Info.MapWidth = LuaToNumber(l, 3);
	CMap::Map.Info.MapHeight = LuaToNumber(l, 4);
	CMap::Map.Info.MapUID = LuaToNumber(l, 5);
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
	
	CIcon *icon = CIcon::GetOrAdd(ident);
	icon->File = file.c_str();
	icon->Size = size;
	icon->Frame = frame;
	icon->G = CPlayerColorGraphic::New(icon->File.utf8().get_data(), icon->Size.x, icon->Size.y);
	
	return 0;
}


/**
**  Get icon data.
**
**  @param l  Lua state.
*/
static int CclGetIconData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string icon_ident = LuaToString(l, 1);
	const CIcon *icon = CIcon::Get(icon_ident);
	if (!icon) {
		LuaError(l, "Icon \"%s\" doesn't exist." _C_ icon_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "File")) {
		lua_pushstring(l, icon->GetFile().utf8().get_data());
		return 1;
	} if (!strcmp(data, "Frame")) {
		lua_pushnumber(l, icon->GetFrame());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetIcons(lua_State *l)
{
	std::vector<std::string> icons;
	for (const CIcon *icon : CIcon::GetAll()) {
		icons.push_back(icon->GetIdent().utf8().get_data());
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
	CursorCclRegister();
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
	lua_register(Lua, "SetUseOpenGL", CclSetUseOpenGL);
	lua_register(Lua, "SetZoomNoResize", CclSetZoomNoResize);
	lua_register(Lua, "SetVideoResolution", CclSetVideoResolution);
	lua_register(Lua, "GetVideoResolution", CclGetVideoResolution);
	lua_register(Lua, "SetVideoFullScreen", CclSetVideoFullScreen);
	lua_register(Lua, "GetVideoFullScreen", CclGetVideoFullScreen);

	lua_register(Lua, "DefinePanelContents", CclDefinePanelContents);
	lua_register(Lua, "DefinePopup", CclDefinePopup);
	lua_register(Lua, "DefineViewports", CclDefineViewports);

	lua_register(Lua, "RightButtonAttacks", CclRightButtonAttacks);
	lua_register(Lua, "RightButtonMoves", CclRightButtonMoves);
	lua_register(Lua, "SetFancyBuildings", CclSetFancyBuildings);

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
