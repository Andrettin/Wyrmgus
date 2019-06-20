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
/**@name detailed_data_element.cpp - The detailed data element source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "detailed_data_element.h"

#include "ui/icon.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void DetailedDataElement::SetIcon(const String &icon_ident)
{
	CIcon *icon = CIcon::Get(icon_ident);
	
	this->Icon = icon;
	
	if (icon != nullptr) {
		icon->Load();
	}
}

void DetailedDataElement::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_hidden", "hidden"), +[](DetailedDataElement *detailed_data_element, const bool hidden){ detailed_data_element->Hidden = hidden; });
	ClassDB::bind_method(D_METHOD("is_hidden"), &DetailedDataElement::IsHidden);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hidden"), "set_hidden", "is_hidden");

	ClassDB::bind_method(D_METHOD("set_description", "description"), +[](DetailedDataElement *detailed_data_element, const String &description){ detailed_data_element->Description = description; });
	ClassDB::bind_method(D_METHOD("get_description"), &DetailedDataElement::GetDescription);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");

	ClassDB::bind_method(D_METHOD("set_quote", "quote"), +[](DetailedDataElement *detailed_data_element, const String &quote){ detailed_data_element->Quote = quote; });
	ClassDB::bind_method(D_METHOD("get_quote"), &DetailedDataElement::GetQuote);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "quote"), "set_quote", "get_quote");

	ClassDB::bind_method(D_METHOD("set_background", "background"), +[](DetailedDataElement *detailed_data_element, const String &background){ detailed_data_element->Background = background; });
	ClassDB::bind_method(D_METHOD("get_background"), &DetailedDataElement::GetBackground);	
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "background"), "set_background", "get_background");
	
	ClassDB::bind_method(D_METHOD("set_icon", "icon_ident"), &DetailedDataElement::SetIcon);
	ClassDB::bind_method(D_METHOD("get_icon"), +[](const DetailedDataElement *detailed_data_element){ return const_cast<CIcon *>(detailed_data_element->GetIcon()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon"), "set_icon", "get_icon");
}
