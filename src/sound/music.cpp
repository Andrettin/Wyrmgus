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
//      (c) Copyright 2002-2021 by Lutz Sammer, Nehal Mistry, Jimmy Salmon
//                                 and Andrettin
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

#include "music.h"

#include "iolib.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "sound/music_player.h"
#include "sound/music_sample.h"
#include "sound/music_type.h"
#include "sound/sound_server.h"

#include <SDL_mixer.h>

bool CallbackMusic;                       /// flag true callback ccl if stops

namespace wyrmgus {

music::music(const std::string &identifier) : data_entry(identifier)
{
}

music::~music()
{
}

void music::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "file") {
		this->file = database::get()->get_music_path(this->get_module()) / value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void music::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "intro_music") {
		for (const std::string &value : values) {
			this->intro_music.push_back(music::get(value));
		}
	} else if (tag == "submusic") {
		for (const std::string &value : values) {
			this->submusic.push_back(music::get(value));
		}
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void music::initialize()
{
	if (this->type != music_type::none) {
		music::music_by_type[this->type].push_back(this);
	}

	if (!this->file.empty()) {
		const std::string filename = LibraryFileName(this->file.string().c_str());
		this->sample = std::make_unique<wyrmgus::music_sample>(filename);
	}

	data_entry::initialize();
}

void music::check() const
{
	if (this->file.empty() && this->get_intro_music().empty() && this->get_submusic().empty()) {
		throw std::runtime_error("Music \"" + this->get_identifier() + "\" has neither a file nor submusic.");
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

void music::load() const
{
	if (this->sample != nullptr && !this->sample->is_loaded()) {
		this->sample->load();
	}

	for (const music *intro_music : this->intro_music) {
		intro_music->load();
	}

	for (const music *submusic : this->submusic) {
		submusic->load();
	}
}

void music::unload()
{
	if (this->sample != nullptr && this->sample->is_loaded()) {
		this->sample->unload();
	}
}

}

/**
**  Callback for when music has finished
**  Note: we are in the sdl audio thread
*/
static void MusicFinishedCallback()
{
}

/**
**  Check if music is finished and play the next song
*/
void CheckMusicFinished()
{
	if (wyrmgus::music_player::get()->is_playing() && Mix_FadingMusic() != MIX_FADING_OUT) {
		wyrmgus::music_player::get()->check_current_music();
	}

	const bool finished = !wyrmgus::music_player::get()->is_playing();

	if (finished && SoundEnabled() && IsMusicEnabled() && CallbackMusic) {
		wyrmgus::music_player::get()->play();
	}
}

/**
**  Init music
*/
void InitMusic()
{
	SetMusicFinishedCallback(MusicFinishedCallback);
}
