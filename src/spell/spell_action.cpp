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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski,
//                                 Joris Dauphin, Jimmy Salmon and Andrettin
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

#include "spell/spell_action.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "spell/apply_status_effects_spell_action.h"
#include "spell/spell_action_adjust_variable.h"
#include "spell/spell_action_adjust_vitals.h"
#include "spell/spell_action_spawn_missile.h"
#include "spell/spell_action_summon.h"

namespace wyrmgus {

std::unique_ptr<spell_action> spell_action::from_gsml_scope(const gsml_data &scope)
{
	const std::string &action_identifier = scope.get_tag();
	std::unique_ptr<spell_action> action;

	if (action_identifier == "adjust_variable") {
		action = std::make_unique<spell_action_adjust_variable>();
	} else if (action_identifier == "adjust_vitals") {
		action = std::make_unique<spell_action_adjust_vitals>();
	} else if (action_identifier == "apply_status_effects") {
		action = std::make_unique<apply_status_effects_spell_action>();
	} else if (action_identifier == "spawn_missile") {
		action = std::make_unique<spell_action_spawn_missile>();
	} else if (action_identifier == "summon") {
		action = std::make_unique<spell_action_summon>();
	} else {
		throw std::runtime_error("Invalid scope spell action: \"" + action_identifier + "\".");
	}

	database::process_gsml_data(action, scope);
	action->check();

	return action;
}

void spell_action::process_gsml_property(const gsml_property &property)
{
	throw std::runtime_error("Invalid property for \"" + this->get_class_identifier() + "\" spell action: \"" + property.get_key() + "\".");
}

void spell_action::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid scope for \"" + this->get_class_identifier() + "\" spell action: \"" + scope.get_tag() + "\".");
}

}
