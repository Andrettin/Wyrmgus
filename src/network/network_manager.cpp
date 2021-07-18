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
//      (c) Copyright 2001-2021 by Lutz Sammer, Andreas Arens, Jimmy Salmon and
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

#include "stratagus.h"

#include "network/network_manager.h"

#include "database/preferences.h"
#include "netconnect.h"
#include "network/client.h"
#include "network/netsockets.h"
#include "network.h"
#include "video/video.h"

namespace wyrmgus {

bool network_manager::setup_server_address(const std::string &server_address, int port)
{
	if (port == 0) {
		port = CNetworkParameter::Instance.defaultPort;
	}

	auto host = std::make_unique<CHost>(server_address.c_str(), port);
	if (host->isValid() == false) {
		//return false if an error occurred
		return false;
	}

	Client.SetServerHost(std::move(host));

	return true;
}

/**
** Setup Network connect state machine for clients
*/
void network_manager::init_client_connect()
{
	NetConnectRunning = 2;
	NetConnectType = 2;

	for (int i = 0; i < PlayerMax; ++i) {
		Hosts[i].Clear();
	}

	ServerSetupState.Clear();
	LocalSetupState.Clear();

	Client.Init(preferences::get()->get_local_player_name(), &NetworkFildes, &ServerSetupState, &LocalSetupState, GetTicks());
}

}
