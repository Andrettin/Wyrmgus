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
//      (c) Copyright 2022 by Andrettin
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

#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "ui/ui.h"

namespace wyrmgus {

class center_on_site_effect final : public effect<CPlayer>
{
public:
	explicit center_on_site_effect(const std::string &site_identifier, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->site = site::get(site_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "center_on_site";
		return class_identifier;
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		if (player != CPlayer::GetThisPlayer()) {
			return;
		}

		const site_game_data *site_game_data = this->site->get_game_data();
		if (!site_game_data->is_on_map()) {
			return;
		}

		int map_layer_index = 0;
		if (site_game_data->get_site_unit() != nullptr) {
			map_layer_index = site_game_data->get_site_unit()->MapLayer->ID;
		} else if (site_game_data->get_map_layer() != nullptr) {
			map_layer_index = site_game_data->get_map_layer()->ID;
		}

		ChangeCurrentMapLayer(map_layer_index);

		if (UI.SelectedViewport != nullptr) {
			QPoint map_pos;

			if (site_game_data->get_site_unit() != nullptr) {
				map_pos = site_game_data->get_site_unit()->get_center_tile_pos();
			} else {
				map_pos = site_game_data->get_map_pos();
			}

			UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(map_pos));
		}
	}

	virtual std::string get_assignment_string() const override
	{
		return std::string();
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	wyrmgus::site *site = nullptr;
};

}
