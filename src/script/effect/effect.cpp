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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "script/effect/effect.h"

#include "config.h"
#include "database/database.h"
#include "dialogue.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace stratagus {

std::unique_ptr<effect> effect::from_sml_property(const sml_property &property)
{
	const std::string &effect_identifier = property.get_key();

	if (effect_identifier == "call_dialogue") {
		return std::make_unique<call_dialogue_effect>(property.get_value());
	} else if (effect_identifier == "create_unit") {
		return std::make_unique<create_unit_effect>(property.get_value());
	}

	throw std::runtime_error("Invalid property effect: \"" + effect_identifier + "\".");
}

std::unique_ptr<effect> effect::from_sml_scope(const sml_data &scope)
{
	const std::string &effect_identifier = scope.get_tag();
	std::unique_ptr<effect> effect;

	if (effect == nullptr) {
		throw std::runtime_error("Invalid scope effect: \"" + effect_identifier + "\".");
	}

	database::process_sml_data(effect, scope);

	return effect;
}


void effect::process_sml_property(const sml_property &property)
{
	throw std::runtime_error("Invalid property for \"" + this->get_class_identifier() + "\" effect: \"" + property.get_key() + "\".");
}

void effect::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid scope for \"" + this->get_class_identifier() + "\" effect: \"" + scope.get_tag() + "\".");
}

call_dialogue_effect::call_dialogue_effect(const std::string &dialogue_identifier)
{
	this->dialogue = dialogue::get(dialogue_identifier);
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void call_dialogue_effect::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "dialogue") {
			this->dialogue = dialogue::get(value);
		} else {
			fprintf(stderr, "Invalid trigger property: \"%s\".\n", key.c_str());
		}
	}
	
	if (this->get_dialogue() == nullptr) {
		fprintf(stderr, "Call dialogue trigger effect has no dialogue.\n");
	}
}

void call_dialogue_effect::do_effect(CPlayer *player) const
{
	this->get_dialogue()->Call(player->Index);
}

std::string call_dialogue_effect::get_string(const CPlayer *player) const
{
	return "Trigger the " + string::highlight(this->get_dialogue()->get_identifier()) + " dialogue";
}

create_unit_effect::create_unit_effect(const std::string &unit_type_identifier)
{
	this->UnitType = CUnitType::get(unit_type_identifier);
}

void create_unit_effect::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "quantity") {
			this->Quantity = std::stoi(value);
		} else if (key == "unit_type") {
			CUnitType *unit_type = CUnitType::get(value);
			this->UnitType = unit_type;
		} else {
			fprintf(stderr, "Invalid trigger property: \"%s\".\n", key.c_str());
		}
	}
	
	if (!this->UnitType) {
		fprintf(stderr, "Create unit trigger effect has no unit type.\n");
	}
}

void create_unit_effect::do_effect(CPlayer *player) const
{
	CUnit *unit = MakeUnitAndPlace(player->StartPos, *this->UnitType, player, player->StartMapLayer);
}

std::string create_unit_effect::get_string(const CPlayer *player) const
{
	return "Receive a " + string::highlight(this->UnitType->get_name()) + " unit";
}

}
