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
//      (c) Copyright 2013-2022 by Joris Dauphin and Andrettin
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

class multiplayer_setup final
{
public:
	static multiplayer_setup &get_local_setup()
	{
		static multiplayer_setup local_setup;
		return local_setup;
	}

	static size_t size()
	{
		return 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 * PlayerMax + 1 * PlayerMax + 1 * PlayerMax;
	}

	multiplayer_setup()
	{
		memset(this->Race, -1, sizeof(Race));
	}

	size_t serialize(unsigned char *p) const;
	size_t deserialize(const unsigned char *p);

	bool operator ==(const multiplayer_setup &rhs) const
	{
		return (ResourcesOption == rhs.ResourcesOption
			&& UnitsOption == rhs.UnitsOption
			&& FogOfWar == rhs.FogOfWar
			&& RevealMap == rhs.RevealMap
			&& TilesetSelection == rhs.TilesetSelection
			&& GameTypeOption == rhs.GameTypeOption
			&& Difficulty == rhs.Difficulty
			&& MapRichness == rhs.MapRichness
			&& Opponents == rhs.Opponents
			&& memcmp(CompOpt, rhs.CompOpt, sizeof(CompOpt)) == 0
			&& memcmp(Ready, rhs.Ready, sizeof(Ready)) == 0
			&& memcmp(Race, rhs.Race, sizeof(Race)) == 0);
	}

	bool operator !=(const multiplayer_setup &rhs) const
	{
		return !(*this == rhs);
	}

public:
	uint8_t ResourcesOption = 0;       /// Resources option
	uint8_t UnitsOption = 0;           /// Unit # option
	uint8_t FogOfWar = 0;              /// Fog of war option
	uint8_t RevealMap = 0;             /// Reveal all the map
	uint8_t TilesetSelection = 0;      /// Tileset select option
	uint8_t GameTypeOption = 0;        /// Game type option
	uint8_t Difficulty = 0;            /// Difficulty option
	uint8_t MapRichness = 0;           /// Map richness option
	uint8_t Opponents = 0;             /// Number of AI opponents
	uint8_t CompOpt[PlayerMax]{};    /// Free slot option selection  {"Available", "Computer", "Closed" }
	uint8_t Ready[PlayerMax]{};      /// Client ready state
	uint8_t Race[PlayerMax]{};       /// Client race selection
};

}
