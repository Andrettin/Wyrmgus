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

#include "stratagus.h"

#include "network/multiplayer_setup.h"

#include "network/net_message.h"

size_t multiplayer_setup::serialize(unsigned char *buf) const
{
	unsigned char *p = buf;

	p += serialize8(p, this->ResourcesOption);
	p += serialize8(p, this->UnitsOption);
	p += serialize8(p, this->FogOfWar);
	p += serialize8(p, this->RevealMap);
	p += serialize8(p, this->TilesetSelection);
	p += serialize8(p, this->GameTypeOption);
	p += serialize8(p, this->Difficulty);
	p += serialize8(p, this->MapRichness);
	p += serialize8(p, this->Opponents);
	for (int i = 0; i < PlayerMax; ++i) {
		p += serialize8(p, this->CompOpt[i]);
	}
	for (int i = 0; i < PlayerMax; ++i) {
		p += serialize8(p, this->Ready[i]);
	}
	for (int i = 0; i < PlayerMax; ++i) {
		p += serialize8(p, this->Race[i]);
	}
	return p - buf;
}

size_t multiplayer_setup::deserialize(const unsigned char *p)
{
	const unsigned char *buf = p;
	p += deserialize8(p, &this->ResourcesOption);
	p += deserialize8(p, &this->UnitsOption);
	p += deserialize8(p, &this->FogOfWar);
	p += deserialize8(p, &this->RevealMap);
	p += deserialize8(p, &this->TilesetSelection);
	p += deserialize8(p, &this->GameTypeOption);
	p += deserialize8(p, &this->Difficulty);
	p += deserialize8(p, &this->MapRichness);
	p += deserialize8(p, &this->Opponents);
	for (int i = 0; i < PlayerMax; ++i) {
		p += deserialize8(p, &this->CompOpt[i]);
	}
	for (int i = 0; i < PlayerMax; ++i) {
		p += deserialize8(p, &this->Ready[i]);
	}
	for (int i = 0; i < PlayerMax; ++i) {
		p += deserialize8(p, &this->Race[i]);
	}
	return p - buf;
}
