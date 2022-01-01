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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski,
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

class spell_action_adjust_vitals final : public spell_action
{
public:
	spell_action_adjust_vitals() : spell_action(true)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "adjust_vitals";
		return identifier;
	}

	virtual void process_sml_property(const sml_property &property) override;
	virtual int Cast(CUnit &caster, const spell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier) override;
	virtual void Parse(lua_State *l, int startIndex, int endIndex) override;

private:
	int HP = 0;         /// Target HP gain.(can be negative)
	int Mana = 0;       /// Target Mana gain.(can be negative)
	int Shield = 0;     /// Target SP gain.(can be negative)
	/// This spell is designed to be used with very small amounts. The spell
	/// can scale up to MaxMultiCast times. Use 0 for infinite.
	int MaxMultiCast = 0;
};

}
