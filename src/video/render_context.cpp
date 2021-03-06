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

#include "video/render_context.h"

#include "video/frame_buffer_object.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace wyrmgus {

render_context::render_context()
{
	this->io_context = std::make_unique<boost::asio::io_context>();
}

render_context::~render_context()
{
}

void render_context::run()
{
	//run the posted OpenGL commands
	this->io_context->run();
	this->io_context->restart();
}

void render_context::post(const std::function<void()> &function)
{
	//execute the function in the current context too, temporarily
	//function();

	this->post_internal(function);
}

void render_context::post_internal(const std::function<void()> &function)
{
	boost::asio::post(*this->io_context, function);
	frame_buffer_object::request_update();
}

}
