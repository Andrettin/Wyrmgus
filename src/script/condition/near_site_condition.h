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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "util/string_util.h"

namespace wyrmgus {

class near_site_condition final : public condition
{
public:
	explicit near_site_condition(const std::string &value)
	{
		this->site = site::get(value);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		return false;
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		return unit->is_near_site(this->site);
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		const std::string site_name = this->site->get_game_data()->get_current_cultural_name();

		return "Near the " + string::highlight(site_name) + " site";
	}

private:
	const wyrmgus::site *site = nullptr;
};

}
