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

#include "unit/build_restriction/build_restriction.h"

namespace wyrmgus {

class on_top_build_restriction final : public build_restriction
{
public:
	virtual std::unique_ptr<build_restriction> duplicate() const override
	{
		auto b = std::make_unique<on_top_build_restriction>();
		b->ParentName = this->ParentName;
		b->Parent = this->Parent;
		b->ReplaceOnDie = this->ReplaceOnDie;
		b->ReplaceOnBuild = this->ReplaceOnBuild;
		return b;
	}

	virtual void process_gsml_property(const gsml_property &property) override;

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, int z) const override;

	std::string ParentName;  /// building that is unit is an addon too.
	unit_type *Parent = nullptr;       /// building that is unit is an addon too.
	int ReplaceOnDie = 0;     /// recreate the parent on destruction
	int ReplaceOnBuild = 0;   /// remove the parent, or just build over it.
};

}
