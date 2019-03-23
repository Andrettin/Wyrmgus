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
/**@name literary_text_page.h - The literary text page header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __LITERARY_TEXT_PAGE_H__
#define __LITERARY_TEXT_PAGE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <core/object.h>
#include <core/ustring.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CLiteraryTextPage : public Object
{
	GDCLASS(CLiteraryTextPage, Object)
	
public:
	CLiteraryTextPage(const int number = -1, CLiteraryTextPage *previous_page = nullptr) : Number(number), PreviousPage(previous_page)
	{
		if (previous_page != nullptr) {
			previous_page->NextPage = this;
		}
	}

	void ProcessConfigData(const CConfigData *config_data);
	
	const String &GetText() const
	{
		return this->Text;
	}
	
	int GetNumber() const
	{
		return this->Number;
	}
	
	CLiteraryTextPage *GetPreviousPage() const
	{
		return this->PreviousPage;
	}
	
	CLiteraryTextPage *GetNextPage() const
	{
		return this->NextPage;
	}
	
private:
	String Text;					/// the text of the page
	int Number = -1;				/// the number of the page
	CLiteraryTextPage *PreviousPage = nullptr;	/// the previous page
	CLiteraryTextPage *NextPage = nullptr;	/// the next page
	
	friend int CclDefineText(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
