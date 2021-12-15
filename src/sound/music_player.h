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

class music;
class music_sample;
enum class music_type;

class music_player final : public singleton<music_player>
{
public:
	music_player();

	bool is_playing() const;

	void play_music_type(const music_type type);
	void play();
	void play_music(const music *music);
	void play_submusic(const music *submusic);
	void play_sample(music_sample *sample);
	const music *get_next_music() const;
	const music *get_next_submusic() const;

	void stop();

	bool are_music_conditions_fulfilled(const music *music) const;
	bool are_current_music_conditions_fulfilled() const;
	void check_current_music();

	int get_current_volume_modifier() const
	{
		return this->current_volume_modifier;
	}

private:
	music_type current_music_type;
	const music *current_music = nullptr;
	const music *current_submusic = nullptr;
	std::set<const music *> played_submusic;
	int current_volume_modifier = 100;
};

}
