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

class missile_type;

class spell_action_spawn_missile final : public spell_action
{
public:
	enum class location_base_type {
		caster,
		target
	};

	/**
	**  This struct is used for defining a missile start/stop location.
	**
	**  It's evaluated like this, and should be more or less flexible.:
	**  base coordinates(caster or target) + (AddX,AddY) + (rand()%AddRandX,rand()%AddRandY)
	*/
	class missile_location final
	{
	public:
		explicit missile_location(const location_base_type base) : Base(base)
		{
		}

		void process_gsml_property(const gsml_property &property);
		void process_gsml_scope(const gsml_data &scope);
		void evaluate(const CUnit &caster, const CUnit *target, const Vec2i &goalPos, PixelPos *res) const;

		location_base_type Base;	/// The base for the location (caster/target)
		int AddX = 0;		/// Add to the X coordinate
		int AddY = 0;		/// Add to the X coordinate
		int AddRandX = 0;	/// Random add to the X coordinate
		int AddRandY = 0;	/// Random add to the X coordinate
	};

	static location_base_type string_to_location_base_type(const std::string &str)
	{
		if (str == "caster") {
			return location_base_type::caster;
		} else if (str == "target") {
			return location_base_type::target;
		}

		throw std::runtime_error("Invalid spawn missile location base type: \"" + str + "\".");
	}

	spell_action_spawn_missile()
		: StartPoint(location_base_type::caster), EndPoint(location_base_type::target)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "spawn_missile";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;
	virtual int Cast(CUnit &caster, const spell &spell,
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
	missile_location StartPoint;	/// Start point description.
	missile_location EndPoint;	/// End point description.
	missile_type *Missile = nullptr;			/// Missile fired on cast
};

}
