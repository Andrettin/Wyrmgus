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

#include "network/multiplayer_host.h"

#include "network/net_message.h"

namespace wyrmgus {

size_t multiplayer_host::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;

	p += serialize32(p, this->Host.toIPv4Address());
	p += serialize16(p, this->Port);
	p += serialize16(p, this->PlyNr);
	p += serialize(p, this->PlyName);

	return p - buf;
}

size_t multiplayer_host::Deserialize(const unsigned char *p)
{
	const unsigned char *buf = p;

	uint32_t ip_address = 0;
	p += deserialize32(p, &ip_address);
	Host = QHostAddress(ip_address);

	p += deserialize16(p, &Port);
	p += deserialize16(p, &PlyNr);
	p += deserialize(p, this->PlyName);
	return p - buf;
}

void multiplayer_host::Clear()
{
	this->Host.clear();
	this->Port = 0;
	this->PlyNr = 0;
	memset(this->PlyName, 0, sizeof(this->PlyName));
}

void multiplayer_host::SetName(const char *name)
{
	strncpy_s(this->PlyName, sizeof(this->PlyName), name, _TRUNCATE);
}

}
