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
//      (c) Copyright 1999-2021 by Vladi Belperchinov-Shabanski,
//                                 Joris Dauphin, Jimmy Salmon and Andrettin
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

#include "spell/spell_action.h"

namespace wyrmgus {

enum class status_effect;

class apply_status_effects_spell_action final : public spell_action
{
public:
	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "apply_status_effects";
		return identifier;
	}

	virtual void process_sml_property(const sml_property &property) override;
	virtual int Cast(CUnit &caster, const spell &spell, CUnit *target, const Vec2i &goal_pos, int z, int modifier) override;

private:
	std::map<status_effect, int> status_effect_cycles;
};

}

