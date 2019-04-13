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
	DATA_TYPE(DetailedDataElement, DataElement)

public:
	DetailedDataElement(const std::string &ident = "", const int index = -1) : DataElement(ident, index)
	{
	}
	
private:
	/**
	**	@brief	Initialize the class
	*/
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(Description);
		REGISTER_PROPERTY(Quote);
		REGISTER_PROPERTY(Background);
		REGISTER_PROPERTY(Icon);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();
	
public:
	ExposedProperty<String, DetailedDataElement> Description;	/// the description of the data element from an in-game universe perspective
	ExposedProperty<String, DetailedDataElement> Quote;			/// a quote relating to the data element
	ExposedProperty<String, DetailedDataElement> Background;	/// the background of the data element, a description from a perspective outside of the game's universe
	Property<CIcon *, DetailedDataElement> Icon = nullptr;		/// the icon of the data element
	
protected:
	static void _bind_methods();
};

#endif
