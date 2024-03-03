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
/**@name master.cpp - The master server. */
//
//      (c) Copyright 2003-2007 by Tom Zickel and Jimmy Salmon
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

#include "network/master.h"

#include "game/game.h"
#include "network/netsockets.h"
#include "network/network.h"
#include "parameters.h"
#include "script.h"
#include "util/exception_util.h"
#include "util/log_util.h"

CMetaClient MetaClient;

/**
**  Set the metaserver to use for internet play.
**
**  @param host  Host to connect
**  @param port  Port to use to connect
*/
void CMetaClient::SetMetaServer(const std::string host, const int port)
{
	metaHost = host;
	metaPort = port;
}

CMetaClient::~CMetaClient()
{
	this->events.clear();
	this->Close();
}

/**
**  Initialize the TCP connection to the Meta Server and send test ping to it.
**
**  @return  -1 fail, 0 success.
*/
QCoro::Task<int> CMetaClient::Init()
{
	if (metaPort == -1) {
		co_return -1;
	}

	// Server socket
	CHost metaServerHost = CHost::from_host_name_and_port(metaHost.c_str(), metaPort);

	// Client socket
	CHost metaClientHost = CHost::from_host_name_and_port(CNetworkParameter::Instance.localHost.c_str(), CNetworkParameter::Instance.localPort);

	if (metaSocket.Open(metaClientHost) == false) {
		fprintf(stderr, "METACLIENT: No free port %d available, aborting\n", metaServerHost.get_port());
		co_return -1;
	}

	try {
		co_await metaSocket.Connect(metaServerHost);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("METACLIENT: Unable to connect to host " + metaServerHost.toString());
		MetaClient.Close();
		co_return -1;
	}

	if (co_await this->Send("PING") == -1) { // not sent
		MetaClient.Close();
		co_return -1;
	}
	if (co_await this->Recv() == -1) { // not received
		MetaClient.Close();
		co_return -1;
	}
	const CClientLog &log = *GetLastMessage();
	if (log.entry.find("PING_OK") != std::string::npos) {
		// Everything is OK
		co_return 0;
	} else {
		fprintf(stderr, "METACLIENT: inappropriate message received from %s\n", metaServerHost.toString().c_str());
		MetaClient.Close();
		co_return -1;
	}
}

/**
**  Close Connection to Master Server
**
**  @return  nothing
*/
void CMetaClient::Close()
{
	if (metaSocket.IsValid()) {
		metaSocket.Close();
	}
}


/**
**  Send a command to the meta server
**
**  @param cmd   command to send
**
**  @returns     -1 if failed, otherwise length of command
*/
QCoro::Task<int> CMetaClient::Send(const std::string cmd)
{
	int ret = -1;
	std::string mes(cmd);

	QByteArray byte_array(mes.c_str(), mes.size());
	byte_array.append("\n");

	const size_t sent_bytes = co_await metaSocket.Send(byte_array);

	if (sent_bytes == 0) {
		ret = -1;
	} else {
		ret = sent_bytes;
	}

	co_return ret;
}

/**
**  Receive reply from Meta Server
**
**  @return error or number of bytes
*/
QCoro::Task<int> CMetaClient::Recv()
{
	if (co_await metaSocket.WaitForDataToRead(5000) == 0) {
		co_return -1;
	}

	const QByteArray read_data = co_await metaSocket.Recv();

	if (read_data.isEmpty()) {
		co_return -1;
	}

	// We know we now have the whole command.
	// Convert to standard notation
	std::string cmd(read_data.data(), strlen(read_data.data()));
	cmd += '\n';
	cmd += '\0';
	auto log = std::make_unique<CClientLog>();
	log->entry = cmd;
	this->events.push_back(std::move(log));
	lastRecvState = read_data.size();
	co_return static_cast<int>(read_data.size());
}
