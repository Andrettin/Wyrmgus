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
//      (c) Copyright 2019-2020 by Andrettin
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
//

#pragma once

#include "player.h"
#include "script/effect/effect.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace stratagus {

class create_unit_effect final : public effect
{
public:
	explicit create_unit_effect(const std::string &unit_type_identifier)
	{
		this->unit_type = CUnitType::get(unit_type_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static std::string class_identifier = "create_unit";
		return class_identifier;
	}

	virtual void do_effect(CPlayer *player) const override
	{
		MakeUnitAndPlace(player->StartPos, *this->unit_type, player, player->StartMapLayer);
	}

	virtual std::string get_string(const CPlayer *player) const override
	{
		return "Receive a " + string::highlight(this->unit_type->get_name()) + " unit";
	}

private:
	const CUnitType *unit_type = nullptr;	/// Unit type to be created
};

}
