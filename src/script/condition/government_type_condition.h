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

#include "player/government_type.h"
#include "script/condition/condition.h"

namespace wyrmgus {

class government_type_condition final : public condition<CPlayer>
{
public:
	explicit government_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->government_type = string_to_government_type(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "government_type";
		return class_identifier;
	}

	virtual bool check(const civilization *civilization) const override
	{
		const CUpgrade *upgrade = CUpgrade::get_government_type_upgrade(this->government_type);
		if (upgrade != nullptr && !upgrade->is_available_for_civilization(civilization)) {
			return false;
		}

		return true;
	}

	virtual bool check(const government_type government_type) const override
	{
		return this->government_type == government_type;
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return player->get_government_type() == this->government_type;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return string::highlight(get_government_type_name(this->government_type)) + " government type";
	}

private:
	wyrmgus::government_type government_type = government_type::none;
};

}
