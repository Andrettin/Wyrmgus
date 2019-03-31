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
/**@name literary_text.h - The literary text header file. */
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

#ifndef __LITERARY_TEXT_H__
#define __LITERARY_TEXT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "property.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CIcon;
class CLiteraryTextPage;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CLiteraryText : public DataElement, public DataType<CLiteraryText>
{
	GDCLASS(CLiteraryText, DataElement)
	
public:
	CLiteraryText()
	{
		this->Properties = {
			{"name", this->Name},
			{"hidden", this->Hidden},
			{"author", this->Author},
			{"translator", this->Translator},
			{"publisher", this->Publisher},
			{"license", this->License},
			{"notes", this->Notes},
			{"publication_year", this->PublicationYear},
			{"initial_page_number", this->InitialPageNumber},
			{"page_numbering_enabled", this->PageNumberingEnabled},
			{"lowercase_roman_numeral_page_numbering", this->LowercaseRomanNumeralPageNumbering},
			{"icon", this->Icon}
		};
	}

	static constexpr const char *ClassIdentifier = "literary_text";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
public:
	const std::vector<CLiteraryText *> &GetSections() const
	{
		return this->Sections;
	}
	
	Array GetSectionsArray() const
	{
		Array sections;
		
		for (CLiteraryText *section : this->GetSections()) {
			sections.push_back(section);
		}
		
		return sections;
	}
	
	CLiteraryText *GetMainText() const
	{
		return this->MainText;
	}
	
	CLiteraryText *GetPreviousSection() const
	{
		return this->PreviousSection;
	}
	
	CLiteraryText *GetNextSection() const
	{
		return this->NextSection;
	}
	
	const std::vector<CLiteraryTextPage *> &GetPages() const
	{
		return this->Pages;
	}
	
	CLiteraryTextPage *GetFirstPage() const
	{
		if (!this->GetPages().empty()) {
			return this->GetPages().front();
		} else {
			return nullptr;
		}
	}
	
	CLiteraryTextPage *GetLastPage() const
	{
		if (!this->GetPages().empty()) {
			return this->GetPages().back();
		} else {
			return nullptr;
		}
	}
	
	/**
	**	@brief	Get the total page count of the literary text, including that of its sections, but ignoring the count of sections with disabled page numbering
	**
	**	@return The page count
	*/
	int GetTotalPageCount() const
	{
		int page_count = 0;
		
		if (this->IsPageNumberingEnabled()) {
			page_count += static_cast<int>(this->GetPages().size());
		}
		
		for (const CLiteraryText *section : this->GetSections()) {
			page_count += section->GetTotalPageCount();
		}

		return page_count;
	}
	
	CLiteraryText *GetSection(const std::string &section_name) const;
	
	bool IsPageNumberingEnabled() const
	{
		return this->PageNumberingEnabled;
	}
	
	bool HasLowercaseRomanNumeralPageNumbering() const
	{
		return this->LowercaseRomanNumeralPageNumbering;
	}
	
private:
	void UpdateSectionPageNumbers() const;

	// getters for Godot

	const String &GetName() const
	{
		return this->Name.Get();
	}
	
	bool IsHidden() const
	{
		return this->Hidden.Get();
	}
	
	const String &GetAuthor() const
	{
		return this->Author.Get();
	}
	
	const String &GetTranslator() const
	{
		return this->Translator.Get();
	}
	
	const String &GetPublisher() const
	{
		return this->Publisher.Get();
	}
	
	const String &GetLicense() const
	{
		return this->License.Get();
	}
	
	const String &GetNotes() const
	{
		return this->Notes.Get();
	}
	
	int GetPublicationYear() const
	{
		return this->PublicationYear.Get();
	}
	
	int GetInitialPageNumber() const
	{
		return this->InitialPageNumber.Get();
	}
	
	CIcon *GetIcon() const
	{
		return this->Icon.Get();
	}
	
public:
	Property<String> Name;			/// name of the literary text
	Property<bool> Hidden = false;	/// whether the literary text is hidden
	Property<String> Author;		/// author of the literary text
	Property<String> Translator;	/// translator of the literary text
	Property<String> Publisher;		/// publisher of the literary text
	Property<String> License;		/// the open-source license the literary text is under, or public domain if that's the case
	Property<String> Notes;			/// notes to appear on the cover of the literary text
	Property<int> PublicationYear = 0;	/// year of publication
	Property<int> InitialPageNumber = 0;	/// page number in which the literary text begins
	Property<bool> PageNumberingEnabled = true;	/// whether page numbering is enabled for the literary text
	Property<bool> LowercaseRomanNumeralPageNumbering = false;	/// whether page numbering should be depicted by lowercase Roman numerals
	Property<CIcon *> Icon {		/// the literary text's icon
		Property<CIcon *>::ValueType(nullptr),
		Property<CIcon *>::GetterType([&]() -> Property<CIcon *>::GetterReturnType {
			if (*this->Icon != nullptr) {
				return *this->Icon;		
			} else if (this->GetMainText() != nullptr) {
				return this->GetMainText()->Icon;		
			} else {
				return *this->Icon;
			}
		})
	};
	
private:
	CLiteraryText *MainText = nullptr;	/// the main literary text to which this one belongs, if it is a section
	std::vector<CLiteraryText *> Sections;	/// the sections of the literary text, e.g. different chapters, or volumes
	CLiteraryText *PreviousSection = nullptr;	/// the previous section to this one, if this is a section
	CLiteraryText *NextSection = nullptr;	/// the next section to this one, if this is a section
	std::vector<CLiteraryTextPage *> Pages;	/// pages of the literary text
	
public:
	friend int CclDefineText(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
