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
//      (c) Copyright 2020-2021 by Andrettin
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

#pragma once

#include "map/site_game_data.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

class build_units_objective final : public quest_objective
{
public:
	explicit build_units_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual void check() const override
	{
		if (this->get_unit_types().empty() && this->get_unit_classes().empty()) {
			throw std::runtime_error("Build units quest objective has neither unit types nor unit classes set for it.");
		}
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::build_units;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		std::string objective_str;
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
			objective_str += " in " + this->get_settlement()->get_game_data()->get_current_cultural_name();
		}

		return objective_str;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		std::vector<const unit_type *> unit_types = this->get_unit_types();

		for (const unit_class *unit_class : this->get_unit_classes()) {
			const unit_type *unit_type = player->get_faction()->get_class_unit_type(unit_class);
			if (unit_type == nullptr) {
				continue;
			}
			unit_types.push_back(unit_type);
		}

		if (unit_types.empty()) {
			return false;
		}

		bool validated = false;
		for (const unit_type *unit_type : unit_types) {
			if (this->get_settlement() != nullptr && !player->has_settlement(this->get_settlement()) && !unit_type->BoolFlag[TOWNHALL_INDEX].value) {
				continue;
			}

			if (!player->HasUnitBuilder(unit_type, this->get_settlement()) || !check_conditions(unit_type, player)) {
				continue;
			}

			validated = true;
			break;
		}

		if (!validated) {
			return false;
		}

		return true;
	}

	bool overlaps_with(const build_units_objective *other_objective) const
	{
		if (container::intersects_with(this->get_unit_classes(), other_objective->get_unit_classes())) {
			return true;
		}

		if (container::intersects_with(this->get_unit_types(), other_objective->get_unit_types())) {
			return true;
		}

		//also check whether one objective's unit type belongs to the other's unit class

		for (const unit_type *unit_type : this->get_unit_types()) {
			if (unit_type->get_unit_class() == nullptr) {
				continue;
			}

			if (vector::contains(other_objective->get_unit_classes(), unit_type->get_unit_class())) {
				return true;
			}
		}

		for (const unit_type *other_unit_type : other_objective->get_unit_types()) {
			if (other_unit_type->get_unit_class() == nullptr) {
				continue;
			}

			if (vector::contains(this->get_unit_classes(), other_unit_type->get_unit_class())) {
				return true;
			}
		}

		return false;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		std::vector<const unit_type *> unit_types = this->get_unit_types();

		for (const unit_class *unit_class : this->get_unit_classes()) {
			const unit_type *unit_type = player->get_faction()->get_class_unit_type(unit_class);
			if (unit_type == nullptr) {
				continue;
			}
			unit_types.push_back(unit_type);
		}

		if (unit_types.empty()) {
			return std::make_pair(true, "You can no longer produce the required unit.");
		}

		bool validated = false;
		std::string validation_error;
		for (const unit_type *unit_type : unit_types) {
			if (this->get_settlement() != nullptr && !player->has_settlement(this->get_settlement()) && !unit_type->BoolFlag[TOWNHALL_INDEX].value) {
				validation_error = "You no longer hold the required settlement.";
				continue;
			}

			if (!player->HasUnitBuilder(unit_type, this->get_settlement()) || !check_conditions(unit_type, player)) {
				validation_error = "You can no longer produce the required unit.";
				continue;
			}

			validated = true;
		}

		if (!validated) {
			return std::make_pair(true, validation_error);
		}

		return quest_objective::check_failure(player);
	}

	virtual void on_unit_built(const CUnit *unit, player_quest_objective *player_quest_objective) const override
	{
		if (!vector::contains(this->get_unit_types(), unit->Type) && !vector::contains(this->get_unit_classes(), unit->Type->get_unit_class())) {
			return;
		}

		if (this->get_settlement() != nullptr && this->get_settlement() != unit->settlement) {
			return;
		}

		player_quest_objective->increment_counter();
	}
};

}
