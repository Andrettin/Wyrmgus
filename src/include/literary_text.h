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
	DATA_TYPE(CLiteraryText, DataElement)
	
public:
	CLiteraryText()
	{
	}

private:
	/**
	**	@brief	Initialize the class
	*/
	static inline bool InitializeClass()
	{
		PROPERTY_KEY("hidden", Hidden);
		PROPERTY_KEY("author", Author);
		PROPERTY_KEY("translator", Translator);
		PROPERTY_KEY("publisher", Publisher);
		PROPERTY_KEY("license", License);
		PROPERTY_KEY("notes", Notes);
		PROPERTY_KEY("publication_year", PublicationYear);
		PROPERTY_KEY("initial_page_number", InitialPageNumber);
		PROPERTY_KEY("page_numbering_enabled", PageNumberingEnabled);
		PROPERTY_KEY("lowercase_roman_numeral_page_numbering", LowercaseRomanNumeralPageNumbering);
		PROPERTY_KEY("icon", Icon);
		
		return true;
	}
	
	static inline bool ClassInitialized = CLiteraryText::InitializeClass();
	
public:
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

public:
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
		Property<CIcon *>::GetterType([this]() -> Property<CIcon *>::ReturnType {
			if (*this->Icon != nullptr) {
				return *this->Icon;		
			} else if (this->GetMainText() != nullptr) {
				return this->GetMainText()->Icon;		
			} else {
				return nullptr;
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
