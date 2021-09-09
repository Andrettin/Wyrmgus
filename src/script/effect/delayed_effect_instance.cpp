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

#include "script/effect/delayed_effect_instance.h"

#include "database/database.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "dialogue.h"
#include "script/effect/scripted_effect.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"

namespace wyrmgus {

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(const scripted_effect_base<scope_type> *scripted_effect, scope_type *scope, const wyrmgus::context &ctx, const int cycles)
	: delayed_effect_instance(scope, ctx, cycles)
{
	this->scripted_effect = scripted_effect;
}

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(const wyrmgus::dialogue *dialogue, scope_type *scope, const wyrmgus::context &ctx, const int cycles)
	: delayed_effect_instance(scope, ctx, cycles)
{
	this->dialogue = dialogue;
}

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(scope_type *scope, const wyrmgus::context &ctx, const int cycles)
	: context(ctx), remaining_cycles(cycles)
{
	if constexpr (std::is_same_v<scope_type, CUnit>) {
		this->scope = scope->acquire_ref();
	} else {
		this->scope = scope;
	}
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "scripted_effect") {
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			this->scripted_effect = player_scripted_effect::get(value);
		} else {
			this->scripted_effect = unit_scripted_effect::get(value);
		}
	} else if (key == "dialogue") {
		this->dialogue = dialogue::get(value);
	} else if (key == "scope") {
		const int scope_index = std::stoi(value);
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			this->scope = CPlayer::Players[scope_index].get();
		} else {
			this->scope = unit_manager::get()->GetSlotUnit(scope_index).acquire_ref();
		}
	} else if (key == "remaining_cycles") {
		this->remaining_cycles = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid delayed effect instance property: \"" + key + "\".");
	}
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "context") {
		this->context = wyrmgus::context();
		database::process_sml_data(this->context, scope);
	} else {
		throw std::runtime_error("Invalid delayed effect instance scope: \"" + scope.get_tag() + "\".");
	}
}

template <typename scope_type>
sml_data delayed_effect_instance<scope_type>::to_sml_data() const
{
	sml_data data;

	if (this->scripted_effect != nullptr) {
		std::string scripted_effect_identifier;
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			scripted_effect_identifier = static_cast<const player_scripted_effect *>(this->scripted_effect)->get_identifier();
		} else {
			scripted_effect_identifier = static_cast<const unit_scripted_effect *>(this->scripted_effect)->get_identifier();
		}
		data.add_property("scripted_effect", scripted_effect_identifier);
	} else {
		data.add_property("dialogue", this->dialogue->get_identifier());
	}

	std::string scope;
	if constexpr (std::is_same_v<scope_type, CPlayer>) {
		scope = std::to_string(this->scope->get_index());
	} else {
		scope = std::to_string(UnitNumber(*this->scope->get()));
	}
	data.add_property("scope", std::move(scope));

	data.add_property("remaining_cycles", std::to_string(this->get_remaining_cycles()));

	data.add_child(this->context.to_sml_data("context"));

	return data;
}

template <typename scope_type>
scope_type *delayed_effect_instance<scope_type>::get_scope() const
{
	if constexpr (std::is_same_v<scope_type, CUnit>) {
		return this->scope->get();
	} else {
		return this->scope;
	}
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::do_effects()
{
	scope_type *scope = this->get_scope();

	if constexpr (std::is_same_v<scope_type, CUnit>) {
		//ignore the effects if the unit has been destroyed in the meantime
		if (scope->Destroyed) {
			return;
		}
	}

	if (this->scripted_effect != nullptr) {
		this->scripted_effect->get_effects().do_effects(scope, this->context);
	} else {
		CPlayer *player = nullptr;

		wyrmgus::context dialogue_ctx;
		dialogue_ctx.source_player = this->context.current_player;
		dialogue_ctx.source_unit = this->context.current_unit;

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			player = this->scope;
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			dialogue_ctx.current_unit = this->scope;
			player = this->scope->get()->Player;
		}

		dialogue_ctx.current_player = player;

		this->dialogue->call(player, dialogue_ctx);
	}
}

template class delayed_effect_instance<CPlayer>;
template class delayed_effect_instance<CUnit>;

}
