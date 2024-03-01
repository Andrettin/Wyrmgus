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

#include <QHostAddress>

/**
 * Number of bytes in the name of a network player,
 * including the terminating null character.
 */
constexpr int NetPlayerNameSize = 16;

namespace wyrmgus {

class multiplayer_host final
{
public:
	multiplayer_host()
	{
		Clear();
	}

	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	void Clear();

	static size_t Size()
	{
		return 4 + 2 + 2 + NetPlayerNameSize;
	}

	void SetName(const char *name);

	QHostAddress Host;
	uint16_t Port = 0;         /// Port on host
	uint16_t PlyNr = 0;        /// Player number
	char PlyName[NetPlayerNameSize]{};  /// Name of player
};

}
