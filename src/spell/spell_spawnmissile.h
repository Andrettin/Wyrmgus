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
//
//      (c) Copyright 1999-2020 by Vladi Belperchinov-Shabanski,
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
//

#pragma once

#include "spell/spell_action.h"

namespace wyrmgus {
	class missile_type;
}

/**
**  Different targets.
*/
enum class LocBaseType {
	LocBaseCaster,
	LocBaseTarget
};

/**
**  This struct is used for defining a missile start/stop location.
**
**  It's evaluated like this, and should be more or less flexible.:
**  base coordinates(caster or target) + (AddX,AddY) + (rand()%AddRandX,rand()%AddRandY)
*/
class SpellActionMissileLocation final
{
public:
	explicit SpellActionMissileLocation(const LocBaseType base) : Base(base) {}

	void process_sml_property(const wyrmgus::sml_property &property);
	void process_sml_scope(const wyrmgus::sml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);
	
	LocBaseType Base;	/// The base for the location (caster/target)
	int AddX = 0;		/// Add to the X coordinate
	int AddY = 0;		/// Add to the X coordinate
	int AddRandX = 0;	/// Random add to the X coordinate
	int AddRandY = 0;	/// Random add to the X coordinate
};

class Spell_SpawnMissile final : public wyrmgus::spell_action
{
public:
	Spell_SpawnMissile() : StartPoint(LocBaseType::LocBaseCaster), EndPoint(LocBaseType::LocBaseTarget)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "spawn_missile";
		return identifier;
	}

	virtual void process_sml_property(const wyrmgus::sml_property &property) override;
	virtual void process_sml_scope(const wyrmgus::sml_data &scope) override;
	virtual void check() const override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual int Cast(CUnit &caster, const wyrmgus::spell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier) override;
	virtual void Parse(lua_State *lua, int startIndex, int endIndex) override;

private:
	int Damage = 0;							/// Missile damage.
	int LightningDamage = 0;				/// Missile lightning damage.
	int TTL = -1;							/// Missile TTL.
	int Delay = 0;							/// Missile original delay.
	bool UseUnitVar = false;				/// Use the caster's damage parameters
	//Wyrmgus start
	bool AlwaysHits = false;				/// The missile spawned from the spell always hits
	bool AlwaysCritical = false;			/// The damage from the spell is always a critical hit (double damage)
	//Wyrmgus end
	SpellActionMissileLocation StartPoint;	/// Start point description.
	SpellActionMissileLocation EndPoint;	/// End point description.
	wyrmgus::missile_type *Missile = nullptr;			/// Missile fired on cast
};
