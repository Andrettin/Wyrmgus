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

#include "player/faction.h"
#include "script/condition/condition.h"

namespace wyrmgus {

class faction_condition final : public condition<CPlayer>
{
public:
	explicit faction_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->faction = faction::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "faction";
		return class_identifier;
	}

	virtual bool check(const civilization *civilization) const override
	{
		return civilization == this->faction->get_civilization();
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return player->get_faction() == this->faction;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)

		return condition<CPlayer>::get_object_string(this->faction, links_allowed) + " faction";
	}

private:
	const wyrmgus::faction *faction = nullptr;
};

}
