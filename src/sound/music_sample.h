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

#include <SDL_mixer.h>

namespace wyrmgus {

class music_sample final
{
public:
	explicit music_sample(const std::filesystem::path &filepath) : filepath(filepath)
	{
		if (!std::filesystem::exists(filepath)) {
			throw std::runtime_error("Music file \"" + filepath.string() + "\" does not exist.");
		}
	}

	~music_sample()
	{
		if (this->is_loaded()) {
			this->unload();
		}
	}

	bool is_loaded() const
	{
		return this->data != nullptr;
	}

	void load()
	{
		this->data = Mix_LoadMUS(this->filepath.string().c_str());
		if (this->data == nullptr) {
			throw std::runtime_error("Failed to decode music file \"" + this->filepath.string() + "\": " + std::string(Mix_GetError()));
		}
	}

	void unload()
	{
		if (!this->is_loaded()) {
			return;
		}

		Mix_FreeMusic(this->data);
		this->data = nullptr;
	}

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	Mix_Music *get_data() const
	{
		return this->data;
	}

private:
	std::filesystem::path filepath;
	Mix_Music *data = nullptr;
};

}
