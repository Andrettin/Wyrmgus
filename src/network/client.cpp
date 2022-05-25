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

#include "network/client.h"

#include "database/database.h"
#include "map/map.h"
#include "map/map_info.h"
#include "network/multiplayer_setup.h"
#include "network/netconnect.h"
#include "network/net_message.h"
#include "network/netsockets.h"
#include "network/network.h"
#include "network/network_manager.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/util.h"
#include "version.h"

static const char *ncconstatenames[] = {
	"ccs_unused",
	"ccs_connecting",          // new client
	"ccs_connected",           // has received slot info
	"ccs_mapinfo",             // has received matching map-info
	"ccs_badmap",              // has received non-matching map-info
	"ccs_synced",              // client is in sync with server
	"ccs_async",               // server user has changed selection
	"ccs_changed",             // client user has made menu selection
	"ccs_detaching",           // client user wants to detach
	"ccs_disconnected",        // client has detached
	"ccs_unreachable",         // server is unreachable
	"ccs_usercanceled",        // user canceled game
	"ccs_nofreeslots",         // server has no more free slots
	"ccs_serverquits",         // server quits
	"ccs_goahead",             // server wants to start game
	"ccs_started",             // server has started game
	"ccs_incompatibleengine",  // incompatible engine version
	"ccs_incompatiblenetwork", // incompatible network version
};

namespace wyrmgus {

client::client()
{
}

client::~client()
{
}

/**
** Send a message to the server, but only if the last packet was a while ago
**
** @param msg    The message to send
** @param tick   current tick
** @param msecs  microseconds to delay
*/
template <typename T>
void client::SendRateLimited(const T &msg, unsigned long tick, unsigned long msecs)
{
	const unsigned long now = tick;
	if (now - networkState.LastFrame < msecs) {
		return;
	}
	networkState.LastFrame = now;
	const unsigned char subtype = msg.GetHeader().GetSubType();
	if (subtype == lastMsgTypeSent) {
		++networkState.MsgCnt;
	} else {
		networkState.MsgCnt = 0;
		lastMsgTypeSent = subtype;
	}
	NetworkSendICMessage(*socket, *serverHost, msg);
	DebugPrint("[%s] Sending (%s:#%d)\n" _C_
		ncconstatenames[networkState.State] _C_
		icmsgsubtypenames[subtype] _C_ networkState.MsgCnt);
}

template<>
void client::SendRateLimited<CInitMessage_Header>(const CInitMessage_Header &msg, unsigned long tick, unsigned long msecs)
{
	const unsigned long now = tick;
	if (now - networkState.LastFrame < msecs) {
		return;
	}
	networkState.LastFrame = now;
	const unsigned char subtype = msg.GetSubType();
	if (subtype == lastMsgTypeSent) {
		++networkState.MsgCnt;
	} else {
		networkState.MsgCnt = 0;
		lastMsgTypeSent = subtype;
	}
	NetworkSendICMessage(*socket, *serverHost, msg);
	DebugPrint("[%s] Sending (%s:#%d)\n" _C_
		ncconstatenames[networkState.State] _C_
		icmsgsubtypenames[subtype] _C_ networkState.MsgCnt);
}

void client::Init(const std::string &name, CUDPSocket *socket, unsigned long tick)
{
	networkState.LastFrame = tick;
	networkState.State = ccs_connecting;
	networkState.MsgCnt = 0;
	lastMsgTypeSent = ICMServerQuit;
	this->server_setup = std::make_unique<multiplayer_setup>();
	this->local_setup = std::make_unique<multiplayer_setup>();
	this->name = name;
	this->socket = socket;
}

void client::DetachFromServer()
{
	networkState.State = ccs_detaching;
	networkState.MsgCnt = 0;
}

bool client::Update_disconnected()
{
	assert_throw(networkState.State == ccs_disconnected);
	const CInitMessage_Header message(MessageInit_FromClient, ICMSeeYou);

	// Spew out 5 and trust in God that they arrive
	for (int i = 0; i < 5; ++i) {
		NetworkSendICMessage(*socket, *serverHost, message);
	}
	networkState.State = ccs_usercanceled;
	return false;
}

bool client::Update_detaching(unsigned long tick)
{
	assert_throw(networkState.State == ccs_detaching);

	if (networkState.MsgCnt < 10) { // 10 retries = 1 second
		Send_GoodBye(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_detaching: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_connecting(unsigned long tick)
{
	assert_throw(networkState.State == ccs_connecting);

	if (networkState.MsgCnt < 48) { // 48 retries = 24 seconds
		Send_Hello(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_connecting: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_connected(unsigned long tick)
{
	assert_throw(networkState.State == ccs_connected);

	if (networkState.MsgCnt < 20) { // 20 retries
		Send_Waiting(tick, 650);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_connected: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

static bool IsLocalSetupInSync(const multiplayer_setup &state1, const multiplayer_setup &state2, int index)
{
	return (state1.Race[index] == state2.Race[index]
		&& state1.Ready[index] == state2.Ready[index]);
}

bool client::Update_synced(unsigned long tick)
{
	assert_throw(networkState.State == ccs_synced);

	if (IsLocalSetupInSync(*this->server_setup, *this->local_setup, NetLocalHostsSlot) == false) {
		networkState.State = ccs_changed;
		networkState.MsgCnt = 0;
		return Update(tick);
	}
	Send_Waiting(tick, 850);
	return true;
}

bool client::Update_changed(unsigned long tick)
{
	assert_throw(networkState.State == ccs_changed);

	if (networkState.MsgCnt < 20) { // 20 retries
		Send_State(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_changed: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_async(unsigned long tick)
{
	assert_throw(networkState.State == ccs_async);

	if (networkState.MsgCnt < 20) { // 20 retries
		Send_Resync(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_async: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_mapinfo(unsigned long tick)
{
	assert_throw(networkState.State == ccs_mapinfo);

	if (networkState.MsgCnt < 20) { // 20 retries
		// ICMMapAck..
		Send_Map(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_mapinfo: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_badmap(unsigned long tick)
{
	assert_throw(networkState.State == ccs_badmap);

	if (networkState.MsgCnt < 20) { // 20 retries
		Send_MapUidMismatch(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_badmap: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_goahead(unsigned long tick)
{
	assert_throw(networkState.State == ccs_goahead);

	if (networkState.MsgCnt < 50) { // 50 retries
		Send_Config(tick);
		return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_goahead: Above message limit %d\n" _C_ networkState.MsgCnt);
		return false;
	}
}

bool client::Update_started(unsigned long tick)
{
	assert_throw(networkState.State == ccs_started);

	if (networkState.MsgCnt < 20) { // 20 retries
		Send_Go(tick);
		return true;
	} else {
		return false; // End the menu..
	}
}

void client::Send_Go(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMGo);

	SendRateLimited(message, tick, 250);
}

void client::Send_Config(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMConfig);

	SendRateLimited(message, tick, 250);
}

void client::Send_MapUidMismatch(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMMapUidMismatch); // MAP Uid doesn't match

	SendRateLimited(message, tick, 650);
}

void client::Send_Map(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMMap);

	SendRateLimited(message, tick, 650);
}

void client::Send_Resync(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMResync);

	SendRateLimited(message, tick, 450);
}

void client::Send_State(unsigned long tick)
{
	const CInitMessage_State message(MessageInit_FromClient, *this->local_setup);

	SendRateLimited(message, tick, 450);
}

void client::Send_Waiting(unsigned long tick, unsigned long msec)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMWaiting);

	SendRateLimited(message, tick, msec);
}

void client::Send_Hello(unsigned long tick)
{
	const CInitMessage_Hello message(name.c_str());

	SendRateLimited(message, tick, 500);
}

void client::Send_GoodBye(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMGoodBye);

	SendRateLimited(message, tick, 100);
}

/*
** @return false when client has finished.
*/
bool client::Update(unsigned long tick)
{
	switch (networkState.State) {
		case ccs_disconnected: return Update_disconnected();
		case ccs_detaching: return Update_detaching(tick);
		case ccs_connecting: return Update_connecting(tick);
		case ccs_connected: return Update_connected(tick);
		case ccs_synced: return Update_synced(tick);
		case ccs_changed: return Update_changed(tick);
		case ccs_async: return Update_async(tick);
		case ccs_mapinfo: return Update_mapinfo(tick);
		case ccs_badmap: return Update_badmap(tick);
		case ccs_goahead: return Update_goahead(tick);
		case ccs_started: return Update_started(tick);
		default: break;
	}
	return true;
}

void client::SetConfig(const CInitMessage_Config &msg)
{
	std::unique_lock<std::shared_mutex> lock(network_manager::get()->get_mutex());

	HostsCount = 0;
	for (int i = 0; i < msg.hostsCount - 1; ++i) {
		if (i != msg.clientIndex) {
			const bool name_changed = std::string_view(Hosts[HostsCount].PlyName) != std::string_view(msg.hosts[i].PlyName);

			Hosts[HostsCount] = msg.hosts[i];

			if (name_changed) {
				emit network_manager::get()->player_name_changed(HostsCount, Hosts[HostsCount].PlyName);
			}

			HostsCount++;
#ifdef DEBUG
			const std::string hostStr = CHost(msg.hosts[i].Host, msg.hosts[i].Port).toString();
			DebugPrint("Client %d = %s [%.*s]\n" _C_
				msg.hosts[i].PlyNr _C_ hostStr.c_str() _C_
				static_cast<int>(sizeof(msg.hosts[i].PlyName)) _C_
				msg.hosts[i].PlyName);
#endif
		} else { // Own client
			NetLocalPlayerNumber = msg.hosts[i].PlyNr;
			DebugPrint("SELF %d [%.*s]\n" _C_ msg.hosts[i].PlyNr _C_
				static_cast<int>(sizeof(msg.hosts[i].PlyName)) _C_
				msg.hosts[i].PlyName);
		}
	}

	// server is last:
	const bool name_changed = std::string_view(Hosts[HostsCount].PlyName) != std::string_view(msg.hosts[msg.hostsCount - 1].PlyName);

	Hosts[HostsCount].Host = serverHost->getIp();
	Hosts[HostsCount].Port = serverHost->getPort();
	Hosts[HostsCount].PlyNr = msg.hosts[msg.hostsCount - 1].PlyNr;
	Hosts[HostsCount].SetName(msg.hosts[msg.hostsCount - 1].PlyName);

	if (name_changed) {
		emit network_manager::get()->player_name_changed(HostsCount, Hosts[HostsCount].PlyName);
	}

	++HostsCount;
	NetPlayers = HostsCount + 1;
}

void client::SetServerHost(std::unique_ptr<CHost> &&host)
{
	this->serverHost = std::move(host);
}

bool client::Parse(const std::array<unsigned char, 1024> &buf)
{
	CInitMessage_Header header;
	header.Deserialize(buf.data());

	if (header.GetType() != MessageInit_FromServer) {
		return true;
	}
	//assert_throw(host == this->serverHost);
	const unsigned char msgsubtype = header.GetSubType();

	DebugPrint("Received %s in state %s\n" _C_ icmsgsubtypenames[msgsubtype]
		_C_ ncconstatenames[networkState.State]);

	switch (msgsubtype) {
		case ICMServerQuit: { // Server user canceled, should work in all states
			networkState.State = ccs_serverquits;
			// No ack here - Server will spew out a few Quit msgs, which has to be enough
			return false;
		}
		case ICMAYT: { // Server is checking for our presence
			Parse_AreYouThere();
			break;
		}
		case ICMGoodBye: { // Server has let us go
			if (networkState.State == ccs_detaching) {
				networkState.State = ccs_disconnected;
				networkState.MsgCnt = 0;
			}
			break;
		}
		case ICMEngineMismatch: { // Stratagus engine version doesn't match
			Parse_EngineMismatch(buf.data());
			return false;
		}
		case ICMProtocolMismatch: { // Network protocol version doesn't match
			Parse_ProtocolMismatch(buf.data());
			return false;
		}
		case ICMGameFull: { // Game is full - server rejected connnection
			Parse_GameFull();
			return false;
		}
		case ICMWelcome: { // Server has accepted us
			Parse_Welcome(buf.data());
			break;
		}
		case ICMMap: { // Server has sent us new map info
			Parse_Map(buf.data());
			break;
		}
		case ICMState: {
			Parse_State(buf.data());
			break;
		}
		case ICMConfig: { // Server gives the go ahead.. - start game
			Parse_Config(buf.data());
			break;
		}
		case ICMResync: { // Server has resynced with us and sends resync data
			Parse_Resync(buf.data());
			break;
		}
		case ICMGo: { // Server's final go ..
			// ccs_started
			DebugPrint("ClientParseStarted ICMGo !!!!!\n");
			return false;
		}
		default: break;
	}
	return true;
}

/**
** Check if the map name looks safe.
**
** A map name looks safe when there are no special characters
** and no .. or // sequences. This way only real valid
** maps from the map directory will be loaded.
**
** @return  true if the map name looks safe.
*/
static bool IsSafeMapName(const char *mapname)
{
	char buf[256];

	if (strncpy_s(buf, sizeof(buf), mapname, sizeof(buf)) != 0) {
		return false;
	}
	if (strstr(buf, "..")) {
		return false;
	}
	if (strstr(buf, "//")) {
		return false;
	}
	if (buf[0] == '\0') {
		return false;
	}

	for (const char *ch = buf; *ch != '\0'; ++ch) {
		if (!isalnum(*ch) && *ch != '/' && *ch != '.' && *ch != '-'
			&& *ch != '(' && *ch != ')' && *ch != '_') {
			return false;
		}
	}
	return true;
}

void client::Parse_Map(const unsigned char *buf)
{
	if (networkState.State != ccs_connected) {
		return;
	}
	CInitMessage_Map msg;

	msg.Deserialize(buf);
	if (!IsSafeMapName(msg.MapPath)) {
		log::log_error("Insecure map name!");
		networkState.State = ccs_badmap;
		return;
	}
	NetworkMapName = std::string(msg.MapPath, sizeof(msg.MapPath));
	const std::filesystem::path map_path = database::get()->get_root_path() / NetworkMapName;
	LoadStratagusMapInfo(map_path);
	if (msg.MapUID != CMap::get()->Info->MapUID) {
		networkState.State = ccs_badmap;
		fprintf(stderr, "Stratagus maps do not match (0x%08x) <-> (0x%08x)\n",
			CMap::get()->Info->MapUID, static_cast<unsigned int>(msg.MapUID));
		return;
	}
	networkState.State = ccs_mapinfo;
	networkState.MsgCnt = 0;
}

void client::Parse_Welcome(const unsigned char *buf)
{
	if (networkState.State != ccs_connecting) {
		return;
	}

	CInitMessage_Welcome msg;

	msg.Deserialize(buf);
	networkState.State = ccs_connected;
	networkState.MsgCnt = 0;
	NetLocalHostsSlot = msg.hosts[0].PlyNr;

	std::unique_lock<std::shared_mutex> lock(network_manager::get()->get_mutex());

	const bool server_player_name_changed = std::string_view(Hosts[0].PlyName) != std::string_view(msg.hosts[0].PlyName);

	Hosts[0].SetName(msg.hosts[0].PlyName); // Name of server player
	CNetworkParameter::Instance.NetworkLag = msg.Lag;
	CNetworkParameter::Instance.gameCyclesPerUpdate = msg.gameCyclesPerUpdate;

	Hosts[0].Host = serverHost->getIp();
	Hosts[0].Port = serverHost->getPort();

	if (server_player_name_changed) {
		emit network_manager::get()->player_name_changed(0, Hosts[0].PlyName);
	}

	for (int i = 1; i < PlayerMax; ++i) {
		bool name_changed = false;

		if (i != NetLocalHostsSlot) {
			name_changed = std::string_view(Hosts[i].PlyName) != std::string_view(msg.hosts[i].PlyName);

			Hosts[i] = msg.hosts[i];
		} else {
			name_changed = std::string_view(Hosts[i].PlyName) != this->name;

			Hosts[i].PlyNr = i;
			Hosts[i].SetName(this->name.c_str());
		}

		if (name_changed) {
			emit network_manager::get()->player_name_changed(i, Hosts[i].PlyName);
		}
	}
}

void client::Parse_State(const unsigned char *buf)
{
	CInitMessage_State msg;

	msg.Deserialize(buf);
	if (networkState.State == ccs_mapinfo) {
		// Server has sent us first state info
		*this->server_setup = msg.State;
		networkState.State = ccs_synced;
		networkState.MsgCnt = 0;
	} else if (networkState.State == ccs_synced
		|| networkState.State == ccs_changed) {
		*this->server_setup = msg.State;
		networkState.State = ccs_async;
		networkState.MsgCnt = 0;
	} else if (networkState.State == ccs_goahead) {
		// Server has sent final state info
		*this->server_setup = msg.State;
		networkState.State = ccs_started;
		networkState.MsgCnt = 0;
	}
}

void client::Parse_Config(const unsigned char *buf)
{
	if (networkState.State != ccs_synced) {
		return;
	}
	CInitMessage_Config msg;

	msg.Deserialize(buf);
	SetConfig(msg);
	networkState.State = ccs_goahead;
	networkState.MsgCnt = 0;
}

void client::Parse_Resync(const unsigned char *buf)
{
	if (networkState.State != ccs_async) {
		return;
	}
	CInitMessage_Resync msg;

	msg.Deserialize(buf);
	for (int i = 1; i < PlayerMax - 1; ++i) {
		if (i != NetLocalHostsSlot) {
			Hosts[i] = msg.hosts[i];
		} else {
			Hosts[i].PlyNr = msg.hosts[i].PlyNr;
			Hosts[i].SetName(name.c_str());
		}
	}
	networkState.State = ccs_synced;
	networkState.MsgCnt = 0;
}

void client::Parse_GameFull()
{
	if (networkState.State != ccs_connecting) {
		return;
	}
	const std::string serverHostStr = serverHost->toString();
	fprintf(stderr, "Server at %s is full!\n", serverHostStr.c_str());
	networkState.State = ccs_nofreeslots;
}

void client::Parse_ProtocolMismatch(const unsigned char *buf)
{
	if (networkState.State != ccs_connecting) {
		return;
	}
	CInitMessage_ProtocolMismatch msg;

	msg.Deserialize(buf);
	const std::string serverHostStr = serverHost->toString();
	fprintf(stderr, "Incompatible network protocol version "
		NetworkProtocolFormatString " <-> " NetworkProtocolFormatString "\n"
		"from %s\n",
		NetworkProtocolFormatArgs(NetworkProtocolVersion), NetworkProtocolFormatArgs(msg.Version),
		serverHostStr.c_str());
	networkState.State = ccs_incompatiblenetwork;
}

void client::Parse_EngineMismatch(const unsigned char *buf)
{
	if (networkState.State != ccs_connecting) {
		return;
	}
	CInitMessage_EngineMismatch msg;

	msg.Deserialize(buf);
	const std::string serverHostStr = serverHost->toString();
	fprintf(stderr, "Incompatible " NAME " version %d <-> %d\nfrom %s\n",
		StratagusVersion, msg.Stratagus, serverHostStr.c_str());
	networkState.State = ccs_incompatibleengine;
}

/**
** Parse a network menu AreYouThere keepalive packet and reply IAmHere.
**
** @param msg message received
*/
void client::Parse_AreYouThere()
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMIAH); // IAmHere

	NetworkSendICMessage(*socket, *serverHost, message);
}

}
