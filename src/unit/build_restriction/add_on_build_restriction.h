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

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/tile.h"
#include "unit/build_restriction/build_restriction.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class add_on_build_restriction final : public build_restriction
{
	class functor final
	{
	public:
		explicit functor(const unit_type *type, const Vec2i &_pos) : Parent(type), pos(_pos)
		{
		}

		bool operator()(const CUnit *const unit) const
		{
			return (unit->Type == Parent && unit->tilePos == this->pos);
		}

	private:
		const unit_type *const Parent;   /// building that is unit is an addon too.
		const Vec2i pos; //functor work position
	};

public:
	virtual std::unique_ptr<build_restriction> duplicate() const override
	{
		auto b = std::make_unique<add_on_build_restriction>();
		b->Offset = this->Offset;
		b->ParentName = this->ParentName;
		b->Parent = this->Parent;
		return b;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->ParentName = value;
		} else {
			build_restriction::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "offset") {
			this->Offset = scope.to_point();
		} else {
			build_restriction::process_gsml_scope(scope);
		}
	}

	virtual void Init() override
	{
		this->Parent = unit_type::get(this->ParentName);
	}

	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, const int z) const override
	{
		Q_UNUSED(builder);
		Q_UNUSED(type);
		Q_UNUSED(ontoptarget);

		Vec2i pos1 = pos - this->Offset;

		if (CMap::get()->Info->IsPointOnMap(pos1, z) == false) {
			return false;
		}

		functor f(Parent, pos1);

		return CMap::get()->Field(pos1, z)->UnitCache.find(f) != nullptr;
	}

	QPoint Offset = QPoint(0, 0); //offset from the main building to place this
	std::string ParentName; /// building that is unit is an addon too.
	unit_type *Parent = nullptr;      /// building that is unit is an addon too.
};

}
