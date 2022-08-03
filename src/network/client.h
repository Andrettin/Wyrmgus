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
//      (c) Copyright 1998-2022 by Lutz Sammer, Andreas Arens, Jimmy Salmon and
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
class CInitMessage_Config;
class CUDPSocket;

namespace wyrmgus {

class multiplayer_setup;

class client final : public QObject, public singleton<client>
{
	Q_OBJECT

	Q_PROPERTY(bool fog_of_war READ has_fog_of_war NOTIFY fog_of_war_changed)
	Q_PROPERTY(bool reveal_map READ is_reveal_map_enabled NOTIFY reveal_map_changed)
	Q_PROPERTY(bool computer_opponents READ has_computer_opponents NOTIFY computer_opponents_changed)
	Q_PROPERTY(int resources_option READ get_resources_option NOTIFY resources_option_changed)
	Q_PROPERTY(int difficulty READ get_difficulty NOTIFY difficulty_changed)

public:
	client();
	~client();

	void Init(const std::string &name, CUDPSocket *socket, unsigned long tick);

	void SetServerHost(std::unique_ptr<CHost> &&host);

	[[nodiscard]]
	boost::asio::awaitable<bool> Parse(const std::array<unsigned char, 1024> &buf);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update(unsigned long tick);

	void DetachFromServer();

	int GetNetworkState() const
	{
		return networkState.State;
	}

	multiplayer_setup &get_server_setup() const
	{
		return *this->server_setup;
	}

	multiplayer_setup &get_local_setup() const
	{
		return *this->local_setup;
	}

	Q_INVOKABLE bool is_player_ready(const int player_index) const;

	bool has_fog_of_war() const;
	bool is_reveal_map_enabled() const;
	bool has_computer_opponents() const;

	Q_INVOKABLE void set_civilization(const int civilization_index);

	int get_resources_option() const;
	int get_difficulty() const;

	Q_INVOKABLE void set_ready(const bool ready);

	Q_INVOKABLE void start_game();

private:
	[[nodiscard]]
	boost::asio::awaitable<bool> Update_disconnected();

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_detaching(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_connecting(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_connected(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_synced(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_changed(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_async(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_mapinfo(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_badmap(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_goahead(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<bool> Update_started(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Go(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Config(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_MapUidMismatch(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Map(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Resync(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_State(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Waiting(unsigned long tick, unsigned long msec);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_Hello(unsigned long tick);

	[[nodiscard]]
	boost::asio::awaitable<void> Send_GoodBye(unsigned long tick);

	template <typename T>
	[[nodiscard]]
	boost::asio::awaitable<void> SendRateLimited(const T &msg, unsigned long tick, unsigned long msecs);

	void SetConfig(const CInitMessage_Config &msg);

	void Parse_GameFull();
	void Parse_ProtocolMismatch(const unsigned char *buf);
	void Parse_EngineMismatch(const unsigned char *buf);
	void Parse_Resync(const unsigned char *buf);
	void Parse_Config(const unsigned char *buf);
	void Parse_State(const unsigned char *buf);
	void Parse_Welcome(const unsigned char *buf);
	void Parse_Map(const unsigned char *buf);

	[[nodiscard]]
	boost::asio::awaitable<void> Parse_AreYouThere();

signals:
	void fog_of_war_changed();
	void reveal_map_changed();
	void computer_opponents_changed();
	void resources_option_changed();
	void difficulty_changed();

private:
	std::string name;
	std::unique_ptr<CHost> serverHost;  /// IP:port of server to join
	NetworkState networkState;
	unsigned char lastMsgTypeSent;  /// Subtype of last InitConfig message sent
	CUDPSocket *socket = nullptr;
	std::unique_ptr<multiplayer_setup> server_setup;
	std::unique_ptr<multiplayer_setup> local_setup;
};

}

constexpr const char *icmsgsubtypenames[] = {
	"Hello",                   // Client Request
	"Config",                  // Setup message configure clients

	"EngineMismatch",          // Stratagus engine version doesn't match
	"ProtocolMismatch",        // Network protocol version doesn't match
	"EngineConfMismatch",      // Engine configuration isn't identical
	"MapUidMismatch",          // MAP UID doesn't match

	"GameFull",                // No player slots available
	"Welcome",                 // Acknowledge for new client connections

	"Waiting",                 // Client has received Welcome and is waiting for Map/State
	"Map",                     // MapInfo (and Mapinfo Ack)
	"State",                   // StateInfo
	"Resync",                  // Ack StateInfo change

	"ServerQuit",              // Server has quit game
	"GoodBye",                 // Client wants to leave game
	"SeeYou",                  // Client has left game

	"Go",                      // Client is ready to run
	"AreYouThere",             // Server asks are you there
	"IAmHere",                 // Client answers I am here
};
