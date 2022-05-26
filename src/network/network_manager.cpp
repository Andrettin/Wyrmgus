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

#include "stratagus.h"

#include "network/network_manager.h"

#include "database/database.h"
#include "database/preferences.h"
#include "map/map.h"
#include "map/map_info.h"
#include "network/netconnect.h"
#include "network/client.h"
#include "network/netsockets.h"
#include "network/network.h"
#include "network/server.h"
#include "util/event_loop.h"
#include "util/path_util.h"
#include "video/video.h"

namespace wyrmgus {

client *network_manager::get_client() const
{
	return client::get();
}

server *network_manager::get_server() const
{
	return server::get();
}

void network_manager::reset()
{
	for (int i = 0; i < PlayerMax; ++i) {
		Hosts[i].Clear();
	}

	this->connected_player_count = 0;
	this->ready_player_count = 0;
}

bool network_manager::setup_server_address(const std::string &server_address, int port)
{
	if (port == 0) {
		port = CNetworkParameter::Instance.default_port;
	}

	auto host = std::make_unique<CHost>(server_address.c_str(), port);
	if (host->isValid() == false) {
		//return false if an error occurred
		return false;
	}

	this->get_client()->SetServerHost(std::move(host));

	return true;
}

/**
** Setup Network connect state machine for clients
*/
void network_manager::init_client_connect()
{
	NetConnectRunning = 2;
	NetConnectType = 2;

	this->reset();

	this->get_client()->Init(preferences::get()->get_local_player_name(), &NetworkFildes, GetTicks());
}

void network_manager::process_client_request()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		if (co_await this->get_client()->Update(GetTicks()) == false) {
			NetConnectRunning = 0;
		}
	});
}

void network_manager::init_server_connect(const QString &map_filepath_qstr, const int open_slots)
{
	NetConnectRunning = 1;
	NetConnectType = 1;

	this->reset();

	const std::filesystem::path map_filepath = path::from_qstring(map_filepath_qstr);
	const std::filesystem::path relative_map_filepath = std::filesystem::relative(map_filepath, database::get()->get_root_path());

	NetworkMapName = path::to_string(relative_map_filepath);

	LoadStratagusMapInfo(map_filepath);

	emit map_info_changed();

	this->get_server()->init(preferences::get()->get_local_player_name(), &NetworkFildes, open_slots);

	std::unique_lock<std::shared_mutex> lock(this->get_mutex());

	// preset the server (initially always slot 0)
	Hosts[0].SetName(preferences::get()->get_local_player_name().c_str());

	emit network_manager::get()->player_name_changed(0, Hosts[0].PlyName);
}

int network_manager::get_network_state() const
{
	return this->get_client()->GetNetworkState();
}

map_info *network_manager::get_map_info() const
{
	return CMap::get()->get_info();
}

QString network_manager::get_player_name(const int player_index) const
{
	std::shared_lock<std::shared_mutex> lock(this->mutex);

	return Hosts[player_index].PlyName;
}

void network_manager::check_players(const multiplayer_setup *setup)
{
	int connected_player_count = 0;
	int ready_player_count = 0;

	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (Hosts[i].PlyName[0] == 0) {
			continue;
		}

		++connected_player_count;

		if (setup->Ready[i] == 1) {
			++ready_player_count;
		}
	}

	this->set_connected_player_count(connected_player_count);
	this->ready_player_count = ready_player_count;
}

}
