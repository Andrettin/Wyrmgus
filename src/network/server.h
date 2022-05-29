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
class CUDPSocket;

namespace wyrmgus {

class multiplayer_host;
class multiplayer_setup;

class server final : public QObject, public singleton<server>
{
	Q_OBJECT

	Q_PROPERTY(bool ready_to_start READ is_ready_to_start NOTIFY ready_to_start_changed)

public:
	server();
	~server();

	void init(const std::string &name, CUDPSocket *socket, const int open_slots);

	[[nodiscard]]
	boost::asio::awaitable<void> Update(unsigned long frameCounter);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host);

	void resync_clients();

private:
	void mark_clients_as_resync();

public:
	void KickClient(int c);

	multiplayer_setup &get_setup() const
	{
		return *this->setup;
	}

	Q_INVOKABLE void set_fog_of_war(const bool fow);
	Q_INVOKABLE void set_reveal_map(const bool reveal_map);
	Q_INVOKABLE void set_computer_opponents(const bool value);
	Q_INVOKABLE void set_player_civilization(const int player_index, const int civilization_index);
	Q_INVOKABLE void set_resources_option(const int value);
	Q_INVOKABLE void set_difficulty(const int difficulty);

	Q_INVOKABLE void start_game();

	[[nodiscard]]
	boost::asio::awaitable<void> init_game();

	bool is_ready_to_start() const
	{
		return this->ready_to_start;
	}
	
	void set_ready_to_start(const bool ready)
	{
		if (ready == this->is_ready_to_start()) {
			return;
		}

		this->ready_to_start = ready;
		emit ready_to_start_changed();
	}

	void check_ready_to_start();

	Q_INVOKABLE bool is_player_ready(const int player_index) const;

private:
	[[nodiscard]]
	boost::asio::awaitable<int> Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_Resync(const int h);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_Waiting(const int h);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_Map(const int h);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_State(const int h, const CInitMessage_State &msg);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_GoodBye(const int h);

	void Parse_SeeYou(const int h);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_AreYouThere(const multiplayer_host &host);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_GameFull(const CHost &host);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Welcome(const multiplayer_host &host, int hostIndex);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Resync(const multiplayer_host &host, int hostIndex);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Map(const multiplayer_host &host);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_State(const multiplayer_host &host);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_GoodBye(const multiplayer_host &host);

signals:
	void ready_to_start_changed();

private:
	std::string name;
	NetworkState networkStates[PlayerMax]; /// Client Host states
	CUDPSocket *socket = nullptr;
	std::unique_ptr<multiplayer_setup> setup;
	bool ready_to_start = false;
};

}
