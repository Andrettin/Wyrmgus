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

#include "engine_interface.h"
#include "util/assert_util.h"
#include "util/exception_util.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace wyrmgus {

event_loop::event_loop()
{
	this->io_context = std::make_unique<boost::asio::io_context>();
	this->io_context_thread = std::thread([this]() {
		this->run_io_context();
	});
}

event_loop::~event_loop()
{
	this->stop();
}

void event_loop::stop()
{
	this->io_context->stop();
	if (this->io_context_thread.joinable()) {
		this->io_context_thread.join();
	}
}

void event_loop::post(const std::function<void()> &function)
{
	boost::asio::post(*this->io_context, [this, function]() {
		try {
			function();
		} catch (const std::exception &exception) {
			exception::report(exception);
			std::terminate();
		}
	});
}

void event_loop::sync(const std::function<void()> &function)
{
	if (engine_interface::get()->is_waiting_for_interface()) {
		function();
		return;
	}

	//post an action, and then wait for it to be completed
	std::future<void> future = this->async(function);
	future.wait();
}

void event_loop::co_spawn(const std::function<boost::asio::awaitable<void>()> &function)
{
	boost::asio::co_spawn(this->io_context->get_executor(), [this, function]() -> boost::asio::awaitable<void> {
		try {
			co_await function();
		} catch (const std::exception &exception) {
			exception::report(exception);
			std::terminate();
		}
	}, boost::asio::detached);
}

boost::asio::awaitable<void> event_loop::await_ms(const uint64_t ms)
{
	boost::asio::steady_timer timer(*this->io_context);
	timer.expires_from_now(std::chrono::milliseconds(ms));
	co_await timer.async_wait(boost::asio::use_awaitable);
}

void event_loop::run_io_context()
{
	auto work_guard = boost::asio::make_work_guard(*this->io_context);
	this->io_context->run();
}

}
