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
/**@name detailed_data_element.h - The detailed data element header file. */
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

#ifndef __DETAILED_DATA_ELEMENT_H__
#define __DETAILED_DATA_ELEMENT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CIcon;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

/**
**	@brief	A more detailed data element, containing properties which e.g. most encyclopedia entries would need to have.
*/
class DetailedDataElement : public DataElement
{
	GDCLASS(DetailedDataElement, DataElement)

public:
	DetailedDataElement(const std::string &ident = "", const int index = -1) : DataElement(ident, index)
	{
	}
	
public:
	bool IsHidden() const
	{
		return this->Hidden;
	}
	
	const String &GetDescription() const
	{
		return this->Description;
	}
	
	const String &GetQuote() const
	{
		return this->Quote;
	}
	
	const String &GetBackground() const
	{
		return this->Background;
	}
	
	virtual CIcon *GetIcon() const
	{
		return this->Icon;
	}
	
protected: //this is protected because lua functions still use it
	void SetIcon(const String &icon_ident);
	
protected: //these are protected because lua functions still use them
	bool Hidden = false;	/// whether the data element is hidden
	String Description;		/// the description of the data element from an in-game universe perspective
	String Quote;			/// a quote relating to the data element
	String Background;		/// the background of the data element, a description from a perspective outside of the game's universe
	CIcon *Icon = nullptr;	/// the icon of the data element
	
protected:
	static void _bind_methods();
};

#endif
