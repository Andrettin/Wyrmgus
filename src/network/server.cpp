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

#include "network/server.h"

#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "network/multiplayer_setup.h"
#include "network/netconnect.h"
#include "network/netsockets.h"
#include "network/network_manager.h"
#include "player/player_type.h"
#include "settings.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/random.h"
#include "version.h"

/**
**  Check if the Stratagus version and Network Protocol match
**
**  @param msg message received
**  @param host  host which send the message
**
**  @return 0 if the versions match, -1 otherwise
*/
[[nodiscard]]
static boost::asio::awaitable<int> CheckVersions(const CInitMessage_Hello &msg, CUDPSocket &socket, const CHost &host)
{
	if (msg.Stratagus != StratagusVersion) {
		const std::string hostStr = host.toString();
		fprintf(stderr, "Incompatible " NAME " version %d <-> %d from %s\n",
			StratagusVersion, msg.Stratagus, hostStr.c_str());

		const CInitMessage_EngineMismatch message;
		co_await NetworkSendICMessage_Log(socket, host, message);
		co_return -1;
	}

	if (msg.Version != NetworkProtocolVersion) {
		const std::string hostStr = host.toString();
		fprintf(stderr, "Incompatible network protocol version "
			NetworkProtocolFormatString " <-> "
			NetworkProtocolFormatString "\n"
			"from %s\n",
			NetworkProtocolFormatArgs(NetworkProtocolVersion),
			NetworkProtocolFormatArgs(msg.Version),
			hostStr.c_str());

		const CInitMessage_ProtocolMismatch message;
		co_await NetworkSendICMessage_Log(socket, host, message);
		co_return -1;
	}
	co_return 0;
}

namespace wyrmgus {

server::server()
{
}

server::~server()
{
}

void server::KickClient(int c)
{
	DebugPrint("kicking client %d\n" _C_ Hosts[c].PlyNr);

	const bool was_ready = static_cast<bool>(this->setup->Ready[c]);

	Hosts[c].Clear();
	this->setup->Ready[c] = 0;
	this->setup->Race[c] = 0;
	this->networkStates[c].Clear();

	//resync other clients
	for (int n = 1; n < PlayerMax - 1; ++n) {
		if (n != c && Hosts[n].PlyNr) {
			this->networkStates[n].State = ccs_async;
		}
	}

	if (was_ready) {
		emit network_manager::get()->player_ready_changed(c, false);
	}

	this->check_ready_to_start();
}

void server::init(const std::string &name, CUDPSocket *socket, const int open_slots)
{
	for (int i = 0; i < PlayerMax; ++i) {
		this->networkStates[i].Clear();
	}

	this->name = name;
	this->socket = socket;

	this->setup = std::make_unique<multiplayer_setup>();

	for (int i = open_slots; i < PlayerMax - 1; ++i) {
		this->setup->CompOpt[i] = 1;
	}

	this->ready_to_start = false;
}

void server::set_fog_of_war(const bool fow)
{
	event_loop::get()->post([this, fow]() {
		const uint8_t fow_uint8 = static_cast<uint8_t>(fow);

		this->setup->FogOfWar = fow_uint8;

		GameSettings.NoFogOfWar = !fow;

		this->resync_clients();
	});
}

void server::set_reveal_map(const bool reveal_map)
{
	event_loop::get()->post([this, reveal_map]() {
		const uint8_t reveal_map_uint8 = static_cast<uint8_t>(reveal_map);

		this->setup->RevealMap = reveal_map_uint8;

		GameSettings.RevealMap = reveal_map;

		this->resync_clients();
	});
}

void server::set_computer_opponents(const bool value)
{
	event_loop::get()->post([this, value]() {
		const uint8_t value_uint8 = static_cast<uint8_t>(value);

		this->setup->Opponents = value_uint8;

		this->resync_clients();
	});
}

void server::set_player_civilization(const int player_index, const int civilization_index)
{
	event_loop::get()->post([this, player_index, civilization_index]() {
		this->setup->Race[player_index] = civilization_index;

		GameSettings.Presets[player_index].Race = civilization_index;

		this->resync_clients();
	});
}

void server::set_resources_option(const int value)
{
	event_loop::get()->post([this, value]() {
		const uint8_t value_uint8 = static_cast<uint8_t>(value);

		this->setup->ResourcesOption = value_uint8;

		GameSettings.Resources = value;

		this->resync_clients();
	});
}

void server::set_difficulty(const int difficulty)
{
	event_loop::get()->post([this, difficulty]() {
		const uint8_t difficulty_uint8 = static_cast<uint8_t>(difficulty);

		this->setup->Difficulty = difficulty_uint8;

		GameSettings.Difficulty = difficulty;

		this->resync_clients();
	});
}

void server::start_game()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		CMap::get()->NoFogOfWar = !static_cast<bool>(this->setup->FogOfWar);

		if (static_cast<bool>(this->setup->RevealMap)) {
			FlagRevealMap = 1;
		}

		co_await this->init_game();

		NetworkGamePrepareGameSettings();

		co_await game::get()->run_map(path::from_string(NetworkMapName));
	});
}

boost::asio::awaitable<void> server::init_game()
{
	assert_throw(this->setup->CompOpt[0] == 0);

	//save it first...
	multiplayer_setup local_setup = *this->setup;

	//make a list of the available player slots.
	std::array<int, PlayerMax> num{};
	std::array<int, PlayerMax> rev{};
	int h = 0;
	for (int i = 0; i < PlayerMax; ++i) {
		if (CMap::get()->Info->get_player_types()[i] == player_type::person) {
			rev[i] = h;
			num[h++] = i;
			DebugPrint("Slot %d is available for an interactive player (%d)\n" _C_ i _C_ rev[i]);
		}
	}
	// Make a list of the available computer slots.
	int n = h;
	for (int i = 0; i < PlayerMax; ++i) {
		if (CMap::get()->Info->get_player_types()[i] == player_type::computer) {
			rev[i] = n++;
			DebugPrint("Slot %d is available for an ai computer player (%d)\n" _C_ i _C_ rev[i]);
		}
	}
	// Make a list of the remaining slots.
	for (int i = 0; i < PlayerMax; ++i) {
		if (CMap::get()->Info->get_player_types()[i] != player_type::person
			&& CMap::get()->Info->get_player_types()[i] != player_type::computer) {
			rev[i] = n++;
			// player_type::nobody - not available to anything..
		}
	}

#ifdef DEBUG
	printf("Initial Server Multiplayer Setup:\n");
	for (int i = 0; i < PlayerMax - 1; ++i) {
		printf("%02d: CO: %d   Race: %d   Host: ", i, this->setup->CompOpt[i], this->setup->Race[i]);
		if (this->setup->CompOpt[i] == 0) {
			const std::string hostStr = CHost(Hosts[i].Host, Hosts[i].Port).toString();
			printf(" %s %s", hostStr.c_str(), Hosts[i].PlyName);
		}
		printf("\n");
	}
#endif

	std::array<int, PlayerMax> org{};
	// Reverse to assign slots to menu setup state positions.
	for (int i = 0; i < PlayerMax; ++i) {
		org[i] = -1;
		for (int j = 0; j < PlayerMax; ++j) {
			if (rev[j] == i) {
				org[i] = j;
				break;
			}
		}
	}

	// Calculate NetPlayers
	NetPlayers = h;
	//Wyrmgus start
//	int compPlayers = this->setup->Opponents;
	const bool compPlayers = this->setup->Opponents > 0;
	//Wyrmgus end
	for (int i = 1; i < h; ++i) {
		if (Hosts[i].PlyNr == 0 && this->setup->CompOpt[i] != 0) {
			NetPlayers--;
		} else if (Hosts[i].PlyName[0] == 0) {
			NetPlayers--;
			//Wyrmgus start
//			if (--compPlayers >= 0) {
			if (compPlayers) {
				//Wyrmgus end
					// Unused slot gets a computer player
				this->setup->CompOpt[i] = 1;
				local_setup.CompOpt[i] = 1;
			} else {
				this->setup->CompOpt[i] = 2;
				local_setup.CompOpt[i] = 2;
			}
		}
	}

	// Compact host list.. (account for computer/closed slots in the middle..)
	for (int i = 1; i < h; ++i) {
		if (Hosts[i].PlyNr == 0) {
			int j;
			for (j = i + 1; j < PlayerMax - 1; ++j) {
				if (Hosts[j].PlyNr) {
					DebugPrint("Compact: Hosts %d -> Hosts %d\n" _C_ j _C_ i);
					Hosts[i] = Hosts[j];
					Hosts[j].Clear();
					std::swap(local_setup.CompOpt[i], local_setup.CompOpt[j]);
					std::swap(local_setup.Race[i], local_setup.Race[j]);
					break;
				}
			}
			if (j == PlayerMax - 1) {
				break;
			}
		}
	}

	// Randomize the position.
	// It can be disabled by writing NoRandomPlacementMultiplayer() in lua files.
	// Players slots are then mapped to players numbers(and colors).

	if (NoRandomPlacementMultiplayer == 1) {
		for (int i = 0; i < PlayerMax; ++i) {
			if (CMap::get()->Info->get_player_types()[i] != player_type::computer) {
				org[i] = Hosts[i].PlyNr;
			}
		}
	} else {
		int j = h;
		for (int i = 0; i < NetPlayers; ++i) {
			assert_throw(j > 0);
			int chosen = random::get()->generate_async(j);

			n = num[chosen];
			Hosts[i].PlyNr = n;
			int k = org[i];
			if (k != n) {
				for (int o = 0; o < PlayerMax; ++o) {
					if (org[o] == n) {
						org[o] = k;
						break;
					}
				}
				org[i] = n;
			}
			DebugPrint("Assigning player %d to slot %d (%d)\n" _C_ i _C_ n _C_ org[i]);

			num[chosen] = num[--j];
		}
	}

	// Complete all setup states for the assigned slots.
	for (int i = 0; i < PlayerMax; ++i) {
		num[i] = 1;
		n = org[i];
		this->setup->CompOpt[n] = local_setup.CompOpt[i];
		this->setup->Race[n] = local_setup.Race[i];
	}

	/* NOW we have NetPlayers in Hosts array, with server multiplayer setup shuffled up to match it.. */

	//
	// Send all clients host:ports to all clients.
	//  Slot 0 is the server!
	//
	NetLocalPlayerNumber = Hosts[0].PlyNr;
	HostsCount = NetPlayers - 1;

	{
		std::unique_lock<std::shared_mutex> lock(network_manager::get()->get_mutex());

		// Move ourselves (server slot 0) to the end of the list
		std::swap(Hosts[0], Hosts[HostsCount]);

		emit network_manager::get()->player_name_changed(0, Hosts[0].PlyName);
		emit network_manager::get()->player_name_changed(HostsCount, Hosts[HostsCount].PlyName);
	}

	// Prepare the final config message:
	CInitMessage_Config message;
	message.hostsCount = NetPlayers;
	for (int i = 0; i < NetPlayers; ++i) {
		message.hosts[i] = Hosts[i];
		message.hosts[i].PlyNr = Hosts[i].PlyNr;
	}

	// Prepare the final state message:
	const CInitMessage_State statemsg(MessageInit_FromServer, *this->setup);

	DebugPrint("Ready, sending InitConfig to %d host(s)\n" _C_ HostsCount);
	// Send all clients host:ports to all clients.
	for (int j = HostsCount; j;) {

	breakout:
		// Send to all clients.
		for (int i = 0; i < HostsCount; ++i) {
			const CHost host(message.hosts[i].Host, message.hosts[i].Port);

			if (num[Hosts[i].PlyNr] == 1) { // not acknowledged yet
				message.clientIndex = i;
				co_await NetworkSendICMessage_Log(*this->socket, host, message);
			} else if (num[Hosts[i].PlyNr] == 2) {
				co_await NetworkSendICMessage_Log(*this->socket, host, statemsg);
			}
		}

		// Wait for acknowledge
		std::array<unsigned char, 1024> buf{};
		while (j && co_await this->socket->WaitForDataToRead(1000)) {
			CHost host;

			size_t len = 0;

			try {
				len = co_await this->socket->Recv(buf, sizeof(buf), &host);
			} catch (const std::exception &exception) {
				exception::report(exception);
#ifdef DEBUG
				const std::string hostStr = host.toString();
				DebugPrint("*Receive ack failed: (%d) from %s\n" _C_ len _C_ hostStr.c_str());
#endif
				continue;
			}

			CInitMessage_Header header;
			header.Deserialize(buf.data());
			const unsigned char type = header.GetType();
			const unsigned char subtype = header.GetSubType();

			if (type == MessageInit_FromClient) {
				switch (subtype) {
				case ICMConfig: {
#ifdef DEBUG
					const std::string hostStr = host.toString();
					DebugPrint("Got ack for InitConfig from %s\n" _C_ hostStr.c_str());
#endif
					const int index = FindHostIndexBy(host);
					if (index != -1) {
						if (num[Hosts[index].PlyNr] == 1) {
							num[Hosts[index].PlyNr]++;
						}
						goto breakout;
					}
					break;
				}
				case ICMGo: {
#ifdef DEBUG
					const std::string hostStr = host.toString();
					DebugPrint("Got ack for InitState from %s\n" _C_ hostStr.c_str());
#endif
					const int index = FindHostIndexBy(host);
					if (index != -1) {
						if (num[Hosts[index].PlyNr] == 2) {
							num[Hosts[index].PlyNr] = 0;
							--j;
							DebugPrint("Removing host %d from waiting list\n" _C_ j);
						}
					}
					break;
				}
				default:
					DebugPrint("Server: Config ACK: Unhandled subtype %d\n" _C_ subtype);
					break;
				}
			} else {
				DebugPrint("Unexpected Message Type %d while waiting for Config ACK\n" _C_ type);
			}
		}
	}

	DebugPrint("DONE: All configs acked - Now starting...\n");
	// Give clients a quick-start kick..
	const CInitMessage_Header message_go(MessageInit_FromServer, ICMGo);
	for (int i = 0; i < HostsCount; ++i) {
		const CHost host(Hosts[i].Host, Hosts[i].Port);
		co_await NetworkSendICMessage_Log(*this->socket, host, message_go);
	}
}

void server::check_ready_to_start()
{
	network_manager::get()->check_players(this->setup.get());

	const int connected_player_count = network_manager::get()->get_connected_player_count();
	const int ready_player_count = network_manager::get()->get_ready_player_count();

	this->set_ready_to_start(connected_player_count > 0 && ready_player_count == connected_player_count);
}

bool server::is_player_ready(const int player_index) const
{
	return static_cast<bool>(this->setup->Ready[player_index]);
}

boost::asio::awaitable<void> server::Send_AreYouThere(const multiplayer_host &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMAYT); // AreYouThere

	co_await NetworkSendICMessage(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Send_GameFull(const CHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGameFull);

	co_await NetworkSendICMessage_Log(*socket, host, message);
}

boost::asio::awaitable<void> server::Send_Welcome(const multiplayer_host &host, int index)
{
	CInitMessage_Welcome message;

	message.hosts[0].PlyNr = index; // Host array slot number
	message.hosts[0].SetName(name.c_str()); // Name of server player
	for (int i = 1; i < PlayerMax - 1; ++i) { // Info about other clients
		if (i != index && Hosts[i].PlyNr) {
			message.hosts[i] = Hosts[i];
		}
	}

	co_await NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Send_Resync(const multiplayer_host &host, int hostIndex)
{
	CInitMessage_Resync message;

	for (int i = 1; i < PlayerMax - 1; ++i) { // Info about other clients
		if (i != hostIndex && Hosts[i].PlyNr) {
			message.hosts[i] = Hosts[i];
		}
	}

	co_await NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Send_Map(const multiplayer_host &host)
{
	const CInitMessage_Map message(NetworkMapName.c_str(), CMap::get()->Info->MapUID);

	co_await NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Send_State(const multiplayer_host &host)
{
	const CInitMessage_State message(MessageInit_FromServer, *this->setup);

	co_await NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Send_GoodBye(const multiplayer_host &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGoodBye);

	co_await NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

boost::asio::awaitable<void> server::Update(unsigned long frameCounter)
{
	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (Hosts[i].PlyNr && Hosts[i].Host && Hosts[i].Port) {
			const unsigned long fcd = frameCounter - networkStates[i].LastFrame;
			if (fcd >= CLIENT_LIVE_BEAT) {
				if (fcd > CLIENT_IS_DEAD) {
					KickClient(i);
				} else if (fcd % 5 == 0) {
					// Probe for the client
					co_await Send_AreYouThere(Hosts[i]);
				}
			}
		}
	}
}

boost::asio::awaitable<void> server::Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host)
{
	const unsigned char msgsubtype = buf[1];
	int index = FindHostIndexBy(host);

	if (index == -1) {
		if (msgsubtype == ICMHello) {
			CInitMessage_Hello msg;

			msg.Deserialize(buf);
			if (co_await CheckVersions(msg, *socket, host)) {
				co_return;
			}
			// Special case: a new client has arrived
			index = co_await Parse_Hello(-1, msg, host);
			networkStates[index].LastFrame = frameCounter;
		}

		co_return;
	}

	networkStates[index].LastFrame = frameCounter;

	switch (msgsubtype) {
	case ICMHello: { // a new client has arrived
		CInitMessage_Hello msg;

		msg.Deserialize(buf);
		co_await Parse_Hello(index, msg, host);
		break;
	}
	case ICMResync:
		co_await Parse_Resync(index);
		break;
	case ICMWaiting:
		co_await Parse_Waiting(index);
		break;
	case ICMMap:
		co_await Parse_Map(index);
		break;
	case ICMState: {
		CInitMessage_State msg;

		msg.Deserialize(buf);
		co_await Parse_State(index, msg);
		break;
	}
	case ICMMapUidMismatch: // Parse_MapUidMismatch(index, buf); break;
	case ICMGoodBye:
		co_await Parse_GoodBye(index);
		break;
	case ICMSeeYou:
		Parse_SeeYou(index);
		break;
	case ICMIAH:
		break;
	default:
		DebugPrint("Server: Unhandled subtype %d from host %d\n" _C_ msgsubtype _C_ index);
		break;
	}
}

void server::resync_clients()
{
	if (NetConnectRunning != 1) {
		return;
	}

	this->mark_clients_as_resync();
}

void server::mark_clients_as_resync()
{
	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (Hosts[i].PlyNr && this->networkStates[i].State == ccs_synced) {
			this->networkStates[i].State = ccs_async;
		}
	}
}

/**
**  Parse the initial 'Hello' message of new client that wants to join the game
**
**  @param h slot number of host msg originates from
**  @param msg message received
**  @param host  host which send the message
**
**  @return host index
*/
boost::asio::awaitable<int> server::Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host)
{
	if (h == -1) { // it is a new client
		for (int i = 1; i < PlayerMax - 1; ++i) {
			// occupy first available slot
			if (this->setup->CompOpt[i] == 0) {
				if (Hosts[i].PlyNr == 0) {
					h = i;
					break;
				}
			}
		}

		if (h != -1) {
			std::unique_lock<std::shared_mutex> lock(network_manager::get()->get_mutex());

			Hosts[h].Host = host.getIp();
			Hosts[h].Port = host.getPort();
			Hosts[h].PlyNr = h;
			Hosts[h].SetName(msg.PlyName);
#ifdef DEBUG
			const std::string hostStr = host.toString();
			DebugPrint("New client %s [%s]\n" _C_ hostStr.c_str() _C_ Hosts[h].PlyName);
#endif
			networkStates[h].State = ccs_connecting;
			networkStates[h].MsgCnt = 0;

			emit network_manager::get()->player_name_changed(h, Hosts[h].PlyName);
		} else {
			// Game is full - reject connnection
			co_await Send_GameFull(host);
			co_return -1;
		}
	}

	// this code path happens until client sends waiting (= has received this message)
	co_await Send_Welcome(Hosts[h], h);

	this->check_ready_to_start();

	networkStates[h].MsgCnt++;
	if (networkStates[h].MsgCnt > 48) {
		// Detects UDP input firewalled or behind NAT firewall clients
		// If packets are missed, clients are kicked by AYT check later..
		KickClient(h);
		co_return -1;
	}

	co_return h;
}

/**
**  Parse client resync request after client user has changed menu selection
**
**  @param h slot number of host msg originates from
*/
boost::asio::awaitable<void> server::Parse_Resync(const int h)
{
	switch (networkStates[h].State) {
	case ccs_mapinfo:
		// a delayed ack - fall through..
	case ccs_async:
		// client has recvd welcome and is waiting for info
		networkStates[h].State = ccs_synced;
		networkStates[h].MsgCnt = 0;
		/* Fall through */
	case ccs_synced: {
		// this code path happens until client falls back to ICMWaiting
		// (indicating Resync has completed)
		co_await Send_Resync(Hosts[h], h);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends resync, but doesn't receive our resync ack....
		}
		break;
	}
	default:
		DebugPrint("Server: ICMResync: Unhandled state %d Host %d\n" _C_ networkStates[h].State _C_ h);
		break;
	}
}

/**
**  Parse client heart beat waiting message
**
**  @param h slot number of host msg originates from
*/
boost::asio::awaitable<void> server::Parse_Waiting(const int h)
{
	switch (networkStates[h].State) {
		// client has recvd welcome and is waiting for info
	case ccs_connecting:
		networkStates[h].State = ccs_connected;
		networkStates[h].MsgCnt = 0;
		/* Fall through */
	case ccs_connected: {
		// this code path happens until client acknowledges the map
		co_await Send_Map(Hosts[h]);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends waiting, but doesn't receive our map....
		}
		break;
	}
	case ccs_mapinfo:
		networkStates[h].State = ccs_synced;
		networkStates[h].MsgCnt = 0;
		for (int i = 1; i < PlayerMax - 1; ++i) {
			if (i != h && Hosts[i].PlyNr) {
				// Notify other clients
				networkStates[i].State = ccs_async;
			}
		}
		/* Fall through */
	case ccs_synced:
		// the wanted state - do nothing.. until start...
		networkStates[h].MsgCnt = 0;
		break;

	case ccs_async: {
		// Server User has changed menu selection. This state is set by MENU code
		// OR we have received a new client/other client has changed data

		// this code path happens until client acknoledges the state change
		// by sending ICMResync
		co_await Send_State(Hosts[h]);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends waiting, but doesn't receive our state info....
		}
		break;
	}
	default:
		DebugPrint("Server: ICMWaiting: Unhandled state %d Host %d\n" _C_ networkStates[h].State _C_ h);
		break;
	}
}

/**
**  Parse client map info acknoledge message
**
**  @param h slot number of host msg originates from
*/
boost::asio::awaitable<void> server::Parse_Map(const int h)
{
	switch (networkStates[h].State) {
		// client has recvd map info waiting for state info
	case ccs_connected:
		networkStates[h].State = ccs_mapinfo;
		networkStates[h].MsgCnt = 0;
		/* Fall through */
	case ccs_mapinfo: {
		// this code path happens until client acknowledges the state info
		// by falling back to ICMWaiting with prev. State synced
		co_await Send_State(Hosts[h]);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends mapinfo, but doesn't receive our state info....
		}
		break;
	}
	default:
		DebugPrint("Server: ICMMap: Unhandled state %d Host %d\n" _C_ networkStates[h].State _C_ h);
		break;
	}
}

/**
**  Parse locate state change notifiction or initial state info request of client
**
**  @param h slot number of host msg originates from
**  @param msg message received
*/
boost::asio::awaitable<void> server::Parse_State(const int h, const CInitMessage_State &msg)
{
	bool player_ready_changed = false;

	switch (networkStates[h].State) {
	case ccs_mapinfo:
		// User State Change right after connect - should not happen, but..
		/* Fall through */
	case ccs_synced:
		// Default case: Client is in sync with us, but notes a local change
		// networkStates[h].State = ccs_async;
		networkStates[h].MsgCnt = 0;

		// Use information supplied by the client:
		if (this->setup->Ready[h] != msg.State.Ready[h]) {
			this->setup->Ready[h] = msg.State.Ready[h];
			player_ready_changed = true;
		}
		this->setup->Race[h] = msg.State.Race[h];

		// Add additional info usage here!

		// Resync other clients (and us..)
		for (int i = 1; i < PlayerMax - 1; ++i) {
			if (Hosts[i].PlyNr) {
				networkStates[i].State = ccs_async;
			}
		}
		/* Fall through */
	case ccs_async: {
		// this code path happens until client acknowledges the state change reply
		// by sending ICMResync
		co_await Send_State(Hosts[h]);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends State, but doesn't receive our state info....
		}
		break;
	}
	default:
		log::log_error("Server: ICMState: Unhandled state " + std::to_string(networkStates[h].State)  + " Host " + std::to_string(h));
		break;
	}

	if (player_ready_changed) {
		emit network_manager::get()->player_ready_changed(h, static_cast<bool>(this->setup->Ready[h]));
	}

	this->check_ready_to_start();
}

/**
**  Parse the disconnect request of a client by sending out good bye
**
**  @param h slot number of host msg originates from
*/
boost::asio::awaitable<void> server::Parse_GoodBye(const int h)
{
	switch (networkStates[h].State) {
		default:
			// We can enter here from _ANY_ state!
			networkStates[h].MsgCnt = 0;
			networkStates[h].State = ccs_detaching;
			/* Fall through */
		case ccs_detaching: {
			// this code path happens until client acknoledges the GoodBye
			// by sending ICMSeeYou;
			co_await Send_GoodBye(Hosts[h]);

			networkStates[h].MsgCnt++;
			if (networkStates[h].MsgCnt > 10) {
				// FIXME: Client sends GoodBye, but doesn't receive our GoodBye....
			}
			break;
		}
	}
}

/**
** Parse the final see you msg of a disconnecting client
**
** @param h slot number of host msg originates from
*/
void server::Parse_SeeYou(const int h)
{
	switch (networkStates[h].State) {
	case ccs_detaching:
		KickClient(h);
		break;

	default:
		DebugPrint("Server: ICMSeeYou: Unhandled state %d Host %d\n" _C_
			networkStates[h].State _C_ h);
		break;
	}
}

}
