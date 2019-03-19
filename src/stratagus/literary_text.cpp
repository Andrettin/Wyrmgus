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
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CLiteraryText::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "author") {
			this->Author = value;
		} else if (key == "translator") {
			this->Translator = value;
		} else if (key == "publisher") {
			this->Publisher = value;
		} else if (key == "copyright_notice") {
			this->CopyrightNotice = value;
		} else if (key == "notes") {
			this->Notes = value;
		} else if (key == "year") {
			this->Year = std::stoi(value);
		} else if (key == "initial_page") {
			this->InitialPage = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid literary text property: \"%s\".\n", key.c_str());
		}
	}
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
	ClassDB::bind_method(D_METHOD("get_name"), &CLiteraryText::GetName);
	ClassDB::bind_method(D_METHOD("get_author"), &CLiteraryText::GetAuthor);
	ClassDB::bind_method(D_METHOD("get_translator"), &CLiteraryText::GetTranslator);
	ClassDB::bind_method(D_METHOD("get_publisher"), &CLiteraryText::GetPublisher);
	ClassDB::bind_method(D_METHOD("get_copyright_notice"), &CLiteraryText::GetCopyrightNotice);
	ClassDB::bind_method(D_METHOD("get_notes"), &CLiteraryText::GetNotes);
	ClassDB::bind_method(D_METHOD("get_year"), &CLiteraryText::GetYear);
	ClassDB::bind_method(D_METHOD("get_initial_page"), &CLiteraryText::GetInitialPage);
}
