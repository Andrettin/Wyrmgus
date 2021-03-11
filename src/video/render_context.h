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
//      (c) Copyright 2021 by Andrettin
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

namespace boost::asio {
	class io_context;
}

namespace wyrmgus {

//a singleton providing an OpenGL render context, to be used by the render which communicates with QtQuick
class render_context final : public singleton<render_context>
{
public:
	render_context();
	~render_context();

	void post(const std::function<void()> &function);

private:
	void post_internal(const std::function<void()> &function);

public:
	std::future<void> async(const std::function<void()> &function)
	{
		//execute the function in the current context too, temporarily
		//function();

		std::shared_ptr<std::promise<void>> promise = std::make_unique<std::promise<void>>();
		std::future<void> future = promise->get_future();

		this->post_internal([promise, function]() {
			function();
			promise->set_value();
		});

		return future;
	}

	//post an action, and then wait for it to be completed
	void sync(const std::function<void()> &function)
	{
		std::future<void> future = this->async(function);
		future.wait();
	}

	void run();

private:
	std::unique_ptr<boost::asio::io_context> io_context;
};

}
