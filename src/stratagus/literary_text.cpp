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
		section->ProcessPropertiesForObject(*page);
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
	for (CLiteraryText *section : this->Sections) {
		section->MainText = this;
	}
	
	CLiteraryText *previous_section = nullptr;
	for (CLiteraryText *section : this->Sections) {
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
	for (CLiteraryText *section : this->Sections) {
		if (String(section_name.c_str()) == section->Name) {
			return section;
		}
	}
	
	return nullptr;
}

void CLiteraryText::UpdateSectionPageNumbers() const
{
	int page_offset = this->InitialPageNumber;
	if (this->PageNumberingEnabled) {
		page_offset += std::max(static_cast<int>(this->GetPages().size()), 1);
	}
	
	for (CLiteraryText *section : this->Sections) {
		if (section->InitialPageNumber == 0) {
			section->InitialPageNumber = page_offset;
		}
		
		section->UpdateSectionPageNumbers();
		
		page_offset = section->InitialPageNumber + section->GetTotalPageCount();
	}
}

void CLiteraryText::_bind_methods()
{
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("set_hidden", "hidden"), [](CLiteraryText *text, const bool hidden){ text->Hidden = hidden; });
	ClassDB::bind_method(D_METHOD("is_hidden"), [](CLiteraryText *text){ return text->Hidden; });
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hidden"), "set_hidden", "is_hidden");
	
	ClassDB::bind_method(D_METHOD("set_author", "author"), [](CLiteraryText *text, const String &author){ text->Author = author; });
	ClassDB::bind_method(D_METHOD("get_author"), [](CLiteraryText *text){ return text->Author; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "author"), "set_author", "get_author");
	
	ClassDB::bind_method(D_METHOD("set_translator", "translator"), [](CLiteraryText *text, const String &translator){ text->Translator = translator; });
	ClassDB::bind_method(D_METHOD("get_translator"), [](CLiteraryText *text){ return text->Translator; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "translator"), "set_translator", "get_translator");
	
	ClassDB::bind_method(D_METHOD("set_publisher", "publisher"), [](CLiteraryText *text, const String &publisher){ text->Publisher = publisher; });
	ClassDB::bind_method(D_METHOD("get_publisher"), [](CLiteraryText *text){ return text->Publisher; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "publisher"), "set_publisher", "get_publisher");
	
	ClassDB::bind_method(D_METHOD("set_license", "license"), [](CLiteraryText *text, const String &license){ text->License = license; });
	ClassDB::bind_method(D_METHOD("get_license"), [](CLiteraryText *text){ return text->License; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "license"), "set_license", "get_license");
	
	ClassDB::bind_method(D_METHOD("set_notes", "notes"), [](CLiteraryText *text, const String &notes){ text->Notes = notes; });
	ClassDB::bind_method(D_METHOD("get_notes"), [](CLiteraryText *text){ return text->Notes; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "notes"), "set_notes", "get_notes");
	
	ClassDB::bind_method(D_METHOD("set_publication_year", "publication_year"), [](CLiteraryText *text, const int publication_year){ text->PublicationYear = publication_year; });
	ClassDB::bind_method(D_METHOD("get_publication_year"), [](CLiteraryText *text){ return text->PublicationYear; });
	ADD_PROPERTY(PropertyInfo(Variant::INT, "publication_year"), "set_publication_year", "get_publication_year");
	
	ClassDB::bind_method(D_METHOD("set_initial_page_number", "initial_page_number"), [](CLiteraryText *text, const int initial_page_number){ text->InitialPageNumber = initial_page_number; });
	ClassDB::bind_method(D_METHOD("get_initial_page_number"), [](CLiteraryText *text){ return text->InitialPageNumber; });
	ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_page_number"), "set_initial_page_number", "get_initial_page_number");
	
	ClassDB::bind_method(D_METHOD("set_page_numbering_enabled", "page_numbering_enabled"), [](CLiteraryText *text, const bool page_numbering_enabled){ text->PageNumberingEnabled = page_numbering_enabled; });
	ClassDB::bind_method(D_METHOD("is_page_numbering_enabled"), [](CLiteraryText *text){ return text->PageNumberingEnabled; });
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "page_numbering_enabled"), "set_page_numbering_enabled", "is_page_numbering_enabled");
	
	ClassDB::bind_method(D_METHOD("set_lowercase_roman_numeral_page_numbering", "lowercase_roman_numeral_page_numbering"), [](CLiteraryText *text, const bool lowercase_roman_numeral_page_numbering){ text->LowercaseRomanNumeralPageNumbering = lowercase_roman_numeral_page_numbering; });
	ClassDB::bind_method(D_METHOD("has_lowercase_roman_numeral_page_numbering"), [](CLiteraryText *text){ return text->LowercaseRomanNumeralPageNumbering; });
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lowercase_roman_numeral_page_numbering"), "set_lowercase_roman_numeral_page_numbering", "has_lowercase_roman_numeral_page_numbering");
	
	ClassDB::bind_method(D_METHOD("get_main_text"), &CLiteraryText::GetMainText);
	ClassDB::bind_method(D_METHOD("get_previous_section"), &CLiteraryText::GetPreviousSection);
	ClassDB::bind_method(D_METHOD("get_next_section"), &CLiteraryText::GetNextSection);
	ClassDB::bind_method(D_METHOD("get_first_page"), &CLiteraryText::GetFirstPage);
	ClassDB::bind_method(D_METHOD("get_last_page"), &CLiteraryText::GetLastPage);
}
