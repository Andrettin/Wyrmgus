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

#include "util/fractional_int.h"
#include "util/singleton.h"

namespace wyrmgus {

class campaign;
class gsml_data;
class gsml_property;
enum class difficulty;
enum class hotkey_setup;

class preferences final : public QObject, public singleton<preferences>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::centesimal_int scale_factor READ get_scale_factor WRITE set_scale_factor NOTIFY scale_factor_changed)
	Q_PROPERTY(QString scale_factor_string READ get_scale_factor_qstring WRITE set_scale_factor_qstring NOTIFY scale_factor_changed)
	Q_PROPERTY(bool fullscreen READ is_fullscreen WRITE set_fullscreen NOTIFY fullscreen_changed)
	Q_PROPERTY(QSize window_size READ get_window_size WRITE set_window_size NOTIFY window_size_changed)
	Q_PROPERTY(int window_width READ get_window_width WRITE set_window_width NOTIFY window_size_changed)
	Q_PROPERTY(int window_height READ get_window_height WRITE set_window_height NOTIFY window_size_changed)
	Q_PROPERTY(bool window_maximized READ is_window_maximized WRITE set_window_maximized NOTIFY window_maximized_changed)
	Q_PROPERTY(int game_speed READ get_game_speed WRITE set_game_speed NOTIFY game_speed_changed)
	Q_PROPERTY(wyrmgus::difficulty difficulty READ get_difficulty WRITE set_difficulty)
	Q_PROPERTY(bool sound_effects_enabled READ are_sound_effects_enabled WRITE set_sound_effects_enabled NOTIFY sound_effects_enabled_changed)
	Q_PROPERTY(int sound_effects_volume READ get_sound_effects_volume WRITE set_sound_effects_volume NOTIFY sound_effects_volume_changed)
	Q_PROPERTY(bool music_enabled READ is_music_enabled WRITE set_music_enabled NOTIFY music_enabled_changed)
	Q_PROPERTY(int music_volume READ get_music_volume WRITE set_music_volume NOTIFY music_volume_changed)
	Q_PROPERTY(wyrmgus::hotkey_setup hotkey_setup READ get_hotkey_setup WRITE set_hotkey_setup)
	Q_PROPERTY(bool autosave MEMBER autosave READ is_autosave_enabled NOTIFY changed)
	Q_PROPERTY(bool hero_symbol MEMBER hero_symbol READ is_hero_symbol_enabled NOTIFY changed)
	Q_PROPERTY(bool pathlines MEMBER pathlines READ are_pathlines_enabled NOTIFY changed)
	Q_PROPERTY(bool player_color_circle MEMBER player_color_circle READ is_player_color_circle_enabled NOTIFY changed)
	Q_PROPERTY(bool hp_bar MEMBER hp_bar READ is_hp_bar_enabled NOTIFY changed)
	Q_PROPERTY(bool resource_bar MEMBER resource_bar READ is_resource_bar_enabled NOTIFY changed)
	Q_PROPERTY(bool show_hotkeys MEMBER show_hotkeys READ is_show_hotkeys_enabled NOTIFY changed)
	Q_PROPERTY(bool show_messages MEMBER show_messages READ is_show_messages_enabled NOTIFY changed)
	Q_PROPERTY(bool show_tips MEMBER show_tips READ is_show_tips_enabled NOTIFY changed)
	Q_PROPERTY(int key_scroll_speed MEMBER key_scroll_speed READ get_key_scroll_speed NOTIFY changed)
	Q_PROPERTY(int mouse_scroll_speed MEMBER mouse_scroll_speed READ get_mouse_scroll_speed NOTIFY changed)
	Q_PROPERTY(bool reverse_mousewheel_scrolling MEMBER reverse_mousewheel_scrolling READ is_reverse_mousewheel_scrolling_enabled NOTIFY changed)
	Q_PROPERTY(bool show_water_borders MEMBER show_water_borders READ is_show_water_borders_enabled NOTIFY changed)
	Q_PROPERTY(bool time_of_day_shading MEMBER time_of_day_shading READ is_time_of_day_shading_enabled NOTIFY changed)
	Q_PROPERTY(QString local_player_name READ get_local_player_name_qstring WRITE set_local_player_name_qstring NOTIFY changed)

public:
	static constexpr int default_game_speed = 30;
	static constexpr int min_game_speed = 1;
	static constexpr int autosave_minutes = 5;

	static std::filesystem::path get_path();
	static std::filesystem::path get_fallback_path();

	preferences();

	void load();
	void load_file();
	Q_INVOKABLE void save() const;
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void initialize();

	const centesimal_int &get_scale_factor() const
	{
		return this->scale_factor;
	}

	void set_scale_factor(const centesimal_int &factor);

	QString get_scale_factor_qstring() const
	{
		return QString::fromStdString(this->scale_factor.to_string());
	}

	void set_scale_factor_qstring(const QString &factor_str)
	{
		this->set_scale_factor(centesimal_int(factor_str.toStdString()));
	}

	bool is_fullscreen() const
	{
		return this->fullscreen;
	}

	void set_fullscreen(const bool fullscreen)
	{
		if (fullscreen == this->is_fullscreen()) {
			return;
		}

		this->fullscreen = fullscreen;

		emit fullscreen_changed();
	}

	const QSize &get_window_size() const
	{
		return this->window_size;
	}

	void set_window_size(const QSize &window_size)
	{
		if (window_size == this->get_window_size()) {
			return;
		}

		this->window_size = window_size;

		emit window_size_changed();
	}

	int get_window_width() const
	{
		return this->get_window_size().width();
	}

	void set_window_width(const int window_width)
	{
		if (window_width == this->get_window_width()) {
			return;
		}

		this->window_size.setWidth(window_width);

		emit window_size_changed();
	}

	int get_window_height() const
	{
		return this->get_window_size().height();
	}

	void set_window_height(const int window_height)
	{
		if (window_height == this->get_window_height()) {
			return;
		}

		this->window_size.setHeight(window_height);

		emit window_size_changed();
	}

	bool is_window_maximized() const
	{
		return this->window_maximized;
	}

	void set_window_maximized(const bool window_maximized)
	{
		if (window_maximized == this->is_window_maximized()) {
			return;
		}

		this->window_maximized = window_maximized;

		emit window_maximized_changed();
	}

	int get_game_speed() const
	{
		return this->game_speed;
	}

	void set_game_speed(int speed)
	{
		speed = std::max(speed, preferences::min_game_speed);

		if (speed == this->get_game_speed()) {
			return;
		}

		this->game_speed = speed;

		this->update_video_sync_speed();

		emit game_speed_changed();
	}

	void change_game_speed(const int change)
	{
		this->set_game_speed(this->get_game_speed() + change);
	}

	void reset_game_speed()
	{
		this->set_game_speed(preferences::default_game_speed);
	}

	void update_video_sync_speed();

	wyrmgus::difficulty get_difficulty() const
	{
		return this->difficulty;
	}

	Q_INVOKABLE int get_difficulty_index() const
	{
		return static_cast<int>(this->get_difficulty());
	}

	void set_difficulty(const difficulty difficulty)
	{
		if (difficulty == this->get_difficulty()) {
			return;
		}

		this->difficulty = difficulty;
	}

	Q_INVOKABLE void set_difficulty_index(const int difficulty_index)
	{
		this->set_difficulty(static_cast<wyrmgus::difficulty>(difficulty_index));
	}

	bool are_sound_effects_enabled() const
	{
		return this->sound_effects_enabled;
	}

	void set_sound_effects_enabled(const bool enabled)
	{
		if (enabled == this->are_sound_effects_enabled()) {
			return;
		}

		this->sound_effects_enabled = enabled;
		emit sound_effects_enabled_changed();
	}

	int get_sound_effects_volume() const
	{
		return this->sound_effects_volume;
	}

	void set_sound_effects_volume(int volume);

	bool is_music_enabled() const
	{
		return this->music_enabled;
	}

	void set_music_enabled(const bool enabled);

	int get_music_volume() const
	{
		return this->music_volume;
	}

	void set_music_volume(int volume);

	wyrmgus::hotkey_setup get_hotkey_setup() const
	{
		return this->hotkey_setup;
	}

	Q_INVOKABLE int get_hotkey_setup_index() const
	{
		return static_cast<int>(this->get_hotkey_setup());
	}

	void set_hotkey_setup(const hotkey_setup hotkey_setup)
	{
		if (hotkey_setup == this->get_hotkey_setup()) {
			return;
		}

		this->hotkey_setup = hotkey_setup;
	}

	Q_INVOKABLE void set_hotkey_setup_index(const int hotkey_setup_index)
	{
		this->set_hotkey_setup(static_cast<wyrmgus::hotkey_setup>(hotkey_setup_index));
	}

	bool is_autosave_enabled() const
	{
		return this->autosave;
	}

	bool is_hero_symbol_enabled() const
	{
		return this->hero_symbol;
	}

	bool are_pathlines_enabled() const
	{
		return this->pathlines;
	}

	bool is_player_color_circle_enabled() const
	{
		return this->player_color_circle;
	}

	bool is_hp_bar_enabled() const
	{
		return this->hp_bar;
	}

	bool is_resource_bar_enabled() const
	{
		return this->resource_bar;
	}

	bool is_show_hotkeys_enabled() const
	{
		return this->show_hotkeys;
	}

	bool is_show_messages_enabled() const
	{
		return this->show_messages;
	}

	bool is_show_tips_enabled() const
	{
		return this->show_tips;
	}

	int get_key_scroll_speed() const
	{
		return this->key_scroll_speed;
	}

	int get_mouse_scroll_speed() const
	{
		return this->mouse_scroll_speed;
	}

	bool is_reverse_mousewheel_scrolling_enabled() const
	{
		return this->reverse_mousewheel_scrolling;
	}

	bool is_show_water_borders_enabled() const
	{
		return this->show_water_borders;
	}

	bool is_time_of_day_shading_enabled() const
	{
		return this->time_of_day_shading;
	}

	const std::string &get_local_player_name() const
	{
		return this->local_player_name;
	}

	QString get_local_player_name_qstring() const
	{
		return QString::fromStdString(this->local_player_name);
	}

	Q_INVOKABLE void set_local_player_name(const std::string &name)
	{
		this->local_player_name = name;
	}

	void set_local_player_name_qstring(const QString &name)
	{
		this->set_local_player_name(name.toStdString());
	}

	void set_local_player_name_from_env();

signals:
	void scale_factor_changed();
	void fullscreen_changed();
	void window_size_changed();
	void window_maximized_changed();
	void game_speed_changed();
	void sound_effects_enabled_changed();
	void sound_effects_volume_changed();
	void music_enabled_changed();
	void music_volume_changed();
	void changed();

private:
	centesimal_int scale_factor = centesimal_int(1);
	bool fullscreen = true;
	QSize window_size = QSize(1066, 600);
	bool window_maximized = false;
	int game_speed = preferences::default_game_speed;
	wyrmgus::difficulty difficulty;
	bool sound_effects_enabled = true;
	int sound_effects_volume = 128;
	bool music_enabled = true;
	int music_volume = 128;
	wyrmgus::hotkey_setup hotkey_setup;
	bool autosave = true;
	bool hero_symbol = false;
	bool pathlines = false;
	bool player_color_circle = false;
	bool hp_bar = false;
	bool resource_bar = false;
	bool show_hotkeys = true;
	bool show_messages = true;
	bool show_tips = true;
	int key_scroll_speed = 1;
	int mouse_scroll_speed = 1; //mouse scroll speed (screenpixels per mousepixel)
	bool reverse_mousewheel_scrolling = false;
	bool show_water_borders = false;
	bool time_of_day_shading = true;
	std::string local_player_name;
};

}

//called by tolua++
extern bool is_show_tips_enabled();
