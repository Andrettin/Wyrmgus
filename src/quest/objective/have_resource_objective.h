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

#include "economy/resource_storage_type.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"

namespace wyrmgus {

class have_resource_objective final : public quest_objective
{
public:
	explicit have_resource_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::have_resource;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "resource") {
			this->resource = resource::get(value);
		} else {
			quest_objective::process_gsml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->resource == nullptr) {
			throw std::runtime_error("Have resource quest objective has no resource set for it.");
		}
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		return "Have " + std::to_string(this->get_quantity()) + " " + this->resource->get_name();
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		const int resource_quantity = player_quest_objective->get_player()->get_resource(this->resource, resource_storage_type::both);
		player_quest_objective->set_counter(resource_quantity);
	}

private:
	const wyrmgus::resource *resource = nullptr;
};

}
