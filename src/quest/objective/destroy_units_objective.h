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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "quest/objective/destroy_unit_objective_base.h"
#include "quest/objective_type.h"
#include "util/vector_util.h"

namespace wyrmgus {

class destroy_units_objective final : public destroy_unit_objective_base
{
public:
	explicit destroy_units_objective(const wyrmgus::quest *quest) : destroy_unit_objective_base(quest)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "settlement") {
			this->settlement = site::get(value);
		} else {
			quest_objective::process_gsml_property(property);
		}
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::destroy_units;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		std::string objective_str;
		bool first = true;

		for (const unit_class *unit_class : this->get_unit_classes()) {
			objective_str += this->get_unit_class_objective_string(unit_class, first);
		}

		for (const unit_type *unit_type : this->get_unit_types()) {
			objective_str += this->get_unit_type_objective_string(unit_type, nullptr, first);
		}

		if (this->settlement != nullptr) {
			objective_str += " in " + this->settlement->get_game_data()->get_current_cultural_name();
		}

		return objective_str;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		if (this->get_faction() != nullptr) {
			const CPlayer *faction_player = GetFactionPlayer(this->get_faction());

			if (faction_player != nullptr && this->settlement != nullptr && !faction_player->has_settlement(this->settlement)) {
				return false;
			}
		}

		return destroy_unit_objective_base::is_quest_acceptance_allowed(player);
	}

	bool overlaps_with(const destroy_units_objective *other_objective) const
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
		Q_UNUSED(player)

		if (this->get_faction() != nullptr) {
			const CPlayer *faction_player = GetFactionPlayer(this->get_faction());

			if (faction_player != nullptr && this->settlement != nullptr && !faction_player->has_settlement(this->settlement)) {
				return std::make_pair(true, "The target no longer exists.");
			}
		}

		return quest_objective::check_failure(player);
	}

	virtual bool is_objective_unit(const CUnit *unit) const override
	{
		if (
			(!vector::contains(this->get_unit_types(), unit->Type) && !vector::contains(this->get_unit_classes(), unit->Type->get_unit_class()))
			|| (this->settlement != nullptr && this->settlement != unit->get_settlement())
		) {
			return false;
		}

		return destroy_unit_objective_base::is_objective_unit(unit);
	}

private:
	const site *settlement = nullptr;
};

}
