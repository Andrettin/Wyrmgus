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

#include "dialogue.h"
#include "game/game.h"
#include "script/effect/delayed_effect_instance.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/scripted_effect.h"
#include "util/string_util.h"

namespace wyrmgus {

template <typename scope_type>
class delayed_effect final : public effect<scope_type>
{
public:
	explicit delayed_effect(const sml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "delayed";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
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
		} else if (key == "cycles") {
			this->delay = std::stoi(value);
		} else {
			effect<scope_type>::process_sml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->scripted_effect == nullptr && this->dialogue == nullptr) {
			throw std::runtime_error("\"delayed\" effect has neither a scripted effect nor a dialogue set for it.");
		}
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const override
	{
		std::unique_ptr<delayed_effect_instance<scope_type>> delayed_effect;
		if (this->scripted_effect != nullptr) {
			delayed_effect = std::make_unique<delayed_effect_instance<scope_type>>(this->scripted_effect, scope, ctx, this->delay);
		} else {
			delayed_effect = std::make_unique<delayed_effect_instance<scope_type>>(this->dialogue, scope, ctx, this->delay);
		}
		game::get()->add_delayed_effect(std::move(delayed_effect));
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = "In " + std::to_string(this->delay) + " cycles:\n";

		if (this->scripted_effect != nullptr) {
			str += this->scripted_effect->get_effects().get_effects_string(scope, ctx, indent + 1, prefix);
		} else {
			str += std::string(indent + 1, '\t') + "Trigger the " + string::highlight(this->dialogue->get_identifier()) + " dialogue";
		}

		return str;
	}

private:
	const wyrmgus::scripted_effect_base<scope_type> *scripted_effect = nullptr;
	const wyrmgus::dialogue *dialogue = nullptr;
	int delay = 0;
};

}
