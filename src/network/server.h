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
//      (c) Copyright 2001-2022 by Lutz Sammer, Andreas Arens, Jimmy Salmon and
//                                 Andrettin
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

#include "network/network_state.h"
#include "util/singleton.h"

class CHost;
class CInitMessage_Hello;
class CInitMessage_State;
class CNetworkHost;
class CUDPSocket;
namespace wyrmgus {

class multiplayer_setup;

class server final : public QObject, public singleton<server>
{
	Q_OBJECT

public:
	server();
	~server();

	void init(const std::string &name, CUDPSocket *socket, const int open_slots);

	void Update(unsigned long frameCounter);
	void Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host);

	void resync_clients();
	void MarkClientsAsResync();
	void KickClient(int c);

	multiplayer_setup &get_setup() const
	{
		return *this->setup;
	}

	Q_INVOKABLE void set_fog_of_war(const bool fow);

private:
	int Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host);
	void Parse_Resync(const int h);
	void Parse_Waiting(const int h);
	void Parse_Map(const int h);
	void Parse_State(const int h, const CInitMessage_State &msg);
	void Parse_GoodBye(const int h);
	void Parse_SeeYou(const int h);

	void Send_AreYouThere(const CNetworkHost &host);
	void Send_GameFull(const CHost &host);
	void Send_Welcome(const CNetworkHost &host, int hostIndex);
	void Send_Resync(const CNetworkHost &host, int hostIndex);
	void Send_Map(const CNetworkHost &host);
	void Send_State(const CNetworkHost &host);
	void Send_GoodBye(const CNetworkHost &host);

private:
	std::string name;
	NetworkState networkStates[PlayerMax]; /// Client Host states
	CUDPSocket *socket = nullptr;
	std::unique_ptr<multiplayer_setup> setup;
};

}
