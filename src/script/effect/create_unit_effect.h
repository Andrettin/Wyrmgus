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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "character.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "player.h"
#include "script/effect/effect.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace wyrmgus {

class create_unit_effect final : public effect<CPlayer>
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
		static const std::string class_identifier = "create_unit";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else if (key == "unit_class") {
			this->unit_class = unit_class::get(value);
		} else if (key == "character") {
			this->character = character::get(value);
		} else if (key == "site") {
			this->site = site::get(value);
		} else if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else if (key == "ttl") {
			this->ttl = std::stoi(value);
		} else {
			effect::process_sml_property(property);
		}
	}

	virtual void process_sml_scope(const sml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "pos") {
			this->pos = scope.to_point();
		} else {
			effect::process_sml_scope(scope);
		}
	}

	virtual void check() const override
	{
		if (this->unit_type == nullptr && this->unit_class == nullptr && this->character == nullptr) {
			throw std::runtime_error("\"create_unit\" effect has neither a unit type, nor a unit class nor a character.");
		}

		if (this->site != nullptr && this->map_template != nullptr) {
			throw std::runtime_error("\"create_unit\" effect has both a site and a map template.");
		}

		if (this->map_template != nullptr && this->pos == QPoint(-1, -1)) {
			throw std::runtime_error("\"create_unit\" effect has a map template but no position.");
		}
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		const wyrmgus::unit_type *unit_type = this->get_unit_type();

		if (unit_type == nullptr) {
			if (this->unit_class != nullptr) {
				unit_type = player->get_class_unit_type(this->unit_class);

				if (unit_type == nullptr) {
					return;
				}
			}
		}

		const QPoint unit_top_left_pos = this->get_tile_pos(player) - unit_type->get_tile_center_pos_offset();
		const int map_layer = this->get_map_layer_index(player);

		CUnit *unit = CreateUnit(unit_top_left_pos, *unit_type, player, map_layer);
		if (this->character != nullptr) {
			unit->set_character(this->character);
		}
		if (this->ttl != 0) {
			unit->TTL = this->ttl;
		}
	}

	virtual std::string get_assignment_string() const override
	{
		std::string str = "Receive ";
		if (this->character != nullptr) {
			str += "the " + string::highlight(this->character->get_full_name()) + " character";
		} else if (this->unit_class != nullptr) {
			str += "a " + string::highlight(this->unit_class->get_name()) + " unit";
		} else {
			str += "a " + string::highlight(this->unit_type->get_name()) + " unit";
		}
		if (this->site != nullptr) {
			str += " at " + this->site->get_name();
		}
		if (this->ttl != 0) {
			str += " for " + std::to_string(this->ttl) + " cycles";
		}
		return str;
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		if (this->character != nullptr) {
			return this->character->get_unit_type();
		}

		return this->unit_type;
	}

	QPoint get_tile_pos(const CPlayer *player) const
	{
		if (this->site != nullptr) {
			const site_game_data *site_game_data = this->site->get_game_data();

			if (site_game_data->get_site_unit() != nullptr) {
				return site_game_data->get_site_unit()->get_center_tile_pos();
			} else {
				return site_game_data->get_map_pos();
			}
		}

		if (this->map_template != nullptr && CMap::get()->is_subtemplate_on_map(this->map_template)) {
			return this->map_template->pos_to_map_pos(this->pos);
		}

		return player->StartPos;
	}

	int get_map_layer_index(const CPlayer *player) const
	{
		if (this->site != nullptr) {
			const site_game_data *site_game_data = this->site->get_game_data();

			if (site_game_data->get_site_unit() != nullptr) {
				return site_game_data->get_site_unit()->MapLayer->ID;
			} else if (site_game_data->get_map_layer() != nullptr) {
				return site_game_data->get_map_layer()->ID;
			}
		}

		if (this->map_template != nullptr && CMap::get()->is_subtemplate_on_map(this->map_template)) {
			return CMap::get()->get_subtemplate_map_layer(this->map_template)->ID;
		}

		return player->StartMapLayer;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to be created
	const wyrmgus::unit_class *unit_class = nullptr; //the unit class to be created
	wyrmgus::character *character = nullptr; //the character to be created
	const wyrmgus::site *site = nullptr; //the site where the unit type is to be located, if any
	const wyrmgus::map_template *map_template = nullptr; //the map template where the unit type is to be located, if any
	QPoint pos = QPoint(-1, -1); //the unit's position in the map template
	int ttl = 0; //the time to live (in cycles) of the created unit, if any; useful for revealers
};

}
