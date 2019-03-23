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
#include "literary_text_page.h"
#include "ui/icon.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
	if (key == "name") {
		this->Name = value.c_str();
	} else if (key == "hidden") {
		this->Hidden = StringToBool(value);
	} else if (key == "author") {
		this->Author = value.c_str();
	} else if (key == "translator") {
		this->Translator = value.c_str();
	} else if (key == "publisher") {
		this->Publisher = value.c_str();
	} else if (key == "copyright_notice") {
		this->CopyrightNotice = value.c_str();
	} else if (key == "notes") {
		this->Notes = value.c_str();
	} else if (key == "publication_year") {
		this->PublicationYear = std::stoi(value);
	} else if (key == "initial_page") {
		this->InitialPage = std::stoi(value);
	} else if (key == "icon") {
		value = FindAndReplaceString(value, "_", "-");
		this->Icon = CIcon::Get(value);
	} else if (key == "section") {
		value = FindAndReplaceString(value, "_", "-");
		CLiteraryText *section = CLiteraryText::Get(value);
		if (section != nullptr) {
			this->Sections.push_back(section);
			section->MainText = this;
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CLiteraryText::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "page") {
		const int page_number = static_cast<int>(this->GetPages().size());
		
		CLiteraryTextPage *previous_page = nullptr;
		if (!this->GetPages().empty()) {
			previous_page = this->GetPages().back();
		}
		
		CLiteraryTextPage *page = new CLiteraryTextPage(page_number, previous_page);
		page->ProcessConfigData(section);
		this->Pages.push_back(page);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the literary text
*/
void CLiteraryText::Initialize()
{
	if (!this->GetPages().empty() && !this->GetSections().empty()) {
		fprintf(stderr, "The literary text \"%s\" has both pages and sections, but if it has sections then it should have no pages directly.\n", this->GetIdent().utf8().get_data());
	}
	
	CLiteraryText *previous_section = nullptr;
	for (CLiteraryText *section : this->GetSections()) {
		if (previous_section != nullptr) {
			previous_section->NextSection = section;
			section->PreviousSection = previous_section;
		}
		
		previous_section = section;
	}
	
	this->Initialized = true;
}

CLiteraryText *CLiteraryText::GetSection(const std::string &section_name) const
{
	for (CLiteraryText *section : this->GetSections()) {
		if (String(section_name.c_str()) == section->GetName()) {
			return section;
		}
	}
	
	return nullptr;
}

void CLiteraryText::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name"), &CLiteraryText::GetName);
	ClassDB::bind_method(D_METHOD("get_author"), &CLiteraryText::GetAuthor);
	ClassDB::bind_method(D_METHOD("is_hidden"), &CLiteraryText::IsHidden);
	ClassDB::bind_method(D_METHOD("get_translator"), &CLiteraryText::GetTranslator);
	ClassDB::bind_method(D_METHOD("get_publisher"), &CLiteraryText::GetPublisher);
	ClassDB::bind_method(D_METHOD("get_copyright_notice"), &CLiteraryText::GetCopyrightNotice);
	ClassDB::bind_method(D_METHOD("get_notes"), &CLiteraryText::GetNotes);
	ClassDB::bind_method(D_METHOD("get_publication_year"), &CLiteraryText::GetPublicationYear);
	ClassDB::bind_method(D_METHOD("get_initial_page"), &CLiteraryText::GetInitialPage);
	ClassDB::bind_method(D_METHOD("get_icon"), &CLiteraryText::GetIcon);
	ClassDB::bind_method(D_METHOD("get_main_text"), &CLiteraryText::GetMainText);
	ClassDB::bind_method(D_METHOD("get_previous_section"), &CLiteraryText::GetPreviousSection);
	ClassDB::bind_method(D_METHOD("get_next_section"), &CLiteraryText::GetNextSection);
	ClassDB::bind_method(D_METHOD("get_first_page"), &CLiteraryText::GetFirstPage);
	ClassDB::bind_method(D_METHOD("get_last_page"), &CLiteraryText::GetLastPage);
}
