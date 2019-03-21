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

#include "data_type.h"

class CIcon;
class CLiteraryTextChapter;
struct lua_State;

class CLiteraryText : public CDataType
{
	GDCLASS(CLiteraryText, CDataType)
	DATA_TYPE_CLASS(CLiteraryText)
	
public:
	~CLiteraryText();
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	String GetName() const
	{
		return this->Name;
	}
	
	bool IsHidden() const
	{
		return this->Hidden;
	}
	
	String GetAuthor() const
	{
		return this->Author;
	}
	
	String GetTranslator() const
	{
		return this->Translator;
	}
	
	String GetPublisher() const
	{
		return this->Publisher;
	}
	
	String GetCopyrightNotice() const
	{
		return this->CopyrightNotice;
	}
	
	String GetNotes() const
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
	
	CLiteraryTextChapter *GetChapter(const std::string &chapter_name) const;
	
private:
	void SetName(const String name)
	{
		this->Name = name;
	}

	void SetHidden(const bool hidden)
	{
		this->Hidden = hidden;
	}

	void SetAuthor(const String author)
	{
		this->Author = author;
	}

	void SetTranslator(const String translator)
	{
		this->Translator = translator;
	}

	void SetPublisher(const String publisher)
	{
		this->Publisher = publisher;
	}

	void SetCopyrightNotice(const String copyright_notice)
	{
		this->CopyrightNotice = copyright_notice;
	}

	void SetNotes(const String notes)
	{
		this->Notes = notes;
	}

	void SetYear(const int year)
	{
		this->Year = year;
	}

	void SetInitialPage(const int initial_page)
	{
		this->InitialPage = initial_page;
	}

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
public:
	std::vector<CLiteraryTextChapter *> Chapters;	/// The chapters of the text
	
	friend int CclDefineText(lua_State *l);
	
protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void TextCclRegister();

#endif
