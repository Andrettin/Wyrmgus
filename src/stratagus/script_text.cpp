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
/**@name script_text.cpp - The text ccl functions. */
//
//      (c) Copyright 2016 by Andrettin
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

#include "text.h"

#include "script.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define a text.
**
**  @param l  Lua state.
*/
static int CclDefineText(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string text_name = LuaToString(l, 1);
	CText *text = GetText(text_name);
	if (!text) {
		text = new CText;
		Texts.push_back(text);
		text->Name = text_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Author")) {
			text->Author = LuaToString(l, -1);
		} else if (!strcmp(value, "Translator")) {
			text->Translator = LuaToString(l, -1);
		} else if (!strcmp(value, "Publisher")) {
			text->Publisher = LuaToString(l, -1);
		} else if (!strcmp(value, "CopyrightNotice")) {
			text->CopyrightNotice = LuaToString(l, -1);
		} else if (!strcmp(value, "Notes")) {
			text->Notes = LuaToString(l, -1);
		} else if (!strcmp(value, "Year")) {
			text->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "InitialPage")) {
			text->InitialPage = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Chapters")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CChapter *chapter = new CChapter;
				text->Chapters.push_back(chapter);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for variations)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					lua_rawgeti(l, -1, k + 1);
					if (!strcmp(value, "name")) {
						chapter->Name = LuaToString(l, -1);
					} else if (!strcmp(value, "introduction")) {
						chapter->Introduction = LuaToBoolean(l, -1);
					} else if (!strcmp(value, "text")) {
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							chapter->Pages.push_back(LuaToString(l, -1, n + 1));
						}
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
					lua_pop(l, 1);
				}
				lua_pop(l, 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetTexts(lua_State *l)
{
	lua_createtable(l, Texts.size(), 0);
	for (size_t i = 1; i <= Texts.size(); ++i)
	{
		lua_pushstring(l, Texts[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get text data.
**
**  @param l  Lua state.
*/
static int CclGetTextData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string text_name = LuaToString(l, 1);
//	const CText *text = GetText(text_name);
	CText *text = GetText(text_name);
	if (!text) {
		LuaError(l, "Text \"%s\" doesn't exist." _C_ text_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Author")) {
		lua_pushstring(l, text->Author.c_str());
		return 1;
	} else if (!strcmp(data, "Translator")) {
		lua_pushstring(l, text->Translator.c_str());
		return 1;
	} else if (!strcmp(data, "Publisher")) {
		lua_pushstring(l, text->Publisher.c_str());
		return 1;
	} else if (!strcmp(data, "CopyrightNotice")) {
		lua_pushstring(l, text->CopyrightNotice.c_str());
		return 1;
	} else if (!strcmp(data, "Notes")) {
		lua_pushstring(l, text->Notes.c_str());
		return 1;
	} else if (!strcmp(data, "Year")) {
		lua_pushnumber(l, text->Year);
		return 1;
	} else if (!strcmp(data, "InitialPage")) {
		lua_pushnumber(l, text->InitialPage);
		return 1;
	} else if (!strcmp(data, "Chapters")) {
		lua_createtable(l, text->Chapters.size(), 0);
		for (size_t i = 1; i <= text->Chapters.size(); ++i)
		{
			lua_pushstring(l, text->Chapters[i-1]->Name.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "ChapterQuantity")) {
		lua_pushnumber(l, text->Chapters.size());
		return 1;
	} else if (!strcmp(data, "ChapterIndex")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetChapter(chapter_name) != NULL) {
			lua_pushnumber(l, text->GetChapter(chapter_name)->ID);
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->Name.c_str());
		}
		return 1;
	} else if (!strcmp(data, "ChapterIntroduction")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetChapter(chapter_name) != NULL) {
			lua_pushboolean(l, text->GetChapter(chapter_name)->Introduction);
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->Name.c_str());
		}
		return 1;
	} else if (!strcmp(data, "ChapterPage")) {
		LuaCheckArgs(l, 4);
		
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetChapter(chapter_name) != NULL) {
			int page = LuaToNumber(l, 4) - 1;
			
			lua_pushstring(l, text->GetChapter(chapter_name)->Pages[page].c_str());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->Name.c_str());
		}
		return 1;
	} else if (!strcmp(data, "ChapterPageQuantity")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetChapter(chapter_name) != NULL) {
			lua_pushnumber(l, text->GetChapter(chapter_name)->Pages.size());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->Name.c_str());
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for quests.
*/
void TextCclRegister()
{
	lua_register(Lua, "DefineText", CclDefineText);
	lua_register(Lua, "GetTexts", CclGetTexts);
	lua_register(Lua, "GetTextData", CclGetTextData);
}

//@}
