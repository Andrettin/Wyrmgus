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

#include "util/singleton.h"

namespace wyrmgus {

class button_level;
class civilization;
class dialogue;
class font;
class font_color;
class player_color;
class sml_data;
class sml_property;
class terrain_type;
class time_of_day;
class unit_class;

class defines final : public QObject, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::font* small_font MEMBER small_font READ get_small_font)
	Q_PROPERTY(wyrmgus::font* game_font MEMBER game_font READ get_game_font)
	Q_PROPERTY(wyrmgus::font_color* default_font_color MEMBER default_font_color READ get_default_font_color)
	Q_PROPERTY(wyrmgus::font_color* default_highlight_font_color MEMBER default_highlight_font_color READ get_default_highlight_font_color)
	Q_PROPERTY(wyrmgus::font_color* ally_font_color MEMBER ally_font_color READ get_ally_font_color)
	Q_PROPERTY(wyrmgus::font_color* enemy_font_color MEMBER enemy_font_color READ get_enemy_font_color)
	Q_PROPERTY(wyrmgus::font_color* magic_font_color MEMBER magic_font_color READ get_magic_font_color)
	Q_PROPERTY(wyrmgus::font_color* unique_font_color MEMBER unique_font_color READ get_unique_font_color)
	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size)
	Q_PROPERTY(QSize icon_size MEMBER icon_size READ get_icon_size)
	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(wyrmgus::player_color* neutral_player_color MEMBER neutral_player_color READ get_neutral_player_color)
	Q_PROPERTY(wyrmgus::civilization* neutral_civilization MEMBER neutral_civilization READ get_neutral_civilization)
	Q_PROPERTY(int minimap_color_index MEMBER minimap_color_index READ get_minimap_color_index)
	Q_PROPERTY(int minimap_non_land_territory_alpha MEMBER minimap_non_land_territory_alpha READ get_minimap_non_land_territory_alpha)
	Q_PROPERTY(wyrmgus::time_of_day* underground_time_of_day MEMBER underground_time_of_day READ get_underground_time_of_day)
	Q_PROPERTY(wyrmgus::terrain_type* border_terrain_type MEMBER border_terrain_type READ get_border_terrain_type)
	Q_PROPERTY(wyrmgus::dialogue* campaign_victory_dialogue MEMBER campaign_victory_dialogue READ get_campaign_victory_dialogue)
	Q_PROPERTY(wyrmgus::dialogue* campaign_defeat_dialogue MEMBER campaign_defeat_dialogue READ get_campaign_defeat_dialogue)
	Q_PROPERTY(wyrmgus::button_level* inventory_button_level MEMBER inventory_button_level READ get_inventory_button_level)
	Q_PROPERTY(wyrmgus::button_level* cancel_button_level MEMBER cancel_button_level READ get_cancel_button_level)
	Q_PROPERTY(wyrmgus::unit_class* town_hall_class MEMBER town_hall_class READ get_town_hall_class)
	Q_PROPERTY(wyrmgus::unit_class* default_population_class MEMBER default_population_class READ get_default_population_class)

public:
	void load(const std::filesystem::path &base_path);
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void initialize();

	font *get_small_font() const
	{
		return this->small_font;
	}

	font *get_game_font() const
	{
		return this->game_font;
	}

	font_color *get_default_font_color() const
	{
		return this->default_font_color;
	}

	font_color *get_default_highlight_font_color() const
	{
		return this->default_highlight_font_color;
	}

	font_color *get_ally_font_color() const
	{
		return this->ally_font_color;
	}

	font_color *get_enemy_font_color() const
	{
		return this->enemy_font_color;
	}

	font_color *get_magic_font_color() const
	{
		return this->magic_font_color;
	}

	font_color *get_unique_font_color() const
	{
		return this->unique_font_color;
	}

	const QSize &get_tile_size() const
	{
		return this->tile_size;
	}

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	const QSize &get_icon_size() const
	{
		return this->icon_size;
	}

	int get_scale_factor() const
	{
		return this->scale_factor;
	}

	QSize get_scaled_tile_size() const
	{
		return this->get_tile_size() * this->get_scale_factor();
	}

	int get_scaled_tile_width() const
	{
		return this->get_tile_width() * this->get_scale_factor();
	}

	int get_scaled_tile_height() const
	{
		return this->get_tile_height() * this->get_scale_factor();
	}

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	player_color *get_neutral_player_color() const
	{
		return this->neutral_player_color;
	}

	civilization *get_neutral_civilization() const
	{
		return this->neutral_civilization;
	}

	int get_minimap_color_index() const
	{
		return this->minimap_color_index;
	}

	int get_minimap_non_land_territory_alpha() const
	{
		return this->minimap_non_land_territory_alpha;
	}

	time_of_day *get_underground_time_of_day() const
	{
		return this->underground_time_of_day;
	}

	terrain_type *get_border_terrain_type() const
	{
		return this->border_terrain_type;
	}

	dialogue *get_campaign_victory_dialogue() const
	{
		return this->campaign_victory_dialogue;
	}

	dialogue *get_campaign_defeat_dialogue() const
	{
		return this->campaign_defeat_dialogue;
	}

	button_level *get_inventory_button_level() const
	{
		return this->inventory_button_level;
	}

	button_level *get_cancel_button_level() const
	{
		return this->cancel_button_level;
	}

	unit_class *get_town_hall_class() const
	{
		return this->town_hall_class;
	}

	unit_class *get_default_population_class() const
	{
		return this->default_population_class;
	}

private:
	font *small_font = nullptr;
	font *game_font = nullptr;
	font_color *default_font_color = nullptr;
	font_color *default_highlight_font_color = nullptr;
	font_color *ally_font_color = nullptr; //the font color for the names of allies
	font_color *enemy_font_color = nullptr; //the font color for the names of enemies
	font_color *magic_font_color = nullptr; //the font color for the names of magic items
	font_color *unique_font_color = nullptr; //the font color for the names of unique items and characters
	QSize tile_size;
	QSize icon_size;
	int scale_factor = 1;
	player_color *conversible_player_color = nullptr;
	player_color *neutral_player_color = nullptr;
	civilization *neutral_civilization = nullptr;
	int minimap_color_index = 0;
	int minimap_non_land_territory_alpha = 64;
	terrain_type *border_terrain_type = nullptr;
	time_of_day *underground_time_of_day = nullptr;
	dialogue *campaign_victory_dialogue = nullptr;
	dialogue *campaign_defeat_dialogue = nullptr;
	button_level *inventory_button_level = nullptr;
	button_level *cancel_button_level = nullptr;
	unit_class *town_hall_class = nullptr;
	unit_class *default_population_class = nullptr;
};

}
