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
/**@name netconnect.h - The network connection setup header file. */
//
//      (c) Copyright 1998-2008 by Lutz Sammer, Andreas Arens and Jimmy Salmon
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

#include "network/net_message.h"
#include "network/netsockets.h"

class CHost;
class CInitMessage_Header;
class CUDPSocket;

/// Network protocol major version
constexpr int NetworkProtocolMajorVersion = StratagusMajorVersion;
/// Network protocol minor version (maximum 99)
constexpr int NetworkProtocolMinorVersion = StratagusMinorVersion;
/// Network protocol patch level (maximum 99)
constexpr int NetworkProtocolPatchLevel = StratagusPatchLevel;
/// Network protocol version (1,2,3) -> 10203
constexpr int NetworkProtocolVersion = (NetworkProtocolMajorVersion * 10000 + NetworkProtocolMinorVersion * 100 + \
	NetworkProtocolPatchLevel);

/// Network protocol printf format string
#define NetworkProtocolFormatString "%d.%d.%d"
/// Network protocol printf format arguments
#define NetworkProtocolFormatArgs(v) (v) / 10000, ((v) / 100) % 100, (v) % 100

// received nothing from client for xx frames?
constexpr int CLIENT_LIVE_BEAT = 60;
constexpr int CLIENT_IS_DEAD = 300;

/**
**  Network Client connect states
*/
enum _net_client_con_state_ {
	ccs_unused = 0,           /// Unused.
	ccs_connecting,           /// New client
	ccs_connected,            /// Has received slot info
	ccs_mapinfo,              /// Has received matching map-info
	ccs_badmap,               /// Has received non-matching map-info
	ccs_synced,               /// Client is in sync with server
	ccs_async,                /// Server user has changed selection
	ccs_changed,              /// Client user has made menu selection
	ccs_detaching,            /// Client user wants to detach
	ccs_disconnected,         /// Client has detached
	ccs_unreachable,          /// Server is unreachable
	ccs_usercanceled,         /// Connection canceled by user
	ccs_nofreeslots,          /// Server has no more free slots
	ccs_serverquits,          /// Server quits
	ccs_goahead,              /// Server wants to start game
	ccs_started,              /// Server has started game
	ccs_incompatibleengine,   /// Incompatible engine version
	ccs_incompatiblenetwork   /// Incompatible netowrk version
};

extern int NetPlayers;                /// Network players

extern int HostsCount;                /// Number of hosts.
extern multiplayer_host Hosts[PlayerMax]; /// Host, port, and number of all players.

extern int NetConnectRunning;              /// Network menu: Setup mode active
extern int NetConnectType;              /// Network menu: Setup mode active
extern int NetLocalHostsSlot;              /// Network menu: Slot # in Hosts array of local client
extern int NetLocalPlayerNumber;           /// Player number of local client

extern std::string NetworkMapName;
extern int NoRandomPlacementMultiplayer;

template <typename T>
[[nodiscard]]
inline boost::asio::awaitable<void> NetworkSendICMessage(CUDPSocket &socket, const CHost &host, const T &msg)
{
	std::unique_ptr<const unsigned char[]> buf = msg.Serialize();
	co_await socket.Send(host, buf.get(), msg.Size());
}

[[nodiscard]]
extern boost::asio::awaitable<void> NetworkSendICMessage(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg);

template <typename T>
[[nodiscard]]
inline boost::asio::awaitable<void> NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const T &msg)
{
	co_await NetworkSendICMessage(socket, host, msg);

#ifdef DEBUG
	const std::string hostStr = host.toString();
	DebugPrint("Sending to %s -> %s\n" _C_ hostStr.c_str()
		_C_ icmsgsubtypenames[msg.GetHeader().GetSubType()]);
#endif
}

[[nodiscard]]
extern boost::asio::awaitable<void> NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg);

extern int FindHostIndexBy(const CHost &host);
extern void NetworkGamePrepareGameSettings();

extern int GetNetworkState();

[[nodiscard]]
extern boost::asio::awaitable<int> NetworkParseSetupEvent(const std::array<unsigned char, 1024> &buf, const CHost &host);  /// Parse a network connect event

extern void NetworkProcessClientRequest();  /// Menu Loop: Send out client request messages
extern void NetworkDetachFromServer();      /// Menu Loop: Client: Send GoodBye to the server and detach
