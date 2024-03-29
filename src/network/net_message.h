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
/**@name net_message.h - The network message header file. */
//
//      (c) Copyright 2013 by Joris Dauphin
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

#include "network/multiplayer_host.h"
#include "network/multiplayer_setup.h"

constexpr int MaxNetworkCommands = 9;  /// Max Commands In A Packet

/**
**  Network init config message subtypes (menu state machine).
*/
enum _ic_message_subtype_ {
	ICMHello,               /// Client Request
	ICMConfig,              /// Setup message configure clients

	ICMEngineMismatch,      /// Stratagus engine version doesn't match
	ICMProtocolMismatch,    /// Network protocol version doesn't match
	ICMEngineConfMismatch,  /// UNUSED:Engine configuration isn't identical
	ICMMapUidMismatch,      /// MAP UID doesn't match

	ICMGameFull,            /// No player slots available
	ICMWelcome,             /// Acknowledge for new client connections

	ICMWaiting,             /// Client has received Welcome and is waiting for Map/State
	ICMMap,                 /// MapInfo (and Mapinfo Ack)
	ICMState,               /// StateInfo
	ICMResync,              /// Ack StateInfo change

	ICMServerQuit,          /// Server has quit game
	ICMGoodBye,             /// Client wants to leave game
	ICMSeeYou,              /// Client has left game

	ICMGo,                  /// Client is ready to run

	ICMAYT,                 /// Server asks are you there
	ICMIAH                  /// Client answers I am here
};

class CInitMessage_Header
{
public:
	CInitMessage_Header() {}
	CInitMessage_Header(unsigned char type, unsigned char subtype) :
		type(type),
		subtype(subtype)
	{}

	unsigned char GetType() const { return type; }
	unsigned char GetSubType() const { return subtype; }

	size_t Serialize(unsigned char *p) const;
	size_t Deserialize(const unsigned char *p);
	static size_t Size() { return 2; }

private:
	unsigned char type = 0;
	unsigned char subtype = 0;
};

class CInitMessage_Hello
{
public:
	CInitMessage_Hello() {}
	explicit CInitMessage_Hello(const char *name);
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);
	static size_t Size() { return CInitMessage_Header::Size() + NetPlayerNameSize + 2 * 4; }
private:
	CInitMessage_Header header;
public:
	char PlyName[NetPlayerNameSize]{};  /// Name of player
	int32_t Stratagus = 0;  /// Stratagus engine version
	int32_t Version = 0;    /// Network protocol version
};

class CInitMessage_Config final
{
public:
	static constexpr int max_hosts = 8;

	CInitMessage_Config();
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);

	static size_t Size()
	{
		return CInitMessage_Header::Size() + 4 + CInitMessage_Config::max_hosts * multiplayer_host::Size();
	}

private:
	CInitMessage_Header header;
public:
	uint8_t clientIndex = 0; /// index of receiver in hosts[]
	uint8_t hostsCount = 0;  /// Number of hosts
	multiplayer_host hosts[CInitMessage_Config::max_hosts]{}; /// Participant information
};

class CInitMessage_EngineMismatch
{
public:
	CInitMessage_EngineMismatch();
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);
	static size_t Size() { return CInitMessage_Header::Size() + 4; }
private:
	CInitMessage_Header header;
public:
	int32_t Stratagus;  /// Stratagus engine version
};

class CInitMessage_ProtocolMismatch
{
public:
	CInitMessage_ProtocolMismatch();
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);
	static size_t Size() { return CInitMessage_Header::Size() + 4; }
private:
	CInitMessage_Header header;
public:
	int32_t Version;  /// Network protocol version
};

class CInitMessage_Welcome final
{
public:
	static constexpr int max_hosts = 8;

	CInitMessage_Welcome();
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);

	static size_t Size()
	{
		return CInitMessage_Header::Size() + CInitMessage_Welcome::max_hosts * multiplayer_host::Size() + 2 * 4;
	}

private:
	CInitMessage_Header header;
public:
	multiplayer_host hosts[CInitMessage_Welcome::max_hosts]{}; /// Participants information
	int32_t Lag = 0;                   /// Lag time
	int32_t gameCyclesPerUpdate = 0;   /// Update frequency
};

class CInitMessage_Map
{
public:
	CInitMessage_Map() {}
	CInitMessage_Map(const char *path, uint32_t mapUID);
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);
	static size_t Size() { return CInitMessage_Header::Size() + 256 + 4; }
private:
	CInitMessage_Header header;
public:
	char MapPath[256]{};
	uint32_t MapUID = 0;  /// UID of map to play.
};

class CInitMessage_State
{
public:
	CInitMessage_State() {}
	CInitMessage_State(int type, const multiplayer_setup &data);
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);

	static size_t Size()
	{
		return CInitMessage_Header::Size() + multiplayer_setup::size();
	}

private:
	CInitMessage_Header header;
public:
	multiplayer_setup State;  /// Server Setup State information
};

class CInitMessage_Resync final
{
public:
	static constexpr int max_hosts = 8;

	CInitMessage_Resync();
	const CInitMessage_Header &GetHeader() const { return header; }
	std::unique_ptr<const unsigned char[]> Serialize() const;
	void Deserialize(const unsigned char *p);

	static size_t Size()
	{
		return CInitMessage_Header::Size() + multiplayer_host::Size() * CInitMessage_Resync::max_hosts;
	}

private:
	CInitMessage_Header header;
public:
	multiplayer_host hosts[CInitMessage_Resync::max_hosts]{}; /// Participant information
};

/**
**  Network message types.
**
**  @todo cleanup the message types.
*/
enum _message_type_ {
	MessageNone,                   /// When Nothing Is Happening
	MessageInit_FromClient,        /// Start connection
	MessageInit_FromServer,        /// Connection reply

	MessageSync,                   /// Heart beat
	MessageSelection,              /// Update a Selection from Team Player
	MessageQuit,                   /// Quit game
	MessageResend,                 /// Resend message

	MessageChat,                   /// Chat message

	MessageCommandStop,            /// Unit command stop
	MessageCommandStand,           /// Unit command stand ground
	MessageCommandDefend,          /// Unit command defend
	MessageCommandFollow,          /// Unit command follow
	MessageCommandMove,            /// Unit command move
	//Wyrmgus start
	MessageCommandPickUp,		   /// Unit command pick up
	//Wyrmgus end
	MessageCommandRepair,          /// Unit command repair
	MessageCommandAutoRepair,      /// Unit command autorepair
	MessageCommandAttack,          /// Unit command attack
	MessageCommandGround,          /// Unit command attack ground
	//Wyrmgus start
	MessageCommandUse,			   /// Unit command use
	MessageCommandTrade,		   /// Unit command trade
	//Wyrmgus end
	MessageCommandPatrol,          /// Unit command patrol
	MessageCommandBoard,           /// Unit command board
	MessageCommandUnload,          /// Unit command unload
	MessageCommandBuild,           /// Unit command build building
	MessageCommandDismiss,         /// Unit command dismiss unit
	MessageCommandResourceLoc,     /// Unit command resource location
	MessageCommandResource,        /// Unit command resource
	MessageCommandReturn,          /// Unit command return goods
	MessageCommandTrain,           /// Unit command train
	MessageCommandCancelTrain,     /// Unit command cancel training
	MessageCommandUpgrade,         /// Unit command upgrade
	MessageCommandCancelUpgrade,   /// Unit command cancel upgrade
	MessageCommandResearch,        /// Unit command research
	MessageCommandCancelResearch,  /// Unit command cancel research
	//Wyrmgus start
	MessageCommandLearnAbility,    /// Unit command learn ability
	MessageCommandBuy,			   /// Unit command buy
	MessageCommandProduceResource, /// Unit command produce resource
	MessageCommandSellResource,	   /// Unit command sell resource
	MessageCommandBuyResource,	   /// Unit command buy resource
	//Wyrmgus end

	MessageExtendedCommand,        /// Command is the next byte

	// ATTN: __MUST__ be last due to spellid encoding!!!
	MessageCommandSpellCast        /// Unit command spell cast
};

/**
**  Network extended message types.
*/
enum _extended_message_type_ {
	ExtendedMessageDiplomacy,     /// Change diplomacy
	ExtendedMessageSharedVision,  /// Change shared vision
	ExtendedMessageSetFaction,	  /// Change faction
	ExtendedMessageSetDynasty,	  /// Change dynasty
	ExtendedMessageAutosellResource	  /// Autosell resource
};

/**
**  Network command message.
*/
class CNetworkCommand
{
public:
	CNetworkCommand() : Unit(0), X(0), Y(0), Dest(0) {}
	void Clear() { this->Unit = this->X = this->Y = this->Dest = 0; }

	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	static size_t Size() { return 2 + 2 + 2 + 2; }

public:
	uint16_t Unit;         /// Command for unit
	uint16_t X;            /// Map position X
	uint16_t Y;            /// Map position Y
	uint16_t Dest;         /// Destination unit
};

/**
**  Extended network command message.
*/
class CNetworkExtendedCommand
{
public:
	CNetworkExtendedCommand() : ExtendedType(0), Arg1(0), Arg2(0), Arg3(0), Arg4(0) {}

	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	static size_t Size() { return 1 + 1 + 2 + 2 + 2; }

	uint8_t  ExtendedType;  /// Extended network command type
	uint8_t  Arg1;          /// Argument 1
	uint16_t Arg2;          /// Argument 2
	uint16_t Arg3;          /// Argument 3
	uint16_t Arg4;          /// Argument 4
};

/**
**  Network chat message.
*/
class CNetworkChat
{
public:
	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	size_t Size() const;

public:
	std::string Text;  /// Message bytes
};

/**
**  Network sync message.
*/
class CNetworkCommandSync
{
public:
	CNetworkCommandSync() : syncSeed(0), syncHash(0) {}
	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	static size_t Size() { return 4 + 4; };

public:
	uint32_t syncSeed;
	uint32_t syncHash;
};

/**
**  Network quit message.
*/
class CNetworkCommandQuit
{
public:
	CNetworkCommandQuit() : player(0) {}
	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	static size_t Size() { return 2; };

public:
	uint16_t player;
};

/**
**  Network Selection Update
*/
class CNetworkSelection
{
public:
	CNetworkSelection() : player(0) {}

	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	size_t Size() const;

public:
	uint16_t player;
	std::vector<uint16_t> Units;  /// Selection Units
};

/**
**  Network packet header.
**
**  Header for the packet.
*/
class CNetworkPacketHeader final
{
public:
	size_t Serialize(unsigned char *buf) const;
	size_t Deserialize(const unsigned char *buf);
	static size_t Size() { return 1 + 1 + 1 * MaxNetworkCommands; }

	std::array<uint8_t, MaxNetworkCommands> Type{};  /// Commands in packet
	uint8_t Cycle = 0;                     /// Destination game cycle
	uint8_t OrigPlayer = 255;                /// Host address
};

/**
**  Network packet.
**
**  This is sent over the network.
*/
class CNetworkPacket final
{
public:
	size_t Serialize(unsigned char *buf, int numcommands) const;
	void Deserialize(const unsigned char *buf, unsigned int len, int *numcommands);
	size_t Size(int numcommands) const;

	CNetworkPacketHeader Header;  //packet Header Info
	std::array<std::vector<unsigned char>, MaxNetworkCommands> Command{};
};

extern size_t serialize32(unsigned char *buf, uint32_t data);
extern size_t serialize32(unsigned char *buf, int32_t data);
extern size_t serialize16(unsigned char *buf, uint16_t data);
extern size_t serialize16(unsigned char *buf, int16_t data);
extern size_t serialize8(unsigned char *buf, uint8_t data);
extern size_t serialize8(unsigned char *buf, int8_t data);
extern size_t serialize(unsigned char *buf, const std::string &s);
extern size_t serialize(unsigned char *buf, const std::vector<unsigned char> &data);
extern size_t deserialize32(const unsigned char *buf, uint32_t *data);
extern size_t deserialize32(const unsigned char *buf, int32_t *data);
extern size_t deserialize16(const unsigned char *buf, uint16_t *data);
extern size_t deserialize16(const unsigned char *buf, int16_t *data);
extern size_t deserialize8(const unsigned char *buf, uint8_t *data);
extern size_t deserialize8(const unsigned char *buf, int8_t *data);
extern size_t deserialize(const unsigned char *buf, std::string &s);
extern size_t deserialize(const unsigned char *buf, std::vector<unsigned char> &data);

template <int N>
inline size_t serialize(unsigned char *buf, const char(&data)[N])
{
	if (buf) {
		memcpy(buf, data, N);
	}
	return N;
}

template <int N>
inline size_t deserialize(const unsigned char *buf, char(&data)[N])
{
	memcpy(data, buf, N);
	return N;
}
