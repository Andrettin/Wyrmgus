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

#include "stratagus.h"

#include "network/netsockets.h"
#include "util/qunique_ptr.h"

#include <QHostInfo>
#include <QTcpSocket>
#include <QUdpSocket>

#include <qcoro/network/qcoroabstractsocket.h>

#ifdef __MORPHOS__
#undef socket
#endif

// CHost

void CHost::from_host_name_and_port(CHost &host, const std::string_view &host_name, const uint16_t port)
{
	const QHostInfo host_info = QHostInfo::fromName(QString::fromStdString(std::string(host_name)));

	for (const QHostAddress &host_address : host_info.addresses()) {
		if (host_address.protocol() == QHostAddress::IPv4Protocol) {
			host.address = host_address;
			host.port = port;
			break;
		}
	}

	if (!host.isValid()) {
		throw std::runtime_error("Failed to resolve the host name and port to an IPv4 address.");
	}
}

CHost CHost::from_port(const uint16_t port)
{
	//create a host with IP as INADDR_ANY from a port in host byte order
	CHost host;
	host.address = QHostAddress(QHostAddress::AnyIPv4);
	host.port = port;
	return host;
}

std::string CHost::toString() const
{
	return this->get_address().toString().toStdString() + ":" + std::to_string(this->get_port());
}

bool CHost::isValid() const
{
	return !this->get_address().isNull() && this->port != 0;
}

// CUDPSocket_Impl

class CUDPSocket_Impl final
{
public:
	~CUDPSocket_Impl()
	{
		if (IsValid()) {
			Close();
		}
	}

	[[nodiscard]]
	bool Open(const CHost &host)
	{
		this->socket = make_qunique<QUdpSocket>();
		return this->socket->bind(host.get_address(), host.get_port());
	}

	void Close()
	{
		this->socket.reset();
	}

	[[nodiscard]]
	QCoro::Task<void> Send(const CHost &host, const void *buf, unsigned int len)
	{
		try {
			this->socket->writeDatagram(reinterpret_cast<const char *>(buf), static_cast<qint64>(len), host.get_address(), host.get_port());
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to send data through a UDP socket to endpoint: " + host.get_address().toString().toStdString() + ":" + std::to_string(host.get_port())));
		}

		co_return;
	}

	[[nodiscard]]
	QCoro::Task<size_t> Recv(std::array<unsigned char, 1024> &buf, CHost *hostFrom)
	{
		try {
			QHostAddress sender_address;
			uint16_t sender_port{};

			const size_t size = this->socket->readDatagram(reinterpret_cast<char *>(buf.data()), buf.size(), &sender_address, &sender_port);

			*hostFrom = CHost(sender_address, sender_port);

			co_return size;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to receive data through a UDP socket."));
		}
	}

	[[nodiscard]]
	size_t HasDataToRead()
	{
		return this->socket->bytesAvailable();
	}

	[[nodiscard]]
	QCoro::Task<size_t> WaitForDataToRead(const int timeout)
	{
		try {
			const bool success = co_await qCoro(this->socket.get()).waitForReadyRead(timeout);

			if (success) {
				co_return static_cast<size_t>(this->socket->bytesAvailable());
			} else {
				co_return 0u;
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to wait for data to receive through a UDP socket."));
		}
	}

	[[nodiscard]]
	bool IsValid() const
	{
		return this->socket != nullptr;
	}

private:
	qunique_ptr<QUdpSocket> socket;
};

// CUDPSocket

CUDPSocket::CUDPSocket()
{
	m_impl = std::make_unique<CUDPSocket_Impl>();
}

CUDPSocket::~CUDPSocket()
{
}

bool CUDPSocket::Open(const CHost &host)
{
	return m_impl->Open(host);
}

void CUDPSocket::Close()
{
	m_impl->Close();
}

QCoro::Task<void> CUDPSocket::Send(const CHost &host, const void *buf, unsigned int len)
{
	co_await m_impl->Send(host, buf, len);
}

QCoro::Task<size_t> CUDPSocket::Recv(std::array<unsigned char, 1024> &buf, CHost *hostFrom)
{
	const size_t res = co_await m_impl->Recv(buf, hostFrom);
	co_return res;
}

size_t CUDPSocket::HasDataToRead()
{
	return m_impl->HasDataToRead();
}

QCoro::Task<size_t> CUDPSocket::WaitForDataToRead(const int timeout)
{
	co_return co_await m_impl->WaitForDataToRead(timeout);
}

bool CUDPSocket::IsValid() const
{
	return m_impl->IsValid();
}

// CTCPSocket_Impl

class CTCPSocket_Impl
{
public:
	~CTCPSocket_Impl()
	{
		if (IsValid()) {
			Close();
		}
	}

	[[nodiscard]]
	bool Open(const CHost &host)
	{
		this->socket = make_qunique<QTcpSocket>();
		return this->socket->bind(host.get_address(), host.get_port());
	}

	void Close()
	{
		this->socket->close();
	}

	[[nodiscard]]
	QCoro::Task<void> Connect(const CHost &host)
	{
		co_await qCoro(this->socket.get()).connectToHost(host.get_address(), host.get_port());
	}

	[[nodiscard]]
	QCoro::Task<size_t> Send(const QByteArray &byte_array)
	{
		try {
			const size_t size = co_await qCoro(this->socket.get()).write(byte_array);

			co_return size;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to send data through a TCP socket."));
		}
	}

	[[nodiscard]]
	QCoro::Task<QByteArray> Recv()
	{
		try {
			const QByteArray byte_array = co_await qCoro(this->socket.get()).readAll();

			co_return byte_array;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to receive data through a TCP socket."));
		}
	}

	[[nodiscard]]
	QCoro::Task<size_t> WaitForDataToRead(const int timeout)
	{
		try {
			const bool success = co_await qCoro(this->socket.get()).waitForReadyRead(timeout);

			if (success) {
				co_return static_cast<size_t>(this->socket->bytesAvailable());
			} else {
				co_return 0u;
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to wait for data to receive through a TCP socket."));
		}
	}

	[[nodiscard]]
	bool IsValid() const
	{
		return this->socket != nullptr;
	}

private:
	qunique_ptr<QTcpSocket> socket;
};

// CTCPSocket

CTCPSocket::CTCPSocket()
{
	m_impl = std::make_unique<CTCPSocket_Impl>();
}

CTCPSocket::~CTCPSocket()
{
}

bool CTCPSocket::Open(const CHost &host)
{
	return m_impl->Open(host);
}

void CTCPSocket::Close()
{
	m_impl->Close();
}

QCoro::Task<void> CTCPSocket::Connect(const CHost &host)
{
	co_await m_impl->Connect(host);
}

QCoro::Task<size_t> CTCPSocket::Send(const QByteArray &byte_array)
{
	const size_t size = co_await m_impl->Send(byte_array);
	co_return size;
}

QCoro::Task<QByteArray> CTCPSocket::Recv()
{
	QByteArray byte_array = co_await m_impl->Recv();
	co_return byte_array;
}

QCoro::Task<size_t> CTCPSocket::WaitForDataToRead(const int timeout)
{
	co_return co_await m_impl->WaitForDataToRead(timeout);
}

bool CTCPSocket::IsValid() const
{
	return m_impl->IsValid();
}
