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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "util/singleton.h"

namespace wyrmgus {

class campaign;
class sml_data;
class sml_property;
enum class difficulty;
enum class hotkey_setup;

class preferences final : public QObject, public singleton<preferences>
{
	Q_OBJECT

	Q_PROPERTY(int scale_factor READ get_scale_factor WRITE set_scale_factor NOTIFY scale_factor_changed)
	Q_PROPERTY(int game_speed READ get_game_speed WRITE set_game_speed NOTIFY game_speed_changed)
	Q_PROPERTY(wyrmgus::difficulty difficulty READ get_difficulty WRITE set_difficulty)
	Q_PROPERTY(bool sound_effects_enabled READ are_sound_effects_enabled WRITE set_sound_effects_enabled NOTIFY sound_effects_enabled_changed)
	Q_PROPERTY(int sound_effects_volume READ get_sound_effects_volume WRITE set_sound_effects_volume NOTIFY sound_effects_volume_changed)
	Q_PROPERTY(bool music_enabled READ is_music_enabled WRITE set_music_enabled NOTIFY music_enabled_changed)
	Q_PROPERTY(int music_volume READ get_music_volume WRITE set_music_volume NOTIFY music_volume_changed)
	Q_PROPERTY(wyrmgus::hotkey_setup hotkey_setup READ get_hotkey_setup WRITE set_hotkey_setup)
	Q_PROPERTY(bool autosave READ is_autosave_enabled  MEMBER autosave NOTIFY changed)
	Q_PROPERTY(bool hero_symbol READ is_hero_symbol_enabled  MEMBER hero_symbol NOTIFY changed)
	Q_PROPERTY(bool pathlines READ are_pathlines_enabled  MEMBER pathlines NOTIFY changed)
	Q_PROPERTY(bool player_color_circle READ is_player_color_circle_enabled MEMBER player_color_circle NOTIFY changed)
	Q_PROPERTY(bool resource_bar READ is_resource_bar_enabled MEMBER resource_bar NOTIFY changed)
	Q_PROPERTY(bool show_messages READ is_show_messages_enabled MEMBER show_messages NOTIFY changed)
	Q_PROPERTY(bool show_tips READ is_show_tips_enabled  MEMBER show_tips NOTIFY changed)
	Q_PROPERTY(QString local_player_name READ get_local_player_name_qstring WRITE set_local_player_name_qstring NOTIFY changed)

public:
	static constexpr int default_game_speed = 30;
	static constexpr int min_game_speed = 1;
	static constexpr int autosave_minutes = 5;

	static std::filesystem::path get_path();
	static std::filesystem::path get_fallback_path();

	preferences();

	void load();
	Q_INVOKABLE void save() const;
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void initialize();

	int get_scale_factor() const
	{
		return this->scale_factor;
	}

	void set_scale_factor(const int factor)
	{
		if (factor == this->get_scale_factor()) {
			return;
		}

		this->scale_factor = factor;
		emit scale_factor_changed();
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

	bool is_resource_bar_enabled() const
	{
		return this->resource_bar;
	}

	bool is_show_messages_enabled() const
	{
		return this->show_messages;
	}

	bool is_show_tips_enabled() const
	{
		return this->show_tips;
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
	void game_speed_changed();
	void sound_effects_enabled_changed();
	void sound_effects_volume_changed();
	void music_enabled_changed();
	void music_volume_changed();
	void changed();

private:
	int scale_factor = 1;
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
	bool resource_bar = false;
	bool show_messages = true;
	bool show_tips = true;
	std::string local_player_name;
};

}

//called by tolua++
extern bool is_show_tips_enabled();
