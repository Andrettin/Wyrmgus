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

#include "script/condition/condition.h"

namespace wyrmgus {

class quest;

class can_accept_quest_condition final : public condition<CPlayer>
{
public:
	explicit can_accept_quest_condition(const wyrmgus::quest *quest);
	explicit can_accept_quest_condition(const std::string &value, const gsml_operator condition_operator);

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "can_accept_quest";
		return class_identifier;
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override;
	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override;

private:
	const wyrmgus::quest *quest = nullptr;
};

}
