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

#include "stratagus.h"

#include "sound/music_player.h"

#include "database/preferences.h"
#include "game/game.h"
#include "player/player.h"
#include "script/condition/and_condition.h"
#include "sound/music.h"
#include "sound/music_sample.h"
#include "sound/music_type.h"
#include "sound/sound_server.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/vector_random_util.h"

#include <SDL_mixer.h>

namespace wyrmgus {

music_player::music_player() : current_music_type(music_type::none)
{
}

bool music_player::is_playing() const
{
	return Mix_PlayingMusic() == 1;
}

void music_player::play_music_type(const music_type type)
{
	if (type == this->current_music_type) {
		return;
	}

	if (type == music_type::menu && game::get()->is_running()) {
		return;
	}

	this->current_music_type = type;
	this->stop();
	this->current_music = nullptr;

	//clear all loaded data when the music type changes, to prevent music from causing too much memory consumption
	music::unload_all();

	if (SoundEnabled() && preferences::get()->is_music_enabled()) {
		this->play();
	}
}

void music_player::play()
{
	if (this->current_music != nullptr) {
		if (this->current_submusic != nullptr) {
			this->played_submusic.insert(this->current_submusic);
		}

		const music *submusic = this->get_next_submusic();
		if (submusic != nullptr) {
			this->play_submusic(submusic);
			return;
		}
	}

	this->play_music(this->get_next_music());
}

void music_player::play_music(const music *music)
{
	this->current_music = music;
	if (music == nullptr) {
		return;
	}

	this->current_submusic = nullptr;
	this->played_submusic.clear();

	//preload the music with all its submusic, so that transition between them is seamless
	try {
		this->current_music->load();
	} catch (const std::exception &exception) {
		exception::report(exception);
		return;
	}

	if (music->get_sample() != nullptr) {
		this->current_volume_modifier = music->get_volume_percent();
		this->play_sample(music->get_sample());
	} else {
		const wyrmgus::music *submusic = this->get_next_submusic();
		if (submusic == nullptr) {
			throw std::runtime_error("Music \"" + music->get_identifier() + "\" has neither a sample nor submusic.");
		}
		this->play_submusic(submusic);
	}
}

void music_player::play_submusic(const music *submusic)
{
	this->current_submusic = submusic;
	this->current_volume_modifier = submusic->get_volume_percent();
	this->play_sample(submusic->get_sample());
}

void music_player::play_sample(music_sample *sample)
{
	try {
		if (!sample->is_loaded()) {
			sample->load();
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
		return;
	}

	const int volume = Mix_VolumeMusic(preferences::get()->get_music_volume() * this->current_volume_modifier / 100 * MIX_MAX_VOLUME / MaxVolume);
	if (volume == 0 && preferences::get()->get_music_volume() != 0) {
		log::log_error("Failed to set volume for playing music.");
	}

	const int result = Mix_PlayMusic(sample->get_data(), 0);
	if (result == -1) {
		log::log_error("Failed to play music file \"" + sample->get_filepath().string() + "\".");
	}
}

const music *music_player::get_next_music() const
{
	if (this->current_music_type == music_type::none) {
		return nullptr;
	}

	const std::vector<const music *> &potential_music_list = music::get_all_of_type(this->current_music_type);

	std::vector<const music *> music_list;

	for (const music *music : potential_music_list) {
		if (!this->are_music_conditions_fulfilled(music)) {
			continue;
		}

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

const music *music_player::get_next_submusic() const
{
	std::vector<const music *> submusic_list;

	for (const music *intro_music : this->current_music->get_intro_music()) {
		if (this->played_submusic.contains(intro_music)) {
			continue;
		}

		if (!this->are_music_conditions_fulfilled(intro_music)) {
			continue;
		}

		submusic_list.push_back(intro_music);
	}

	if (submusic_list.empty()) {
		//no intro available, or all intro submusic played, now go forward to the main music
		for (const music *submusic : this->current_music->get_submusic()) {
			if (this->played_submusic.contains(submusic)) {
				continue;
			}

			if (!this->are_music_conditions_fulfilled(submusic)) {
				continue;
			}

			submusic_list.push_back(submusic);
		}
	}

	if (!submusic_list.empty()) {
		return vector::get_random_async(submusic_list);
	} else {
		return nullptr;
	}
}

void music_player::stop()
{
	Mix_FadeOutMusic(200);
}

bool music_player::are_music_conditions_fulfilled(const music *music) const
{
	if (music->get_conditions() == nullptr) {
		return true;
	}

	const CPlayer *this_player = CPlayer::GetThisPlayer();

	if (this_player == nullptr) {
		return false;
	}

	if (!music->get_conditions()->check(CPlayer::GetThisPlayer(), read_only_context::from_scope(this_player))) {
		return false;
	}

	return true;
}

bool music_player::are_current_music_conditions_fulfilled() const
{
	if (this->current_music != nullptr && !this->are_music_conditions_fulfilled(this->current_music)) {
		return false;
	}

	if (this->current_submusic != nullptr && !this->are_music_conditions_fulfilled(this->current_submusic)) {
		return false;
	}

	return true;
}

void music_player::check_current_music()
{
	//check whether the current music or submusic still fulfill their conditions, and if not stop the currently-playing sample, so that a new one will be picked
	if (!this->are_current_music_conditions_fulfilled()) {
		this->stop();
	}
}

}
