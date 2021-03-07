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

class defines;
class parameters;

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::parameters* parameters READ get_parameters CONSTANT)
	Q_PROPERTY(wyrmgus::defines* defines READ get_defines CONSTANT)
	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)

public:
	engine_interface();
	~engine_interface();

	void post(const std::function<void()> &function);

	std::future<void> async(const std::function<void()> &function)
	{
		std::shared_ptr<std::promise<void>> promise = std::make_unique<std::promise<void>>();;
		std::future<void> future = promise->get_future();

		this->post([promise, function]() {
			function();
			promise->set_value();
		});

		return future;
	}

	void sync(const std::function<void()> &function)
	{
		//post an action, and then wait for it to be completed
		std::future<void> future = this->async(function);
		future.wait();
	}

	void run_event_loop();

	parameters *get_parameters() const;
	defines *get_defines() const;

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	Q_INVOKABLE void exit();

signals:
	void running_changed();

private:
	std::unique_ptr<boost::asio::io_context> io_context;
	bool running = false;
};

}
