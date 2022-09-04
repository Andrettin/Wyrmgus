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
//      (c) Copyright 2022 by Andrettin
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

#include "economy/resource.h"
#include "economy/resource_storage_type.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace wyrmgus {

class resource;

class resource_percent_effect final : public effect<CPlayer>
{
public:
	explicit resource_percent_effect(const resource *resource, const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator), resource(resource)
	{
		this->percent = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "resource_percent";
		return identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->resource != nullptr);
		assert_throw(this->resource->is_final());
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		const int stored_resource_quantity = player->get_resource(this->resource);
		player->set_resource(this->resource, stored_resource_quantity * this->percent / 100, resource_storage_type::overall);
	}

	virtual void do_addition_effect(CPlayer *player) const override
	{
		const int stored_resource_quantity = player->get_resource(this->resource);
		player->change_resource(this->resource, stored_resource_quantity * this->percent / 100, false);
	}

	virtual void do_subtraction_effect(CPlayer *player) const override
	{
		const int stored_resource_quantity = player->get_resource(this->resource);
		player->change_resource(this->resource, stored_resource_quantity * this->percent / 100 * -1, false);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set " + string::highlight(this->resource->get_name()) + " to " + std::to_string(this->percent) + "%";
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->percent) + "% " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->percent) + "% " + string::highlight(this->resource->get_name());
	}

private:
	const wyrmgus::resource *resource = nullptr;
	int percent = 0;
};

}
