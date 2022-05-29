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

#pragma once

#include "util/singleton.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace boost::asio {
	class thread_pool;
}

namespace wyrmgus {

//a singleton providing a thread pool
class thread_pool final : public singleton<thread_pool>
{
public:
	thread_pool();
	~thread_pool();

	void stop();

	void post(const std::function<void()> &function);

	void co_spawn(const std::function<boost::asio::awaitable<void>()> &function);
	void co_spawn_sync(const std::function<boost::asio::awaitable<void>()> &function);

	template <typename function_type>
	[[nodiscard]]
	inline std::invoke_result_t<function_type> co_spawn_awaitable(function_type &&function)
	{
		return boost::asio::co_spawn(this->pool->get_executor(), std::move(function), boost::asio::use_awaitable);
	}

	[[nodiscard]]
	boost::asio::awaitable<void> await_ms(const uint64_t ms);

	[[nodiscard]]
	boost::asio::awaitable<void> await_future(std::future<void> &&future);

private:
	std::unique_ptr<boost::asio::thread_pool> pool;
};

}
