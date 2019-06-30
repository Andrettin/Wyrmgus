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

#pragma warning(push, 0)
#include <core/ustring.h>
#pragma warning(pop)

#include <stdexcept>
#include <vector>

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CConfigProperty
{
public:
	CConfigProperty(const String &key, CConfigOperator property_operator, const String &value) : Key(key), Operator(property_operator), Value(value)
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
	bool ProcessForObject(T &data_element) const
	{
		const PropertyInfo *property_info = nullptr;
		
		//it would be better to replace this with a search in a map, but the option to get a property info directly by its corresponding string identifier doesn't seem to be available in Godot
		List<PropertyInfo> property_list;
		data_element.get_property_list(&property_list);
		for (List<PropertyInfo>::Element *element = property_list.front(); element != nullptr; element = element->next()) {
			const PropertyInfo &current_property_info = element->get();
			if (current_property_info.name == this->Key || current_property_info.name == ("_" + this->Key)) {
				property_info = &current_property_info;
				break;
			}
		}
		
		if (property_info != nullptr) {
			bool ok;
			Variant property_value;
			if (property_info->type == Variant::STRING) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for string property \"" + std::string(this->Key.utf8().get_data()) + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(this->Value);
			} else if (property_info->type == Variant::INT) {
				if (this->Operator == CConfigOperator::Assignment) {
					property_value = Variant(this->Value.to_int());
				} else if (this->Operator == CConfigOperator::Addition) {
					property_value = Variant(int(data_element.get(property_info->name)) + this->Value.to_int());
				} else if (this->Operator == CConfigOperator::Subtraction) {
					property_value = Variant(int(data_element.get(property_info->name)) - this->Value.to_int());
				}
			} else if (property_info->type == Variant::BOOL) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for boolean property \"" + std::string(this->Key.utf8().get_data()) + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(StringToBool(this->Value));
			} else if (property_info->type == Variant::OBJECT) {
				if (this->Operator != CConfigOperator::Assignment) {
					throw std::runtime_error("Wrong operator enumeration index for object property \"" + std::string(this->Key.utf8().get_data()) + "\": " + std::to_string((long long) this->Operator) + ".");
				}
				
				property_value = Variant(this->Value); //for objects, call the setter with a string, and the appropriate conversion will be done in it
			} else {
				throw std::runtime_error("Failed to set property \"" + std::string(this->Key.utf8().get_data()) + "\", as the variant type of the property is neither string, nor integer, nor boolean.");
			}
			
			data_element.set(property_info->name, property_value, &ok);
			if (!ok) {
				throw std::runtime_error("Failed to set property \"" + std::string(this->Key.utf8().get_data()) + "\" to \"" + std::string(this->Value.utf8().get_data()) + "\".");
			}
			
			return true;
		}
		
		//if the operator is for addition, see if the class has any method with "add_" prefixed to the given property name
		if (this->Operator == CConfigOperator::Addition) {
			String addition_method_name = "add_to_";
			addition_method_name += this->Key;
			Variant value = this->Value;
			if (data_element.has_method(addition_method_name)) {
				const Variant *args[1] = { &value };
				data_element.call(addition_method_name, args, 1, Variant::CallError());
				return true;
			}
		}
		
		//if the operator is for subtraction, see if the class has any method with "remove_" prefixed to the given property name
		if (this->Operator == CConfigOperator::Subtraction) {
			String subtraction_method_name = "remove_from_";
			subtraction_method_name += this->Key;
			Variant value = this->Value;
			if (data_element.has_method(subtraction_method_name)) {
				const Variant *args[1] = { &value };
				data_element.call(subtraction_method_name, args, 1, Variant::CallError());
				return true;
			}
		}
		
		return false;
	}
	
	String Key;
	CConfigOperator Operator;
	String Value;
};

#endif
