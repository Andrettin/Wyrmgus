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

#include "netconnect.h"

#include "database/preferences.h"
#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "master.h"
#include "network/client.h"
#include "network/network_state.h"
#include "network.h"
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

// received nothing from client for xx frames?
static constexpr int CLIENT_LIVE_BEAT = 60;
static constexpr int CLIENT_IS_DEAD = 300;

int HostsCount;                        /// Number of hosts.
CNetworkHost Hosts[PlayerMax];         /// Host and ports of all players.

int NetConnectRunning = 0;             /// Network menu: Setup mode active
int NetConnectType = 0;             /// Network menu: Setup mode active
int NetLocalHostsSlot;                 /// Network menu: Slot # in Hosts array of local client
int NetLocalPlayerNumber;              /// Player number of local client

int NetPlayers;                         /// How many network players
std::string NetworkMapName;             /// Name of the map received with ICMMap
static int NoRandomPlacementMultiplayer = 0; /// Disable the random placement of players in muliplayer mode

CServerSetup ServerSetupState; // Server selection state for Multiplayer clients
CServerSetup LocalSetupState;  // Local selection state for Multiplayer clients

class CServer
{
public:
	void Init(const std::string &name, CUDPSocket *socket, CServerSetup *serverSetup);

	void Update(unsigned long frameCounter);
	void Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host);

	void MarkClientsAsResync();
	void KickClient(int c);
private:
	int Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host);
	void Parse_Resync(const int h);
	void Parse_Waiting(const int h);
	void Parse_Map(const int h);
	void Parse_State(const int h, const CInitMessage_State &msg);
	void Parse_GoodBye(const int h);
	void Parse_SeeYou(const int h);

	void Send_AreYouThere(const CNetworkHost &host);
	void Send_GameFull(const CHost &host);
	void Send_Welcome(const CNetworkHost &host, int hostIndex);
	void Send_Resync(const CNetworkHost &host, int hostIndex);
	void Send_Map(const CNetworkHost &host);
	void Send_State(const CNetworkHost &host);
	void Send_GoodBye(const CNetworkHost &host);
private:
	std::string name;
	NetworkState networkStates[PlayerMax]; /// Client Host states
	CUDPSocket *socket;
	CServerSetup *serverSetup;
};

static CServer Server;

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

template <typename T>
static void NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const T &msg)
{
	NetworkSendICMessage(socket, host, msg);

#ifdef DEBUG
	const std::string hostStr = host.toString();
	DebugPrint("Sending to %s -> %s\n" _C_ hostStr.c_str()
		_C_ icmsgsubtypenames[msg.GetHeader().GetSubType()]);
#endif
}

static void NetworkSendICMessage_Log(CUDPSocket &socket, const CHost &host, const CInitMessage_Header &msg)
{
	NetworkSendICMessage(socket, host, msg);

#ifdef DEBUG
	const std::string hostStr = host.toString();
	DebugPrint("Sending to %s -> %s\n" _C_ hostStr.c_str()
		_C_ icmsgsubtypenames[msg.GetSubType()]);
#endif
}

// CServer

void CServer::KickClient(int c)
{
	DebugPrint("kicking client %d\n" _C_ Hosts[c].PlyNr);
	Hosts[c].Clear();
	serverSetup->Ready[c] = 0;
	serverSetup->Race[c] = 0;
	networkStates[c].Clear();
	// Resync other clients
	for (int n = 1; n < PlayerMax - 1; ++n) {
		if (n != c && Hosts[n].PlyNr) {
			networkStates[n].State = ccs_async;
		}
	}
}

void CServer::Init(const std::string &name, CUDPSocket *socket, CServerSetup *serverSetup)
{
	for (int i = 0; i < PlayerMax; ++i) {
		networkStates[i].Clear();
		//Hosts[i].Clear();
	}
	this->serverSetup = serverSetup;
	this->name = name;
	this->socket = socket;
}

void CServer::Send_AreYouThere(const CNetworkHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMAYT); // AreYouThere

	NetworkSendICMessage(*socket, CHost(host.Host, host.Port), message);
}

void CServer::Send_GameFull(const CHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGameFull);

	NetworkSendICMessage_Log(*socket, host, message);
}

void CServer::Send_Welcome(const CNetworkHost &host, int index)
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

void CServer::Send_Resync(const CNetworkHost &host, int hostIndex)
{
	CInitMessage_Resync message;

	for (int i = 1; i < PlayerMax - 1; ++i) { // Info about other clients
		if (i != hostIndex && Hosts[i].PlyNr) {
			message.hosts[i] = Hosts[i];
		}
	}
	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void CServer::Send_Map(const CNetworkHost &host)
{
	const CInitMessage_Map message(NetworkMapName.c_str(), CMap::get()->Info->MapUID);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void CServer::Send_State(const CNetworkHost &host)
{
	const CInitMessage_State message(MessageInit_FromServer, *serverSetup);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void CServer::Send_GoodBye(const CNetworkHost &host)
{
	const CInitMessage_Header message(MessageInit_FromServer, ICMGoodBye);

	NetworkSendICMessage_Log(*socket, CHost(host.Host, host.Port), message);
}

void CServer::Update(unsigned long frameCounter)
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

void CServer::MarkClientsAsResync()
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
int CServer::Parse_Hello(int h, const CInitMessage_Hello &msg, const CHost &host)
{
	if (h == -1) { // it is a new client
		for (int i = 1; i < PlayerMax - 1; ++i) {
			// occupy first available slot
			if (serverSetup->CompOpt[i] == 0) {
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
void CServer::Parse_Resync(const int h)
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
void CServer::Parse_Waiting(const int h)
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
void CServer::Parse_Map(const int h)
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
void CServer::Parse_State(const int h, const CInitMessage_State &msg)
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
			serverSetup->Ready[h] = msg.State.Ready[h];
			serverSetup->Race[h] = msg.State.Race[h];
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
void CServer::Parse_GoodBye(const int h)
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
void CServer::Parse_SeeYou(const int h)
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

void CServer::Parse(unsigned long frameCounter, const unsigned char *buf, const CHost &host)
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
		if (NetConnectRunning == 2 && Client.GetNetworkState() == ccs_started) {
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
		if (Client.Parse(buf) == false) {
			NetConnectRunning = 0;
		}
	} else if (NetConnectRunning == 1) { // server
		Server.Parse(FrameCounter, buf.data(), host);
	}
	return 1;
}

/**
** Client Menu Loop: Send out client request messages
*/
void NetworkProcessClientRequest()
{
	if (Client.Update(GetTicks()) == false) {
		NetConnectRunning = 0;
	}
}

int GetNetworkState()
{
	return Client.GetNetworkState();
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
	Server.Update(FrameCounter);
}

/**
** Server user has finally hit the start game button
*/
void NetworkServerStartGame()
{
	assert_throw(ServerSetupState.CompOpt[0] == 0);

	// save it first..
	LocalSetupState = ServerSetupState;

	// Make a list of the available player slots.
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
	printf("INITIAL ServerSetupState:\n");
	for (int i = 0; i < PlayerMax - 1; ++i) {
		printf("%02d: CO: %d   Race: %d   Host: ", i, ServerSetupState.CompOpt[i], ServerSetupState.Race[i]);
		if (ServerSetupState.CompOpt[i] == 0) {
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
//	int compPlayers = ServerSetupState.Opponents;
	bool compPlayers = ServerSetupState.Opponents > 0;
	//Wyrmgus end
	for (int i = 1; i < h; ++i) {
		if (Hosts[i].PlyNr == 0 && ServerSetupState.CompOpt[i] != 0) {
			NetPlayers--;
		} else if (Hosts[i].PlyName[0] == 0) {
			NetPlayers--;
			//Wyrmgus start
//			if (--compPlayers >= 0) {
			if (compPlayers) {
			//Wyrmgus end
				// Unused slot gets a computer player
				ServerSetupState.CompOpt[i] = 1;
				LocalSetupState.CompOpt[i] = 1;
			} else {
				ServerSetupState.CompOpt[i] = 2;
				LocalSetupState.CompOpt[i] = 2;
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
					std::swap(LocalSetupState.CompOpt[i], LocalSetupState.CompOpt[j]);
					std::swap(LocalSetupState.Race[i], LocalSetupState.Race[j]);
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
		ServerSetupState.CompOpt[n] = LocalSetupState.CompOpt[i];
		ServerSetupState.Race[n] = LocalSetupState.Race[i];
	}

	/* NOW we have NetPlayers in Hosts array, with ServerSetupState shuffled up to match it.. */

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
	const CInitMessage_State statemsg(MessageInit_FromServer, ServerSetupState);

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
	Client.DetachFromServer();
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
	ServerSetupState.Clear();
	LocalSetupState.Clear(); // Unused when we are server
	Server.Init(preferences::get()->get_local_player_name(), &NetworkFildes, &ServerSetupState);

	// preset the server (initially always slot 0)
	Hosts[0].SetName(preferences::get()->get_local_player_name().c_str());

	for (int i = openslots; i < PlayerMax - 1; ++i) {
		ServerSetupState.CompOpt[i] = 1;
	}
}

/**
** Notify state change by menu user to connected clients
*/
void NetworkServerResyncClients()
{
	if (NetConnectRunning == 1) {
		Server.MarkClientsAsResync();
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
		printf("%02d: CO: %d   Race: %d   Name: ", i, ServerSetupState.CompOpt[i], ServerSetupState.Race[i]);
		if (ServerSetupState.CompOpt[i] == 0) {
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
		GameSettings.Presets[num[i]].Race = ServerSetupState.Race[num[i]];
		switch (ServerSetupState.CompOpt[num[i]]) {
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
		if (ServerSetupState.CompOpt[comp[i]] == 2) { // closed..
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

