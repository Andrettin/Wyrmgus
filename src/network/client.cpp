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
#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "network/multiplayer_setup.h"
#include "network/netconnect.h"
#include "network/net_message.h"
#include "network/netsockets.h"
#include "network/network.h"
#include "network/network_manager.h"
#include "player/player.h"
#include "settings.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/log_util.h"
#include "util/path_util.h"
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
boost::asio::awaitable<void> client::SendRateLimited(const T &msg, unsigned long tick, unsigned long msecs)
{
	const unsigned long now = tick;
	if (now - networkState.LastFrame < msecs) {
		co_return;
	}
	networkState.LastFrame = now;
	const unsigned char subtype = msg.GetHeader().GetSubType();
	if (subtype == lastMsgTypeSent) {
		++networkState.MsgCnt;
	} else {
		networkState.MsgCnt = 0;
		lastMsgTypeSent = subtype;
	}

	co_await NetworkSendICMessage(*socket, *serverHost, msg);

	DebugPrint("[%s] Sending (%s:#%d)\n" _C_
		ncconstatenames[networkState.State] _C_
		icmsgsubtypenames[subtype] _C_ networkState.MsgCnt);
}

template<>
boost::asio::awaitable<void> client::SendRateLimited<CInitMessage_Header>(const CInitMessage_Header &msg, unsigned long tick, unsigned long msecs)
{
	const unsigned long now = tick;
	if (now - networkState.LastFrame < msecs) {
		co_return;
	}
	networkState.LastFrame = now;
	const unsigned char subtype = msg.GetSubType();
	if (subtype == lastMsgTypeSent) {
		++networkState.MsgCnt;
	} else {
		networkState.MsgCnt = 0;
		lastMsgTypeSent = subtype;
	}

	co_await NetworkSendICMessage(*socket, *serverHost, msg);

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

bool client::is_player_ready(const int player_index) const
{
	return static_cast<bool>(this->server_setup->Ready[player_index]);
}

bool client::has_fog_of_war() const
{
	return static_cast<bool>(this->server_setup->FogOfWar);
}

bool client::is_reveal_map_enabled() const
{
	return static_cast<bool>(this->server_setup->RevealMap);
}

bool client::has_computer_opponents() const
{
	return static_cast<bool>(this->server_setup->Opponents);
}

int client::get_resources_option() const
{
	return static_cast<int>(this->server_setup->ResourcesOption);
}

int client::get_difficulty() const
{
	return static_cast<int>(this->server_setup->Difficulty);
}

void client::set_civilization(const int civilization_index)
{
	event_loop::get()->post([this, civilization_index]() {
		GameSettings.Presets[NetLocalHostsSlot].Race = civilization_index;
		this->local_setup->Race[NetLocalHostsSlot] = civilization_index;
	});
}

void client::start_game()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		CPlayer::SetThisPlayer(CPlayer::Players[1].get());

		CMap::get()->NoFogOfWar = !this->has_fog_of_war();

		if (this->is_reveal_map_enabled()) {
			FlagRevealMap = 1;
		}

		NetworkGamePrepareGameSettings();

		co_await game::get()->run_map(path::from_string(NetworkMapName));
	});
}

boost::asio::awaitable<bool> client::Update_disconnected()
{
	assert_throw(networkState.State == ccs_disconnected);
	const CInitMessage_Header message(MessageInit_FromClient, ICMSeeYou);

	// Spew out 5 and trust in God that they arrive
	for (int i = 0; i < 5; ++i) {
		co_await NetworkSendICMessage(*socket, *serverHost, message);
	}
	networkState.State = ccs_usercanceled;
	co_return false;
}

boost::asio::awaitable<bool> client::Update_detaching(unsigned long tick)
{
	assert_throw(networkState.State == ccs_detaching);

	if (networkState.MsgCnt < 10) { // 10 retries = 1 second
		co_await Send_GoodBye(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_detaching: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_connecting(unsigned long tick)
{
	assert_throw(networkState.State == ccs_connecting);

	if (networkState.MsgCnt < 48) { // 48 retries = 24 seconds
		co_await Send_Hello(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_connecting: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_connected(unsigned long tick)
{
	assert_throw(networkState.State == ccs_connected);

	if (networkState.MsgCnt < 20) { // 20 retries
		co_await Send_Waiting(tick, 650);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_connected: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

static bool IsLocalSetupInSync(const multiplayer_setup &state1, const multiplayer_setup &state2, int index)
{
	return (state1.Race[index] == state2.Race[index]
		&& state1.Ready[index] == state2.Ready[index]);
}

boost::asio::awaitable<bool> client::Update_synced(unsigned long tick)
{
	assert_throw(networkState.State == ccs_synced);

	if (IsLocalSetupInSync(*this->server_setup, *this->local_setup, NetLocalHostsSlot) == false) {
		networkState.State = ccs_changed;
		networkState.MsgCnt = 0;
		co_return co_await Update(tick);
	}
	co_await Send_Waiting(tick, 850);
	co_return true;
}

boost::asio::awaitable<bool> client::Update_changed(unsigned long tick)
{
	assert_throw(networkState.State == ccs_changed);

	if (networkState.MsgCnt < 20) { // 20 retries
		co_await Send_State(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_changed: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_async(unsigned long tick)
{
	assert_throw(networkState.State == ccs_async);

	if (networkState.MsgCnt < 20) { // 20 retries
		co_await Send_Resync(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_async: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_mapinfo(unsigned long tick)
{
	assert_throw(networkState.State == ccs_mapinfo);

	if (networkState.MsgCnt < 20) { // 20 retries
		// ICMMapAck..
		co_await Send_Map(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_mapinfo: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_badmap(unsigned long tick)
{
	assert_throw(networkState.State == ccs_badmap);

	if (networkState.MsgCnt < 20) { // 20 retries
		co_await Send_MapUidMismatch(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_badmap: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_goahead(unsigned long tick)
{
	assert_throw(networkState.State == ccs_goahead);

	if (networkState.MsgCnt < 50) { // 50 retries
		co_await Send_Config(tick);
		co_return true;
	} else {
		networkState.State = ccs_unreachable;
		DebugPrint("ccs_goahead: Above message limit %d\n" _C_ networkState.MsgCnt);
		co_return false;
	}
}

boost::asio::awaitable<bool> client::Update_started(unsigned long tick)
{
	assert_throw(networkState.State == ccs_started);

	if (networkState.MsgCnt < 20) { // 20 retries
		co_await Send_Go(tick);
		co_return true;
	} else {
		co_return false; //end the menu...
	}
}

boost::asio::awaitable<void> client::Send_Go(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMGo);

	co_await SendRateLimited(message, tick, 250);
}

boost::asio::awaitable<void> client::Send_Config(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMConfig);

	co_await SendRateLimited(message, tick, 250);
}

boost::asio::awaitable<void> client::Send_MapUidMismatch(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMMapUidMismatch); // MAP Uid doesn't match

	co_await SendRateLimited(message, tick, 650);
}

boost::asio::awaitable<void> client::Send_Map(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMMap);

	co_await SendRateLimited(message, tick, 650);
}

boost::asio::awaitable<void> client::Send_Resync(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMResync);

	co_await SendRateLimited(message, tick, 450);
}

boost::asio::awaitable<void> client::Send_State(unsigned long tick)
{
	const CInitMessage_State message(MessageInit_FromClient, *this->local_setup);

	co_await SendRateLimited(message, tick, 450);
}

boost::asio::awaitable<void> client::Send_Waiting(unsigned long tick, unsigned long msec)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMWaiting);

	co_await SendRateLimited(message, tick, msec);
}

boost::asio::awaitable<void> client::Send_Hello(unsigned long tick)
{
	const CInitMessage_Hello message(name.c_str());

	co_await SendRateLimited(message, tick, 500);
}

boost::asio::awaitable<void> client::Send_GoodBye(unsigned long tick)
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMGoodBye);

	co_await SendRateLimited(message, tick, 100);
}

/*
** @return false when client has finished.
*/
boost::asio::awaitable<bool> client::Update(unsigned long tick)
{
	switch (networkState.State) {
		case ccs_disconnected:
			co_return co_await Update_disconnected();
		case ccs_detaching:
			co_return co_await Update_detaching(tick);
		case ccs_connecting:
			co_return co_await Update_connecting(tick);
		case ccs_connected:
			co_return co_await Update_connected(tick);
		case ccs_synced:
			co_return co_await Update_synced(tick);
		case ccs_changed:
			co_return co_await Update_changed(tick);
		case ccs_async:
			co_return co_await Update_async(tick);
		case ccs_mapinfo:
			co_return co_await Update_mapinfo(tick);
		case ccs_badmap:
			co_return co_await Update_badmap(tick);
		case ccs_goahead:
			co_return co_await Update_goahead(tick);
		case ccs_started:
			co_return co_await Update_started(tick);
		default:
			break;
	}

	co_return true;
}

void client::SetConfig(const CInitMessage_Config &msg)
{
	std::unique_lock<std::shared_mutex> lock(network_manager::get()->get_mutex());

	assert_throw(msg.hostsCount <= CInitMessage_Config::max_hosts);

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

boost::asio::awaitable<bool> client::Parse(const std::array<unsigned char, 1024> &buf)
{
	CInitMessage_Header header;
	header.Deserialize(buf.data());

	if (header.GetType() != MessageInit_FromServer) {
		co_return true;
	}

	//assert_throw(host == this->serverHost);
	const unsigned char msgsubtype = header.GetSubType();

	DebugPrint("Received %s in state %s\n" _C_ icmsgsubtypenames[msgsubtype]
		_C_ ncconstatenames[networkState.State]);

	switch (msgsubtype) {
		case ICMServerQuit: { // Server user canceled, should work in all states
			networkState.State = ccs_serverquits;
			// No ack here - Server will spew out a few Quit msgs, which has to be enough
			co_return false;
		}
		case ICMAYT: { // Server is checking for our presence
			co_await Parse_AreYouThere();
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
			co_return false;
		}
		case ICMProtocolMismatch: { // Network protocol version doesn't match
			Parse_ProtocolMismatch(buf.data());
			co_return false;
		}
		case ICMGameFull: { // Game is full - server rejected connnection
			Parse_GameFull();
			co_return false;
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
		case ICMGo: { //server's final go...
			// ccs_started
			DebugPrint("ClientParseStarted ICMGo !!!!!\n");
			co_return false;
		}
		default:
			break;
	}

	co_return true;
}

void client::Parse_Map(const unsigned char *buf)
{
	if (networkState.State != ccs_connected) {
		return;
	}

	CInitMessage_Map msg;

	msg.Deserialize(buf);

	NetworkMapName = std::string(msg.MapPath);

	const std::filesystem::path map_path = database::get()->get_root_path() / path::from_string(NetworkMapName);
	LoadStratagusMapInfo(map_path);
	if (msg.MapUID != CMap::get()->Info->MapUID) {
		networkState.State = ccs_badmap;
		fprintf(stderr, "Stratagus maps do not match (0x%08x) <-> (0x%08x)\n",
			CMap::get()->Info->MapUID, static_cast<unsigned int>(msg.MapUID));
		return;
	}

	networkState.State = ccs_mapinfo;
	networkState.MsgCnt = 0;

	emit network_manager::get()->map_info_changed();
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

	for (int i = 1; i < CInitMessage_Welcome::max_hosts; ++i) {
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

	switch (networkState.State) {
		case ccs_mapinfo:
		case ccs_synced:
		case ccs_changed:
		case ccs_goahead: {
			std::unique_lock<std::shared_mutex> lock(this->mutex);

			const multiplayer_setup old_setup = *this->server_setup;

			*this->server_setup = msg.State;
			networkState.MsgCnt = 0;

			for (int i = 1; i < PlayerMax - 1; ++i) {
				if (old_setup.Ready[i] != this->server_setup->Ready[i]) {
					emit network_manager::get()->player_ready_changed(i, static_cast<bool>(this->server_setup->Ready[i]));
				}
			}

			if (old_setup.FogOfWar != this->server_setup->FogOfWar) {
				GameSettings.NoFogOfWar = !this->has_fog_of_war();
				emit fog_of_war_changed();
			}

			if (old_setup.RevealMap != this->server_setup->RevealMap) {
				GameSettings.RevealMap = this->is_reveal_map_enabled();
				emit reveal_map_changed();
			}

			if (old_setup.Opponents != this->server_setup->Opponents) {
				emit computer_opponents_changed();
			}

			if (old_setup.ResourcesOption != this->server_setup->ResourcesOption) {
				GameSettings.Resources = this->get_resources_option();
				emit resources_option_changed();
			}

			if (old_setup.Difficulty != this->server_setup->Difficulty) {
				GameSettings.Difficulty = this->get_difficulty();
				emit difficulty_changed();
			}

			network_manager::get()->check_players(this->server_setup.get());
			break;
		}
		default:
			break;
	}

	switch (networkState.State) {
		case ccs_mapinfo:
			// Server has sent us first state info
			networkState.State = ccs_synced;
			break;
		case ccs_synced:
		case ccs_changed:
			networkState.State = ccs_async;
			break;
		case ccs_goahead:
			// Server has sent final state info
			networkState.State = ccs_started;
			break;
		default:
			break;
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
	for (int i = 1; i < CInitMessage_Resync::max_hosts; ++i) {
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
boost::asio::awaitable<void> client::Parse_AreYouThere()
{
	const CInitMessage_Header message(MessageInit_FromClient, ICMIAH); // IAmHere

	co_await NetworkSendICMessage(*socket, *serverHost, message);
}

}
