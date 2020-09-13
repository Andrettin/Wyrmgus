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

class SpellActionTypeAdjustVariable final
{
public:
	int Enable = 0;                 /// Value to affect to this field.
	int Value = 0;                  /// Value to affect to this field.
	int Max = 0;                    /// Value to affect to this field.
	int Increase = 0;               /// Value to affect to this field.

	char ModifEnable = 0;           /// true if we modify this field.
	char ModifValue = 0;            /// true if we modify this field.
	char ModifMax = 0;              /// true if we modify this field.
	char ModifIncrease = 0;         /// true if we modify this field.

	char InvertEnable = 0;          /// true if we invert this field.
	int AddValue = 0;               /// Add this value to this field.
	int AddMax = 0;                 /// Add this value to this field.
	int AddIncrease = 0;            /// Add this value to this field.
	int IncreaseTime = 0;           /// How many time increase the Value field.
	char TargetIsCaster = 0;        /// true if the target is the caster.
};

class Spell_AdjustVariable final : public wyrmgus::spell_action
{
public:
	Spell_AdjustVariable();

	~Spell_AdjustVariable() {
		delete [](this->Var);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "adjust_variable";
		return identifier;
	}

	virtual void process_sml_property(const wyrmgus::sml_property &property) override;
	virtual void process_sml_scope(const wyrmgus::sml_data &scope) override;
	virtual int Cast(CUnit &caster, const wyrmgus::spell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier) override;
	virtual void Parse(lua_State *l, int startIndex, int endIndex) override;

private:
	SpellActionTypeAdjustVariable *Var = nullptr;
};
