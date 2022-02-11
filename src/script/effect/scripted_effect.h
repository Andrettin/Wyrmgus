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

#include "database/data_entry.h"
#include "database/data_type.h"
#include "script/effect/effect_list.h"

class CPlayer;
class CUnit;

namespace wyrmgus {

template <typename scope_type>
class effect_list;

//the class for a predefined, reusable scripted effect
template <typename scope_type>
class scripted_effect_base
{
public:
	void process_gsml_property(const gsml_property &property)
	{
		this->effects.process_gsml_property(property);
	}

	void process_gsml_scope(const gsml_data &scope)
	{
		this->effects.process_gsml_scope(scope);
	}

	void check() const
	{
		this->get_effects().check();
	}

	const effect_list<scope_type> &get_effects() const
	{
		return this->effects;
	}

private:
	effect_list<scope_type> effects;
};

class player_scripted_effect final : public data_entry, public data_type<player_scripted_effect>, public scripted_effect_base<CPlayer>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "player_scripted_effect";
	static constexpr const char *database_folder = "player_scripted_effects";

	explicit player_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}
};

class unit_scripted_effect final : public data_entry, public data_type<unit_scripted_effect>, public scripted_effect_base<CUnit>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "unit_scripted_effect";
	static constexpr const char *database_folder = "unit_scripted_effects";

	explicit unit_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}
};

}
