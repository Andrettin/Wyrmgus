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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "script/effect/effect.h"

#include "config.h"
#include "database/database.h"
#include "script/effect/accept_quest_effect.h"
#include "script/effect/any_unit_of_type_effect.h"
#include "script/effect/call_dialogue_effect.h"
#include "script/effect/character_unit_effect.h"
#include "script/effect/complete_quest_effect.h"
#include "script/effect/create_unit_effect.h"
#include "script/effect/delayed_effect.h"
#include "script/effect/hidden_effect.h"
#include "script/effect/if_effect.h"
#include "script/effect/kill_character_effect.h"
#include "script/effect/last_created_unit_effect.h"
#include "script/effect/level_check_effect.h"
#include "script/effect/neutral_player_effect.h"
#include "script/effect/random_effect.h"
#include "script/effect/random_list_effect.h"
#include "script/effect/remove_character_effect.h"
#include "script/effect/remove_unit_effect.h"
#include "script/effect/resource_effect.h"
#include "script/effect/scripted_effect_effect.h"
#include "script/effect/unique_effect.h"

namespace wyrmgus {

template <typename scope_type>
std::unique_ptr<effect<scope_type>> effect<scope_type>::from_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const sml_operator effect_operator = property.get_operator();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, CPlayer>) {
		if (key == "accept_quest") {
			return std::make_unique<accept_quest_effect>(value, effect_operator);
		} else if (key == "complete_quest") {
			return std::make_unique<complete_quest_effect>(value, effect_operator);
		} else if (key == "create_unit") {
			return std::make_unique<create_unit_effect>(value, effect_operator);
		} else if (key == "kill_character") {
			return std::make_unique<kill_character_effect>(value, effect_operator);
		} else if (key == "remove_character") {
			return std::make_unique<remove_character_effect>(value, effect_operator);
		} else if (resource::try_get(key) != nullptr) {
			return std::make_unique<resource_effect>(resource::get(key), value, effect_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, CUnit>) {
		if (key == "remove_unit") {
			return std::make_unique<remove_unit_effect>(value, effect_operator);
		} else if (key == "unique") {
			return std::make_unique<unique_effect>(value, effect_operator);
		}
	}

	if (key == "call_dialogue") {
		return std::make_unique<call_dialogue_effect<scope_type>>(value, effect_operator);
	} else if (key == "scripted_effect") {
		return std::make_unique<scripted_effect_effect<scope_type>>(value, effect_operator);
	}

	throw std::runtime_error("Invalid property effect: \"" + key + "\".");
}

template <typename scope_type>
std::unique_ptr<effect<scope_type>> effect<scope_type>::from_sml_scope(const sml_data &scope)
{
	const std::string &effect_identifier = scope.get_tag();
	std::unique_ptr<effect> effect;

	if (effect_identifier == "character_unit") {
		effect = std::make_unique<character_unit_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "delayed") {
		effect = std::make_unique<delayed_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "hidden") {
		effect = std::make_unique<hidden_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "if") {
		effect = std::make_unique<if_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "neutral_player") {
		effect = std::make_unique<neutral_player_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "random") {
		effect = std::make_unique<random_effect<scope_type>>(scope.get_operator());
	} else if (effect_identifier == "random_list") {
		effect = std::make_unique<random_list_effect<scope_type>>(scope.get_operator());
	} else {
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			if (effect_identifier == "any_unit_of_type") {
				effect = std::make_unique<any_unit_of_type_effect>(scope.get_operator());
			} else if (effect_identifier == "create_unit") {
				effect = std::make_unique<create_unit_effect>(scope.get_operator());
			} else if (effect_identifier == "last_created_unit") {
				effect = std::make_unique<last_created_unit_effect>(scope.get_operator());
			}
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			if (effect_identifier == "level_check") {
				effect = std::make_unique<level_check_effect>(scope.get_operator());
			}
		}
	}

	if (effect == nullptr) {
		throw std::runtime_error("Invalid scope effect: \"" + effect_identifier + "\".");
	}

	database::process_sml_data(effect, scope);

	return effect;
}

template <typename scope_type>
effect<scope_type>::effect(const sml_operator effect_operator) : effect_operator(effect_operator)
{
}

template <typename scope_type>
void effect<scope_type>::process_sml_property(const sml_property &property)
{
	throw std::runtime_error("Invalid property for \"" + this->get_class_identifier() + "\" effect: \"" + property.get_key() + "\".");
}

template <typename scope_type>
void effect<scope_type>::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid scope for \"" + this->get_class_identifier() + "\" effect: \"" + scope.get_tag() + "\".");
}

template <typename scope_type>
void effect<scope_type>::do_effect(scope_type *scope, const context &ctx) const
{
	switch (this->effect_operator) {
		case sml_operator::assignment:
			this->do_assignment_effect(scope, ctx);
			break;
		case sml_operator::addition:
			this->do_addition_effect(scope);
			break;
		case sml_operator::subtraction:
			this->do_subtraction_effect(scope);
			break;
		default:
			throw std::runtime_error("Invalid effect operator: \"" + std::to_string(static_cast<int>(this->effect_operator)) + "\".");
	}
}

template <typename scope_type>
std::string effect<scope_type>::get_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
{
	switch (this->effect_operator) {
		case sml_operator::assignment:
			return this->get_assignment_string(scope, ctx, indent, prefix);
		case sml_operator::addition:
			return this->get_addition_string();
		case sml_operator::subtraction:
			return this->get_subtraction_string();
		default:
			throw std::runtime_error("Invalid effect operator: \"" + std::to_string(static_cast<int>(this->effect_operator)) + "\".");
	}
}

template class effect<CPlayer>;
template class effect<CUnit>;

}
