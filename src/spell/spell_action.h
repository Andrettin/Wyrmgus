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

#include "vec2i.h"

class CConfigData;
class CUnit;
struct lua_State;

namespace wyrmgus {

class sml_data;
class sml_property;
class spell;

/**
**  Generic spell action virtual class.
**  Spells are sub class of this one
*/
class spell_action
{
public:
	static std::unique_ptr<spell_action> from_sml_scope(const sml_data &scope);

	explicit spell_action(const bool mod = false) : ModifyManaCaster(mod)
	{
	};

	virtual ~spell_action()
	{
	};

	virtual const std::string &get_class_identifier() const = 0;
	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);
	virtual void check() const {}
	virtual int Cast(CUnit &caster, const wyrmgus::spell &spell, CUnit *target, const Vec2i &goalPos, int z, int modifier) = 0;

	virtual void Parse(lua_State *l, int startIndex, int endIndex)
	{
		Q_UNUSED(l)
		Q_UNUSED(startIndex)
		Q_UNUSED(endIndex)

		throw std::runtime_error("Lua definition not supported for \"" + this->get_class_identifier() + "\" spell action.");
	}

	const bool ModifyManaCaster;
};

}
