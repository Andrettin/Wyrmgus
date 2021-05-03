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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "stratagus.h"

#include "quest/objective/quest_objective.h"

#include "character.h"
#include "faction.h"
#include "map/site.h"
#include "quest/objective/build_units_objective.h"
#include "quest/objective/destroy_faction_objective.h"
#include "quest/objective/destroy_hero_objective.h"
#include "quest/objective/destroy_unique_objective.h"
#include "quest/objective/destroy_units_objective.h"
#include "quest/objective/found_faction_objective.h"
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
	} else if (identifier == "destroy_units") {
		return std::make_unique<destroy_units_objective>(quest);
	} else if (identifier == "gather_resource") {
		return std::make_unique<gather_resource_objective>(quest);
	} else if (identifier == "have_resource") {
		return std::make_unique<have_resource_objective>(quest);
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
		return std::make_unique<destroy_hero_objective>(value, quest);
	} else if (key == "destroy_unique") {
		return std::make_unique<destroy_unique_objective>(value, quest);
	} else if (key == "found_faction") {
		return std::make_unique<found_faction_objective>(value, quest);
	} else if (key == "hero_must_survive") {
		return std::make_unique<hero_must_survive_objective>(value, quest);
	} else if (key == "recruit_hero") {
		return std::make_unique<recruit_hero_objective>(value, quest);
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
	} else if (key == "unit_class") {
		this->unit_classes.clear();
		this->unit_classes.push_back(unit_class::get(value));
	} else if (key == "unit_type") {
		this->unit_types.clear();
		this->unit_types.push_back(unit_type::get(value));
	} else if (key == "upgrade") {
		this->upgrade = CUpgrade::get(value);
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

std::string quest_objective::get_unit_name_objective_string(const std::string &unit_name, const unit_type *unit_type, bool &first) const
{
	std::string str;

	if (first) {
		if (this->get_objective_type() == objective_type::destroy_units) {
			str = unit_type->get_destroy_verb_string();
		} else {
			str = unit_type->get_build_verb_string();
		}
		str += " ";

		if (this->get_quantity() > 1) {
			str += std::to_string(this->get_quantity());
		} else {
			str += string::get_indefinite_article(unit_name);
		}
		str += " ";
		first = false;
	} else {
		str += "/";
	}

	if (this->get_quantity() > 1) {
		str += string::get_plural_form(unit_name);
	} else {
		str += unit_name;
	}

	return str;
}

std::string quest_objective::get_unit_class_objective_string(const unit_class *unit_class, bool &first) const
{
	if (unit_class->get_unit_types().empty()) {
		throw std::runtime_error("Tried to generate an objective string for a unit class which has no unit types attached to it.");
	}

	const std::string &unit_class_name = unit_class->get_name();
	const unit_type *unit_type = unit_class->get_unit_types().front();
	return this->get_unit_name_objective_string(unit_class_name, unit_type, first);
}

std::string quest_objective::get_unit_type_objective_string(const unit_type *unit_type, const CPlayer *player, bool &first) const
{
	const std::string &unit_type_name = unit_type->GetDefaultName(player);
	return this->get_unit_name_objective_string(unit_type_name, unit_type, first);
}

bool quest_objective::overlaps_with(const quest_objective *other_objective) const
{
	if (this->get_objective_type() != other_objective->get_objective_type()) {
		return false;
	}

	switch (this->get_objective_type()) {
		case objective_type::build_units:
			return static_cast<const build_units_objective *>(this)->overlaps_with(static_cast<const build_units_objective *>(other_objective));
		case objective_type::destroy_units:
			return static_cast<const destroy_units_objective *>(this)->overlaps_with(static_cast<const destroy_units_objective *>(other_objective));
		case objective_type::gather_resource:
			return static_cast<const gather_resource_objective *>(this)->overlaps_with(static_cast<const gather_resource_objective *>(other_objective));
		default:
			//don't consider objectives which when fulfilled would necessarily invalidate the other (like destroy hero, recruit hero or destroy faction) to be overlapping; it is preferable in this case to allow different quests with the same objective to be accepted
			//don't consider passive quest objectives (like hero must survive) to be overlapping
			return false;
	}
}

}
