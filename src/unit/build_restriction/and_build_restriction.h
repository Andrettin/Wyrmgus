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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
//		and Andrettin
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

#include "unit/build_restriction/build_restriction.h"

extern int CclDefineUnitType(lua_State *l);

namespace wyrmgus {

class and_build_restriction final : public build_restriction
{
public:
	std::unique_ptr<and_build_restriction> duplicate_derived() const
	{
		auto b = std::make_unique<and_build_restriction>();

		for (const auto &build_restriction : this->restrictions) {
			b->restrictions.push_back(build_restriction->duplicate());
		}

		return b;
	}

	virtual std::unique_ptr<build_restriction> duplicate() const override
	{
		return this->duplicate_derived();
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->restrictions.push_back(build_restriction::from_gsml_property(property));
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->restrictions.push_back(build_restriction::from_gsml_scope(scope));
	}

	virtual void Init() override
	{
		for (const auto &build_restriction : this->restrictions) {
			build_restriction->Init();
		}
	}

	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, const int z) const override
	{
		for (const auto &b : this->restrictions) {
			if (!b->Check(builder, type, pos, ontoptarget, z)) {
				return false;
			}
		}

		return true;
	}

	const std::vector<std::unique_ptr<build_restriction>> &get_restrictions() const
	{
		return this->restrictions;
	}

	void add_restriction(std::unique_ptr<build_restriction> &&restriction)
	{
		this->restrictions.push_back(std::move(restriction));
	}

private:
	std::vector<std::unique_ptr<build_restriction>> restrictions;

	friend int ::CclDefineUnitType(lua_State *l);
	friend void ::ParseBuildingRules(lua_State *l, std::vector<std::unique_ptr<build_restriction>> &blist);
};

}
