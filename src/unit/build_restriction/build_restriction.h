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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

class CUnit;
struct lua_State;

enum class DistanceTypeType
{
	Equal,
	NotEqual,
	LessThan,
	LessThanEqual,
	GreaterThan,
	GreaterThanEqual
};

namespace wyrmgus {

class gsml_data;
class gsml_property;
class unit_type;

class build_restriction
{
public:
	static std::unique_ptr<build_restriction> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<build_restriction> from_gsml_scope(const gsml_data &scope);

	virtual ~build_restriction()
	{
	}

	virtual std::unique_ptr<build_restriction> duplicate() const = 0;
	virtual void process_gsml_property(const gsml_property &property);
	virtual void process_gsml_scope(const gsml_data &scope);

	virtual void Init()
	{
	}

	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, const int z) const = 0;
};

}

extern void ParseBuildingRules(lua_State *l, std::vector<std::unique_ptr<build_restriction>> &blist);
