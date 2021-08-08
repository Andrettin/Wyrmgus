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

#include "stratagus.h"

#include "player/faction_history.h"

#include "database/sml_data.h"
#include "player/diplomacy_state.h"
#include "player/faction.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void faction_history::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "resources") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const sml_operator sml_operator = property.get_operator();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			const int quantity = std::stoi(value);

			if (sml_operator == sml_operator::assignment) {
				this->resources[resource] = quantity;
			} else if (sml_operator == sml_operator::addition) {
				this->resources[resource] += quantity;
			} else if (sml_operator == sml_operator::subtraction) {
				this->resources[resource] -= quantity;
			} else {
				throw std::runtime_error("Invalid faction resource operator: \"" + std::to_string(static_cast<int>(sml_operator)) + "\".");
			}
		});
	} else if (tag == "diplomacy_state") {
		const faction *other_faction = nullptr;
		std::optional<diplomacy_state> state;
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "faction") {
				other_faction = faction::get(value);
			} else if (key == "state") {
				state = string_to_diplomacy_state(value);
			} else {
				throw std::runtime_error("Invalid diplomacy state property: \"" + key + "\".");
			}
		});

		if (other_faction == nullptr) {
			throw std::runtime_error("Diplomacy state has no faction.");
		}

		if (!state.has_value()) {
			throw std::runtime_error("Diplomacy state has no state.");
		}

		const bool is_vassalage = is_vassalage_diplomacy_state(state.value());

		if (is_vassalage) {
			//a faction can only have one overlord, so remove any other vassalage states
			map::remove_value_if(this->diplomacy_states, [](const diplomacy_state state) {
				return is_vassalage_diplomacy_state(state);
			});
		}

		this->diplomacy_states[other_faction] = state.value();
	} else {
		data_entry_history::process_sml_scope(scope);
	}
}

void faction_history::remove_acquired_upgrade_class(const upgrade_class *upgrade_class)
{
	vector::remove(this->acquired_upgrade_classes, upgrade_class);
}

void faction_history::remove_acquired_upgrade(const CUpgrade *upgrade)
{
	vector::remove(this->acquired_upgrades, upgrade);
}

void faction_history::remove_explored_settlement(const site *settlement)
{
	vector::remove(this->explored_settlements, settlement);
}

}
