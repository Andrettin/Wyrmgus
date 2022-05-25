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
/**@name net_message.cpp - The network message code. */
//
//      (c) Copyright 2013-2022 by Joris Dauphin and Andrettin
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

#include "network/net_message.h"

#include "network/multiplayer_setup.h"
#include "network/net_lowlevel.h"
#include "network/netconnect.h"
#include "network/network.h"
#include "util/util.h"
#include "version.h"

size_t serialize32(unsigned char *buf, uint32_t data)
{
	if (buf) {
		*reinterpret_cast<uint32_t *>(buf) = htonl(data);
	}
	return sizeof(data);
}

size_t serialize32(unsigned char *buf, int32_t data)
{
	if (buf) {
		*reinterpret_cast<int32_t *>(buf) = htonl(data);
	}
	return sizeof(data);
}

size_t serialize16(unsigned char *buf, uint16_t data)
{
	if (buf) {
		*reinterpret_cast<uint16_t *>(buf) = htons(data);
	}
	return sizeof(data);
}

size_t serialize16(unsigned char *buf, int16_t data)
{
	if (buf) {
		*reinterpret_cast<int16_t *>(buf) = htons(data);
	}
	return sizeof(data);
}

size_t serialize8(unsigned char *buf, uint8_t data)
{
	if (buf) {
		*buf = data;
	}
	return sizeof(data);
}

size_t serialize8(unsigned char *buf, int8_t data)
{
	if (buf) {
		*buf = data;
	}
	return sizeof(data);
}

size_t serialize(unsigned char *buf, const std::string &s)
{
	if (buf) {
		buf += serialize16(buf, uint16_t(s.size()));
		memcpy(buf, s.c_str(), s.size());
		buf += s.size();
		//Wyrmgus start
//		if ((s.size() & 0x03) != 0) {
//			memset(buf, 0, s.size() & 0x03);
//		}
		//Wyrmgus end
	}
	//Wyrmgus start
//	return 2 + ((s.size() + 3) & ~0x03); // round up to multiple of 4 for alignment.
	return 2 + (s.size() + 3);
	//Wyrmgus end
}

size_t serialize(unsigned char *buf, const std::vector<unsigned char> &data)
{
	if (buf) {
		if (data.empty()) {
			return serialize16(buf, uint16_t(data.size()));
		}
		buf += serialize16(buf, uint16_t(data.size()));
		memcpy(buf, &data[0], data.size());
		buf += data.size();
		//Wyrmgus start
//		if ((data.size() & 0x03) != 0) {
//			memset(buf, 0, data.size() & 0x03);
//		}
		//Wyrmgus end
	}
	//Wyrmgus start
//	return 2 + ((data.size() + 3) & ~0x03); // round up to multiple of 4 for alignment.
	return 2 + (data.size() + 3);
	//Wyrmgus end
}

size_t deserialize32(const unsigned char *buf, uint32_t *data)
{
	*data = ntohl(*reinterpret_cast<const uint32_t *>(buf));
	return sizeof(*data);
}

size_t deserialize32(const unsigned char *buf, int32_t *data)
{
	*data = ntohl(*reinterpret_cast<const int32_t *>(buf));
	return sizeof(*data);
}

size_t deserialize16(const unsigned char *buf, uint16_t *data)
{
	*data = ntohs(*reinterpret_cast<const uint16_t *>(buf));
	return sizeof(*data);
}

size_t deserialize16(const unsigned char *buf, int16_t *data)
{
	*data = ntohs(*reinterpret_cast<const int16_t *>(buf));
	return sizeof(*data);
}

size_t deserialize8(const unsigned char *buf, uint8_t *data)
{
	*data = *buf;
	return sizeof(*data);
}

size_t deserialize8(const unsigned char *buf, int8_t *data)
{
	*data = *buf;
	return sizeof(*data);
}

size_t deserialize(const unsigned char *buf, std::string &s)
{
	uint16_t size;

	buf += deserialize16(buf, &size);
	s = std::string(reinterpret_cast<const char *>(buf), size);
	//Wyrmgus start
//	return 2 + ((s.size() + 3) & ~0x03); // round up to multiple of 4 for alignment.
	return 2 + (s.size() + 3);
	//Wyrmgus end
}

size_t deserialize(const unsigned char *buf, std::vector<unsigned char> &data)
{
	uint16_t size;

	buf += deserialize16(buf, &size);
	data.assign(buf, buf + size);
	//Wyrmgus start
//	return 2 + ((data.size() + 3) & ~0x03); // round up to multiple of 4 for alignment.
	return 2 + (data.size() + 3);
	//Wyrmgus end
}

//  CInitMessage_Header

size_t CInitMessage_Header::Serialize(unsigned char *p) const
{
	unsigned char *buf = p;
	p += serialize8(p, type);
	p += serialize8(p, subtype);
	return p - buf;
}

size_t CInitMessage_Header::Deserialize(const unsigned char *p)
{
	const unsigned char *buf = p;
	p += deserialize8(p, &type);
	p += deserialize8(p, &subtype);
	return p - buf;
}

// CInitMessage_Hello

CInitMessage_Hello::CInitMessage_Hello(const char *name) :
	header(MessageInit_FromClient, ICMHello)
{
	strncpy_s(this->PlyName, sizeof(this->PlyName), name, _TRUNCATE);
	this->Stratagus = StratagusVersion;
	this->Version = NetworkProtocolVersion;
}

std::unique_ptr<const unsigned char[]> CInitMessage_Hello::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += serialize(p, this->PlyName);
	p += serialize32(p, this->Stratagus);
	p += serialize32(p, this->Version);
	return buf;
}

void CInitMessage_Hello::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += deserialize(p, this->PlyName);
	p += deserialize32(p, &this->Stratagus);
	p += deserialize32(p, &this->Version);
}

// CInitMessage_Config

CInitMessage_Config::CInitMessage_Config() :
	header(MessageInit_FromServer, ICMConfig)
{
}

std::unique_ptr<const unsigned char[]> CInitMessage_Config::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += serialize8(p, this->clientIndex);
	p += serialize8(p, this->hostsCount);
	for (int i = 0; i != PlayerMax; ++i) {
		p += this->hosts[i].Serialize(p);
	}
	return buf;
}

void CInitMessage_Config::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += deserialize8(p, &this->clientIndex);
	p += deserialize8(p, &this->hostsCount);
	for (int i = 0; i != PlayerMax; ++i) {
		p += this->hosts[i].Deserialize(p);
	}
}

// CInitMessage_EngineMismatch

CInitMessage_EngineMismatch::CInitMessage_EngineMismatch() :
	header(MessageInit_FromServer, ICMEngineMismatch)
{
	this->Stratagus = StratagusVersion;
}

std::unique_ptr<const unsigned char[]> CInitMessage_EngineMismatch::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += serialize32(p, this->Stratagus);
	return buf;
}

void CInitMessage_EngineMismatch::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += deserialize32(p, &this->Stratagus);
}

// CInitMessage_ProtocolMismatch

CInitMessage_ProtocolMismatch::CInitMessage_ProtocolMismatch() :
	header(MessageInit_FromServer, ICMProtocolMismatch)
{
	this->Version = NetworkProtocolVersion;
}

std::unique_ptr<const unsigned char[]> CInitMessage_ProtocolMismatch::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += serialize32(p, this->Version);
	return buf;
}

void CInitMessage_ProtocolMismatch::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += deserialize32(p, &this->Version);
}

// CInitMessage_Welcome

CInitMessage_Welcome::CInitMessage_Welcome() :
	header(MessageInit_FromServer, ICMWelcome)
{
	this->Lag = CNetworkParameter::Instance.NetworkLag;
	this->gameCyclesPerUpdate = CNetworkParameter::Instance.gameCyclesPerUpdate;
}

std::unique_ptr<const unsigned char[]> CInitMessage_Welcome::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	for (int i = 0; i < PlayerMax; ++i) {
		p += this->hosts[i].Serialize(p);
	}
	p += serialize32(p, this->Lag);
	p += serialize32(p, this->gameCyclesPerUpdate);
	return buf;
}

void CInitMessage_Welcome::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	for (int i = 0; i < PlayerMax; ++i) {
		p += this->hosts[i].Deserialize(p);
	}
	p += deserialize32(p, &this->Lag);
	p += deserialize32(p, &this->gameCyclesPerUpdate);
}

// CInitMessage_Map

CInitMessage_Map::CInitMessage_Map(const char *path, uint32_t mapUID) :
	header(MessageInit_FromServer, ICMMap),
	MapUID(mapUID)
{
	strncpy_s(MapPath, sizeof(MapPath), path, _TRUNCATE);
}

std::unique_ptr<const unsigned char[]> CInitMessage_Map::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += serialize(p, MapPath);
	p += serialize32(p, this->MapUID);
	return buf;
}

void CInitMessage_Map::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += deserialize(p, this->MapPath);
	p += deserialize32(p, &this->MapUID);
}

// CInitMessage_State

CInitMessage_State::CInitMessage_State(int type, const multiplayer_setup &data)
	: header(type, ICMState), State(data)
{
}

std::unique_ptr<const unsigned char[]> CInitMessage_State::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	p += this->State.serialize(p);
	return buf;
}

void CInitMessage_State::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	p += this->State.deserialize(p);
}

// CInitMessage_Resync

CInitMessage_Resync::CInitMessage_Resync() :
	header(MessageInit_FromServer, ICMResync)
{
}

std::unique_ptr<const unsigned char[]> CInitMessage_Resync::Serialize() const
{
	auto buf = std::make_unique<unsigned char[]>(Size());
	unsigned char *p = buf.get();

	p += header.Serialize(p);
	for (int i = 0; i < PlayerMax; ++i) {
		p += this->hosts[i].Serialize(p);
	}
	return buf;
}

void CInitMessage_Resync::Deserialize(const unsigned char *p)
{
	p += header.Deserialize(p);
	for (int i = 0; i < PlayerMax; ++i) {
		p += this->hosts[i].Deserialize(p);
	}
}

// CNetworkCommand

size_t CNetworkCommand::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;
	p += serialize16(p, this->Unit);
	p += serialize16(p, this->X);
	p += serialize16(p, this->Y);
	p += serialize16(p, this->Dest);
	return p - buf;
}

size_t CNetworkCommand::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;
	p += deserialize16(p, &this->Unit);
	p += deserialize16(p, &this->X);
	p += deserialize16(p, &this->Y);
	p += deserialize16(p, &this->Dest);
	return p - buf;
}

// CNetworkExtendedCommand

size_t CNetworkExtendedCommand::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;
	p += serialize8(p, this->ExtendedType);
	p += serialize8(p, this->Arg1);
	p += serialize16(p, this->Arg2);
	p += serialize16(p, this->Arg3);
	p += serialize16(p, this->Arg4);
	return p - buf;
}

size_t CNetworkExtendedCommand::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;
	p += deserialize8(p, &this->ExtendedType);
	p += deserialize8(p, &this->Arg1);
	p += deserialize16(p, &this->Arg2);
	p += deserialize16(p, &this->Arg3);
	p += deserialize16(p, &this->Arg4);
	return p - buf;
}

// CNetworkChat

size_t CNetworkChat::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;
	p += serialize(p, this->Text);
	return p - buf;
}

size_t CNetworkChat::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;
	p += deserialize(p, this->Text);
	return p - buf;
}

size_t CNetworkChat::Size() const
{
	size_t size = 0;
	size += serialize(nullptr, this->Text);
	return size;
}

// CNetworkCommandSync

size_t CNetworkCommandSync::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;
	p += serialize32(p, this->syncSeed);
	p += serialize32(p, this->syncHash);
	return p - buf;
}

size_t CNetworkCommandSync::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;
	p += deserialize32(p, &this->syncSeed);
	p += deserialize32(p, &this->syncHash);
	return p - buf;
}

// CNetworkCommandQuit

size_t CNetworkCommandQuit::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;
	p += serialize16(p, this->player);
	return p - buf;
}

size_t CNetworkCommandQuit::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;
	p += deserialize16(p, &this->player);
	return p - buf;
}

// CNetworkSelection

size_t CNetworkSelection::Serialize(unsigned char *buf) const
{
	unsigned char *p = buf;

	p += serialize16(p, this->player);
	p += serialize16(p, uint16_t(this->Units.size()));
	for (size_t i = 0; i != this->Units.size(); ++i) {
		p += serialize16(p, Units[i]);
	}
	return p - buf;
}

size_t CNetworkSelection::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;

	uint16_t size;
	p += deserialize16(p, &this->player);
	p += deserialize16(p, &size);
	this->Units.resize(size);
	for (size_t i = 0; i != this->Units.size(); ++i) {
		p += deserialize16(p, &Units[i]);
	}
	return p - buf;
}

size_t CNetworkSelection::Size() const
{
	return 2 + 2 + 2 * Units.size();
}

// CNetworkPacketHeader

size_t CNetworkPacketHeader::Serialize(unsigned char *p) const
{
	if (p != nullptr) {
		for (int i = 0; i != MaxNetworkCommands; ++i) {
			p += serialize8(p, this->Type[i]);
		}
		p += serialize8(p, this->Cycle);
		p += serialize8(p, this->OrigPlayer);
	}
	return MaxNetworkCommands + 1 + 1;
}

size_t CNetworkPacketHeader::Deserialize(const unsigned char *buf)
{
	const unsigned char *p = buf;

	for (int i = 0; i != MaxNetworkCommands; ++i) {
		p += deserialize8(p, &this->Type[i]);
	}
	p += deserialize8(p, &this->Cycle);
	p += deserialize8(p, &this->OrigPlayer);
	return p - buf;
}

// CNetworkPacket

size_t CNetworkPacket::Serialize(unsigned char *buf, int numcommands) const
{
	unsigned char *p = buf;

	p += this->Header.Serialize(p);
	for (int i = 0; i != numcommands; ++i) {
		p += serialize(p, this->Command[i]);
	}
	return p - buf;
}

void CNetworkPacket::Deserialize(const unsigned char *p, unsigned int len, int *commandCount)
{
	this->Header.Deserialize(p);
	p += CNetworkPacketHeader::Size();
	len -= CNetworkPacketHeader::Size();

	for (*commandCount = 0; len != 0; ++*commandCount) {
		const size_t r = deserialize(p, this->Command[*commandCount]);
		p += r;
		len -= r;
	}
}

size_t CNetworkPacket::Size(int numcommands) const
{
	size_t size = 0;

	size += this->Header.Serialize(nullptr);
	for (int i = 0; i != numcommands; ++i) {
		size += serialize(nullptr, this->Command[i]);
	}
	return size;
}
