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

#include "map/map_template.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "util/string_util.h"

namespace wyrmgus {

class location_condition final : public condition
{
public:
	location_condition()
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else {
			throw std::runtime_error("Invalid location condition property: \"" + key + "\".");
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "min_pos") {
			this->min_pos = scope.to_point();
		} else if (tag == "max_pos") {
			this->max_pos = scope.to_point();
		} else {
			throw std::runtime_error("Invalid location condition scope: \"" + tag + "\".");
		}
	}

	virtual void check_validity() const override
	{
		if (this->map_template == nullptr) {
			throw std::runtime_error("\"location\" condition has no map template.");
		}
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		return true;
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		if (min_pos == QPoint(-1, -1) && max_pos == QPoint(-1, -1)) {
			return unit->is_in_subtemplate_area(this->map_template);
		}

		const QRect &subtemplate_rect = unit->MapLayer->get_subtemplate_rect(this->map_template);

		if (!subtemplate_rect.isValid()) {
			return false;
		}

		QPoint min_map_pos(-1, -1);
		if (this->min_pos != QPoint(-1, -1)) {
			min_map_pos = this->map_template->pos_to_map_pos(this->min_pos);
		} else {
			min_map_pos = subtemplate_rect.topLeft();
		}

		QPoint max_map_pos(-1, -1);
		if (this->max_pos != QPoint(-1, -1)) {
			max_map_pos = this->map_template->pos_to_map_pos(this->max_pos);
		} else {
			max_map_pos = subtemplate_rect.bottomRight();
		}

		return unit->is_in_tile_rect(QRect(min_map_pos, max_map_pos), unit->MapLayer->ID);
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		std::string str = "Is in ";

		if (this->min_pos != QPoint(-1, -1) || this->max_pos != QPoint(-1, -1)) {
			str += "a given part of ";
		}

		str += "the " + string::highlight(this->map_template->get_name()) + " map area";

		return str;
	}

private:
	const wyrmgus::map_template *map_template = nullptr;
	QPoint min_pos = QPoint(-1, -1);
	QPoint max_pos = QPoint(-1, -1);
};

}
