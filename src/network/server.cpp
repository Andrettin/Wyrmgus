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

#include "map/map.h"
#include "map/map_info.h"
#include "network/multiplayer_setup.h"
#include "network/netconnect.h"
#include "network/netsockets.h"
#include "settings.h"
#include "version.h"

/**
**  Check if the Stratagus version and Network Protocol match
**
**  @param msg message received
**  @param host  host which send the message
**
**  @return 0 if the versions match, -1 otherwise
*/
static int CheckVersions(const CInitMessage_Hello &msg, CUDPSocket &socket, const CHost &host)
{
	if (msg.Stratagus != StratagusVersion) {
		const std::string hostStr = host.toString();
		fprintf(stderr, "Incompatible " NAME " version %d <-> %d from %s\n",
			StratagusVersion, msg.Stratagus, hostStr.c_str());

		const CInitMessage_EngineMismatch message;
		NetworkSendICMessage_Log(socket, host, message);
		return -1;
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
		NetworkSendICMessage_Log(socket, host, message);
		return -1;
	}
	return 0;
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
}

void server::set_fog_of_war(const bool fow)
{
	const uint8_t fow_uint8 = static_cast<uint8_t>(fow);

	this->setup->FogOfWar = fow_uint8;

	this->resync_clients();
	GameSettings.NoFogOfWar = !fow;
}

void server::Send_AreYouThere(const CNetworkHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMAYT); // AreYouThere

	NetworkSendICMessage(*socket, CHost(host.Host, host.Port), message);
}

void server::Send_GameFull(const CHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGameFull);

	NetworkSendICMessage_Log(*socket, host, message);
}

void server::Send_Welcome(const CNetworkHost &host, int index)
{
	CInitMessage_Welcome message;

	message.hosts[0].PlyNr = index; // Host array slot number
	message.hosts[0].SetName(name.c_str()); // Name of server player
	for (int i = 1; i < PlayerMax - 1; ++i) { // Info about other clients
		if (i != index && Hosts[i].PlyNr) {
			message.hosts[i] = Hosts[i];
		}
	}
	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void server::Send_Resync(const CNetworkHost &host, int hostIndex)
{
	CInitMessage_Resync message;

	for (int i = 1; i < PlayerMax - 1; ++i) { // Info about other clients
		if (i != hostIndex && Hosts[i].PlyNr) {
			message.hosts[i] = Hosts[i];
		}
	}
	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void server::Send_Map(const CNetworkHost &host)
{
	const CInitMessage_Map message(NetworkMapName.c_str(), CMap::get()->Info->MapUID);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void server::Send_State(const CNetworkHost &host)
{
	const CInitMessage_State message(MessageInit_FromServer, *this->setup);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void server::Send_GoodBye(const CNetworkHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGoodBye);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void server::Update(unsigned long frameCounter)
{
	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (Hosts[i].PlyNr && Hosts[i].Host && Hosts[i].Port) {
			const unsigned long fcd = frameCounter - networkStates[i].LastFrame;
			if (fcd >= CLIENT_LIVE_BEAT) {
				if (fcd > CLIENT_IS_DEAD) {
					KickClient(i);
				} else if (fcd % 5 == 0) {
					// Probe for the client
					Send_AreYouThere(Hosts[i]);
				}
			}
		}
	}
}

void server::Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host)
{
	const unsigned char msgsubtype = buf[1];
	int index = FindHostIndexBy(host);

	if (index == -1) {
		if (msgsubtype == ICMHello) {
			CInitMessage_Hello msg;

			msg.Deserialize(buf);
			if (CheckVersions(msg, *socket, host)) {
				return;
			}
			// Special case: a new client has arrived
			index = Parse_Hello(-1, msg, host);
			networkStates[index].LastFrame = frameCounter;
		}
		return;
	}
	networkStates[index].LastFrame = frameCounter;
	switch (msgsubtype) {
	case ICMHello: { // a new client has arrived
		CInitMessage_Hello msg;

		msg.Deserialize(buf);
		Parse_Hello(index, msg, host);
		break;
	}
	case ICMResync: Parse_Resync(index); break;
	case ICMWaiting: Parse_Waiting(index); break;
	case ICMMap: Parse_Map(index); break;

	case ICMState: {
		CInitMessage_State msg;

		msg.Deserialize(buf);
		Parse_State(index, msg);
		break;
	}
	case ICMMapUidMismatch: // Parse_MapUidMismatch(index, buf); break;
	case ICMGoodBye: Parse_GoodBye(index); break;
	case ICMSeeYou: Parse_SeeYou(index); break;
	case ICMIAH: break;

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

	this->MarkClientsAsResync();
}

void server::MarkClientsAsResync()
{
	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (Hosts[i].PlyNr && networkStates[i].State == ccs_synced) {
			networkStates[i].State = ccs_async;
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
int server::Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host)
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
		} else {
			// Game is full - reject connnection
			Send_GameFull(host);
			return -1;
		}
	}
	// this code path happens until client sends waiting (= has received this message)
	Send_Welcome(Hosts[h], h);

	networkStates[h].MsgCnt++;
	if (networkStates[h].MsgCnt > 48) {
		// Detects UDP input firewalled or behind NAT firewall clients
		// If packets are missed, clients are kicked by AYT check later..
		KickClient(h);
		return -1;
	}
	return h;
}

/**
**  Parse client resync request after client user has changed menu selection
**
**  @param h slot number of host msg originates from
*/
void server::Parse_Resync(const int h)
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
		Send_Resync(Hosts[h], h);

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
void server::Parse_Waiting(const int h)
{
	switch (networkStates[h].State) {
		// client has recvd welcome and is waiting for info
	case ccs_connecting:
		networkStates[h].State = ccs_connected;
		networkStates[h].MsgCnt = 0;
		/* Fall through */
	case ccs_connected: {
		// this code path happens until client acknowledges the map
		Send_Map(Hosts[h]);

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
		Send_State(Hosts[h]);

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
void server::Parse_Map(const int h)
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
		Send_State(Hosts[h]);

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
void server::Parse_State(const int h, const CInitMessage_State &msg)
{
	switch (networkStates[h].State) {
	case ccs_mapinfo:
		// User State Change right after connect - should not happen, but..
		/* Fall through */
	case ccs_synced:
		// Default case: Client is in sync with us, but notes a local change
		// networkStates[h].State = ccs_async;
		networkStates[h].MsgCnt = 0;
		// Use information supplied by the client:
		this->setup->Ready[h] = msg.State.Ready[h];
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
		Send_State(Hosts[h]);

		networkStates[h].MsgCnt++;
		if (networkStates[h].MsgCnt > 50) {
			// FIXME: Client sends State, but doesn't receive our state info....
		}
		break;
	}
	default:
		DebugPrint("Server: ICMState: Unhandled state %d Host %d\n" _C_ networkStates[h].State _C_ h);
		break;
	}
}

/**
**  Parse the disconnect request of a client by sending out good bye
**
**  @param h slot number of host msg originates from
*/
void server::Parse_GoodBye(const int h)
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
		Send_GoodBye(Hosts[h]);

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
