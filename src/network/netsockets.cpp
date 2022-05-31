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

#include "util/event_loop.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#ifdef __MORPHOS__
#undef socket
#endif

// CHost

boost::asio::awaitable<void> CHost::from_host_name_and_port(CHost &host, const std::string_view &host_name, const uint16_t port)
{
	boost::asio::ip::tcp::resolver resolver(event_loop::get()->get_io_context());
	boost::asio::ip::tcp::resolver::iterator it = co_await resolver.async_resolve(host_name, std::to_string(port), boost::asio::use_awaitable);

	boost::asio::ip::tcp::resolver::iterator it_end;

	for (; it != it_end; ++it) {
		boost::asio::ip::address address = it->endpoint().address();

		if (address.is_v4()) {
			host.ip = htonl(address.to_v4().to_ulong());
			host.port = htons(port);
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
	host.ip = INADDR_ANY;
	host.port = htons(port);
	return host;
}

std::string CHost::toString() const
{
	const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4(ntohl(this->ip)), ntohs(this->port));
	return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

bool CHost::isValid() const
{
	return this->ip != 0 && this->port != 0;
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
		this->endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(ntohl(host.getIp())), ntohs(host.getPort()));
		this->socket = std::make_unique<boost::asio::ip::udp::socket>(event_loop::get()->get_io_context(), this->endpoint);
		return this->socket->is_open();
	}

	void Close()
	{
		this->socket->close();
	}

	[[nodiscard]]
	boost::asio::awaitable<void> Send(const CHost &host, const void *buf, unsigned int len)
	{
		boost::asio::ip::udp::endpoint receiver_endpoint(boost::asio::ip::address_v4(ntohl(host.getIp())), ntohs(host.getPort()));

		try {
			co_await this->socket->async_send_to(boost::asio::buffer(buf, len), receiver_endpoint, boost::asio::use_awaitable);
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to send data through a UDP socket to endpoint: " + receiver_endpoint.address().to_string() + ":" + std::to_string(receiver_endpoint.port())));
		}
	}

	[[nodiscard]]
	boost::asio::awaitable<size_t> Recv(std::array<unsigned char, 1024> &buf, int len, CHost *hostFrom)
	{
		try {
			boost::asio::ip::udp::endpoint sender_endpoint;

			const size_t size = co_await this->socket->async_receive_from(boost::asio::buffer(buf.data(), buf.size()), sender_endpoint, boost::asio::use_awaitable);

			*hostFrom = CHost(htonl(sender_endpoint.address().to_v4().to_ulong()), htons(sender_endpoint.port()));

			co_return size;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to receive data through a UDP socket."));
		}
	}

	void SetNonBlocking()
	{
		this->socket->non_blocking(true);
	}

	[[nodiscard]]
	size_t HasDataToRead()
	{
		return this->socket->available();
	}

	[[nodiscard]]
	boost::asio::awaitable<size_t> WaitForDataToRead(const int timeout)
	{
		try {
			boost::asio::steady_timer timer(event_loop::get()->get_io_context());
			timer.expires_from_now(std::chrono::milliseconds(timeout));

			bool timed_out = false;

			timer.async_wait([this, &timed_out](const boost::system::error_code &error_code) {
				if (error_code) {
					return;
				}

				this->socket->cancel();
				timed_out = true;
			});

			co_await this->socket->async_wait(boost::asio::ip::udp::socket::wait_read, boost::asio::use_awaitable);
			timer.cancel();

			if (timed_out) {
				co_return 0;
			} else {
				co_return this->socket->available();
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to wait for data to receive through a UDP socket."));
		}
	}

	[[nodiscard]]
	bool IsValid() const
	{
		return this->socket != nullptr && this->socket->is_open();
	}

private:
	boost::asio::ip::udp::endpoint endpoint;
	std::unique_ptr<boost::asio::ip::udp::socket> socket;
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

boost::asio::awaitable<void> CUDPSocket::Send(const CHost &host, const void *buf, unsigned int len)
{
	co_await m_impl->Send(host, buf, len);
}

boost::asio::awaitable<size_t> CUDPSocket::Recv(std::array<unsigned char, 1024> &buf, int len, CHost *hostFrom)
{
	const size_t res = co_await m_impl->Recv(buf, len, hostFrom);
	co_return res;
}

void CUDPSocket::SetNonBlocking()
{
	m_impl->SetNonBlocking();
}

size_t CUDPSocket::HasDataToRead()
{
	return m_impl->HasDataToRead();
}

boost::asio::awaitable<size_t> CUDPSocket::WaitForDataToRead(const int timeout)
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
		this->endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(ntohl(host.getIp())), ntohs(host.getPort()));
		this->socket = std::make_unique<boost::asio::ip::tcp::socket>(event_loop::get()->get_io_context(), this->endpoint);
		return this->socket->is_open();
	}

	void Close()
	{
		this->socket->close();
	}

	[[nodiscard]]
	boost::asio::awaitable<void> Connect(const CHost &host)
	{
		boost::asio::ip::tcp::endpoint connect_endpoint(boost::asio::ip::address_v4(ntohl(host.getIp())), ntohs(host.getPort()));

		co_await this->socket->async_connect(connect_endpoint, boost::asio::use_awaitable);
	}

	[[nodiscard]]
	boost::asio::awaitable<size_t> Send(const void *buf, unsigned int len)
	{
		try {
			const size_t size = co_await this->socket->async_send(boost::asio::buffer(buf, len), boost::asio::use_awaitable);

			co_return size;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to send data through a TCP socket."));
		}
	}

	[[nodiscard]]
	boost::asio::awaitable<size_t> Recv(std::array<char, 1024> &buf)
	{
		try {
			const size_t size = co_await this->socket->async_receive(boost::asio::buffer(buf.data(), buf.size()), boost::asio::use_awaitable);

			co_return size;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to receive data through a TCP socket."));
		}
	}

	void SetNonBlocking()
	{
		this->socket->non_blocking(true);
	}

	[[nodiscard]]
	boost::asio::awaitable<size_t> WaitForDataToRead(const int timeout)
	{
		try {
			boost::asio::steady_timer timer(event_loop::get()->get_io_context());
			timer.expires_from_now(std::chrono::milliseconds(timeout));

			bool timed_out = false;

			timer.async_wait([this, &timed_out](const boost::system::error_code &error_code) {
				if (error_code) {
					return;
				}

				this->socket->cancel();
				timed_out = true;
			});

			co_await this->socket->async_wait(boost::asio::ip::tcp::socket::wait_read, boost::asio::use_awaitable);
			timer.cancel();

			if (timed_out) {
				co_return 0;
			} else {
				co_return this->socket->available();
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to wait for data to receive through a TCP socket."));
		}
	}

	[[nodiscard]]
	bool IsValid() const
	{
		return this->socket != nullptr && this->socket->is_open();
	}

private:
	boost::asio::ip::tcp::endpoint endpoint;
	std::unique_ptr<boost::asio::ip::tcp::socket> socket;
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

boost::asio::awaitable<void> CTCPSocket::Connect(const CHost &host)
{
	co_await m_impl->Connect(host);
}

boost::asio::awaitable<size_t> CTCPSocket::Send(const void *buf, unsigned int len)
{
	const size_t size = co_await m_impl->Send(buf, len);
	co_return size;
}

boost::asio::awaitable<size_t> CTCPSocket::Recv(std::array<char, 1024> &buf)
{
	const size_t res = co_await m_impl->Recv(buf);
	co_return res;
}

void CTCPSocket::SetNonBlocking()
{
	m_impl->SetNonBlocking();
}

boost::asio::awaitable<size_t> CTCPSocket::WaitForDataToRead(const int timeout)
{
	co_return co_await m_impl->WaitForDataToRead(timeout);
}

bool CTCPSocket::IsValid() const
{
	return m_impl->IsValid();
}
