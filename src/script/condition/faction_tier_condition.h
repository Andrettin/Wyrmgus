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

#include "player/faction_tier.h"
#include "player/player.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace wyrmgus {

class faction_tier_condition final : public condition<CPlayer>
{
public:
	explicit faction_tier_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->tier = string_to_faction_tier(value);
	}

	virtual void check_validity() const override
	{
		if (this->tier == faction_tier::none) {
			throw std::runtime_error("\"faction_tier\" condition has no faction tier.");
		}
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_equality(player);
	}

	virtual bool check_equality(const CPlayer *player) const override
	{
		return player->get_faction_tier() == this->tier;
	}

	virtual bool check_less_than(const CPlayer *player) const override
	{
		return player->get_faction_tier() < this->tier;
	}

	virtual bool check_greater_than(const CPlayer *player) const override
	{
		return player->get_faction_tier() > this->tier;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return this->get_equality_string();
	}

	virtual std::string get_equality_string() const override
	{
		return string::highlight(get_faction_tier_name(this->tier)) + " faction tier";
	}

	virtual std::string get_inequality_string() const override
	{
		return "Not the " + string::highlight(get_faction_tier_name(this->tier)) + " faction tier";
	}

	virtual std::string get_less_than_string() const override
	{
		return "Lower than " + string::highlight(get_faction_tier_name(this->tier)) + " faction tier";
	}

	virtual std::string get_less_than_or_equality_string() const override
	{
		return string::highlight(get_faction_tier_name(this->tier)) + " or lower faction tier";
	}

	virtual std::string get_greater_than_string() const override
	{
		return "Higher than " + string::highlight(get_faction_tier_name(this->tier)) + " faction tier";
	}

	virtual std::string get_greater_than_or_equality_string() const override
	{
		return string::highlight(get_faction_tier_name(this->tier)) + " or higher faction tier";
	}

private:
	faction_tier tier = faction_tier::none;
};

}
