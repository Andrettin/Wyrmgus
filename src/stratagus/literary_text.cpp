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
	if (key == "section") {
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
		const int page_number = static_cast<int>(this->GetPages().size()) + 1;
		
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
	CLiteraryText *previous_section = nullptr;
	for (CLiteraryText *section : this->GetSections()) {
		if (previous_section != nullptr) {
			previous_section->NextSection = section;
			section->PreviousSection = previous_section;
		}
		
		previous_section = section;
	}
	
	this->Initialized = true;
	
	//update the initial page numbers of each literary text section, according to the quantity of pages before
	if (CLiteraryText::AreAllInitialized()) {
		for (CLiteraryText *literary_text : CLiteraryText::GetAll()) {
			if (literary_text->GetMainText() != nullptr) {
				continue;
			}
			
			if (literary_text->InitialPageNumber == 0) {
				literary_text->InitialPageNumber = 1;
			}
			
			literary_text->UpdateSectionPageNumbers();
		}
	}
}

CLiteraryText *CLiteraryText::GetSection(const std::string &section_name) const
{
	for (CLiteraryText *section : this->GetSections()) {
		if (String(section_name.c_str()) == section->Name) {
			return section;
		}
	}
	
	return nullptr;
}

void CLiteraryText::UpdateSectionPageNumbers() const
{
	int page_offset = this->InitialPageNumber;
	if (this->IsPageNumberingEnabled()) {
		page_offset += std::max(static_cast<int>(this->GetPages().size()), 1);
	}
	
	for (CLiteraryText *section : this->GetSections()) {
		if (section->InitialPageNumber == 0) {
			section->InitialPageNumber = page_offset;
		}
		
		section->UpdateSectionPageNumbers();
		
		page_offset = section->InitialPageNumber + section->GetTotalPageCount();
	}
}

void CLiteraryText::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_author"), [](const CLiteraryText *literary_text){ return literary_text->Author.Get(); });
	ClassDB::bind_method(D_METHOD("is_hidden"), [](const CLiteraryText *literary_text){ return literary_text->Hidden.Get(); });
	ClassDB::bind_method(D_METHOD("get_translator"), [](const CLiteraryText *literary_text){ return literary_text->Translator.Get(); });
	ClassDB::bind_method(D_METHOD("get_publisher"), [](const CLiteraryText *literary_text){ return literary_text->Publisher.Get(); });
	ClassDB::bind_method(D_METHOD("get_license"), [](const CLiteraryText *literary_text){ return literary_text->License.Get(); });
	ClassDB::bind_method(D_METHOD("get_notes"), [](const CLiteraryText *literary_text){ return literary_text->Notes.Get(); });
	ClassDB::bind_method(D_METHOD("get_publication_year"), [](const CLiteraryText *literary_text){ return literary_text->PublicationYear.Get(); });
	ClassDB::bind_method(D_METHOD("get_initial_page_number"), [](const CLiteraryText *literary_text){ return literary_text->InitialPageNumber.Get(); });
	ClassDB::bind_method(D_METHOD("get_icon"), [](const CLiteraryText *literary_text){ return literary_text->Icon.Get(); });
	ClassDB::bind_method(D_METHOD("get_sections"), &CLiteraryText::GetSectionsArray);
	ClassDB::bind_method(D_METHOD("get_main_text"), &CLiteraryText::GetMainText);
	ClassDB::bind_method(D_METHOD("get_previous_section"), &CLiteraryText::GetPreviousSection);
	ClassDB::bind_method(D_METHOD("get_next_section"), &CLiteraryText::GetNextSection);
	ClassDB::bind_method(D_METHOD("get_first_page"), &CLiteraryText::GetFirstPage);
	ClassDB::bind_method(D_METHOD("get_last_page"), &CLiteraryText::GetLastPage);
	ClassDB::bind_method(D_METHOD("is_page_numbering_enabled"), &CLiteraryText::IsPageNumberingEnabled);
	ClassDB::bind_method(D_METHOD("has_lowercase_roman_numeral_page_numbering"), &CLiteraryText::HasLowercaseRomanNumeralPageNumbering);
}
