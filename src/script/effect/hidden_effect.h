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

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"

namespace wyrmgus {

template <typename scope_type>
class hidden_effect final : public effect<scope_type>
{
public:
	explicit hidden_effect(const sml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "hidden";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		this->effects.process_sml_property(property);
	}

	virtual void process_sml_scope(const sml_data &scope) override
	{
		this->effects.process_sml_scope(scope);
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const override
	{
		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::string();
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	effect_list<scope_type> effects;
};

}
