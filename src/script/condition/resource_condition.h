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

#include "player/player.h"
#include "economy/resource.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace wyrmgus {

class resource_condition final : public condition<CPlayer>
{
public:
	explicit resource_condition(const resource *resource, const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator), resource(resource)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "resource";
		return class_identifier;
	}

	virtual void check_validity() const override
	{
		if (this->resource == nullptr) {
			throw std::runtime_error("\"resource\" condition has no resource.");
		}

		assert_throw(this->resource->is_final());
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_greater_than_or_equality(player);
	}

	virtual bool check_equality(const CPlayer *player) const override
	{
		return player->get_resource(this->resource) == this->quantity;
	}

	virtual bool check_less_than(const CPlayer *player) const override
	{
		return player->get_resource(this->resource) < this->quantity;
	}

	virtual bool check_greater_than(const CPlayer *player) const override
	{
		return player->get_resource(this->resource) > this->quantity;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return this->get_greater_than_or_equality_string();
	}

	virtual std::string get_equality_string() const override
	{
		return std::to_string(this->quantity) + " " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_inequality_string() const override
	{
		return "Not " + std::to_string(this->quantity) + " " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_less_than_string() const override
	{
		return "Less than " + std::to_string(this->quantity) + " " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_less_than_or_equality_string() const override
	{
		return std::to_string(this->quantity) + " or less " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_greater_than_string() const override
	{
		return "More than " + std::to_string(this->quantity) + " " + string::highlight(this->resource->get_name());
	}

	virtual std::string get_greater_than_or_equality_string() const override
	{
		return std::to_string(this->quantity) + " or more " + string::highlight(this->resource->get_name());
	}

private:
	const wyrmgus::resource *resource = nullptr;
	int quantity = 0;
};

}
