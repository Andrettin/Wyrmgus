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
/**@name text.h - The text header file. */
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

#ifndef __TEXT_H__
#define __TEXT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"

#include <core/object.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CLiteraryTextChapter;
struct lua_State;

class CLiteraryText : public CDataType, public Object
{
	GDCLASS(CLiteraryText, Object)
	DATA_TYPE_CLASS(CLiteraryText)
	
public:
	~CLiteraryText();
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	String GetAuthor() const
	{
		return this->Author.c_str();
	}
	
	String GetTranslator() const
	{
		return this->Translator.c_str();
	}
	
	String GetPublisher() const
	{
		return this->Publisher.c_str();
	}
	
	String GetCopyrightNotice() const
	{
		return this->CopyrightNotice.c_str();
	}
	
	String GetNotes() const
	{
		return this->Notes.c_str();
	}
	
	int GetYear() const
	{
		return this->Year;
	}
	
	int GetInitialPage() const
	{
		return this->InitialPage;
	}
	
	CLiteraryTextChapter *GetChapter(const std::string &chapter_name) const;
	
private:
	std::string Name;				/// Name of the text
	std::string Author;				/// Author of the text
	std::string Translator;			/// Translator of the text
	std::string Publisher;			/// Publisher of the text
	std::string CopyrightNotice;	/// Copyright notice explaining that this text is in the public domain, or is licensed under an open-source license
	std::string Notes;				/// Notes to appear on the cover of the text
	int Year = 0;					/// Year of publication
	int InitialPage = 1;			/// Page in which the text begins
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
