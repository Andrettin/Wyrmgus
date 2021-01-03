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
//      (c) Copyright 2020 by Andrettin
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

#include "stratagus.h"

#include "music_player.h"

#include "sound/music.h"
#include "sound/music_sample.h"
#include "sound/music_type.h"
#include "sound/sound_server.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

music_player::music_player() : current_music_type(music_type::none)
{
}

void music_player::play(const music_type type)
{
	this->current_music_type = type;

	if (SoundEnabled() && IsMusicEnabled()) {
		this->play();
	}
}

void music_player::play()
{
	this->play_music(this->get_next_music());
}

void music_player::play_music(const music *music)
{
	this->current_music = music;
	if (music == nullptr) {
		return;
	}

	this->play_sample(music->get_sample());
}

void music_player::play_sample(music_sample *sample)
{
	if (!sample->is_loaded()) {
		sample->load();
	}

	Mix_PlayMusic(sample->get_data(), 0);
}

const music *music_player::get_next_music() const
{
	if (this->current_music_type == music_type::none) {
		return nullptr;
	}

	const std::vector<const music *> &potential_music_list = music::get_all_of_type(this->current_music_type);

	std::vector<const music *> music_list;

	for (const music *music : potential_music_list) {
		if (music == this->current_music && !music_list.empty()) {
			//don't play the same music twice if that can be avoided
			continue;
		}
		if (music_list.size() == 1 && music_list.front() == this->current_music) {
			music_list.clear();
		}

		music_list.push_back(music);
	}

	if (!music_list.empty()) {
		return vector::get_random_async(music_list);
	} else {
		return nullptr;
	}
}

}
