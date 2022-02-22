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
//      (c) Copyright 2022 by Andrettin
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

namespace wyrmgus {

struct pmp_tile final
{
	uint16_t texture_1 = 0;
	uint16_t texture_2 = 0;
	uint32_t priority = 0;
};

struct pmp_patch final
{
	static constexpr size_t tile_size = 16;

	explicit pmp_patch(std::ifstream &ifstream)
	{
		ifstream.read(reinterpret_cast<char *>(this->tiles.data()), this->tiles.size() * sizeof(pmp_tile));
	}

	std::array<pmp_tile, tile_size *tile_size> tiles{};
};

struct pmp_string final
{
	explicit pmp_string(std::ifstream &ifstream)
	{
		ifstream.read(reinterpret_cast<char *>(&this->length), sizeof(this->length));
		this->data.resize(this->length, 0);
		ifstream.read(this->data.data(), this->data.size());
	}

	std::string_view to_string_view() const
	{
		return std::string_view(this->data.data(), this->data.size());
	}

	uint32_t length = 0;
	std::vector<char> data;
};

struct pmp final
{
	explicit pmp(std::ifstream &ifstream)
	{
		//process header
		ifstream.read(this->magic.data(), this->magic.size());
		ifstream.read(reinterpret_cast<char *>(&this->version), sizeof(this->version));
		ifstream.read(reinterpret_cast<char *>(&this->data_size), sizeof(this->data_size));

		//process data
		ifstream.read(reinterpret_cast<char *>(&this->map_size), sizeof(this->map_size));

		const size_t height_map_size = (this->map_size * pmp_patch::tile_size + 1) * (this->map_size * pmp_patch::tile_size + 1);
		this->height_map.resize(height_map_size, 0);

		for (size_t i = 0; i < this->height_map.size(); ++i) {
			const size_t ifstream_pos = ifstream.tellg();
			try {
				uint16_t &height_map_value = this->height_map[i];
				ifstream.read(reinterpret_cast<char *>(&height_map_value), sizeof(uint16_t));
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to read height map value at index " + std::to_string(i) + ", streamer position " + std::to_string(ifstream_pos) + "."));
			}
		}

		ifstream.read(reinterpret_cast<char *>(&this->num_terrain_textures), sizeof(this->num_terrain_textures));

		for (uint32_t i = 0; i < this->num_terrain_textures; ++i) {
			this->terrain_textures.push_back(pmp_string(ifstream));
		}

		const uint32_t num_patches = this->map_size * this->map_size;
		for (uint32_t i = 0; i < num_patches; ++i) {
			this->patches.push_back(pmp_patch(ifstream));
		}
	}

	//header
	std::array<char, 4> magic{}; //always "PSMP"
	uint32_t version = 0;
	uint32_t data_size = 0;

	//data
	uint32_t map_size = 0;
	std::vector<uint16_t> height_map; //vector size should equal (map_size*16 + 1)*(map_size*16 + 1)
	uint32_t num_terrain_textures = 0;
	std::vector<pmp_string> terrain_textures; //terrain texture filenames; size should equal num_terrain_textures
	std::vector<pmp_patch> patches; //size should equal map_size*map_size
};

}
