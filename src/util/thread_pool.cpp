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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/thread_pool.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

namespace wyrmgus {

thread_pool::thread_pool()
{
	const size_t thread_count = std::max<size_t>(std::thread::hardware_concurrency(), 1);
	this->pool = std::make_unique<boost::asio::thread_pool>(thread_count);
}

thread_pool::~thread_pool()
{
	this->pool->stop();
	this->pool->join();
}

void thread_pool::post(const std::function<void()> &function)
{
	boost::asio::post(*this->pool, function);
}

void thread_pool::co_spawn(const std::function<boost::asio::awaitable<void>()> &function)
{
	boost::asio::co_spawn(this->pool->get_executor(), function, boost::asio::detached);
}

void thread_pool::co_spawn_sync(const std::function<boost::asio::awaitable<void>()> &function)
{
	//spawn coroutine and wait until it is complete
	std::promise<void> promise;
	std::future<void> future = promise.get_future();

	this->co_spawn([&promise, &function]() -> boost::asio::awaitable<void> {
		try {
			co_await function();
			promise.set_value();
		} catch (...) {
			promise.set_exception(std::current_exception());
		}
	});

	future.wait();
}

}
