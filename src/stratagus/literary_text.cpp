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
/**@name literary_text.cpp - The literary text source file. */
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

#include "config.h"
#include "text_chapter.h"
#include "ui/icon.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CLiteraryText::~CLiteraryText()
{
	for (CLiteraryTextChapter *chapter : this->Chapters) {
		delete chapter;
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CLiteraryText::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "icon") {
		value = FindAndReplaceString(value, "_", "-");
		this->Icon = CIcon::Get(value);
	} else {
		return false;
	}
	
	return true;
}

CLiteraryTextChapter *CLiteraryText::GetChapter(const std::string &chapter_name) const
{
	for (CLiteraryTextChapter *chapter : this->Chapters) {
		if (chapter_name == chapter->Name) {
			return chapter;
		}
	}
	
	return nullptr;
}

void CLiteraryText::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_set_name", "name"), &CLiteraryText::SetName);
	ClassDB::bind_method(D_METHOD("get_name"), &CLiteraryText::GetName);
	ClassDB::bind_method(D_METHOD("_set_author", "author"), &CLiteraryText::SetAuthor);
	ClassDB::bind_method(D_METHOD("get_author"), &CLiteraryText::GetAuthor);
	ClassDB::bind_method(D_METHOD("_set_hidden", "hidden"), &CLiteraryText::SetHidden);
	ClassDB::bind_method(D_METHOD("is_hidden"), &CLiteraryText::IsHidden);
	ClassDB::bind_method(D_METHOD("_set_translator", "translator"), &CLiteraryText::SetTranslator);
	ClassDB::bind_method(D_METHOD("get_translator"), &CLiteraryText::GetTranslator);
	ClassDB::bind_method(D_METHOD("_set_publisher", "publisher"), &CLiteraryText::SetPublisher);
	ClassDB::bind_method(D_METHOD("get_publisher"), &CLiteraryText::GetPublisher);
	ClassDB::bind_method(D_METHOD("_set_copyright_notice", "copyright_notice"), &CLiteraryText::SetCopyrightNotice);
	ClassDB::bind_method(D_METHOD("get_copyright_notice"), &CLiteraryText::GetCopyrightNotice);
	ClassDB::bind_method(D_METHOD("_set_notes", "notes"), &CLiteraryText::SetNotes);
	ClassDB::bind_method(D_METHOD("get_notes"), &CLiteraryText::GetNotes);
	ClassDB::bind_method(D_METHOD("_set_year", "year"), &CLiteraryText::SetYear);
	ClassDB::bind_method(D_METHOD("get_year"), &CLiteraryText::GetYear);
	ClassDB::bind_method(D_METHOD("_set_initial_page", "initial_page"), &CLiteraryText::SetInitialPage);
	ClassDB::bind_method(D_METHOD("get_initial_page"), &CLiteraryText::GetInitialPage);
	ClassDB::bind_method(D_METHOD("get_icon"), &CLiteraryText::GetIcon);
	
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "_set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "author"), "_set_author", "get_author");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "hidden"), "_set_hidden", "is_hidden");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "translator"), "_set_translator", "get_translator");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "publisher"), "_set_publisher", "get_publisher");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "copyright_notice"), "_set_copyright_notice", "get_copyright_notice");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "notes"), "_set_notes", "get_notes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "year"), "_set_year", "get_year");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_page"), "_set_initial_page", "get_initial_page");
}
