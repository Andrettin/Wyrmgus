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
//      (c) Copyright 2013-2022 by Joris Dauphin, cybermind and Andrettin
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

class CHost final
{
public:
	[[nodiscard]]
	static boost::asio::awaitable<void> from_host_name_and_port(CHost &host, const std::string_view &host_name, const uint16_t port);

	[[nodiscard]]
	static boost::asio::awaitable<CHost> from_host_name_and_port(const std::string_view &host_name, const uint16_t port)
	{
		//create a host instance from a host name and a port in host byte order 
		CHost host;
		
		co_await CHost::from_host_name_and_port(host, host_name, port);

		co_return host;
	}

	[[nodiscard]]
	static CHost from_port(const uint16_t port);

	CHost()
	{
	}

	explicit CHost(const uint32_t ip, const uint16_t port) : ip(ip), port(port)
	{
	}

	uint32_t getIp() const
	{
		return this->ip;
	}

	uint16_t getPort() const
	{
		return this->port;
	}

	std::string toString() const;
	bool isValid() const;

	bool operator ==(const CHost &rhs) const
	{
		return this->ip == rhs.ip && this->port == rhs.port;
	}

	bool operator !=(const CHost &rhs) const
	{
		return !(*this == rhs);
	}

private:
	uint32_t ip = 0; //in network byte order
	uint16_t port = 0; //in network byte order
};

class CUDPSocket_Impl;
class CTCPSocket_Impl;

class CUDPSocket final
{
public:
	CUDPSocket();
	~CUDPSocket();

	[[nodiscard]]
	bool Open(const CHost &host);

	void Close();

	[[nodiscard]]
	boost::asio::awaitable<void> Send(const CHost &host, const void *buf, unsigned int len);

	[[nodiscard]]
	boost::asio::awaitable<size_t> Recv(std::array<unsigned char, 1024> &buf, int len, CHost *hostFrom);

	void SetNonBlocking();

	[[nodiscard]]
	size_t HasDataToRead();

	[[nodiscard]]
	boost::asio::awaitable<size_t> WaitForDataToRead(const int timeout);

	bool IsValid() const;

private:
	std::unique_ptr<CUDPSocket_Impl> m_impl;
};

// Class representing TCP socket used in communication
class CTCPSocket
{
public:
	CTCPSocket();
	~CTCPSocket();

	[[nodiscard]]
	bool Open(const CHost &host);

	void Close();

	[[nodiscard]]
	boost::asio::awaitable<void> Connect(const CHost &host);

	[[nodiscard]]
	boost::asio::awaitable<size_t> Send(const void *buf, unsigned int len);

	[[nodiscard]]
	boost::asio::awaitable<size_t> Recv(std::array<char, 1024> &buf);

	void SetNonBlocking();

	[[nodiscard]]
	boost::asio::awaitable<size_t> WaitForDataToRead(const int timeout);

	bool IsValid() const;
private:
	std::unique_ptr<CTCPSocket_Impl> m_impl;
};
