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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "database/defines_base.h"
#include "util/centesimal_int.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("dialogue.h")
Q_MOC_INCLUDE("economy/resource.h")
Q_MOC_INCLUDE("map/map_presets.h")
Q_MOC_INCLUDE("map/map_projection.h")
Q_MOC_INCLUDE("map/terrain_type.h")
Q_MOC_INCLUDE("player/civilization.h")
Q_MOC_INCLUDE("player/player_color.h")
Q_MOC_INCLUDE("population/population_class.h")
Q_MOC_INCLUDE("time/season_schedule.h")
Q_MOC_INCLUDE("time/time_of_day.h")
Q_MOC_INCLUDE("time/time_of_day_schedule.h")
Q_MOC_INCLUDE("ui/button_level.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/interface_style.h")
Q_MOC_INCLUDE("ui/resource_icon.h")
Q_MOC_INCLUDE("unit/unit_class.h")
Q_MOC_INCLUDE("video/font.h")
Q_MOC_INCLUDE("video/font_color.h")

class CGraphic;
class CPlayerColorGraphic;
class CUpgrade;

namespace archimedes {
	class map_projection;
}

namespace wyrmgus {

class button_level;
class civilization;
class dialogue;
class font;
class font_color;
class icon;
class interface_style;
class map_presets;
class music;
class player_color;
class population_class;
class resource;
class resource_icon;
class season;
class season_schedule;
class terrain_type;
class time_of_day;
class time_of_day_schedule;
class unit_class;
enum class faction_type;
enum class tile_transition_type;
enum class trigger_type;

class defines final : public defines_base, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::interface_style* default_interface_style MEMBER default_interface_style READ get_default_interface_style)
	Q_PROPERTY(wyrmgus::font* small_font MEMBER small_font)
	Q_PROPERTY(wyrmgus::font* game_font MEMBER game_font)
	Q_PROPERTY(wyrmgus::font_color* default_font_color MEMBER default_font_color READ get_default_font_color)
	Q_PROPERTY(wyrmgus::font_color* default_highlight_font_color MEMBER default_highlight_font_color READ get_default_highlight_font_color)
	Q_PROPERTY(wyrmgus::font_color* ally_font_color MEMBER ally_font_color READ get_ally_font_color)
	Q_PROPERTY(wyrmgus::font_color* enemy_font_color MEMBER enemy_font_color READ get_enemy_font_color)
	Q_PROPERTY(wyrmgus::font_color* magic_font_color MEMBER magic_font_color READ get_magic_font_color)
	Q_PROPERTY(wyrmgus::font_color* unique_font_color MEMBER unique_font_color READ get_unique_font_color)
	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size NOTIFY changed)
	Q_PROPERTY(QSize icon_size MEMBER icon_size READ get_icon_size)
	Q_PROPERTY(QSize resource_icon_size MEMBER resource_icon_size READ get_resource_icon_size)
	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(wyrmgus::player_color* neutral_player_color MEMBER neutral_player_color READ get_neutral_player_color NOTIFY changed)
	Q_PROPERTY(wyrmgus::civilization* neutral_civilization MEMBER neutral_civilization READ get_neutral_civilization NOTIFY changed)
	Q_PROPERTY(int minimap_color_index MEMBER minimap_color_index READ get_minimap_color_index)
	Q_PROPERTY(int minimap_non_land_territory_alpha MEMBER minimap_non_land_territory_alpha READ get_minimap_non_land_territory_alpha)
	Q_PROPERTY(QColor selected_border_color MEMBER selected_border_color READ get_selected_border_color)
	Q_PROPERTY(QColor autocast_border_color MEMBER autocast_border_color READ get_autocast_border_color)
	Q_PROPERTY(QColor magic_item_border_color MEMBER magic_item_border_color READ get_magic_item_border_color)
	Q_PROPERTY(QColor unique_item_border_color MEMBER unique_item_border_color READ get_unique_item_border_color)
	Q_PROPERTY(int map_area_top_margin MEMBER map_area_top_margin READ get_map_area_top_margin NOTIFY changed)
	Q_PROPERTY(int map_area_bottom_margin MEMBER map_area_bottom_margin READ get_map_area_bottom_margin NOTIFY changed)
	Q_PROPERTY(wyrmgus::resource* time_resource MEMBER time_resource)
	Q_PROPERTY(wyrmgus::resource* wealth_resource MEMBER wealth_resource NOTIFY changed)
	Q_PROPERTY(wyrmgus::time_of_day* underground_time_of_day MEMBER underground_time_of_day)
	Q_PROPERTY(wyrmgus::time_of_day_schedule* default_time_of_day_schedule MEMBER default_time_of_day_schedule)
	Q_PROPERTY(wyrmgus::season_schedule* default_season_schedule MEMBER default_season_schedule)
	Q_PROPERTY(archimedes::map_projection* default_map_projection MEMBER default_map_projection)
	Q_PROPERTY(wyrmgus::terrain_type* ford_terrain_type MEMBER ford_terrain_type)
	Q_PROPERTY(std::filesystem::path border_image_file MEMBER border_image_file WRITE set_border_image_file)
	Q_PROPERTY(QSize border_frame_size MEMBER border_frame_size)
	Q_PROPERTY(unsigned char border_opacity MEMBER border_opacity READ get_border_opacity)
	Q_PROPERTY(wyrmgus::dialogue* campaign_victory_dialogue MEMBER campaign_victory_dialogue READ get_campaign_victory_dialogue)
	Q_PROPERTY(wyrmgus::dialogue* campaign_defeat_dialogue MEMBER campaign_defeat_dialogue READ get_campaign_defeat_dialogue)
	Q_PROPERTY(wyrmgus::button_level* inventory_button_level MEMBER inventory_button_level READ get_inventory_button_level)
	Q_PROPERTY(wyrmgus::button_level* cancel_button_level MEMBER cancel_button_level READ get_cancel_button_level)
	Q_PROPERTY(wyrmgus::unit_class* town_hall_class MEMBER town_hall_class READ get_town_hall_class)
	Q_PROPERTY(bool population_enabled MEMBER population_enabled READ is_population_enabled)
	Q_PROPERTY(wyrmgus::resource_icon* population_resource_icon MEMBER population_resource_icon)
	Q_PROPERTY(wyrmgus::population_class* default_population_class MEMBER default_population_class)
	Q_PROPERTY(wyrmgus::unit_class* default_population_unit_class MEMBER default_population_unit_class)
	Q_PROPERTY(wyrmgus::unit_class* default_water_population_unit_class MEMBER default_water_population_unit_class)
	Q_PROPERTY(wyrmgus::unit_class* default_space_population_unit_class MEMBER default_space_population_unit_class)
	Q_PROPERTY(wyrmgus::resource_icon* food_icon MEMBER food_icon READ get_food_icon)
	Q_PROPERTY(wyrmgus::resource_icon* score_icon MEMBER score_icon READ get_score_icon)
	Q_PROPERTY(wyrmgus::resource_icon* mana_icon MEMBER mana_icon READ get_mana_icon)
	Q_PROPERTY(int default_cycles_per_year MEMBER default_cycles_per_year)
	Q_PROPERTY(int forest_regeneration_threshold MEMBER forest_regeneration_threshold READ get_forest_regeneration_threshold)
	Q_PROPERTY(int destroyed_overlay_terrain_decay_threshold MEMBER destroyed_overlay_terrain_decay_threshold READ get_destroyed_overlay_terrain_decay_threshold)
	Q_PROPERTY(int scaled_tile_width READ get_scaled_tile_width CONSTANT)
	Q_PROPERTY(int scaled_tile_height READ get_scaled_tile_height CONSTANT)
	Q_PROPERTY(bool deselect_in_mine MEMBER deselect_in_mine READ is_deselect_in_mine_enabled)
	Q_PROPERTY(int population_per_unit MEMBER population_per_unit READ get_population_per_unit)
	Q_PROPERTY(wyrmgus::icon* default_quest_icon MEMBER default_quest_icon)
	Q_PROPERTY(wyrmgus::map_presets* map_editor_default_map_presets MEMBER map_editor_default_map_presets)
	Q_PROPERTY(QString default_menu_background_file READ get_default_menu_background_file_qstring NOTIFY changed)
	Q_PROPERTY(QStringList loading_background_files READ get_loading_background_files_qstring_list NOTIFY changed)
	Q_PROPERTY(QStringList tips READ get_tips_qstring_list NOTIFY changed)

public:
	using singleton<defines>::get;

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	interface_style *get_default_interface_style() const
	{
		return this->default_interface_style;
	}

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

	QSize get_scaled_icon_size() const
	{
		return this->get_icon_size() * this->get_scale_factor();
	}

	const QSize &get_resource_icon_size() const
	{
		return this->resource_icon_size;
	}

	const centesimal_int &get_scale_factor() const;

	QSize get_scaled_tile_size() const
	{
		return this->get_tile_size() * this->get_scale_factor();
	}

	int get_scaled_tile_width() const
	{
		return (this->get_tile_width() * this->get_scale_factor()).to_int();
	}

	int get_scaled_tile_height() const
	{
		return (this->get_tile_height() * this->get_scale_factor()).to_int();
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

	const QColor &get_selected_border_color() const
	{
		return this->selected_border_color;
	}

	const QColor &get_autocast_border_color() const
	{
		return this->autocast_border_color;
	}

	const QColor &get_magic_item_border_color() const
	{
		return this->magic_item_border_color;
	}

	const QColor &get_unique_item_border_color() const
	{
		return this->unique_item_border_color;
	}

	int get_map_area_top_margin() const
	{
		return this->map_area_top_margin;
	}

	int get_map_area_bottom_margin() const
	{
		return this->map_area_bottom_margin;
	}

	const resource *get_time_resource() const
	{
		return this->time_resource;
	}

	const resource *get_wealth_resource() const
	{
		return this->wealth_resource;
	}

	const time_of_day *get_underground_time_of_day() const
	{
		return this->underground_time_of_day;
	}

	time_of_day_schedule *get_default_time_of_day_schedule() const
	{
		return this->default_time_of_day_schedule;
	}

	season_schedule *get_default_season_schedule() const
	{
		return this->default_season_schedule;
	}

	const map_projection *get_default_map_projection() const
	{
		return this->default_map_projection;
	}

	const terrain_type *get_ford_terrain_type() const
	{
		return this->ford_terrain_type;
	}

	void set_border_image_file(const std::filesystem::path &filepath);

	const std::shared_ptr<CPlayerColorGraphic> &get_border_graphics() const
	{
		return this->border_graphics;
	}

	QPoint get_border_offset() const;

	unsigned char get_border_opacity() const
	{
		return this->border_opacity;
	}

	const std::vector<int> &get_border_transition_tiles(const tile_transition_type transition_type) const
	{
		static std::vector<int> empty_vector;

		const auto find_iterator = this->border_transition_tiles.find(transition_type);
		if (find_iterator != this->border_transition_tiles.end()) {
			return find_iterator->second;
		}

		return empty_vector;
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

	bool is_population_enabled() const
	{
		return this->population_enabled;
	}

	const resource_icon *get_population_resource_icon() const
	{
		return this->population_resource_icon;
	}

	const population_class *get_default_population_class() const
	{
		return this->default_population_class;
	}

	const unit_class *get_default_population_unit_class() const
	{
		return this->default_population_unit_class;
	}

	const unit_class *get_default_water_population_unit_class() const
	{
		return this->default_water_population_unit_class;
	}

	const unit_class *get_default_space_population_unit_class() const
	{
		return this->default_space_population_unit_class;
	}

	const CUpgrade *get_faction_type_upgrade(const faction_type faction_type)
	{
		const auto find_iterator = this->faction_type_upgrades.find(faction_type);
		if (find_iterator != this->faction_type_upgrades.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const std::shared_ptr<CGraphic> &get_icon_frame_graphics() const
	{
		return this->icon_frame_graphics;
	}

	const std::shared_ptr<CGraphic> &get_pressed_icon_frame_graphics() const
	{
		return this->pressed_icon_frame_graphics;
	}

	const std::shared_ptr<CGraphic> &get_command_button_frame_graphics() const
	{
		return this->command_button_frame_graphics;
	}

	const std::shared_ptr<CGraphic> &get_bar_frame_graphics() const
	{
		return this->bar_frame_graphics;
	}

	const std::shared_ptr<CGraphic> &get_infopanel_frame_graphics() const
	{
		return this->infopanel_frame_graphics;
	}

	const std::shared_ptr<CGraphic> &get_progress_bar_graphics() const
	{
		return this->progress_bar_graphics;
	}

	resource_icon *get_food_icon() const
	{
		return this->food_icon;
	}

	resource_icon *get_score_icon() const
	{
		return this->score_icon;
	}

	resource_icon *get_mana_icon() const
	{
		return this->mana_icon;
	}

	int get_cycles_per_year(const int current_year) const;

	int get_forest_regeneration_threshold() const
	{
		return this->forest_regeneration_threshold;
	}

	int get_destroyed_overlay_terrain_decay_threshold() const
	{
		return this->destroyed_overlay_terrain_decay_threshold;
	}

	bool is_deselect_in_mine_enabled() const
	{
		return this->deselect_in_mine;
	}

	int get_population_per_unit() const
	{
		return this->population_per_unit;
	}

	const std::map<trigger_type, int> get_trigger_type_none_random_weights() const
	{
		return this->trigger_type_none_random_weights;
	}

	const icon *get_default_quest_icon() const
	{
		return this->default_quest_icon;
	}

	const map_presets *get_map_editor_default_map_presets() const
	{
		return this->map_editor_default_map_presets;
	}

	QString get_default_menu_background_file_qstring() const;
	void set_default_menu_background_file(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_default_menu_background_file(const std::string &filepath)
	{
		this->set_default_menu_background_file(std::filesystem::path(filepath));
	}

	QStringList get_loading_background_files_qstring_list() const;
	Q_INVOKABLE void add_loading_background_file(const std::string &filepath);
	Q_INVOKABLE void remove_loading_background_file(const std::string &filepath);

	const std::vector<std::string> &get_tips() const
	{
		return this->tips;
	}

	QStringList get_tips_qstring_list() const;

	Q_INVOKABLE void add_tip(const std::string &tip)
	{
		this->tips.push_back(tip);
	}

	Q_INVOKABLE void remove_tip(const std::string &tip);

	bool is_0_ad_template_name_ignored(const std::string &template_name) const
	{
		return this->ignored_0_ad_template_names.contains(template_name);
	}

	int get_0_ad_template_resource_amount(const std::string &template_name) const
	{
		const auto find_iterator = this->zero_ad_template_resource_amounts.find(template_name);

		if (find_iterator != this->zero_ad_template_resource_amounts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	int get_0_ad_water_height_multiplier() const
	{
		return this->zero_ad_water_height_multiplier;
	}

	Q_INVOKABLE QStringList get_translation_locales_qstring_list() const;
	Q_INVOKABLE QString get_translation_name_qstring(const QString &locale_qstr) const;

signals:
	void changed();

private:
	interface_style *default_interface_style = nullptr;
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
	QSize resource_icon_size;
	player_color *conversible_player_color = nullptr;
	player_color *neutral_player_color = nullptr;
	civilization *neutral_civilization = nullptr;
	int minimap_color_index = 0;
	int minimap_non_land_territory_alpha = 64;
	QColor selected_border_color;
	QColor autocast_border_color;
	QColor magic_item_border_color;
	QColor unique_item_border_color;
	int map_area_top_margin = 0;
	int map_area_bottom_margin = 0;
	std::filesystem::path border_image_file;
	std::shared_ptr<CPlayerColorGraphic> border_graphics;
	QSize border_frame_size;
	unsigned char border_opacity = 255;
	std::map<tile_transition_type, std::vector<int>> border_transition_tiles;
	resource *time_resource = nullptr;
	resource *wealth_resource = nullptr;
	time_of_day *underground_time_of_day = nullptr;
	time_of_day_schedule *default_time_of_day_schedule = nullptr;
	season_schedule *default_season_schedule = nullptr;
	map_projection *default_map_projection = nullptr;
	terrain_type *ford_terrain_type = nullptr;
	dialogue *campaign_victory_dialogue = nullptr;
	dialogue *campaign_defeat_dialogue = nullptr;
	button_level *inventory_button_level = nullptr;
	button_level *cancel_button_level = nullptr;
	unit_class *town_hall_class = nullptr;
	bool population_enabled = false;
	resource_icon *population_resource_icon = nullptr;
	population_class *default_population_class = nullptr;
	unit_class *default_population_unit_class = nullptr;
	unit_class *default_water_population_unit_class = nullptr;
	unit_class *default_space_population_unit_class = nullptr;
	std::map<faction_type, const CUpgrade *> faction_type_upgrades;
	std::shared_ptr<CGraphic> icon_frame_graphics;
	std::shared_ptr<CGraphic> pressed_icon_frame_graphics;
	std::shared_ptr<CGraphic> command_button_frame_graphics;
	std::shared_ptr<CGraphic> bar_frame_graphics;
	std::shared_ptr<CGraphic> infopanel_frame_graphics;
	std::shared_ptr<CGraphic> progress_bar_graphics;
	resource_icon *food_icon = nullptr;
	resource_icon *score_icon = nullptr;
	resource_icon *mana_icon = nullptr;
	int default_cycles_per_year = 1800;
	std::map<int, int> cycles_per_year_after;
	int forest_regeneration_threshold = 0;
	int destroyed_overlay_terrain_decay_threshold = 0;
	bool deselect_in_mine = true; //deselect workers when they enter a mine
	int population_per_unit = 0; //the number of people a unit represents
	std::map<trigger_type, int> trigger_type_none_random_weights; //the weight for no trigger happening for a given trigger type's random trigger selection
	icon *default_quest_icon = nullptr;
	map_presets *map_editor_default_map_presets = nullptr;
	std::filesystem::path default_menu_background_file;
	std::vector<std::filesystem::path> loading_background_files;
	std::vector<std::string> tips;
	std::set<std::string> ignored_0_ad_template_names;
	std::map<std::string, int> zero_ad_template_resource_amounts;
	int zero_ad_water_height_multiplier = 0;
	std::map<std::string, std::string> translations; //translation names mapped to their locales
};

}
