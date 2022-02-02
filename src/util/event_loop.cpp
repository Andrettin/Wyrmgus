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
//      (c) Copyright 2022 by Andrettin
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

#include "util/event_loop.h"

#ifdef USE_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

namespace wyrmgus {

event_loop::event_loop()
{
	this->io_context = std::make_unique<boost::asio::io_context>();

	this->start();
}

event_loop::~event_loop()
{
	if (!this->io_context->stopped()) {
		this->stop();
	}
}

void event_loop::start()
{
	this->io_context_thread = std::thread([this]() {
		boost::asio::executor_work_guard work_guard(this->io_context->get_executor());

		this->io_context->run();
	});
}

void event_loop::stop()
{
	this->io_context->stop();
	this->io_context_thread.join();
}

void event_loop::co_spawn(const std::function<void()> &function)
{
	boost::asio::co_spawn(this->io_context->get_executor(), [function]() -> boost::asio::awaitable<void> {
		function();
		co_return;
	}, boost::asio::detached);
}

}
