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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/singleton.h"

class CUDPSocket;

namespace wyrmgus {

class client;
class map_info;
class multiplayer_setup;
class server;

class network_manager final : public QObject, public singleton<network_manager>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::client* client READ get_client CONSTANT)
	Q_PROPERTY(wyrmgus::server* server READ get_server CONSTANT)
	Q_PROPERTY(wyrmgus::map_info* map_info READ get_map_info NOTIFY map_info_changed)
	Q_PROPERTY(int connected_player_count READ get_connected_player_count NOTIFY connected_player_count_changed)

public:
	network_manager();
	~network_manager();

	client *get_client() const;
	server *get_server() const;

	void reset();

	[[nodiscard]]
	QCoro::Task<bool> setup_server_address(const std::string server_address, int port);

	Q_INVOKABLE QCoro::QmlTask setup_server_address(const QString &server_address, const int port = 0)
	{
		return this->setup_server_address(server_address.toStdString(), port);
	}

	Q_INVOKABLE void init_client_connect();

	Q_INVOKABLE QCoro::QmlTask process_client_request()
	{
		return this->process_client_request_coro();
	}

	[[nodiscard]]
	QCoro::Task<void> process_client_request_coro();

	Q_INVOKABLE void init_server_connect(const QString &map_filepath_qstr, const int open_slots);

	Q_INVOKABLE QCoro::QmlTask process_server_request()
	{
		return this->process_server_request_coro();
	}

	[[nodiscard]]
	QCoro::Task<void> process_server_request_coro();

	void prepare_game_settings(const multiplayer_setup &setup);

	Q_INVOKABLE int get_network_state() const;

	map_info *get_map_info() const;

	Q_INVOKABLE QString get_player_name(const int player_index) const;

	int get_connected_player_count() const
	{
		return this->connected_player_count;
	}

	void set_connected_player_count(const int count)
	{
		if (count == this->get_connected_player_count()) {
			return;
		}

		this->connected_player_count = count;
		emit connected_player_count_changed();
	}

	int get_ready_player_count() const
	{
		return this->ready_player_count;
	}

	void check_players(const multiplayer_setup &setup);

	CUDPSocket *get_file_descriptor() const
	{
		return this->file_descriptor.get();
	}

signals:
	void server_address_setup_completed(const bool result);
	void map_info_changed();
	void player_name_changed(const int player_index, const QString &name);
	void player_ready_changed(const int player_index, const bool ready);
	void connected_player_count_changed();

private:
	int connected_player_count = 0;
	int ready_player_count = 0;
	std::unique_ptr<CUDPSocket> file_descriptor; //network file descriptor; UDP Socket for communication
};

}
