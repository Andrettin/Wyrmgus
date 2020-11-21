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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "quest_objective.h"

#include "character.h"
#include "faction.h"
#include "map/site.h"
#include "objective_type.h"
#include "quest.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace wyrmgus {

quest_objective::quest_objective(const wyrmgus::objective_type objective_type, const wyrmgus::quest *quest)
	: objective_type(objective_type), quest(quest)
{
	if (objective_type == objective_type::hero_must_survive) {
		this->quantity = 0;
	}
}

void quest_objective::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "quantity") {
		this->quantity = std::stoi(value);
	} else if (key == "objective_string") {
		this->objective_string = value;
	} else if (key == "settlement") {
		this->settlement = site::get(value);
	} else if (key == "faction") {
		this->faction = faction::get(value);
	} else if (key == "character") {
		this->character = character::get(value);
	} else if (key == "unit_class") {
		this->unit_classes.clear();
		this->unit_classes.push_back(unit_class::get(value));
	} else if (key == "unit_type") {
		this->unit_types.clear();
		this->unit_types.push_back(unit_type::get(value));
	} else {
		throw std::runtime_error("Invalid quest objective property: \"" + key + "\".");
	}
}

void quest_objective::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(unit_class::get(value));
		}
	} else if (tag == "unit_types") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}
	} else {
		throw std::runtime_error("Invalid quest objective scope: \"" + scope.get_tag() + "\".");
	}
}

void quest_objective::check() const
{
	switch (this->get_objective_type()) {
		case objective_type::build_units:
			if (this->get_unit_types().empty() && this->get_unit_classes().empty()) {
				throw std::runtime_error("Build units quest objective has neither unit types nor unit classes set for it.");
			}
			break;
		case objective_type::hero_must_survive:
			if (this->get_character() == nullptr) {
				throw std::runtime_error("Hero must survive quest objective has no character set for it.");
			}
			break;
		default:
			break;
	}
}

std::string quest_objective::generate_objective_string(const CPlayer *player) const
{
	std::string objective_str;

	switch (this->get_objective_type()) {
		case objective_type::build_units: {
			bool first = true;

			for (const unit_class *unit_class : this->get_unit_classes()) {
				const unit_type *unit_type = player->get_class_unit_type(unit_class);

				if (unit_type == nullptr) {
					continue;
				}

				objective_str += this->get_unit_type_objective_string(unit_type, player, first);
			}

			for (const unit_type *unit_type : this->get_unit_types()) {
				objective_str += this->get_unit_type_objective_string(unit_type, player, first);
			}

			if (this->get_settlement() != nullptr) {
				objective_str += " in " + this->get_settlement()->get_current_cultural_name();
			}

			break;
		}
		case objective_type::hero_must_survive:
			return this->get_character()->get_full_name() + " must survive";
		default:
			return std::string();
	}

	return objective_str;
}

std::string quest_objective::get_unit_type_objective_string(const unit_type *unit_type, const CPlayer *player, bool &first) const
{
	const std::string unit_type_name = unit_type->GetDefaultName(player);

	std::string str;

	if (first) {
		str = unit_type->get_build_verb_string() + " ";
		if (this->get_quantity() > 1) {
			str += std::to_string(this->get_quantity());
		} else {
			str += string::get_indefinite_article(unit_type_name);
		}
		str += " ";
		first = false;
	} else {
		str += "/";
	}

	if (this->get_quantity() > 1) {
		str += string::get_plural_form(unit_type_name);
	} else {
		str += unit_type_name;
	}

	return str;
}

}
