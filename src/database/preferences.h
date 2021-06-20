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
	Q_PROPERTY(wyrmgus::difficulty difficulty READ get_difficulty WRITE set_difficulty)
	Q_PROPERTY(bool sound_effects_enabled READ are_sound_effects_enabled WRITE set_sound_effects_enabled NOTIFY sound_effects_enabled_changed)
	Q_PROPERTY(int sound_effects_volume READ get_sound_effects_volume WRITE set_sound_effects_volume NOTIFY sound_effects_volume_changed)
	Q_PROPERTY(bool music_enabled READ is_music_enabled WRITE set_music_enabled NOTIFY music_enabled_changed)
	Q_PROPERTY(int music_volume READ get_music_volume WRITE set_music_volume NOTIFY music_volume_changed)
	Q_PROPERTY(wyrmgus::hotkey_setup hotkey_setup READ get_hotkey_setup WRITE set_hotkey_setup)
	Q_PROPERTY(bool show_tips MEMBER show_tips NOTIFY changed)

public:
	static std::filesystem::path get_path();
	static std::filesystem::path get_fallback_path();

	preferences();

	void load();
	Q_INVOKABLE void save() const;
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

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

	bool is_show_tips_enabled() const
	{
		return this->show_tips;
	}

signals:
	void scale_factor_changed();
	void sound_effects_enabled_changed();
	void sound_effects_volume_changed();
	void music_enabled_changed();
	void music_volume_changed();
	void changed();

private:
	int scale_factor = 1;
	wyrmgus::difficulty difficulty;
	bool sound_effects_enabled = true;
	int sound_effects_volume = 128;
	bool music_enabled = true;
	int music_volume = 128;
	wyrmgus::hotkey_setup hotkey_setup;
	bool show_tips = true;
};

}

//called by tolua++
extern bool is_show_tips_enabled();
