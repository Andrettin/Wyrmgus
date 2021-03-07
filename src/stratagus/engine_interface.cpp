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

#include "stratagus.h"

#include "engine_interface.h"

#include "database/defines.h"
#include "parameters.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace wyrmgus {

engine_interface::engine_interface()
{
	this->io_context = std::make_unique<boost::asio::io_context>();
}

engine_interface::~engine_interface()
{
}

parameters *engine_interface::get_parameters() const
{
	return parameters::get();
}

defines *engine_interface::get_defines() const
{
	return defines::get();
}

void engine_interface::run_event_loop()
{
	//run the commands posted from the Qt thread
	this->io_context->run();
	this->io_context->restart();
}

void engine_interface::post(const std::function<void()> &function)
{
	boost::asio::post(*this->io_context, function);
}

}
