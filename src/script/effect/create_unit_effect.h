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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "map/map_layer.h"
#include "map/site.h"
#include "player.h"
#include "script/effect/effect.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace wyrmgus {

class create_unit_effect final : public effect
{
public:
	explicit create_unit_effect(const sml_operator effect_operator)
		: effect(effect_operator)
	{
	}

	explicit create_unit_effect(const std::string &unit_type_identifier, const sml_operator effect_operator)
		: create_unit_effect(effect_operator)
	{
		this->unit_type = unit_type::get(unit_type_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static std::string class_identifier = "create_unit";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else if (key == "site") {
			this->site = site::get(value);
		} else if (key == "ttl") {
			this->ttl = std::stoi(value);
		} else {
			effect::process_sml_property(property);
		}
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		QPoint unit_top_left_pos = this->site && this->site->get_site_unit() ? this->site->get_site_unit()->get_center_tile_pos() : player->StartPos;
		unit_top_left_pos -= this->unit_type->get_tile_center_pos_offset();
		const int map_layer = this->site && this->site->get_site_unit() ? this->site->get_site_unit()->MapLayer->ID : player->StartMapLayer;

		CUnit *unit = MakeUnitAndPlace(unit_top_left_pos, *this->unit_type, player, map_layer);
		if (this->ttl != 0) {
			unit->TTL = this->ttl;
		}
	}

	virtual std::string get_assignment_string() const override
	{
		std::string str = "Receive a " + string::highlight(this->unit_type->get_name()) + " unit";
		if (this->site != nullptr) {
			str += " at " + this->site->get_name();
		}
		if (this->ttl != 0) {
			str += " for " + std::to_string(this->ttl) + " cycles";
		}
		return str;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to be created
	const site *site = nullptr; //the site where the unit type is to be located, if any
	int ttl = 0; //the time to live (in cycles) of the created unit, if any; useful for revealers
};

}
