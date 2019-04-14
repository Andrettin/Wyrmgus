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
/**@name property.cpp - The property source file. */
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

#include "property.h"

#include "data_type.h"
#include "item/item_slot.h"
#include "language/language.h"
#include "literary_text.h"
#include "ui/icon.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Convert to a type from a string
**
**	@param	str	The string to be converted
**
**	@return	The converted variable
*/
template <typename T>
static T ConvertFromString(const std::string &str)
{
	if constexpr(std::is_same_v<T, int>) {
		return std::stoi(str);
	} else if constexpr(std::is_same_v<T, bool>) {
		return StringToBool(str);
	} else if constexpr(std::is_same_v<T, String>) {
		return String(str.c_str());
	} else if constexpr(std::is_pointer_v<T> && std::is_base_of_v<DataType<std::remove_const_t<std::remove_pointer_t<T>>>, std::remove_const_t<std::remove_pointer_t<T>>>) {
		std::string ident = FindAndReplaceString(str, "_", "-");
		return std::remove_const_t<std::remove_pointer_t<T>>::Get(ident);
	} else {
		fprintf(stderr, "Cannot convert from a string to the designated type.\n");
		return T();
	}
}

/**
**	@brief	The operator for assignment of a string for a property
**
**	@param	rhs	The string which has to be converted to the property
**
**	@return	The property
*/
template <typename T>
const PropertyBase<T> &PropertyBase<T>::operator =(const std::string &rhs)
{
	if constexpr(
		std::is_same_v<T, int>
		|| std::is_same_v<T, bool>
		|| std::is_same_v<T, String>
		|| (std::is_pointer_v<T> && std::is_base_of_v<DataType<std::remove_const_t<std::remove_pointer_t<T>>>, std::remove_const_t<std::remove_pointer_t<T>>>)
	) {
		return *this = ConvertFromString<T>(rhs);
	} else {
		fprintf(stderr, "The operator = is not valid for this type.\n");
		return *this;
	}
}

/**
**	@brief	The operator for addition of a string for a property
**
**	@param	rhs	The string which has to be converted for the property
**
**	@return	The property
*/
template <typename T>
const PropertyBase<T> &PropertyBase<T>::operator +=(const std::string &rhs)
{
	if constexpr(
		std::is_same_v<T, int>
		|| std::is_same_v<T, bool>
		|| std::is_same_v<T, String>
	) {
		return *this += ConvertFromString<T>(rhs);
	} else if constexpr(is_specialization_of<T, std::vector>::value) {
		typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
		this->GetModifiable().push_back(new_value);
		return *this;
	} else {
		fprintf(stderr, "The operator += is not valid for this type.\n");
		return *this;
	}
}

/**
**	@brief	The operator for subtraction of a string for a property
**
**	@param	rhs	The string which has to be converted for the property
**
**	@return	The property
*/
template <typename T>
const PropertyBase<T> &PropertyBase<T>::operator -=(const std::string &rhs)
{
	if constexpr(
		std::is_same_v<T, int>
		|| std::is_same_v<T, bool>
	) {
		return *this -= ConvertFromString<T>(rhs);
	} else if constexpr(is_specialization_of<T, std::vector>::value) {
		typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
		this->GetModifiable().erase(std::remove(this->GetModifiable().begin(), this->GetModifiable().end(), new_value), this->GetModifiable().end());
		return *this;
	} else {
		fprintf(stderr, "The operator -= is not valid for this type.\n");
		return *this;
	}
}

template const PropertyBase<int> &PropertyBase<int>::operator =(const std::string &rhs);
template const PropertyBase<bool> &PropertyBase<bool>::operator =(const std::string &rhs);
template const PropertyBase<String> &PropertyBase<String>::operator =(const std::string &rhs);
template const PropertyBase<CIcon *> &PropertyBase<CIcon *>::operator =(const std::string &rhs);
template const PropertyBase<CLanguage *> &PropertyBase<CLanguage *>::operator =(const std::string &rhs);
template const PropertyBase<ItemSlot *> &PropertyBase<ItemSlot *>::operator =(const std::string &rhs);
template const PropertyBase<const ItemSlot *> &PropertyBase<const ItemSlot *>::operator =(const std::string &rhs);
template const PropertyBase<std::vector<CLiteraryText *>> &PropertyBase<std::vector<CLiteraryText *>>::operator =(const std::string &rhs);
template const PropertyBase<std::vector<CUnitType *>> &PropertyBase<std::vector<CUnitType *>>::operator =(const std::string &rhs);
template const PropertyBase<int> &PropertyBase<int>::operator +=(const std::string &rhs);
template const PropertyBase<bool> &PropertyBase<bool>::operator +=(const std::string &rhs);
template const PropertyBase<String> &PropertyBase<String>::operator +=(const std::string &rhs);
template const PropertyBase<CIcon *> &PropertyBase<CIcon *>::operator +=(const std::string &rhs);
template const PropertyBase<CLanguage *> &PropertyBase<CLanguage *>::operator +=(const std::string &rhs);
template const PropertyBase<ItemSlot *> &PropertyBase<ItemSlot *>::operator +=(const std::string &rhs);
template const PropertyBase<const ItemSlot *> &PropertyBase<const ItemSlot *>::operator +=(const std::string &rhs);
template const PropertyBase<std::vector<CLiteraryText *>> &PropertyBase<std::vector<CLiteraryText *>>::operator +=(const std::string &rhs);
template const PropertyBase<std::vector<CUnitType *>> &PropertyBase<std::vector<CUnitType *>>::operator +=(const std::string &rhs);
template const PropertyBase<int> &PropertyBase<int>::operator -=(const std::string &rhs);
template const PropertyBase<bool> &PropertyBase<bool>::operator -=(const std::string &rhs);
template const PropertyBase<String> &PropertyBase<String>::operator -=(const std::string &rhs);
template const PropertyBase<CIcon *> &PropertyBase<CIcon *>::operator -=(const std::string &rhs);
template const PropertyBase<CLanguage *> &PropertyBase<CLanguage *>::operator -=(const std::string &rhs);
template const PropertyBase<ItemSlot *> &PropertyBase<ItemSlot *>::operator -=(const std::string &rhs);
template const PropertyBase<const ItemSlot *> &PropertyBase<const ItemSlot *>::operator -=(const std::string &rhs);
template const PropertyBase<std::vector<CLiteraryText *>> &PropertyBase<std::vector<CLiteraryText *>>::operator -=(const std::string &rhs);
template const PropertyBase<std::vector<CUnitType *>> &PropertyBase<std::vector<CUnitType *>>::operator -=(const std::string &rhs);
