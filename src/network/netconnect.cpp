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
#include "util/util.h"
#include "video/video.h"

int HostsCount;                        /// Number of hosts.
multiplayer_host Hosts[PlayerMax];         /// Host and ports of all players.

int NetConnectRunning = 0;             /// Network menu: Setup mode active
int NetConnectType = 0;             /// Network menu: Setup mode active
int NetLocalHostsSlot;                 /// Network menu: Slot # in Hosts array of local client
int NetLocalPlayerNumber;              /// Player number of local client

int NetPlayers;                         /// How many network players
std::string NetworkMapName;             /// Name of the map received with ICMMap
int NoRandomPlacementMultiplayer = 0; /// Disable the random placement of players in muliplayer mode

/**
** Send an InitConfig message across the Network
**
** @param host Host to send to (network byte order).
** @param port Port of host to send to (network byte order).
** @param msg The message to send
*/
boost::asio::awaitable<void> NetworkSendICMessage(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg)
{
	auto buf = std::make_unique<unsigned char[]>(msg.Size());
	msg.Serialize(buf.get());
	co_await socket.Send(host, buf.get(), msg.Size());
}

boost::asio::awaitable<void> NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg)
{
	co_await NetworkSendICMessage(socket, host, msg);

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
boost::asio::awaitable<int> NetworkParseSetupEvent(const std::array<unsigned char, 1024> &buf, const CHost &host)
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
		co_return 0;
	}
#ifdef DEBUG
	const unsigned char msgsubtype = header.GetSubType();
	const std::string hostStr = host.toString();
	DebugPrint("Received %s (%d) from %s\n" _C_
			   icmsgsubtypenames[int(msgsubtype)] _C_ msgsubtype _C_
			   hostStr.c_str());
#endif
	if (NetConnectRunning == 2) { // client
		if (co_await client::get()->Parse(buf) == false) {
			NetConnectRunning = 0;
		}
	} else if (NetConnectRunning == 1) { // server
		co_await server::get()->Parse(FrameCounter, buf.data(), host);
	}
	co_return 1;
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
** Terminate and detach Network connect state machine for the client
*/
void NetworkDetachFromServer()
{
	client::get()->DetachFromServer();
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

