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

namespace wyrmgus {

class renderer;

//a singleton providing an OpenGL render context, to be used by the render which communicates with QtQuick
class render_context final : public singleton<render_context>
{
public:
	void set_commands(std::vector<std::function<void(renderer *)>> &&commands);

	void run(renderer *renderer)
	{
		std::vector<std::function<void(wyrmgus::renderer *)>> commands;

		{
			std::lock_guard<std::mutex> lock(this->mutex);
			commands = std::move(this->commands);
		}

		//run the posted OpenGL commands
		for (const std::function<void(wyrmgus::renderer *)> &command : commands) {
			command(renderer);
		}

		{
			std::lock_guard<std::mutex> lock(this->mutex);
			//if no new commands have been set while we were rendering, store the old commands for being run again if necessary (e.g. if the window is resized)
			if (this->commands.empty()) {
				this->commands = std::move(commands);
			}
		}
	}

	void set_free_texture_commands(std::vector<std::function<void()>> &&commands);
	void free_textures();

private:
	std::vector<std::function<void(renderer *)>> commands;
	std::vector<std::function<void()>> free_texture_commands;
	std::mutex mutex;
};

}
