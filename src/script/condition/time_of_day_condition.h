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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "map/tile.h"
#include "script/condition/condition.h"
#include "time/time_of_day.h"
#include "util/string_util.h"

namespace wyrmgus {

template <typename scope_type>
class time_of_day_condition final : public condition<scope_type>
{
public:
	explicit time_of_day_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->time_of_day = time_of_day::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "time_of_day";
		return class_identifier;
	}

	const CMapLayer *get_scope_map_layer(const scope_type *scope) const
	{
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			return CMap::get()->MapLayers[scope->StartMapLayer].get();
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			return scope->MapLayer;
		}
	}

	QPoint get_scope_tile_pos(const scope_type *scope) const
	{
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			return scope->StartPos;
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			return scope->get_center_tile_pos();
		}
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const CMapLayer *map_layer = this->get_scope_map_layer(scope);

		if (map_layer == nullptr) {
			return false;
		}

		const QPoint tile_pos = this->get_scope_tile_pos(scope);

		const wyrmgus::time_of_day *tile_time_of_day = map_layer->get_tile_time_of_day(tile_pos);
		return tile_time_of_day == this->time_of_day;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return string::highlight(this->time_of_day->get_name()) + " time of day";
	}

private:
	const wyrmgus::time_of_day *time_of_day = nullptr;
};

}
