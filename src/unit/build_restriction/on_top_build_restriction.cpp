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

#include "stratagus.h"

#include "unit/build_restriction/on_top_build_restriction.h"

#include "actions.h"
#include "database/gsml_property.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_settings.h"
#include "map/tile.h"
#include "unit/unit.h"
#include "unit/unit_cache.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

class AliveConstructedAndSameTypeAs final
{
public:
	explicit AliveConstructedAndSameTypeAs(const unit_type &unitType) : type(&unitType) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsAlive() && unit->Type == type && unit->CurrentAction() != UnitAction::Built;
	}

private:
	const unit_type *type = nullptr;
};

namespace wyrmgus {

void on_top_build_restriction::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->ParentName = value;
	} else if (key == "replace_on_die") {
		this->ReplaceOnDie = string::to_bool(value);
	} else if (key == "replace_on_build") {
		this->ReplaceOnBuild = string::to_bool(value);
	} else {
		build_restriction::process_gsml_property(property);
	}
}

void on_top_build_restriction::Init()
{
	this->Parent = unit_type::get(this->ParentName);
}

bool on_top_build_restriction::Check(const CUnit *builder, const unit_type &, const QPoint &pos, CUnit *&ontoptarget, int z) const
{
	if (CMap::get()->get_settings()->is_unit_type_disabled(this->Parent)) {
		return true;
	}

	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	ontoptarget = nullptr;

	CUnitCache &cache = CMap::get()->Field(pos, z)->UnitCache;

	CUnitCache::iterator it = std::find_if(cache.begin(), cache.end(), AliveConstructedAndSameTypeAs(*this->Parent));

	if (it != cache.end() && (*it)->tilePos == pos) {
		CUnit &found = **it;
		std::vector<CUnit *> table;
		Vec2i endPos(found.tilePos + found.Type->get_tile_size() - QSize(1, 1));
		Select(found.tilePos, endPos, table, found.MapLayer->ID);
		for (std::vector<CUnit *>::iterator it2 = table.begin(); it2 != table.end(); ++it2) {
			if (*it == *it2) {
				continue;
			}
			if (builder == *it2) {
				continue;
			}
			//Wyrmgus start
			// allow to build if a decoration is present under the deposit
			if ((*it2)->Type->BoolFlag[DECORATION_INDEX].value) {
				continue;
			}
			//Wyrmgus end

			if (found.Type->get_domain() == (*it2)->Type->get_domain()) {
				return false;
			}
		}
		ontoptarget = *it;
		return true;
	}
	return false;
}

}
