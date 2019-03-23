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
/**@name data_type.cpp - The data type source file. */
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

#include "stratagus.h"

#include "data_type.h"

#include "config.h"

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDataType::ProcessConfigData(const CConfigData *config_data)
{
	List<PropertyInfo> property_list;
	this->get_property_list(&property_list);
	Map<StringName, PropertyInfo> properties_by_name;
	for (List<PropertyInfo>::Element *element = property_list.front(); element != nullptr; element = element->next()) {
		const PropertyInfo &property_info = element->get();
		properties_by_name[property_info.name] = property_info;
	}
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		Map<StringName, PropertyInfo>::Element *element = properties_by_name.find(key.c_str());
		if (element == nullptr) {
			element = properties_by_name.find(("_" + key).c_str());
		}
		
		if (element != nullptr) {
			const PropertyInfo &property_info = element->get();
			bool ok;
			Variant property_value;
			if (property_info.type == Variant::STRING) {
				property_value = Variant(String(value.c_str()));
			} else if (property_info.type == Variant::INT) {
				property_value = Variant(std::stoi(value));
			} else if (property_info.type == Variant::BOOL) {
				property_value = Variant(StringToBool(value));
			} else {
				fprintf(stderr, "Failed to set %s property \"%s\", as the variant type of the property is neither string, nor integer, nor boolean.\n", config_data->Tag.c_str(), key.c_str());
				continue;
			}
			
			this->set(property_info.name, property_value, &ok);
			if (!ok) {
				fprintf(stderr, "Failed to set %s property \"%s\" to \"%s\".\n", config_data->Tag.c_str(), key.c_str(), value.c_str());
			}
		} else if (!this->ProcessConfigDataProperty(key, value)) {
			fprintf(stderr, "Invalid %s property: \"%s\".\n", config_data->Tag.c_str(), key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (!this->ProcessConfigDataSection(section)) {
			fprintf(stderr, "Invalid %s section: \"%s\".\n", config_data->Tag.c_str(), section->Tag.c_str());
		}
	}
	
	this->Initialize();
}

void CDataType::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_ident"), &CDataType::GetIdent);
}
