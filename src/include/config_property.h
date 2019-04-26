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
/**@name config_property.h - The config property header file. */
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

#ifndef __CONFIG_PROPERTY_H__
#define __CONFIG_PROPERTY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "config_operator.h"

#include <stdexcept>
#include <string>

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CConfigProperty
{
public:
	CConfigProperty(const std::string &key, CConfigOperator property_operator, const std::string &value) : Key(key), Operator(property_operator), Value(value)
	{
	}
	
	/**
	**	@brief	Process the config property
	**
	**	@param	data_element	The data element for which the property is being processed
	**	@param	element			The property info element for the property, for the provided data element
	**
	**	@return	True if the property was found, or false otherwise
	*/
	template <typename T>
	bool Process(T &data_element) const
	{
		const PropertyInfo *property_info = nullptr;
		
		//it would be better to replace this with a search in a map, but the option to get a property info directly by its corresponding string identifier doesn't seem to be available in Godot
		List<PropertyInfo> property_list;
		data_element.get_property_list(&property_list);
		for (List<PropertyInfo>::Element *element = property_list.front(); element != nullptr; element = element->next()) {
			const PropertyInfo &current_property_info = element->get();
			if (current_property_info.name == this->Key.c_str() || current_property_info.name == ("_" + this->Key).c_str()) {
				property_info = &current_property_info;
				break;
			}
		}
		
		if (property_info != nullptr) {
			bool ok;
			Variant property_value;
			if (property_info->type == Variant::STRING) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for string property \"" + this->Key + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(String(this->Value.c_str()));
			} else if (property_info->type == Variant::INT) {
				if (this->Operator == CConfigOperator::Assignment) {
					property_value = Variant(std::stoi(this->Value));
				} else if (this->Operator == CConfigOperator::Addition) {
					property_value = Variant(int(data_element.get(property_info->name)) + std::stoi(this->Value));
				} else if (this->Operator == CConfigOperator::Subtraction) {
					property_value = Variant(int(data_element.get(property_info->name)) - std::stoi(this->Value));
				}
			} else if (property_info->type == Variant::BOOL) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for boolean property \"" + this->Key + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(StringToBool(this->Value));
			} else if (property_info->type == Variant::OBJECT) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for string property \"" + this->Key + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(String(this->Value.c_str())); //for objects, call the setter with a string, and the appropriate conversion will be done in it
			} else {
				throw std::runtime_error("Failed to set property \"" + this->Key + "\", as the variant type of the property is neither string, nor integer, nor boolean.");
			}
			
			data_element.set(property_info->name, property_value, &ok);
			if (!ok) {
				throw std::runtime_error("Failed to set property \"" + this->Key + "\" to \"" + this->Value + "\".");
			}
			
			return true;
		}
		
		return false;
	}
	
	std::string Key;
	CConfigOperator Operator;
	std::string Value;
};

#endif
