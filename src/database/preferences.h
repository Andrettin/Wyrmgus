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

class preferences final : public QObject, public singleton<preferences>
{
	Q_OBJECT

	Q_PROPERTY(int scale_factor READ get_scale_factor WRITE set_scale_factor)
	Q_PROPERTY(wyrmgus::difficulty difficulty READ get_difficulty WRITE set_difficulty)
	Q_PROPERTY(int sound_volume READ get_sound_volume WRITE set_sound_volume NOTIFY sound_volume_changed)
	Q_PROPERTY(int music_volume READ get_music_volume WRITE set_music_volume NOTIFY music_volume_changed)

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

	int get_sound_volume() const
	{
		return this->sound_volume;
	}

	void set_sound_volume(int volume);

	int get_music_volume() const
	{
		return this->music_volume;
	}

	void set_music_volume(int volume);

signals:
	void sound_volume_changed();
	void music_volume_changed();

private:
	int scale_factor = 1;
	wyrmgus::difficulty difficulty;
	int sound_volume = 128;
	int music_volume = 128;
};

}
