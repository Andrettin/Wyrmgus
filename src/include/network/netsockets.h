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
/**@name netsockets.h - TCP and UDP sockets. */
//
//      (c) Copyright 2013 by Joris Dauphin and cybermind
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

class CHost
{
public:
	CHost() {}
	explicit CHost(const char *name, int port);
	explicit CHost(unsigned long ip, int port) : ip(ip), port(port) {}
	unsigned long getIp() const { return ip; }
	int getPort() const { return port; }
	std::string toString() const;
	bool isValid() const;

	bool operator == (const CHost &rhs) const { return ip == rhs.ip && port == rhs.port; }
	bool operator != (const CHost &rhs) const { return !(*this == rhs); }
private:
	unsigned long ip = 0;
	int port = 0;
};

class CUDPSocket_Impl;
class CTCPSocket_Impl;

class CUDPSocket
{
public:
	CUDPSocket();
	~CUDPSocket();
	bool Open(const CHost &host);
	void Close();
	void Send(const CHost &host, const void *buf, unsigned int len);
	int Recv(std::array<unsigned char, 1024> &buf, int len, CHost *hostFrom);
	void SetNonBlocking();
	//
	int HasDataToRead(int timeout);
	bool IsValid() const;

#ifdef DEBUG
	class CStatistic
	{
		friend class CUDPSocket;
	public:
		CStatistic();
		void clear();
	public:
		unsigned int sentPacketsCount;
		unsigned int receivedPacketsCount;
		unsigned int sentBytesCount;
		unsigned int receivedBytesCount;
		unsigned int receivedErrorCount;
		unsigned int receivedBytesExpectedCount;
		unsigned int biggestSentPacketSize;
		unsigned int biggestReceivedPacketSize;
	};

	void clearStatistic() { m_statistic.clear(); }
	const CStatistic &getStatistic() const { return m_statistic; }
private:
	CStatistic m_statistic;
#endif

private:
	std::unique_ptr<CUDPSocket_Impl> m_impl;
};

// Class representing TCP socket used in communication
class CTCPSocket
{
public:
	CTCPSocket();
	~CTCPSocket();
	bool Open(const CHost &host);
	void Close();
	bool Connect(const CHost &host);
	int Send(const void *buf, unsigned int len);
	int Recv(void *buf, int len);
	void SetNonBlocking();
	//
	int HasDataToRead(int timeout);
	bool IsValid() const;
private:
	std::unique_ptr<CTCPSocket_Impl> m_impl;
};
