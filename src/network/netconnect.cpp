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
/**@name netconnect.cpp - The network high level connection code. */
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

#include "network/netconnect.h"

#include "database/preferences.h"
#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "network/master.h"
#include "network/client.h"
#include "network/multiplayer_setup.h"
#include "network/network_state.h"
#include "network/network.h"
#include "network/server.h"
#include "parameters.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "settings.h"
#include "ui/interface.h"
#include "util/assert_util.h"
#include "util/random.h"
#include "util/util.h"
#include "version.h"
#include "video/video.h"

int HostsCount;                        /// Number of hosts.
CNetworkHost Hosts[PlayerMax];         /// Host and ports of all players.

int NetConnectRunning = 0;             /// Network menu: Setup mode active
int NetConnectType = 0;             /// Network menu: Setup mode active
int NetLocalHostsSlot;                 /// Network menu: Slot # in Hosts array of local client
int NetLocalPlayerNumber;              /// Player number of local client

int NetPlayers;                         /// How many network players
std::string NetworkMapName;             /// Name of the map received with ICMMap
static int NoRandomPlacementMultiplayer = 0; /// Disable the random placement of players in muliplayer mode

/**
** Send an InitConfig message across the Network
**
** @param host Host to send to (network byte order).
** @param port Port of host to send to (network byte order).
** @param msg The message to send
*/
void NetworkSendICMessage(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg)
{
	auto buf = std::make_unique<unsigned char[]>(msg.Size());
	msg.Serialize(buf.get());
	socket.Send(host, buf.get(), msg.Size());
}

void NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg)
{
	NetworkSendICMessage(socket, host, msg);

#ifdef DEBUG
	const std::string hostStr = host.toString();
	DebugPrint("Sending to %s -> %s\n" _C_ hostStr.c_str()
		_C_ icmsgsubtypenames[msg.GetSubType()]);
#endif
}

// Functions

/**
**  Parse a setup event. (Command type <= MessageInitEvent)
**
**  @param buf Packet received
**  @param size size of the received packet.
**  @param host  host which send the message
**
**  @return 1 if packet is an InitConfig message, 0 otherwise
*/
int NetworkParseSetupEvent(const std::array<unsigned char, 1024> &buf, const CHost &host)
{
	assert_throw(NetConnectRunning != 0);

	CInitMessage_Header header;
	header.Deserialize(buf.data());
	const unsigned char msgtype = header.GetType();
	if ((msgtype == MessageInit_FromClient && NetConnectRunning != 1)
		|| (msgtype == MessageInit_FromServer && NetConnectRunning != 2)) {
		if (NetConnectRunning == 2 && client::get()->GetNetworkState() == ccs_started) {
			// Client has acked ready to start and receives first real network packet.
			// This indicates that we missed the 'Go' in started state and the game
			// has been started by the server, so do the same for the client.
			NetConnectRunning = 0; // End the menu..
		}
		return 0;
	}
#ifdef DEBUG
	const unsigned char msgsubtype = header.GetSubType();
	const std::string hostStr = host.toString();
	DebugPrint("Received %s (%d) from %s\n" _C_
			   icmsgsubtypenames[int(msgsubtype)] _C_ msgsubtype _C_
			   hostStr.c_str());
#endif
	if (NetConnectRunning == 2) { // client
		if (client::get()->Parse(buf) == false) {
			NetConnectRunning = 0;
		}
	} else if (NetConnectRunning == 1) { // server
		server::get()->Parse(FrameCounter, buf.data(), host);
	}
	return 1;
}

/**
** Client Menu Loop: Send out client request messages
*/
void NetworkProcessClientRequest()
{
	if (client::get()->Update(GetTicks()) == false) {
		NetConnectRunning = 0;
	}
}

int GetNetworkState()
{
	return client::get()->GetNetworkState();
}

int FindHostIndexBy(const CHost &host)
{
	for (int i = 0; i != PlayerMax; ++i) {
		if (Hosts[i].Host == host.getIp() && Hosts[i].Port == host.getPort()) {
			return i;
		}
	}
	return -1;
}

/**
** Server Menu Loop: Send out server request messages
*/
void NetworkProcessServerRequest()
{
	if (GameRunning) {
		return;
		// Game already started...
	}

	server::get()->Update(FrameCounter);
}

/**
** Server user has finally hit the start game button
*/
void NetworkServerStartGame()
{
	assert_throw(server::get()->get_setup().CompOpt[0] == 0);

	//save it first...
	multiplayer_setup local_setup = server::get()->get_setup();

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
		printf("%02d: CO: %d   Race: %d   Host: ", i, server::get()->get_setup().CompOpt[i], server::get()->get_setup().Race[i]);
		if (server::get()->get_setup().CompOpt[i] == 0) {
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
//	int compPlayers = server::get()->get_setup().Opponents;
	const bool compPlayers = server::get()->get_setup().Opponents > 0;
	//Wyrmgus end
	for (int i = 1; i < h; ++i) {
		if (Hosts[i].PlyNr == 0 && server::get()->get_setup().CompOpt[i] != 0) {
			NetPlayers--;
		} else if (Hosts[i].PlyName[0] == 0) {
			NetPlayers--;
			//Wyrmgus start
//			if (--compPlayers >= 0) {
			if (compPlayers) {
			//Wyrmgus end
				// Unused slot gets a computer player
				server::get()->get_setup().CompOpt[i] = 1;
				local_setup.CompOpt[i] = 1;
			} else {
				server::get()->get_setup().CompOpt[i] = 2;
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
		server::get()->get_setup().CompOpt[n] = local_setup.CompOpt[i];
		server::get()->get_setup().Race[n] = local_setup.Race[i];
	}

	/* NOW we have NetPlayers in Hosts array, with server multiplayer setup shuffled up to match it.. */

	//
	// Send all clients host:ports to all clients.
	//  Slot 0 is the server!
	//
	NetLocalPlayerNumber = Hosts[0].PlyNr;
	HostsCount = NetPlayers - 1;

	// Move ourselves (server slot 0) to the end of the list
	std::swap(Hosts[0], Hosts[HostsCount]);

	// Prepare the final config message:
	CInitMessage_Config message;
	message.hostsCount = NetPlayers;
	for (int i = 0; i < NetPlayers; ++i) {
		message.hosts[i] = Hosts[i];
		message.hosts[i].PlyNr = Hosts[i].PlyNr;
	}

	// Prepare the final state message:
	const CInitMessage_State statemsg(MessageInit_FromServer, server::get()->get_setup());

	DebugPrint("Ready, sending InitConfig to %d host(s)\n" _C_ HostsCount);
	// Send all clients host:ports to all clients.
	for (int j = HostsCount; j;) {

breakout:
		// Send to all clients.
		for (int i = 0; i < HostsCount; ++i) {
			const CHost host(message.hosts[i].Host, message.hosts[i].Port);

			if (num[Hosts[i].PlyNr] == 1) { // not acknowledged yet
				message.clientIndex = i;
				NetworkSendICMessage_Log(NetworkFildes, host, message);
			} else if (num[Hosts[i].PlyNr] == 2) {
				NetworkSendICMessage_Log(NetworkFildes, host, statemsg);
			}
		}

		// Wait for acknowledge
		std::array<unsigned char, 1024> buf{};
		while (j && NetworkFildes.HasDataToRead(1000)) {
			CHost host;
			const int len = NetworkFildes.Recv(buf, sizeof(buf), &host);
			if (len < 0) {
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

	DebugPrint("DONE: All configs acked - Now starting..\n");
	// Give clients a quick-start kick..
	const CInitMessage_Header message_go(MessageInit_FromServer, ICMGo);
	for (int i = 0; i < HostsCount; ++i) {
		const CHost host(Hosts[i].Host, Hosts[i].Port);
		NetworkSendICMessage_Log(NetworkFildes, host, message_go);
	}
}

/**
** Terminate and detach Network connect state machine for the client
*/
void NetworkDetachFromServer()
{
	client::get()->DetachFromServer();
}

/**
** Setup Network connect state machine for the server
*/
void NetworkInitServerConnect(int openslots)
{
	NetConnectRunning = 1;
	NetConnectType = 1;

	for (int i = 0; i < PlayerMax; ++i) {
		Hosts[i].Clear();
	}

	server::get()->Init(preferences::get()->get_local_player_name(), &NetworkFildes);

	// preset the server (initially always slot 0)
	Hosts[0].SetName(preferences::get()->get_local_player_name().c_str());

	for (int i = openslots; i < PlayerMax - 1; ++i) {
		server::get()->get_setup().CompOpt[i] = 1;
	}
}

/**
** Notify state change by menu user to connected clients
*/
void NetworkServerResyncClients()
{
	if (NetConnectRunning == 1) {
		server::get()->MarkClientsAsResync();
	}
}

/**
** Multiplayer network game final race and player type setup.
*/
void NetworkGamePrepareGameSettings()
{
	DebugPrint("NetPlayers = %d\n" _C_ NetPlayers);

	GameSettings.NetGameType = SettingsMultiPlayerGame;

#ifdef DEBUG
	for (int i = 0; i < PlayerMax - 1; i++) {
		printf("%02d: CO: %d   Race: %d   Name: ", i, server::get()->get_setup().CompOpt[i], server::get()->get_setup().Race[i]);
		if (server::get()->get_setup().CompOpt[i] == 0) {
			for (int h = 0; h != HostsCount; ++h) {
				if (Hosts[h].PlyNr == i) {
					printf("%s", Hosts[h].PlyName);
				}
			}
			if (i == NetLocalPlayerNumber) {
				printf("%s (localhost)", preferences::get()->get_local_player_name().c_str());
			}
		}
		printf("\n");
	}
#endif

	// Make a list of the available player slots.
	std::array<int, PlayerMax> num{};
	std::array<int, PlayerMax> comp{};
	int c = 0;
	int h = 0;
	for (int i = 0; i < PlayerMax; i++) {
		if (CMap::get()->Info->get_player_types()[i] == player_type::person) {
			num[h++] = i;
		}
		if (CMap::get()->Info->get_player_types()[i] == player_type::computer) {
			comp[c++] = i; // available computer player slots
		}
	}
	for (int i = 0; i < h; i++) {
		GameSettings.Presets[num[i]].Race = server::get()->get_setup().Race[num[i]];
		switch (server::get()->get_setup().CompOpt[num[i]]) {
			case 0: {
				GameSettings.Presets[num[i]].Type = player_type::person;
				break;
			}
			case 1:
				GameSettings.Presets[num[i]].Type = player_type::computer;
				break;
			case 2:
				GameSettings.Presets[num[i]].Type = player_type::nobody;
			default:
				break;
		}
	}
	for (int i = 0; i < c; i++) {
		if (server::get()->get_setup().CompOpt[comp[i]] == 2) { // closed..
			GameSettings.Presets[comp[i]].Type = player_type::nobody;
			DebugPrint("Settings[%d].Type == Closed\n" _C_ comp[i]);
		}
	}

#ifdef DEBUG
	for (int i = 0; i != HostsCount; ++i) {
		assert_throw(GameSettings.Presets[Hosts[i].PlyNr].Type == player_type::person);
	}
	assert_throw(GameSettings.Presets[NetLocalPlayerNumber].Type == player_type::person);
#endif
}

/**
**  Removes Randomization of Player position in Multiplayer mode
**
**  @param l  Lua state.
*/
static int CclNoRandomPlacementMultiplayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	NoRandomPlacementMultiplayer = 1;
	return 0;
}

void NetworkCclRegister()
{
	lua_register(Lua, "NoRandomPlacementMultiplayer", CclNoRandomPlacementMultiplayer);
}

