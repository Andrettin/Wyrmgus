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
--  Variables
----------------------------------------------------------------------------*/

std::vector<std::function<void()>> ClassClearFunctions;

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
	if (this->Initialized) {
		fprintf(stderr, "Data element \"%s\" is being redefined.\n", this->Ident.c_str());
	}
	
	for (const CConfigProperty &config_property : config_data->Properties) {
		try {
			if (config_property.Process(*this)) {
				continue;
			}
		} catch (std::exception &exception) {
			fprintf(stderr, "%s\n", exception.what());
		}
		
		PropertyCommonBase *property = this->GetProperty(config_property.Key);
		if (property != nullptr) {
			if (config_property.Operator == CConfigOperator::Assignment) {
				*property = config_property.Value;
			} else if (config_property.Operator == CConfigOperator::Addition) {
				*property += config_property.Value;
			} else if (config_property.Operator == CConfigOperator::Subtraction) {
				*property -= config_property.Value;
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
	ClassDB::bind_method(D_METHOD("get_index"), &DataElement::GetIndex);
	ClassDB::bind_method(D_METHOD("set_name", "name"), [](DataElement *data_element, const String &name){ data_element->Name = name; });
	ClassDB::bind_method(D_METHOD("get_name"), &DataElement::GetName);
	
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
}
