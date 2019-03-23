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

#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CIcon;
class CLiteraryTextPage;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CLiteraryText : public CDataType
{
	GDCLASS(CLiteraryText, CDataType)
	DATA_TYPE_CLASS(CLiteraryText)
	
public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const String &GetName() const
	{
		return this->Name;
	}
	
	bool IsHidden() const
	{
		return this->Hidden;
	}
	
	const String &GetAuthor() const
	{
		return this->Author;
	}
	
	const String &GetTranslator() const
	{
		return this->Translator;
	}
	
	const String &GetPublisher() const
	{
		return this->Publisher;
	}
	
	const String &GetCopyrightNotice() const
	{
		return this->CopyrightNotice;
	}
	
	const String &GetNotes() const
	{
		return this->Notes;
	}
	
	int GetYear() const
	{
		return this->Year;
	}
	
	int GetInitialPage() const
	{
		return this->InitialPage;
	}
	
	CIcon *GetIcon() const
	{
		return this->Icon;
	}
	
	const std::vector<CLiteraryText *> &GetSections() const
	{
		return this->Sections;
	}
	
	int GetSectionIndex() const
	{
		return this->SectionIndex;
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
	
	CLiteraryText *GetSection(const std::string &section_name) const;
	
	bool IsIntroduction() const
	{
		return this->Introduction;
	}
	
private:
	String Name;					/// name of the text
	bool Hidden = false;			/// whether the literary text is hidden
	String Author;					/// author of the text
	String Translator;				/// translator of the text
	String Publisher;				/// publisher of the text
	String CopyrightNotice;			/// copyright notice explaining that this text is in the public domain, or is licensed under an open-source license
	String Notes;					/// notes to appear on the cover of the text
	int Year = 0;					/// year of publication
	int InitialPage = 1;			/// page in which the text begins
	CIcon *Icon = nullptr;			/// the text's icon
	CLiteraryText *MainText = nullptr;	/// the main text to which this one belongs, if it is a section
	std::vector<CLiteraryText *> Sections;	/// the sections of the literary text, e.g. different chapters, or volumes
	int SectionIndex = -1;						/// the section index of this literary text, if it is a section
	CLiteraryText *PreviousSection = nullptr;	/// the previous section to this one, if this is a section
	CLiteraryText *NextSection = nullptr;	/// the next section to this one, if this is a section
	bool Introduction = false;		/// whether this is an introductory chapter
	std::vector<CLiteraryTextPage *> Pages;	/// pages of the literary text
	
public:
	friend int CclDefineText(lua_State *l);
	
protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void TextCclRegister();

#endif
