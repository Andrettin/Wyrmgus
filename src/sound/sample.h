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
//      (c) Copyright 1998-2021 by Lutz Sammer, Fabrice Rossi,
//                                 Jimmy Salmon and Andrettin
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

/**
**  RAW samples.
*/
class sample final
{
public:
	explicit sample(const std::filesystem::path &filepath) : filepath(filepath)
	{
		if (!std::filesystem::exists(filepath)) {
			throw std::runtime_error("Sound file \"" + filepath.string() + "\" does not exist.");
		}
	}

	~sample()
	{
		this->unload();
	}

	bool is_loaded() const
	{
		return this->chunk != nullptr;
	}

	void load()
	{
		this->chunk = Mix_LoadWAV(this->filepath.string().c_str());
		if (this->chunk == nullptr) {
			throw std::runtime_error("Failed to decode audio file \"" + this->filepath.string() + "\": " + std::string(Mix_GetError()));
		}
	}

	void unload()
	{
		if (!this->is_loaded()) {
			return;
		}

		Mix_FreeChunk(this->chunk);
		this->chunk = nullptr;
	}

	virtual int Read(void *buf, int len)
	{
		Q_UNUSED(buf)
		Q_UNUSED(len)

		return 0;
	}

	const uint8_t *get_buffer() const
	{
		return this->chunk->abuf;
	}

	int get_length() const
	{
		return static_cast<int>(this->chunk->alen);
	}

	Mix_Chunk *get_chunk() const
	{
		return this->chunk;
	}

private:
	std::filesystem::path filepath;
	Mix_Chunk *chunk = nullptr; //sample buffer
};

}
