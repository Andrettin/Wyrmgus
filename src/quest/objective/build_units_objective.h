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

#include "ai/ai_local.h"
#include "map/site_game_data.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/quest.h"
#include "quest/player_quest_objective.h"
#include "script/condition/and_condition.h"
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

		if (this->settlement != nullptr) {
			objective_str += " in " + this->settlement->get_game_data()->get_current_cultural_name();
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
			if (this->settlement != nullptr && !player->has_settlement(this->settlement) && !unit_type->BoolFlag[TOWNHALL_INDEX].value && !this->get_quest()->has_settlement_objective(this->settlement)) {
				continue;
			}

			if (!player->HasUnitBuilder(unit_type, this->settlement) || !check_conditions(unit_type, player)) {
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
			if (this->settlement != nullptr && !player->has_settlement(this->settlement) && !unit_type->BoolFlag[TOWNHALL_INDEX].value && !this->get_quest()->has_settlement_objective(this->settlement)) {
				validation_error = "You no longer hold the required settlement.";
				continue;
			}

			if (!player->HasUnitBuilder(unit_type, this->settlement) || !check_conditions(unit_type, player)) {
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

		if (this->settlement != nullptr && this->settlement != unit->get_settlement()) {
			return;
		}

		player_quest_objective->increment_counter();
	}

	virtual void check_ai(PlayerAi *ai_player, const player_quest_objective *player_quest_objective) const override
	{
		CPlayer *player = ai_player->Player;

		const unit_type *unit_type_to_build = nullptr;
		if (!this->get_unit_types().empty()) {
			unit_type_to_build = this->get_unit_types().front();
		} else {
			unit_type_to_build = player->get_class_unit_type(this->get_unit_classes().front());
		}

		if (unit_type_to_build->BoolFlag[TOWNHALL_INDEX].value) {
			return; //town hall construction can't be handled by requests
		}

		int units_to_build = this->get_quantity() - player_quest_objective->get_counter();

		for (const AiBuildQueue &queue : ai_player->UnitTypeBuilt) { //count transport capacity under construction to see if should request more
			if (this->settlement != nullptr && this->settlement != queue.settlement) {
				continue;
			}

			if (!vector::contains(this->get_unit_types(), queue.Type) && !vector::contains(this->get_unit_classes(), queue.Type->get_unit_class())) {
				continue;
			}

			units_to_build -= queue.Want - queue.Made;
		}

		if (units_to_build > 0) {
			AiAddUnitTypeRequest(*unit_type_to_build, units_to_build, 0, this->settlement);
		}
	}

private:
	const site *settlement = nullptr;
};

}
