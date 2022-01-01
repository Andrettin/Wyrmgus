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
	class unit_type;
}

class Spell_SpawnPortal final : public wyrmgus::spell_action
{
public:
	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "spawn_portal";
		return identifier;
	}

	virtual int Cast(CUnit &caster, const wyrmgus::spell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier) override;
	virtual void Parse(lua_State *l, int startIndex, int endIndex) override;

private:
	wyrmgus::unit_type *PortalType = nullptr;   /// The unit type spawned
	int TTL = 0;                 /// Time to live for summoned portal. 0 means infinite
	bool CurrentPlayer = false;      /// If true, summon portal for caster's player rather than neutral
};
