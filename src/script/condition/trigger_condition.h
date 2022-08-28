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

#include "script/condition/condition.h"
#include "script/trigger.h"
#include "util/vector_util.h"

namespace wyrmgus {

class trigger_condition final : public condition<CPlayer>
{
public:
	explicit trigger_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->trigger = trigger::get(value);
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(player);
		Q_UNUSED(ctx);

		//checks whether a trigger has already fired

		return vector::contains(trigger::DeactivatedTriggers, this->trigger->get_identifier()); //this works fine for global triggers, but for player triggers perhaps it should check only the player?
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return std::string();
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const wyrmgus::trigger *trigger = nullptr;
};

}
