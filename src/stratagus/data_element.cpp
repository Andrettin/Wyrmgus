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
/**@name data_element.cpp - The data element source file. */
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

#include "data_element.h"

#include "config.h"
#include "config_operator.h"
#include "property.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void DataElement::ProcessConfigData(const CConfigData *config_data)
{
	List<PropertyInfo> property_list;
	this->get_property_list(&property_list);
	Map<StringName, PropertyInfo> properties_by_name;
	for (List<PropertyInfo>::Element *element = property_list.front(); element != nullptr; element = element->next()) {
		const PropertyInfo &property_info = element->get();
		properties_by_name[property_info.name] = property_info;
	}
	
	for (const CConfigProperty &config_property : config_data->Properties) {
		Map<StringName, PropertyInfo>::Element *element = properties_by_name.find(config_property.Key.c_str());
		if (element == nullptr) {
			element = properties_by_name.find(("_" + config_property.Key).c_str());
		}
		
		if (element != nullptr) {
			const PropertyInfo &property_info = element->get();
			bool ok;
			Variant property_value;
			if (property_info.type == Variant::STRING) {
				if (config_property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for string property \"%s\": %i.\n", config_property.Key.c_str(), config_property.Operator);
					continue;
				}
				
				property_value = Variant(String(config_property.Value.c_str()));
			} else if (property_info.type == Variant::INT) {
				if (config_property.Operator == CConfigOperator::Assignment) {
					property_value = Variant(std::stoi(config_property.Value));
				} else if (config_property.Operator == CConfigOperator::Addition) {
					property_value = Variant(int(this->get(property_info.name)) + std::stoi(config_property.Value));
				} else if (config_property.Operator == CConfigOperator::Subtraction) {
					property_value = Variant(int(this->get(property_info.name)) - std::stoi(config_property.Value));
				}
			} else if (property_info.type == Variant::BOOL) {
				if (config_property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for boolean property \"%s\": %i.\n", config_property.Key.c_str(), config_property.Operator);
					continue;
				}
				
				property_value = Variant(StringToBool(config_property.Value));
			} else {
				fprintf(stderr, "Failed to set %s property \"%s\", as the variant type of the property is neither string, nor integer, nor boolean.\n", config_data->Tag.c_str(), config_property.Key.c_str());
				continue;
			}
			
			this->set(property_info.name, property_value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to set %s property \"%s\" to \"%s\".\n", config_data->Tag.c_str(), config_property.Key.c_str(), config_property.Value.c_str());
			}
			
			continue;
		}
		
		std::map<std::string, PropertyCommonBase &>::iterator find_iterator = this->Properties.find(config_property.Key);
		if (find_iterator != this->Properties.end()) {
			PropertyCommonBase &property = find_iterator->second;
			if (config_property.Operator == CConfigOperator::Assignment) {
				property = config_property.Value;
			} else if (config_property.Operator == CConfigOperator::Addition) {
				property += config_property.Value;
			} else if (config_property.Operator == CConfigOperator::Subtraction) {
				property -= config_property.Value;
			} else {
				fprintf(stderr, "Invalid operator enumeration index for property \"%s\": %i.\n", config_property.Key.c_str(), config_property.Operator);
			}
			
			continue;
		}
		
		if (!this->ProcessConfigDataProperty(config_property.Key, config_property.Value)) {
			fprintf(stderr, "Invalid %s property: \"%s\".\n", config_data->Tag.c_str(), config_property.Key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (!this->ProcessConfigDataSection(section)) {
			fprintf(stderr, "Invalid %s section: \"%s\".\n", config_data->Tag.c_str(), section->Tag.c_str());
		}
	}
	
	this->Initialize();
}

void DataElement::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_ident"), &DataElement::GetIdent);
	ClassDB::bind_method(D_METHOD("get_name"), [](const DataElement *data_element){ return data_element->Name.Get(); });
}
