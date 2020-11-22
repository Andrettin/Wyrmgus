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

#include "quest/quest_objective.h"

#include "character.h"
#include "faction.h"
#include "map/site.h"
#include "quest/objective/build_units_objective.h"
#include "quest/objective/destroy_faction_objective.h"
#include "quest/objective/destroy_hero_objective.h"
#include "quest/objective/destroy_unique_objective.h"
#include "quest/objective/destroy_units_objective.h"
#include "quest/objective/gather_resource_objective.h"
#include "quest/objective/have_resource_objective.h"
#include "quest/objective/hero_must_survive_objective.h"
#include "quest/objective/recruit_hero_objective.h"
#include "quest/objective/research_upgrade_objective.h"
#include "quest/objective_type.h"
#include "quest/quest.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace wyrmgus {

std::unique_ptr<quest_objective> quest_objective::try_from_identifier(const std::string &identifier, const wyrmgus::quest *quest)
{
	if (identifier == "build_units") {
		return std::make_unique<build_units_objective>(quest);
	} else if (identifier == "destroy_faction") {
		return std::make_unique<destroy_faction_objective>(quest);
	} else if (identifier == "destroy_unique") {
		return std::make_unique<destroy_unique_objective>(quest);
	} else if (identifier == "destroy_units") {
		return std::make_unique<destroy_units_objective>(quest);
	} else if (identifier == "gather_resource") {
		return std::make_unique<gather_resource_objective>(quest);
	} else if (identifier == "have_resource") {
		return std::make_unique<have_resource_objective>(quest);
	} else if (identifier == "recruit_hero") {
		return std::make_unique<recruit_hero_objective>(quest);
	} else if (identifier == "research_upgrade") {
		return std::make_unique<research_upgrade_objective>(quest);
	}

	return nullptr;
}

std::unique_ptr<quest_objective> quest_objective::from_sml_property(const sml_property &property, const wyrmgus::quest *quest)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "destroy_hero") {
		auto objective = std::make_unique<destroy_hero_objective>(quest);
		objective->character = character::get(value);
		return objective;
	} else if (key == "hero_must_survive") {
		auto objective = std::make_unique<hero_must_survive_objective>(quest);
		objective->character = character::get(value);
		return objective;
	} else {
		throw std::runtime_error("Invalid quest objective property: \"" + key + "\".");
	}
}

std::unique_ptr<quest_objective> quest_objective::from_sml_scope(const sml_data &scope, const wyrmgus::quest *quest)
{
	const std::string &tag = scope.get_tag();
	std::unique_ptr<quest_objective> objective;

	if (objective = quest_objective::try_from_identifier(tag, quest)) {
	} else {
		throw std::runtime_error("Invalid quest objective scope: \"" + tag + "\".");
	}

	database::process_sml_data(objective, scope);

	return objective;
}

quest_objective::quest_objective(const wyrmgus::quest *quest)
	: quest(quest), index(quest->get_objectives().size())
{
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
