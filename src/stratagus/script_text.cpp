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
//      (c) Copyright 2016-2019 by Andrettin
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

#include "literary_text.h"

#include "literary_text_page.h"
#include "script.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

static int CclGetTexts(lua_State *l)
{
	lua_createtable(l, CLiteraryText::GetAll().size(), 0);
	for (size_t i = 1; i <= CLiteraryText::GetAll().size(); ++i)
	{
		lua_pushstring(l, CLiteraryText::GetAll()[i-1]->GetIdent().utf8().get_data());
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
	std::string text_ident = LuaToString(l, 1);
	const CLiteraryText *text = CLiteraryText::Get(text_ident);
	if (!text) {
		LuaError(l, "Text \"%s\" doesn't exist." _C_ text_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, text->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Author")) {
		lua_pushstring(l, text->GetAuthor().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Translator")) {
		lua_pushstring(l, text->GetTranslator().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Publisher")) {
		lua_pushstring(l, text->GetPublisher().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "CopyrightNotice")) {
		lua_pushstring(l, text->GetLicense().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Notes")) {
		lua_pushstring(l, text->GetNotes().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Year")) {
		lua_pushnumber(l, text->GetPublicationYear());
		return 1;
	} else if (!strcmp(data, "InitialPage")) {
		lua_pushnumber(l, text->GetInitialPageNumber());
		return 1;
	} else if (!strcmp(data, "Chapters")) {
		lua_createtable(l, text->GetSections().size(), 0);
		for (size_t i = 1; i <= text->GetSections().size(); ++i)
		{
			lua_pushstring(l, text->GetSections()[i-1]->GetName().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "ChapterQuantity")) {
		lua_pushnumber(l, text->GetSections().size());
		return 1;
	} else if (!strcmp(data, "ChapterIndex")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetSection(chapter_name) != nullptr) {
			lua_pushnumber(l, text->GetSection(chapter_name)->GetSectionIndex());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->GetIdent().utf8().get_data());
		}
		return 1;
	} else if (!strcmp(data, "ChapterIntroduction")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetSection(chapter_name) != nullptr) {
			lua_pushboolean(l, text->GetSection(chapter_name)->IsIntroduction());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->GetIdent().utf8().get_data());
		}
		return 1;
	} else if (!strcmp(data, "ChapterPage")) {
		LuaCheckArgs(l, 4);
		
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetSection(chapter_name) != nullptr) {
			int page = LuaToNumber(l, 4) - 1;
			
			lua_pushstring(l, text->GetSection(chapter_name)->GetPages()[page]->GetText().utf8().get_data());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->GetIdent().utf8().get_data());
		}
		return 1;
	} else if (!strcmp(data, "ChapterPageQuantity")) {
		LuaCheckArgs(l, 3);
		std::string chapter_name = LuaToString(l, 3);
		if (text->GetSection(chapter_name) != nullptr) {
			lua_pushnumber(l, text->GetSection(chapter_name)->GetPages().size());
		} else {
			LuaError(l, "Chapter \"%s\" doesn't exist for text \"%s\"" _C_ chapter_name.c_str() _C_ text->GetIdent().utf8().get_data());
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
	lua_register(Lua, "GetTexts", CclGetTexts);
	lua_register(Lua, "GetTextData", CclGetTextData);
}
