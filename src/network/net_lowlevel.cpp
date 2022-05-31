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
/**@name lowlevel.cpp - The network lowlevel. */
//
//      (c) Copyright 2000-2007 by Lutz Sammer and Jimmy Salmon
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

#include "network/net_lowlevel.h"

#include "util/assert_util.h"
#include "util/log_util.h"

#include <fcntl.h>

#ifdef USE_WIN32
#include <windows.h>
#include <winsock.h>
#endif

#if defined(__MORPHOS__)
#undef closesocket
#define closesocket CloseSocket
#define ioctl(x,y,z) IoctlSocket(x,y,(char *)z)
#endif

//----------------------------------------------------------------------------
//  Low level functions
//----------------------------------------------------------------------------

#ifdef USE_WINSOCK // {

/**
**  Hardware dependend network init.
*/
int NetInit()
{
	WSADATA wsaData;

	// Start up the windows networking
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		log::log_error("Couldn't initialize Winsock 2.");
		return -1;
	}
	return 0;
}

/**
**  Hardware dependend network exit.
*/
void NetExit()
{
	// Clean up windows networking
	if (WSACleanup() == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEINPROGRESS) {
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
}
#endif // } !USE_WINSOCK

#if !defined(USE_WINSOCK) // {
/**
**  Hardware dependend network init.
*/
int NetInit()
{
	return 0;
}

/**
**  Hardware dependend network exit.
*/
void NetExit()
{
}
#endif // } !USE_WINSOCK

/**
**  Resolve host in name or dotted quad notation.
**
**  @param host  Host name (f.e. 192.168.0.0 or stratagus.net)
*/
unsigned long NetResolveHost(const std::string &host)
{
	if (!host.empty()) {
		#ifdef __MORPHOS__
		unsigned long addr = inet_addr((UBYTE *)host.c_str()); // try dot notation
		#else
		unsigned long addr = inet_addr(host.c_str()); // try dot notation
		#endif
		if (addr == INADDR_NONE) {
			#ifdef __MORPHOS__
			struct hostent *he = gethostbyname((UBYTE *)host.c_str());
			#else
			struct hostent *he = gethostbyname(host.c_str());
			#endif
			if (he) {
				addr = 0;
				assert_throw(he->h_length == 4);
				memcpy(&addr, he->h_addr, he->h_length);
			}
		}
		return addr;
	}
	return INADDR_NONE;
}
